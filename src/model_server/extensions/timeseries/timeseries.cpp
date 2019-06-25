/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unordered_set>
#include <cmath>

#include <core/util/hash_value.hpp>
#include <core/storage/sframe_data/groupby_aggregate.hpp>
#include <model_server/extensions/timeseries/timeseries.hpp>

using namespace turi;

namespace turi {
namespace timeseries {

//////////////////////////////////////////////////////////////////////////////
//
//                     TimeSeries helper utilities
//
//////////////////////////////////////////////////////////////////////////////

/**
 * Resample helper: Parse and split operators.
 *
 * \param[in]     sframe            Input data.
 * \param[in]     operators         Operators.
 * \param[in,out] agg_ops_vec       Aggregators used in the timeseries.
 * \param[in,out] ret_column_names  Names of the columns in output timeseries.
 */
void parse_split_log_operators(
      const gl_sframe& sframe,
      const std::map<std::string,
           aggregate::groupby_descriptor_type>& operators,
      std::vector<std::pair<std::vector<std::string>,
                       std::shared_ptr<group_aggregate_value>>>& agg_ops,
      std::vector<std::string>& ret_column_names) {

  std::vector<std::shared_ptr<group_aggregate_value>> agg_ops_vec;
  std::vector<std::vector<std::string>> agg_columns_vec;
  for (const auto& op: operators) {
    std::shared_ptr<group_aggregate_value> aggregator;
    if (op.second.m_aggregator->name() == "Sum" &&
        sframe[op.second.m_group_columns[0]].dtype() == flex_type_enum::VECTOR){
      aggregator = get_builtin_group_aggregator("__builtin__vector__sum__");
    } else if (op.second.m_aggregator->name() == "Avg" &&
        sframe[op.second.m_group_columns[0]].dtype() == flex_type_enum::VECTOR){
      aggregator = get_builtin_group_aggregator("__builtin__vector__avg__");
    } else {
      aggregator = op.second.m_aggregator;
    }
    agg_columns_vec.push_back(op.second.m_group_columns);
    ret_column_names.push_back(op.first);
    agg_ops_vec.push_back(aggregator);
  }

  // Log stuff!
  logstream(LOG_INFO) << "\tGroups: ";
  for (auto cols: agg_columns_vec) {
    for(auto col: cols) {
      logstream(LOG_INFO) << col << ",";
    }
    logstream(LOG_INFO) << " | ";
  }
  logstream(LOG_INFO) << "\tOperations: ";
  for (auto i: agg_ops_vec) logstream(LOG_INFO) << i << ",";
  logstream(LOG_INFO) << std::endl;

  // Prepare the operators
  for (size_t i = 0;i < agg_columns_vec.size(); ++i) {
    std::vector<std::string> column_names;
    for (const auto& col : agg_columns_vec[i]) {
      // Avoid copying empty column string (e.g aggregate::COUNT())
      if (!col.empty()) {
        column_names.push_back(col);
      }
    }
    agg_ops.push_back( {column_names, agg_ops_vec[i]} );
  }

}

/**
 * Resample helper: Validate the names and types of the aggregators.
 *
 * \param[in] sframe          SFrame being operated on.
 * \param[in] agg_ops           Aggregate ops.
 * \param[in] interpolation_fn  Interpolation functions.
 */
void validate_aggregators_and_interpolators(
        const gl_sframe& sframe,
        const std::vector<std::pair<std::vector<std::string>,
               std::shared_ptr<group_aggregate_value>>>& agg_ops,
        const interpolator_type& interpolation_fn) {

  std::vector<std::string> source_column_names = sframe.column_names();
  std::vector<flex_type_enum> source_types = sframe.column_types();
  DASSERT_EQ(source_types.size(), source_column_names.size());

  std::map<std::string, size_t> source_column_to_index;
  for (size_t i = 0;i < source_column_names.size(); ++i) {
    source_column_to_index[source_column_names[i]] = i;
  }
  DASSERT_EQ(source_column_names.size(), source_column_to_index.size());

  // Check that each aggregate operation is valid.
  for (const auto& agg_op: agg_ops) {
    // check that the column name is valid
    if (agg_op.first.size() > 0) {
      for(unsigned int index = 0; index < agg_op.first.size(); index++) {
        auto& col_name = agg_op.first[index];
        if (!source_column_to_index.count(col_name)) {
          log_and_throw(
              "Timeseries does not contain the column '" + col_name + "'.");
        }

        // Skip arg operations.
        if(turi::registered_arg_functions.count(agg_op.second->name()) != 0
            && index > 0) {
          continue;
        }

        // Type validation.
        size_t column_number = source_column_to_index.at(col_name);
        if (!agg_op.second->support_type(source_types[column_number])) {
          log_and_throw("Unsupported type. Requested aggregation: " +
              agg_op.second->name() + " cannot be performed on the column " +
              col_name + ".");
        }
        if (!interpolation_fn->support_type(source_types[column_number])) {
          log_and_throw("Unsupported type. Requested interpolation : " +
              interpolation_fn->name() + " cannot be performed on the column "
              + col_name + ".");
        }
      }
    }
  }
}

/**
 *
 * Resample helper: Filter out only the relevant columns.
 *
 * \param[in] agg_ops  Aggregate operations.
 * \param[in,out] relevant_column_names Relevant columns.
 */
void get_relevant_columns(
      const std::vector<std::pair<std::vector<std::string>,
               std::shared_ptr<group_aggregate_value>>>& agg_ops,
      std::vector<std::string>& relevant_column_names) {

  std::set<std::string> agg_columns;
  for (const auto& agg_op: agg_ops) {
    for(auto& col_name : agg_op.first) {
      agg_columns.insert(col_name);
    }
  }

  for (const auto& col: agg_columns) {
    // Argmax may contain duplicate columns.
    if (col != "" && std::find(relevant_column_names.begin(),
          relevant_column_names.end(), col) == relevant_column_names.end()) {
      relevant_column_names.push_back(col);
    }
  }
}

/**
 * Resample helper: Get the return column types from the aggregates.
 *
 * \param[in]     sframe            SFrame being operated on.
 * \param[in]     ret_column_names  Relevant column names.
 * \param[in,out] agg_ops           Interpolation functions.
 * \param[in,out] ret_column_types  Relevant column types.
 */
void get_return_column_types(
      const gl_sframe& sframe,
      const std::vector<std::string>& ret_column_names,
      std::vector<std::pair<std::vector<std::string>,
               std::shared_ptr<group_aggregate_value>>>& agg_ops,
      interpolator_type& interpolation_fn,
      std::vector<flex_type_enum>& ret_column_types) {

  std::vector<flex_type_enum> source_types = sframe.column_types();

  for (const auto& agg_op: agg_ops) {
    std::vector<flex_type_enum> input_types;
    for(auto col_name : agg_op.first) {
      size_t id = sframe.column_index(col_name);
      input_types.push_back(source_types[id]);
    }
    auto output_type = agg_op.second->set_input_types(input_types);
    ret_column_types.push_back(interpolation_fn->set_input_types({output_type}));
  }
}

/**
 * Resample helper: Index the column used by each aggregate.
 *
 * \param[in] sframe             SFrame being operated on.
 * \param[in,out] agg_ops        Interpolation functions.
 * \param[in,out] agg_op_col_ids Column IDs (needed for writing)
 */
void get_column_ids_for_aggregates(
     const gl_sframe& sframe,
     const std::vector<std::pair<std::vector<std::string>,
               std::shared_ptr<group_aggregate_value>>>& agg_ops,
     std::vector<std::vector<size_t>>& agg_op_col_ids) {

  for (const auto& agg_op: agg_ops) {
    std::vector<size_t> cids;
    for(auto col_name : agg_op.first) {
      size_t id = sframe.column_index(col_name);
      cids.push_back(id);
    }

    // Use index if the aggregator doesn't need a column. (e.g. COUNT)
    if (cids.size() > 0) {
      agg_op_col_ids.push_back(cids);
    } else {
      agg_op_col_ids.push_back({0});
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
//
//                     Timeseries functions.
//
//////////////////////////////////////////////////////////////////////////////

gl_timeseries::~gl_timeseries() {}

gl_sarray date_range(const flexible_type &start_time,const flexible_type
    &end_time,const flexible_type & period) {
    gl_sarray_writer writer(flex_type_enum::DATETIME,1);
    flexible_type current_time(start_time);

    while(current_time <= end_time) {
      writer.write(current_time,0);
      current_time += period;
    }
    return writer.close();
}

bool _check_sorted(const gl_sarray & input_sarray,const std::string & flag) {
  auto range = input_sarray.range_iterator();
  auto iter = range.begin();
  enum ORDER {ASCENDING,DESCENDING};
  ORDER order = ASCENDING;
  if(flag == "desc")
    order = DESCENDING;

  if (iter == range.end())
    return true;

  flexible_type old_element = *iter;
  ++iter;
  while(iter != range.end()) {
    const auto & elem = *iter;
    if(order == ASCENDING) {
      if (old_element >= elem)
        return false;
    } else if(order == DESCENDING) {
      if (old_element <= elem)
        return false;
    }
    old_element = elem;
    ++iter;
  }
  return true;
}

/**
 * Get a version for the object.
 */
void gl_timeseries::load_version(iarchive& iarc, size_t version) {
  if (version > get_version()) {
    log_and_throw(
      "This model version cannot be loaded. Please re-save your model.");
  }

  // Everything else
  iarc >> m_index_col_name
       >> m_value_col_names
       >> m_initialized;

  const std::string & prefix = iarc.get_prefix();
  m_sframe = gl_sframe(prefix);
}

/**
 * Save the object using Turi's oarc.
 */
void gl_timeseries::save_impl(oarchive& oarc) const {
  oarc << m_index_col_name
       << m_value_col_names
       << m_initialized;
  const std::string & prefix = oarc.get_prefix();
  m_sframe.save(prefix);
}

void gl_timeseries::init(const gl_sframe & input_sf, const std::string & name,
    bool is_sorted,std::vector<int64_t> ranges) {

  if(m_initialized) {
    log_and_throw("Timeseries is already initialized");
  }

  m_index_col_name = name;
  std::vector<std::string> all_column_names = input_sf.column_names();
  bool does_index_col_exist = false;
  for(size_t i=0;i<all_column_names.size();i++) {
    if(all_column_names[i] != m_index_col_name) {
      m_value_col_names.push_back(all_column_names[i]);
    } else {
      does_index_col_exist = true;
    }
  }

  if(!does_index_col_exist) {
    log_and_throw(std::string(
        "The index column '" + name + "' does not exist in the input sframe."));
  }
  if(input_sf[name].dtype() != flex_type_enum::DATETIME &&
     input_sf[name].dtype() != flex_type_enum::INTEGER) {
    log_and_throw(std::string(
        "The index column '" + name + "' must be of type flex_int or flex_date_time"));
  }
  if(ranges.size() < 2) {
    log_and_throw(std::string(
        "The ranges argument should have at least two elements"));
  }

  gl_sframe refined_input_sf = input_sf;
  if(ranges[0] > 0 || ranges[1] > 0) {
    if(ranges[0] < 0)
      ranges[0] = 0;
    if(ranges[1] < 0)
      ranges[1] = input_sf.size();
    refined_input_sf = input_sf[{ranges[0],ranges[1]}];
  }
  std::string asc("asc");
  if (!is_sorted && !_check_sorted(refined_input_sf[m_index_col_name],asc)) {
    logstream(LOG_INFO) << "index column " << m_index_col_name
                        << " is not sorted. We will sort it." << std::endl;

    // TODO: Temporary row_number column to do a stable sort, better solution!
    std::vector<std::string> sort_col_names;
    sort_col_names.push_back(m_index_col_name);
    sort_col_names.push_back("_temp_row_num_used_for_stable_sorting");
    refined_input_sf =
      refined_input_sf.add_row_number("_temp_row_num_used_for_stable_sorting")
                      .sort(sort_col_names,true);
    refined_input_sf.remove_column("_temp_row_num_used_for_stable_sorting");
  }

  // for convenience reasons we restructure the input sframe such that
  // index column to be the first column.
  m_sframe.add_column(refined_input_sf[m_index_col_name],m_index_col_name);
  m_sframe.add_columns(refined_input_sf[m_value_col_names]);
  m_initialized = true;
}

/**
 * Python wrapper for resampling.
 */
gl_timeseries gl_timeseries::resample_wrapper(
        double period,
        const flex_list& downsample_params,
        const flex_list& upsample_params,
        const std::string& label,
        const std::string& close) const {
  DASSERT_EQ(downsample_params.size(), 3);
  DASSERT_EQ(upsample_params.size(), 1);

  // Parse downsample_params
  // Columns on which the aggregate is performed.
  std::vector<std::vector<std::string>> ds_columns;
  for (const auto& lc: downsample_params.at(0).get<flex_list>()) {
    std::vector<std::string> lst;
    for(const auto& c: lc.get<flex_list>()) {
      lst.push_back(c);
    }
    ds_columns.push_back(lst);
  }

  // Output columns for the aggregate.
  std::vector<std::string> ds_output_columns;
  for (const auto& c: downsample_params.at(1).get<flex_list>()) {
    ds_output_columns.push_back(c);
  }

  // Downsample operations.
  std::vector<std::string> ds_ops;
  for (const auto& c: downsample_params.at(2).get<flex_list>()) {
    ds_ops.push_back(c);
  }

  // Parse upsample_params
  interpolator_type int_op = get_builtin_interpolator(upsample_params[0]);

  // Check that output column names are all unique (skip empty ones)
  std::unordered_set<std::string> all_cols;
  for (auto s: ds_output_columns) {
    if (!s.empty()) {
      if (std::find(all_cols.begin(), all_cols.end(), s) != all_cols.end()) {
        log_and_throw(
         "Downsampling output column names must be unique. "+s+" is repeated.");
      } else {
        all_cols.insert(s);
      }
    }
  }

  // Construct the aggregate operators.
  std::vector<std::string> ret_column_names {m_index_col_name};
  std::map<std::string, aggregate::groupby_descriptor_type> operators;
  DASSERT_EQ(ds_columns.size(), ds_output_columns.size());
  DASSERT_EQ(ds_columns.size(), ds_ops.size());
  for (size_t i = 0; i < ds_columns.size(); i++) {
    const auto& agg_op = aggregate::groupby_descriptor_type(
                                             ds_ops[i], ds_columns[i]);
    std::string candidate_name = ds_output_columns[i];
    const std::string& agg_op_display_name = agg_op.m_aggregator->name();

    // For empty column names, provide a name.
    if (candidate_name.empty()) {
      std::string root_candidate_name = "";
      if(turi::registered_arg_functions.count(agg_op_display_name) == 0) {
        for (auto& col_name: agg_op.m_group_columns) {
          if (root_candidate_name.empty()) {
            if (col_name != "") {
              root_candidate_name += " of " + col_name;
            }
          } else {
            root_candidate_name += "_" + col_name;
          }
        }
        root_candidate_name = agg_op_display_name + root_candidate_name;
      } else {
        if(agg_op.m_group_columns.size() != 2) {
          log_and_throw("Arg functions takes exactly two arguments.");
        }
        const std::string& c1 = agg_op.m_group_columns[0];
        const std::string& c2 = agg_op.m_group_columns[1];
        root_candidate_name += c2 + " for " + agg_op_display_name + " of " + c1;
      }
      candidate_name = root_candidate_name;

      // Keep trying to come up with a unique column name
      size_t ctr = 1;
      while (std::find(ret_column_names.begin(),
                       ret_column_names.end(),
                       candidate_name) != ret_column_names.end()) {
        candidate_name = root_candidate_name + "." + std::to_string(ctr);
        ++ctr;
      }
    }
    operators[candidate_name] = agg_op;
  }


  // Call the resample method of the underlying timeseries.
  return this->resample(period, operators, int_op, label, close);
}



gl_timeseries gl_timeseries::resample(const flex_float& period,
      const std::map<std::string,
           aggregate::groupby_descriptor_type>& operators,
      interpolator_type interpolation_fn,
      const std::string& label,
      const std::string& closed) const {

  _check_if_initialized();

  // Convert the inputs into the right units & enums.
  // --------------------------------------------------------------------------
  const size_t MICROSECONDS = 1000000;
  const size_t micro_period = period * MICROSECONDS;
  if (m_sframe.size() == 0) {
    return *this;
  }

  enum LABEL_OFFSET {LABEL_LEFT, LABEL_RIGHT};
  LABEL_OFFSET label_offset;
  if(label == "right") {
    label_offset = LABEL_RIGHT;
  } else {
    label_offset = LABEL_LEFT;
  }
  enum CLOSE_METHOD {LEFT, RIGHT};
  CLOSE_METHOD cm;
  if (closed == "right") {
    cm = RIGHT;
  } else {
    cm = LEFT;
  }

  // Parse & validate the input.
  // --------------------------------------------------------------------------
  std::vector<std::pair<std::vector<std::string>,
                   std::shared_ptr<group_aggregate_value>>> agg_ops;
  std::vector<std::string> ret_column_names {m_index_col_name};
  parse_split_log_operators(m_sframe, operators, agg_ops, ret_column_names);

  // Validate the aggregates & column names.
  validate_aggregators_and_interpolators(m_sframe, agg_ops, interpolation_fn);

  // Filter out only the used columns.
  std::vector<std::string> input_column_names {m_index_col_name};
  get_relevant_columns(agg_ops, input_column_names);
  gl_sframe relevant_sframe = m_sframe[input_column_names];


  // Prepare the output time-series.
  // --------------------------------------------------------------------------
  std::vector<flex_type_enum> ret_column_types {this->get_index_col_type()};
  get_return_column_types(relevant_sframe, ret_column_names,
      agg_ops, interpolation_fn, ret_column_types);

  std::vector<std::vector<size_t>> agg_op_col_ids;
  get_column_ids_for_aggregates(relevant_sframe, agg_ops, agg_op_col_ids);

  // Resample code!
  // --------------------------------------------------------------------------
  DASSERT_EQ(ret_column_names.size(), ret_column_types.size());
  gl_sframe_writer writer(ret_column_names, ret_column_types, 1);

  // Assume uniform timezone.
  flex_date_time first_time = relevant_sframe[m_index_col_name][0];
  int32_t tz = first_time.time_zone_offset();

  // timestamp -> bucket_id
  auto get_bucket_id = [=](const flex_date_time& t) {
    if (cm == LEFT) {
      return (t.posix_timestamp() * MICROSECONDS + t.microsecond()) /
        micro_period;
    } else {
      return (-1 + t.posix_timestamp() * MICROSECONDS + t.microsecond()) /
        micro_period;
    }
  };

  // bucket_id -> timestamp
  auto get_timestamp = [=](const size_t bucket_id) {
    int64_t idx = 0;
    if (label_offset == LABEL_LEFT) {
      idx = bucket_id * micro_period;
    } else {
      idx = (bucket_id + 1) * micro_period;
    }
    return flex_date_time(idx/MICROSECONDS, tz, idx % MICROSECONDS);
  };

  // Initialize bucket_ids, and indices.
  size_t curr_bucket = get_bucket_id(first_time);
  size_t prev_bucket = size_t(-1);
  flexible_type curr_index = first_time;
  size_t ret_size = ret_column_names.size();
  std::vector<flexible_type> curr_values(ret_size, FLEX_UNDEFINED);
  std::vector<flexible_type> prev_values(ret_size, FLEX_UNDEFINED);

  auto range = relevant_sframe.range_iterator();
  auto iter = range.begin();
  DASSERT_TRUE(iter->size() > 0);

  while(true) {
    const auto& elem = *iter;
    curr_index = elem[0].get<flex_date_time>();
    size_t bucket_id = get_bucket_id(curr_index);

    // Current bucket done. Wrap up.
    if ((bucket_id != curr_bucket) || (iter == range.end())) {
      DASSERT_EQ(curr_values.size(), ret_size);
      DASSERT_EQ(prev_values.size(), ret_size);

      curr_values[0] = get_timestamp(curr_bucket);
      for (size_t i = 0; i < agg_ops.size(); i++) {
        DASSERT_LT(i + 1, curr_values.size());
        DASSERT_LT(i, agg_ops.size());
        curr_values[i + 1] = (agg_ops[i]).second->emit();
        (agg_ops[i].second).reset((agg_ops[i].second)->new_instance());
      }

      // Write all previous buckets.
      if (prev_bucket != size_t(-1)) {
        // Write previous bucket.
        writer.write(prev_values, 0);

        // Write all intermediate buckets (interpolation).
        std::vector<flexible_type> interp(ret_size, FLEX_UNDEFINED);
        for (size_t bid = prev_bucket + 1; bid < curr_bucket; ++bid) {
          DASSERT_EQ(interp.size(), prev_values.size());
          DASSERT_EQ(interp.size(), curr_values.size());
          interp[0] = get_timestamp(bid);
          for (size_t i = 1; i < interp.size(); i++) {
            interp[i] = interpolation_fn->interpolate(interp[0], prev_values[0],
                curr_values[0], prev_values[i], curr_values[i]);
          }
          writer.write(interp, 0);
        }
      }

      // Write final bucket.
      if (iter == range.end()) {
        writer.write(curr_values, 0);
      }
      prev_values = curr_values;
      prev_bucket = curr_bucket;
    }

    // No more points left. Write the final bucket.
    if (iter == range.end()) {
      break;
    }

    // Aggregate!
    DASSERT_EQ(agg_ops.size(), agg_op_col_ids.size());
    for (size_t i = 0; i < agg_ops.size(); i++) {
      if (agg_op_col_ids[i].size() == 1) {
        size_t id = agg_op_col_ids[i][0];
        (agg_ops[i].second)->add_element_simple(elem[id]);
      } else {
        DASSERT_EQ(agg_op_col_ids[i].size(), 2);
        size_t id1 = agg_op_col_ids[i][0];
        size_t id2 = agg_op_col_ids[i][1];
        (agg_ops[i].second)->add_element({elem[id1], elem[id2]});
      }
    }
    ++iter;
    curr_bucket = bucket_id;
  }

  // Convert to timeseries.
  gl_timeseries g_ts;
  g_ts.init(writer.close(), m_index_col_name, true);
  return g_ts;

}

gl_timeseries gl_timeseries::tshift(const flex_float & delta) {
  _check_if_initialized();
  auto sa_index = m_sframe[m_index_col_name];
  auto sa_index_shifted = sa_index + delta;

  auto shifted_sframe = m_sframe[m_value_col_names];
  shifted_sframe.add_column(sa_index_shifted,m_index_col_name);

  gl_timeseries g_ts;
  g_ts.init(shifted_sframe,m_index_col_name,true);
  return g_ts;

}

gl_timeseries gl_timeseries::shift(const int64_t & steps){
  _check_if_initialized();

  if(steps == 0)
    return *this;

  auto sframe_no_index = m_sframe[m_value_col_names];
  auto len_sf = sframe_no_index.size();
  auto num_missing = std::abs(steps);

  // The part of the SFrame that will be part of the result. The rest of the
  // rows will be missing values. sf_cut.size() + num_missing == len_sf
  auto sf_cut = gl_sframe();
  if (steps < 0) {
    sf_cut = sframe_no_index[{num_missing,int64_t(len_sf)}];
  } else {
    sf_cut = sframe_no_index[{0,int64_t(len_sf) - num_missing}];
  }

  // Fill a correctly-typed SArray with missing values for each value column
  std::vector<flexible_type> none_input;
  for(int i = 0; i < num_missing; i++) {
    none_input.push_back(flex_undefined());
  }

  std::map< std::string,gl_sarray> none_sas;
  for(auto &val_name : m_value_col_names) {
    gl_sarray none_sa(none_input, sframe_no_index[val_name].dtype());
    none_sas[val_name] = none_sa;
  }

  gl_sframe none_sf(none_sas);
  auto result_sframe = gl_sframe();
  if(steps < 0)
    result_sframe = sf_cut.append(none_sf);
  else
    result_sframe = none_sf.append(sf_cut);

  // Add back the index
  result_sframe.add_column(m_sframe[m_index_col_name],m_index_col_name);

  gl_timeseries g_ts;
  g_ts.init(result_sframe,m_index_col_name,true);
  return g_ts;
}

// This helper function starts writing the remaining rows of one of
// the input sframes to the output sframe. It starts from the
// current position of the sframe iterator and fills columns of the
// output sframe started from the offset index.
void _write_remaining_of_sframe(std::vector<flexible_type> & v,
                  int offset,
                  std::shared_ptr<gl_sframe_range> & range,
                  std::shared_ptr<gl_sframe_range::iterator> & range_iter,
                  gl_sframe_writer & w) {

  while((*range_iter) != range->end()) {
    const auto & elem = *(*range_iter);
    size_t num_cols = elem.size();

    for(size_t j=0;j< (num_cols-1);j++) {
        v[offset + j] = elem[j+1];
    }
    v[0] = elem[0];
    ++(*range_iter);
    w.write(v,0);
  }
}

gl_timeseries gl_timeseries::ts_union(const gl_timeseries & other_ts) {

  std::vector<std::string> col_names; // return column names.
  std::vector<flex_type_enum> col_types; // return column types.

  const std::vector<std::string>& ref_col_names =
    this->m_sframe.column_names();
  const std::vector<flex_type_enum>& ref_col_types =
    this->m_sframe.column_types();
  const std::vector<std::string>& other_col_names =
    other_ts.m_sframe.column_names();
  std::unordered_set<std::string> ref_col_names_set(ref_col_names.begin(),
      ref_col_names.end());

  const auto & other_size = other_ts.m_sframe.size();
  const auto & this_size = this->m_sframe.size();

  if(other_size == 0) {
    return *this;
  }
  if(this_size == 0) {
    return other_ts;
  }
  if(ref_col_names.size() != other_col_names.size()) {
    log_and_throw("The two TimeSeries must have the same number of columns");
  }

  // check if column names match
  for(size_t j=1;j< (other_col_names.size());j++) {
      if(ref_col_names_set.find(other_col_names[j]) == ref_col_names_set.end()) {
        log_and_throw("Column name '" + other_col_names[j] +
         "' in the second TimeSeries does not exist in the first TimeSeries.");
      }
  }

  // reorder columns of the sframe for other_ts if it is needed
  gl_sframe other_sf = other_ts.m_sframe.select_columns(ref_col_names);

  // check if column types match
  for(size_t j=0;j< ref_col_names.size();j++) {
      if(other_sf[ref_col_names[j]].dtype() !=
          this->m_sframe[ref_col_names[j]].dtype()) {
         log_and_throw("Type of the column '" + ref_col_names[j] +
             "' in two TimeSeries being combined.");
      }
  }

  const std::string& index_col_name = this->get_index_col_name();
  const flexible_type& min_index_this = this->m_sframe[index_col_name][0];
  const flexible_type& max_index_this =
    this->m_sframe[index_col_name][this_size-1];
  const flexible_type& min_index_other = other_sf[index_col_name][0];
  const flexible_type& max_index_other = other_sf[index_col_name][other_size-1];

  std::vector<gl_sframe> input_sframes = {this->m_sframe,other_sf};
  // if two gl_timeseries do not overlap, then collapse to append() operation.
  gl_timeseries g_ts;
  if(max_index_this <= min_index_other) {
    auto appended_sframe = input_sframes[0].append(input_sframes[1]);
    appended_sframe.materialize();
    g_ts.init(appended_sframe,this->get_index_col_name(),true);
    return g_ts;
  }
  else if (max_index_other <= min_index_this) {
    auto appended_sframe = input_sframes[1].append(input_sframes[0]);
    appended_sframe.materialize();
    g_ts.init(appended_sframe,this->get_index_col_name(),true);
    return g_ts;
  }

  std::vector<std::shared_ptr<gl_sframe_range>> sframe_range;
  std::vector<std::shared_ptr<gl_sframe_range::iterator>> sframe_range_iter;
  std::vector<flexible_type> values(input_sframes[0].num_columns(),
      flex_undefined());
  gl_sframe_writer writer(ref_col_names,ref_col_types, 1);

  for(size_t i=0;i < input_sframes.size();i++) {
    sframe_range.push_back(
        std::make_shared<gl_sframe_range>(input_sframes[i].range_iterator()));
    sframe_range_iter.push_back(
        std::make_shared<gl_sframe_range::iterator>((*sframe_range[i]).begin()));
  }

  // This is the simple implementation for two-way union.
  // For multi-way union this implementation must change.
  bool first_ts_finished = false;
  bool second_ts_finished = false;

  while(true) {
      std::fill(values.begin(),values.end(),flex_undefined());
      if((*sframe_range_iter[0]) == (*sframe_range[0]).end()) {
        first_ts_finished = true;
      }
      if((*sframe_range_iter[1]) == (*sframe_range[1]).end()) {
        second_ts_finished = true;
      }

      if(first_ts_finished && second_ts_finished) break;

      if(second_ts_finished){
        _write_remaining_of_sframe(values,1,sframe_range[0],sframe_range_iter[0],
            writer);
        break;
      }
      if(first_ts_finished){
        _write_remaining_of_sframe(values,1,sframe_range[1],sframe_range_iter[1],
            writer);
        break;
      }

      const auto & elem1 = *(*sframe_range_iter[0]);
      const auto & elem2 = *(*sframe_range_iter[1]);

      if(elem1[0] <= elem2[0]) {
          for(size_t j=0;j< elem1.size();j++) values[j] = elem1[j];
          ++(*sframe_range_iter[0]);
      }
      else {
          for(size_t j=0;j< elem2.size();j++) values[j] = elem2[j];
          ++(*sframe_range_iter[1]);
      }
      writer.write(values,0);
  }

  g_ts.init(writer.close(),this->get_index_col_name(),true);
  return g_ts;
}

gl_timeseries gl_timeseries::index_join(const gl_timeseries & other_ts,const
    std::string & how, const std::string & index_column_name) {

  std::vector<std::shared_ptr<gl_sframe_range>> sframe_range;
  std::vector<std::shared_ptr<gl_sframe_range::iterator>> sframe_range_iter;
  std::vector<gl_sframe> input_sframes = {this->m_sframe,other_ts.m_sframe};

  std::vector<std::string> col_names;
  std::vector<flex_type_enum> col_types;

  size_t num_cols_join_ts = 1;
  col_names.push_back(index_column_name);
  col_types.push_back(flex_type_enum::DATETIME);
  std::set<std::string> column_name_set;

  for (size_t i=0;i < input_sframes.size();i++) {
    std::vector<std::string> cur_col_names = input_sframes[i].column_names();
    for(size_t j=1;j< (cur_col_names.size());j++) {
      // disambiguate column names in join
      if(column_name_set.find(cur_col_names[j]) != column_name_set.end()) {
        col_names.push_back(std::string(cur_col_names[j]+".").append(
              std::to_string(i)));
      }
      else {
        col_names.push_back(cur_col_names[j]);
        column_name_set.insert(cur_col_names[j]);
      }
      col_types.push_back(input_sframes[i][cur_col_names[j]].dtype());
    }
    num_cols_join_ts += input_sframes[i].num_columns() - 1;
  }
  std::vector<flexible_type> values(num_cols_join_ts,flex_undefined());

  gl_sframe_writer writer(col_names,col_types, 1);

  //This enum keeps track of 'how' method and avoids string comparison later on.
  enum JOIN_TYPE { LEFT, INNER, OUTER, RIGHT};
  JOIN_TYPE jt;
  if (how == "inner") jt = INNER;
  else if(how == "left") jt = LEFT;
  else if(how == "right") jt = RIGHT;
  else jt = OUTER;

  for(size_t i=0;i < input_sframes.size();i++) {
    sframe_range.push_back(
        std::make_shared<gl_sframe_range>(input_sframes[i].range_iterator()));
    sframe_range_iter.push_back(
        std::make_shared<gl_sframe_range::iterator>((*sframe_range[i]).begin()));
  }

  gl_timeseries g_ts;
  // This is the simple implementation for two-way join.
  // For multi-way join this implementation must change.
  bool first_ts_finished = false;
  bool second_ts_finished = false;

  while(true) {
      std::fill(values.begin(),values.end(),flex_undefined());
      if((*sframe_range_iter[0]) == (*sframe_range[0]).end()) {
        first_ts_finished = true;
      }
      if((*sframe_range_iter[1]) == (*sframe_range[1]).end()) {
        second_ts_finished = true;
      }

      if(first_ts_finished && second_ts_finished) break;

      if(second_ts_finished){
        if(jt == OUTER || jt == LEFT) {
          _write_remaining_of_sframe(values,1,sframe_range[0],
              sframe_range_iter[0], writer);
        }
        break;
      }
      if(first_ts_finished){
        if(jt == OUTER || jt == RIGHT) {
          _write_remaining_of_sframe(values, input_sframes[0].num_columns(),
              sframe_range[1],sframe_range_iter[1],writer);
        }
        break;
      }

      const auto & elem1 = *(*sframe_range_iter[0]);
      const auto & elem2 = *(*sframe_range_iter[1]);

      bool equal = (elem1[0] == elem2[0]);
      size_t offset = input_sframes[0].num_columns();
      bool should_output_tuple = false;
      if (equal) {
        for(size_t j=0;j< elem1.size();j++) values[j] = elem1[j];
        ++(*sframe_range_iter[0]);

        for(size_t j=1;j< elem2.size();j++) values[j+offset-1] = elem2[j];
        ++(*sframe_range_iter[1]);
        should_output_tuple = true;
      }
      else {
        if(elem1[0] < elem2[0]) {
          if(jt==LEFT || jt==OUTER) {
            for(size_t j=0;j< elem1.size();j++) values[j] = elem1[j];
            should_output_tuple = true;
          }
          ++(*sframe_range_iter[0]);
        }
        else {
          if(jt == RIGHT || jt==OUTER) {
            for(size_t j=1;j< elem2.size();j++) values[j+offset-1] = elem2[j];
            values[0] = elem2[0];
            should_output_tuple = true;
          }
          ++(*sframe_range_iter[1]);
        }
      }

      if(should_output_tuple) {
        writer.write(values,0);
      }
  }

  g_ts.init(writer.close(),index_column_name,true);
  return g_ts;
}

gl_timeseries gl_timeseries::slice(const flexible_type &start_time, const
    flexible_type &end_time, const std::string &closed) const {

  if(start_time.get_type() != flex_type_enum::DATETIME) {
    log_and_throw("Parameter 'start_time' must be flex_date_time");
  }
  if(end_time.get_type() != flex_type_enum::DATETIME) {
    log_and_throw("Parameter 'end_time' must be flex_date_time");
  }

  auto index_col = m_sframe[m_index_col_name];
  gl_sarray sel1;
  gl_sarray sel2;
  if(closed == "left") {
    sel1 = (index_col >= start_time);
    sel2 = (index_col < end_time);
  } else if(closed == "right") {
    sel1 = (index_col > start_time);
    sel2 = (index_col <= end_time);
  } else if(closed == "both") {
    sel1 = (index_col >= start_time);
    sel2 = (index_col <= end_time);
  } else if(closed == "neither") {
    sel1 = (index_col > start_time);
    sel2 = (index_col < end_time);
  } else {
    log_and_throw("Invalid value for parameter 'closed'");
  }

  gl_sframe range_sf = m_sframe[sel1 && sel2];
  gl_timeseries ret_ts;
  ret_ts.init(range_sf, m_index_col_name, true);
  return ret_ts;
}

gl_grouped_timeseries gl_timeseries::group(std::vector<std::string> key_columns) {
  gl_grouped_timeseries ret;
  ret.group(this->get_sframe(), this->m_index_col_name, key_columns);
  return ret;
}

void gl_timeseries::add_column(const gl_sarray& data, const std::string& name) {
   m_sframe.add_column(data,name);
   std::vector<std::string> all_column_names = m_sframe.column_names();
   all_column_names.erase(std::remove(all_column_names.begin(),
         all_column_names.end(), m_index_col_name), all_column_names.end());
   m_value_col_names = all_column_names;
}

void gl_timeseries::remove_column(const std::string& name) {
  if (name == m_index_col_name) {
    log_and_throw("Index column cannot be removed.");
  }
  m_sframe.remove_column(name);
  m_value_col_names.erase(std::remove(m_value_col_names.begin(),
        m_value_col_names.end(),name), m_value_col_names.end());

}

} // timeseries
} // turicreate
