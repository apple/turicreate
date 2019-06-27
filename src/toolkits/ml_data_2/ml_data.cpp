/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/ml_data_iterators.hpp>
#include <toolkits/ml_data_2/data_storage/ml_data_row_format.hpp>
#include <toolkits/ml_data_2/data_storage/ml_data_row_translation.hpp>
#include <toolkits/ml_data_2/data_storage/ml_data_block_manager.hpp>
#include <toolkits/ml_data_2/side_features.hpp>
#include <toolkits/ml_data_2/data_storage/util.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

using namespace turi::v2::ml_data_internal;

namespace turi { namespace v2 {

////////////////////////////////////////////////////////////////////////////////
//
//  The ml_data internal stuff.
//
////////////////////////////////////////////////////////////////////////////////

ml_data::ml_data(const std::map<std::string, flexible_type>& options)
    : _metadata(nullptr)
    , incoming_data(new _data_for_filling)
{
  incoming_data->options = default_options();

  for(const auto& p : options) {
    if(!incoming_data->options.count(p.first)) {
      ASSERT_MSG(false, (std::string("Option ") + p.first + " not recognized; "
                         + "if new, please add to default_options() in ml_data base class.").c_str());
    }

    incoming_data->options[p.first] = p.second;
  }

  incoming_data->immutable_metadata = false;
}

/**
 *  Construct an ml_data object based on previous ml_data metadata.
 */
ml_data::ml_data(const std::shared_ptr<ml_metadata>& __metadata,
                 bool immutable_metadata)
    : _metadata(__metadata)
    , incoming_data(new _data_for_filling)
{
  incoming_data->options = default_options();  //

  for(const auto& p : __metadata->options)
    incoming_data->options.insert(p);

  incoming_data->immutable_metadata = immutable_metadata;
}

// ml_data is cheap to copy.  However, it cannot be copied before
// fill() is called.

ml_data::ml_data(const ml_data& other) {

  // This checks to make sure it's copyable.
  *this = other;
}

const ml_data& ml_data::operator=(const ml_data& other) {

  ASSERT_MSG(other.incoming_data == nullptr,
             "ml_data cannot be copied until filling is done.");

  _metadata                   = other._metadata;
  rm                          = other.rm;
  side_features               = other.side_features;
  _row_start                  = other._row_start;
  _row_end                    = other._row_end;
  _original_num_rows          = other._original_num_rows;
  _max_row_size               = other._max_row_size;
  row_block_size              = other.row_block_size;
  data_blocks                 = other.data_blocks;
  block_manager               = other.block_manager;
  untranslated_columns        = other.untranslated_columns;

  incoming_data.reset(nullptr);

  return *this;
}

/**  Sets the data source.
 *
 *   If target_column is null, then there is no target column.
 */
void ml_data::set_data(const sframe& data,
                       const std::string& target_column_name,
                       const std::vector<std::string>& partial_column_ordering,
                       const column_mode_map mode_overrides) {

  ASSERT_MSG(incoming_data != nullptr,
             "set_data called out of order; cannot be called after fill() or load().");

  incoming_data->data                 = data;
  incoming_data->target_column_name   = target_column_name;
  incoming_data->column_ordering      = partial_column_ordering;
  incoming_data->mode_overrides       = mode_overrides;
}



/**  Sets the data source.
 *
 *   An overload of the previous one.  Here, the target is supplied separately
 *   as a one-column sframe.
 *
 */
void ml_data::set_data(const sframe& data,
                       const sframe& target,
                       const std::vector<std::string>& partial_column_ordering,
                       const column_mode_map mode_overrides) {

  ASSERT_MSG(incoming_data != nullptr,
             "set_data called out of order; cannot be called after fill() or load().");

  // If it's just an empty sframe, treat target column as
  // non-existant.
  if(target.num_columns() == 0) {
    set_data(data, "", partial_column_ordering, mode_overrides);
  }

  if(target.num_columns() != 1) {
    log_and_throw("Target SFrame can only be a single column.");
  }

  // Verify the target column is correct.
  std::string target_column_name = target.column_name(0);

  if(data.contains_column(target_column_name)) {
    log_and_throw((std::string("Target column has same name as column in data SFrame (")
                   + target_column_name + ")").c_str());
  }

  set_data(data.add_column(target.select_column(0), target_column_name),
           target_column_name,
           partial_column_ordering,
           mode_overrides);
}

/**  Add in the side data to the mix.
 *
 */
void ml_data::add_side_data(const sframe& data,
                            const std::string& forced_join_column,
                            const column_mode_map mode_overrides) {

  ASSERT_MSG(incoming_data != nullptr,
             "add_side_data called out of order; cannot be called after fill() or load().");

  if(data.num_columns() == 0)
    return;

  incoming_data->incoming_side_features.push_back(
      {data, forced_join_column, mode_overrides});
}

/** Convenience function -- short for calling set_data(data,
 * target_column), then fill().
 */
void ml_data::fill(const sframe& data,
                   const std::string& target_column) {

  set_data(data, target_column);
  fill();
}


/** Convenience function -- short for calling set_data(data,
 *  target), then fill().
 */
void ml_data::fill(const sframe& data,
                   const sframe& target) {

  set_data(data, target);
  fill();
}

/** Call this function when all the data is added.  This executes
 *  the filling process based on everything given.
 */
void ml_data::fill() {

  ASSERT_MSG(incoming_data != nullptr,
             "fill called out of order; cannot be called twice or after load().");

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1.  Set up the metadata if need be.

  bool in_training_mode;

  if(_metadata == nullptr) {
    in_training_mode = true;
    _setup_ml_metadata();
  } else {
    in_training_mode = false;
  }

  ASSERT_TRUE(incoming_data != nullptr);

  bool immutable_metadata = incoming_data->immutable_metadata;
  const sframe& raw_data = incoming_data->data;
  bool empty_incoming_data = (raw_data.num_rows() == 0);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2. Deal with the side features.  If there are new ones, we
  // need to add them in.

  side_features = _metadata->side_features;

  if(!in_training_mode && ! incoming_data->incoming_side_features.empty()) {

    if(side_features == nullptr) {
      if(!_metadata->options.at("ignore_new_columns_after_train"))
        log_and_throw("New side features cannot be added if not included on first ml_data_construction.");

    } else {
      // Add the new side features to the
      side_features.reset(new ml_data_side_features(*side_features));

      for(const auto& p : incoming_data->incoming_side_features) {

        side_features->add_and_index_side_data(
            p.data,
            p.mode_overrides,
            _metadata->options,
            false,
            immutable_metadata,
            p.forced_join_column);
      }
    }
  }


  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Set up the row sizes

  _row_start            = 0;
  _row_end              = raw_data.num_rows();
  _original_num_rows    = _row_end - _row_start;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: Set up metadata and input iterators

  // Set up all the proper data input sources for iterating through the data.
  // The full metadata has the target as well.  This way we can treat it just
  // like a column.  The bookkeeping here is easier, and it gets unpacked on
  // ml_data_iterator end.

  {
    const std::vector<std::string>& raw_column_names = raw_data.column_names();
    std::set<std::string> raw_column_name_set(raw_column_names.begin(), raw_column_names.end());

    std::vector<column_metadata_ptr> full_metadata = _metadata->columns;

    std::vector<std::string> missing_columns;

    for(size_t c_idx = 0; c_idx < full_metadata.size(); ++c_idx) {
      const std::string& column_name = full_metadata[c_idx]->name;
      if(raw_column_name_set.count(column_name) == 0) {
        missing_columns.push_back(column_name);
      } else {
        raw_column_name_set.erase(column_name);
      }
    }

    bool using_target = (_metadata->has_target()
                         && raw_data.contains_column(_metadata->target_column_name()));

    if(using_target)
      raw_column_name_set.erase(_metadata->target_column_name());

    // Now, do we having any missing columns?  Has anything been added
    // wrongly?

    if(!empty_incoming_data && !missing_columns.empty()) {
      std::ostringstream ss;

      ss << "Provided data missing required columns: ";

      for(size_t i = 0; i < missing_columns.size() - 1; ++i) {
        ss << missing_columns[i] << ", ";
      }
      ss << missing_columns.back() << ".";

      log_and_throw(ss.str());
    }

    if(!raw_column_name_set.empty()) {
      std::ostringstream ss;

      bool ignore = _metadata->options.at("ignore_new_columns_after_train");

      if(ignore)
        ss << "Discarding additional columns present in provided data that do not match known schema: ";
      else
        ss << "Additional columns present in provided data that do not match known schema: ";

      {
        size_t i = 0;
        for(const auto& s : raw_column_name_set) {
          ss << s;
          ++i;
          if(i != raw_column_name_set.size())
            ss << ", ";
        }
      }

      ss << ".";

      if(ignore) {
        logstream(LOG_WARNING) << ss.str();
      } else {
        log_and_throw(ss.str());
      }
    }

    if(using_target) {

      std::shared_ptr<sarray<flexible_type> > target
          = raw_data.select_column(_metadata->target_column_name());

      check_type_consistent_with_mode(
          _metadata->target_column_name(), target->get_type(), _metadata->target_column_mode());

      full_metadata.push_back(_metadata->target);
    }

    // Set up the row metadata object.  This is the main thing used
    // below.
    rm.setup(full_metadata, using_target);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 5.  Call the main filling functions.

  _fill_data_blocks(in_training_mode);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 6.  Set up the untranslated columns

  _setup_untranslated_columns(raw_data);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 7.  Finalize the metadata stuff

  if(in_training_mode) {
    _metadata->set_training_index_sizes_to_current_column_sizes();

    // Set up some of the cached values in the metadata
    _metadata->setup_cached_values();
  } else {

#ifndef NDEBUG

    for(size_t c_idx = 0; c_idx < _metadata->num_columns(); ++c_idx) {
      DASSERT_LE(_metadata->index_size(c_idx), _metadata->column_size(c_idx));
    }

#endif
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 8.  Set up the block manager

  _create_block_manager();

  ////////////////////////////////////////////////////////////////////////////////
  // Step 9.  Clear out the incoming data.

  incoming_data = std::unique_ptr<_data_for_filling>();

  DASSERT_TRUE(data_blocks != nullptr);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 9.  Perform any postprocessing steps on the data.

  const bool sort_by_first_two_columns_always
      = _metadata->options.at("sort_by_first_two_columns");

  const bool sort_by_first_two_columns_on_train
      = _metadata->options.at("sort_by_first_two_columns_on_train");

  const bool sort_by_first_two_columns
      = (sort_by_first_two_columns_always
         || (in_training_mode && sort_by_first_two_columns_on_train));

  // Now sort all the data if needed.
  if(sort_by_first_two_columns)
    _sort_user_item_data_blocks();

}

/**
 * Returns the maximum row size present in the data.  This information is
 * calculated when the data is indexed and the ml_data structure is filled.
 * A buffer sized to this is guaranteed to hold any row encountered while
 * iterating through the data.
 */
size_t ml_data::max_row_size() const {

  size_t __max_row_size = _max_row_size;

  if(side_features != nullptr)
    __max_row_size += side_features->max_additional_row_size();

  return __max_row_size;
}

/** Checks to make sure the ml_data structure is indeed iterable.
 *
 */
void ml_data::_check_is_iterable() const {
  ASSERT_MSG(incoming_data == nullptr,
             "ml_data must have fill() called before it is iterable.");

  ASSERT_MSG(_metadata != nullptr,
             "ml_data is not iterable if uninitialized.");
}


/** Return an iterator over part of the data.  See
 *  iterators/ml_data_iterator.hpp for documentation on the returned
 *  iterator.
 */
ml_data_iterator ml_data::get_iterator(
    size_t thread_idx, size_t num_threads,
    bool add_side_information_if_present,
    bool use_reference_encoding) const {

  _check_is_iterable();

  std::map<std::string, flexible_type> iter_options =
      { {"add_side_information_if_present", add_side_information_if_present},
        {"use_reference_encoding", use_reference_encoding} };

  ml_data_iterator it;

  it.setup(*this, rm, thread_idx, num_threads, iter_options);

  return it;
}


/** Return a block iterator over part of the data.  See
 *  iterators/ml_data_block_iterator.hpp for documentation on the
 *  returned iterator.
 */
ml_data_block_iterator ml_data::get_block_iterator(
    size_t thread_idx, size_t num_threads,
    bool add_side_information_if_present,
    bool use_reference_encoding) const {

  _check_is_iterable();

  std::map<std::string, flexible_type> iter_options =
      { {"add_side_information_if_present", add_side_information_if_present},
        {"use_reference_encoding", use_reference_encoding} };

  ml_data_block_iterator it;

  it.setup(*this, rm, thread_idx, num_threads, iter_options);

  return it;
}

  /** Occasionally, we need to create a tempororay indexer for a
   *  specific column.  This allows us to do just that.
   *
   */
ml_data::indexer_type ml_data::create_indexer(
      const std::string& column_name,
      ml_column_mode mode,
      flex_type_enum column_type,
      const std::string& indexer_type,
      const std::map<std::string, flexible_type>& _options) {

  std::map<std::string, flexible_type> options = default_options();

  for(const auto& p : _options) {
    if(!options.count(p.first)) {
      ASSERT_MSG(false, (std::string("Option ") + p.first + " not recognized; "
                         + "if new, please add to default_options() in ml_data base class.").c_str());
    }

    options[p.first] = p.second;
  }

  return column_indexer::factory_create(
      { {"indexer_type",             to_variant(indexer_type)},
        {"column_name",              to_variant(column_name)},
        {"mode",                     to_variant(mode)},
        {"original_column_type",     to_variant(column_type)},
        {"options",                  to_variant(options)} });
}


/**
 * Serialize the object (save).
 *
 *  \note metadata is not saved with the object; this must be saved
 *  seperately.
 */
void ml_data::save(turi::oarchive& oarc) const {

  ASSERT_MSG(incoming_data == nullptr,
             "ml_data must have fill() called before it is serializable.");

  ASSERT_TRUE(data_blocks != nullptr);

  oarc << get_version();

  bool has_new_side_features
      = (side_features.get() != _metadata->side_features.get());

  std::map<std::string, variant_type> data;

  data["_row_start"]               = to_variant(_row_start);
  data["_row_end"]                 = to_variant(_row_end);
  data["_max_row_size"]            = to_variant(_max_row_size);
  data["row_block_size"]           = to_variant(row_block_size);
  data["currently_using_target"]   = to_variant(rm.has_target);
  data["has_new_side_features"]    = to_variant(has_new_side_features);
  data["has_untranslated_columns"] = to_variant(has_untranslated_columns());

  variant_deep_save(data, oarc);

  // Save the metadata
  oarc << _metadata;

  // Save the data
  oarc << data_blocks;

  if(has_new_side_features) {
    side_features->save_without_metadata(oarc);
  }

  if(has_untranslated_columns()) {
    oarc << untranslated_columns;
  }
}

/**
 * Load the object.
 *
 * \note metadata is not saved with the object; this must be set
 *  before the ml_data object is deserialized.
 */
void ml_data::load(turi::iarchive& iarc) {
  size_t version;
  iarc >> version;
  ASSERT_TRUE(version == 1);

  if(incoming_data != nullptr) {
    incoming_data.reset(nullptr);
  }

  std::map<std::string, variant_type> data;

  variant_deep_load(data, iarc);

  _row_start                  = variant_get_value<size_t>(data["_row_start"]);
  _row_end                    = variant_get_value<size_t>(data["_row_end"]);
  _max_row_size               = variant_get_value<size_t>(data["_max_row_size"]);
  row_block_size              = variant_get_value<size_t>(data["row_block_size"]);
  bool currently_using_target = variant_get_value<bool>(data["currently_using_target"]);
  bool has_new_side_features  = variant_get_value<bool>(data["has_new_side_features"]);

  bool has_untranslated_columns = (data.count("has_untranslated_columns")
                                   ? variant_get_value<bool>(data["has_untranslated_columns"])
                                   : false);

  iarc >> _metadata;

  iarc >> data_blocks;

  if(has_new_side_features) {
    side_features.reset(new ml_data_side_features(_metadata->columns));
    side_features->load_with_metadata_present(iarc);
  } else {
    side_features = _metadata->side_features;
  }

  // Reconstruct the full_metadata

  std::vector<column_metadata_ptr> full_metadata = _metadata->columns;

  if(currently_using_target) {
    full_metadata.push_back(_metadata->target);
  }

  if(has_untranslated_columns) {
    iarc >> untranslated_columns;
  } else {
    untranslated_columns.clear();
  }

  // Set up the row metadata object.  This is the main thing used
  // below.
  rm.setup(full_metadata, currently_using_target);

  // Set up the block manager
  _create_block_manager();
}

/**
 * Create a subsampled copy of the current ml_data structure.  This
 * allows us quickly create a subset of the data to be used for things
 * like sgd, etc.
 *
 * If n_rows < size(), exactly n_rows are sampled IID from the
 * dataset.  Otherwise, a copy of the current ml_data is returned.
 *
 */
ml_data ml_data::create_subsampled_copy(size_t n_rows, size_t random_seed) const{
  size_t data_size = num_rows();

  if(n_rows >= num_rows())
    return *this;

  // Sample without replacement.  Do this in a hacktastic way
  std::vector<size_t> samples(n_rows);

  // Choose them evenly
  for(size_t i = 0; i < n_rows; ++i) {
    samples[i] = (i * data_size) / n_rows;
  }

  random::seed(random_seed);

  // Now randomize this.
  for(size_t i = 0; i < n_rows; ++i) {
    size_t lb = (i > 0) ? (samples[i - 1] + 1) : 0;
    size_t ub = (i < n_rows - 1) ? (samples[i + 1] - 1) : data_size - 1;
    samples[i] = random::fast_uniform<size_t>(lb, ub);
  }

  // Break them up into groups
  DASSERT_TRUE(std::is_sorted(samples.begin(), samples.end()));

  return select_rows(samples);
}

/**
 *  Create a copy of the current ml_data structure, selecting the rows
 *  given by selection_indices.
 *
 *  \param selection_indices A vector of row indices that must be in
 *  sorted order.  Duplicates are allowed.  The returned ml_data
 *  contains all the rows given by selection_indices.
 *
 *  \return A new ml_data object with containing only the rows given
 *  by selection_indices.
 */
ml_data ml_data::select_rows(const std::vector<size_t>& selection_indices) const{

  if(!std::is_sorted(selection_indices.begin(), selection_indices.end())) {
    ASSERT_MSG(false, "selection_indices argument needs to be in sorted order.");
  }

  size_t n_rows = selection_indices.size();

  ml_data out = *this;

  const size_t n_full_blocks = n_rows / row_block_size;
  const size_t n_remaining_rows = n_rows % row_block_size;
  const size_t n_total_blocks = n_full_blocks + ((n_remaining_rows > 0) ? 1 : 0);
  const size_t data_size = num_rows();

  size_t max_n_threads = thread::cpu_count();

  out.data_blocks.reset(new sarray<row_data_block>);
  out.data_blocks->open_for_write(max_n_threads);

  // Sample retreival function.
  auto get_sample = [&](size_t idx) {
    DASSERT_LE(idx, n_rows);
    size_t sample_idx =  (idx < n_rows) ? selection_indices[idx] : data_size;

    if(idx < n_rows)
      DASSERT_LT(sample_idx, data_size);

    return sample_idx;
  };

  // Break them up into groups

  in_parallel([&, selection_indices](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {

      size_t out_block_start_idx = (thread_idx * n_total_blocks) / num_threads;
      size_t out_block_end_idx = ((thread_idx + 1) * n_total_blocks) / num_threads;

      size_t samples_row_start = out_block_start_idx * row_block_size;
      size_t samples_row_end = std::min(out_block_end_idx * row_block_size, n_rows);

      size_t sample_first = get_sample(samples_row_start);
      size_t sample_end = samples_row_end <= n_rows ? get_sample(samples_row_end) : data_size + 1;

      size_t ml_data_row_start = sample_first;

      size_t ml_data_row_end = std::min(this->size(), sample_end + 1);

      DASSERT_LE(ml_data_row_start, ml_data_row_end);

      size_t n_rows_needed = samples_row_end - samples_row_start;

      DASSERT_TRUE(thread_idx + 1 == num_threads
                   || n_rows_needed % row_block_size == 0);

      DASSERT_LE(ml_data_row_start, ml_data_row_end);

      auto it_out = out.data_blocks->get_output_iterator(thread_idx);

      // Set up these values
      ml_data_internal::row_data_block block;

      ml_data sliced_data = this->slice(ml_data_row_start, ml_data_row_end);
      size_t rows_in_block = 0;
      size_t row_count = 0;
      size_t sample_index = samples_row_start;

      for(ml_data_iterator it = sliced_data.get_iterator();!it.done();) {

        DASSERT_LT(it.unsliced_row_index(), ml_data_row_end);
        DASSERT_LE(ml_data_row_start, it.unsliced_row_index());

        ////////////////////////////////////////////////////////////
        //
        // Step 1: Advance to the next chosen row

        size_t selection_index = get_sample(sample_index);

        // An optimization -- if the next index is not in this block,
        // then do a seek -- this will massively speed up sparse
        // selections.
        if(row_count < n_rows_needed) {

          DASSERT_TRUE(!it.done());

          if(selection_index > it.unsliced_row_index()
             && size_t(selection_index / row_block_size) > size_t(it.unsliced_row_index() / row_block_size)) {
            it.seek(it.row_index() + (selection_index - it.unsliced_row_index()));
          }

          while(it.unsliced_row_index() < selection_index) {
            ++it;
            DASSERT_TRUE(!it.done());
          }
        } else {
          break;
        }

        ////////////////////////////////////////////////////////////
        // Step 2: Write that row out.

        while(it.unsliced_row_index() == get_sample(sample_index)
              && row_count < n_rows_needed) {

          entry_value_iterator row_start = it.current_data_iter();

          append_row_to_row_data_block(rm, block, row_start);

          ++rows_in_block, ++row_count, ++sample_index;

          if(rows_in_block == row_block_size || row_count == n_rows_needed) {
            *it_out = block;
            ++it_out;
            block.entry_data.clear();
            rows_in_block = 0;
          }
        }
      }

      DASSERT_EQ(row_count, n_rows_needed);
      DASSERT_TRUE(block.entry_data.empty());
    });

  out.data_blocks->close();

  // Set up the block manager in the target thing.
  out._create_block_manager();

  // Clean up some of the other things.
  out._row_start         = 0;
  out._row_end           = n_rows;
  out._original_num_rows = n_rows;

  return out;
}

/**
 *  Create a sliced copy of the current ml_data structure.  This
 *  copy is cheap.
 */
ml_data ml_data::slice(size_t slice_row_start, size_t slice_row_end) const {
  ASSERT_LE(slice_row_start, num_rows());
  ASSERT_LE(slice_row_end, num_rows());

  ml_data out = *this;

  out._row_start = _row_start + slice_row_start;
  out._row_end   = _row_start + slice_row_end;

  return out;
}

/**
 *  Create a sliced copy of the current ml_data structure.  This
 *  copy is cheap.
 */
ml_data ml_data::absolute_slice(size_t slice_row_start, size_t slice_row_end) const {
  ASSERT_LE(slice_row_end, _original_num_rows);
  ASSERT_LE(slice_row_start, slice_row_end);

  ml_data out = *this;

  out._row_start = slice_row_start;
  out._row_end   = slice_row_end;

  return out;
}

  /** Translates the ml_data_entry row format to the original flexible
   *  types.
   */
std::vector<flexible_type> ml_data::translate_row_to_original(
    const std::vector<ml_data_entry>& v) const {

  return ml_data_internal::translate_row_to_original(metadata(), v);
}

  /** Translates the ml_data_entry_global_index row format to the original flexible
   *  types.
   */
std::vector<flexible_type> ml_data::translate_row_to_original(
    const std::vector<ml_data_entry_global_index>& v) const {

  return ml_data_internal::translate_row_to_original(metadata(), v);
}

/** Translates the original dense row format to the original flexible
 *  types.
 */
std::vector<flexible_type> ml_data::translate_row_to_original(const DenseVector& v) const {
  return ml_data_internal::translate_row_to_original(metadata(), v);
}

/** Translates the original dense row format to the original flexible
 *  types.
 */
std::vector<flexible_type> ml_data::translate_row_to_original(const SparseVector& v) const {
  return ml_data_internal::translate_row_to_original(metadata(), v);
}

/** Convenience function to create the block manager given the current
 *  data in the model.
 */
void ml_data::_create_block_manager() {
  block_manager.reset(
      new ml_data_block_manager(
          metadata(), rm, row_block_size, data_blocks, untranslated_columns));
}


}}
