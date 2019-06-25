/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <limits>
#include <core/util/cityhash_tc.hpp>
#include <core/random/random.hpp>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/feature_engineering/transform_utils.hpp>
#include <toolkits/feature_engineering/count_featurizer.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

void count_featurizer::init_options(const std::map<std::string,
                                        flexible_type>& _options) {
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_integer_option(
      "num_bits",
      "The number of bits to hash to. O(2^num_bits) memory is required",
      20,
      1,
      64,
      false);

  options.create_integer_option(
      "random_seed",
      "A random seed",
      3141,
      0,
      std::numeric_limits<int>::max(),
      false);

  options.create_real_option(
      "laplace_smearing",
      "Differential privacy mechanism to allow this feature transformer to be used without another data split",
      0,
      0,
      std::numeric_limits<int>::max(),
      false);


  options.create_string_option(
      "target",
      "The column name of the target column.",
      FLEX_UNDEFINED,
      false);

  options.create_string_option(
      "count_column_prefix",
      "The prefix to use for the column name of each count column.",
      "count_",
      false);


  options.create_string_option(
      "prob_column_prefix",
      "The prefix to use for the column name of each probability column.",
      "prob_",
      false);

  //Set options
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}


/**
 * Define and set options manager options.
 */
void count_featurizer::init_transformer(
    const std::map<std::string, flexible_type>& _options){
  DASSERT_TRUE(options.get_option_info().size() == 0);

  // Copy over the options (exclude features)
  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if (k.first != "features" && k.first != "exclude"){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }

  init_options(opts);

  // Set features
  unprocessed_features = _options.at("features");
  exclude = _options.at("exclude");
  if ((int) exclude == 1) {
    state["features"] = to_variant(FLEX_UNDEFINED);
    state["excluded_features"] = to_variant(unprocessed_features);
  } else {
    state["features"] = to_variant(unprocessed_features);
    state["excluded_features"] = to_variant(FLEX_UNDEFINED);
  }
}


/**
 *Validates input data.
 */
void count_featurizer::fit(gl_sframe data) {
  DASSERT_TRUE(state.count("features") > 0);
  DASSERT_TRUE(options.get_option_info().size() > 0);

  DASSERT_TRUE(state.count("features") > 0);
  DASSERT_TRUE(options.get_option_info().size() > 0);

  m_state = std::make_shared<transform_state>();
  size_t num_bits = (size_t) options.value("num_bits");
  m_state->seed =  (size_t) options.value("random_seed");
  m_state->laplace_smearing = (double) options.value("laplace_smearing");
  m_state->count_column_prefix = (std::string) options.value("count_column_prefix");
  m_state->prob_column_prefix = (std::string) options.value("prob_column_prefix");
  std::string target_column = (std::string) options.value("target");

  // Get the set of features to work with.
  feature_columns = transform_utils::get_column_names(
      data, exclude, unprocessed_features);
  // look for the target column and erase it
  {
    auto target_iter = std::find(feature_columns.begin(), feature_columns.end(), target_column);
    if (target_iter != feature_columns.end()) feature_columns.erase(target_iter);
  }
  // Select the features of the right type.
  feature_columns = transform_utils::select_valid_features(data, feature_columns,
                      { flex_type_enum::STRING,
                        flex_type_enum::INTEGER });
  // Validate the features.
  transform_utils::validate_feature_columns(data.column_names(), feature_columns);


  if (!data.contains_column(target_column)) {
    log_and_throw("SFrame does not contain target column");
  }

  auto y_column = data.select_column(target_column);
  if (y_column.dtype() != flex_type_enum::STRING && y_column.dtype() != flex_type_enum::INTEGER) {
    log_and_throw("Target column must be a string or integer");
  }

  // subselect data to only contain the columns we care about.
  // also conveniently reordering them.
  auto all_columns = feature_columns;
  all_columns.push_back(target_column);
  data = data.select_columns(all_columns);
  // convert column names to column numbers
  std::map<std::string, size_t> column_name_to_number;
  for (size_t i = 0;i < all_columns.size(); ++i) column_name_to_number[all_columns[i]] = i;

  random::seed(m_state->seed);
  // get the unique values for the y column
  auto& counters = m_state->counters;
  std::unordered_map<flexible_type, size_t> y_value_to_index;
  for (const auto& y_value: y_column.unique().range_iterator()) {
    y_value_to_index[y_value] = m_state->y_values.size();
    m_state->y_values.push_back(y_value);
  }

  // initialize the counter matrix. counter_matrix is
  // m_state_y_values.size() * feature_columns.size() worth of count min sketches
  counters.resize(m_state->y_values.size());
  for (auto& column_counters : counters) {
    column_counters.resize(feature_columns.size(), sketches::countmin<flexible_type>(num_bits));
  }
  /*
   * Loop through all the data and increment the counters.
   */
  data.materialize_to_callback(
      [&](size_t thread_id, const std::shared_ptr<sframe_rows>& rows) {
        for(const auto& row: *rows) {
          DASSERT_TRUE(row.size() > 1);
          // get the last column. This is the y column
          auto iter = y_value_to_index.find(row[row.size() - 1]);
          ASSERT_TRUE(iter != y_value_to_index.end());
          auto y_index = iter->second;
          for (size_t i = 0; i < row.size() - 1; ++i) {
            counters[y_index][i].atomic_add(row[i]);
          }
        }
        return false;
      });
  fitted = true;
}

/**
 * Inverse laplace CDF. returns Laplace(0, scale) sample given U([0,1]).
 */
static inline double inv_laplace_cdf(double u, double laplace_scale) {
  // where u ~ U([-0.5, 0.5])
  // and b is laplace_scale
  // where X = -b sgn(u)ln(1-2|u|)

  // shift it away from 0 to get u to (0, 1]
  u = std::max<double>(u, std::numeric_limits<double>::epsilon());
  // shift it to become (-0.5, 0.5]
  u -= 0.5;
  if (u < 0.0) return laplace_scale * std::log(1.0 + 2 * u);
  else return -laplace_scale * std::log(1.0 - 2 * u);
}

/**
 * Generates a SArray of vector type where each element has length 'vector_length'
 * and each element is a laplace value randomly generated from
 * Laplace(0, laplace_scale)
 * This procedure is set up to be determinisitic. i.e. fixing a seed,
 * and an input sequential array, the output will always be consistent.
 * i.e. randomness is produced by hashing the input array.
 * The output is rounded up via std::ceil()
 */
static gl_sarray make_random_integer_laplace_array(const gl_sarray& sequential_array,
                                                   size_t seed,
                                                   double laplace_scale,
                                                   size_t vector_length) {
  seed = hash64(seed);

  return sequential_array.apply(
      [seed, laplace_scale, vector_length](const flexible_type& val)->flexible_type{
        // here u is [0, 1]
        auto valhash = hash64_combine(seed, val.hash());
        flex_vec ret(vector_length);
        for (size_t i = 0; i < vector_length; ++i) {
          double u = double(hash64_combine(valhash, hash64(i))) /
                     double(std::numeric_limits<size_t>::max());
          double l = inv_laplace_cdf(u, laplace_scale);
          if (l >= 0) ret[i] = std::ceil(l);
          else ret[i] = std::floor(l);
        }
        return ret;
      }, flex_type_enum::VECTOR);
}


/**
 * Generates a SArray of vector type where each element has length 'vector_length'
 * and each element is a laplace value randomly generated from
 * Laplace(0, laplace_scale)
 * This procedure is set up to be determinisitic. i.e. fixing a seed,
 * and an input sequential array, the output will always be consistent.
 * i.e. randomness is produced by hashing the input array.
 */
static gl_sarray make_random_real_laplace_array(const gl_sarray& sequential_array,
                                                size_t seed,
                                                double laplace_scale,
                                                size_t vector_length) {
  seed = hash64(seed);

  return sequential_array.apply(
      [seed, laplace_scale, vector_length](const flexible_type& val)->flexible_type{
        // here u is [0, 1]
        auto valhash = hash64_combine(seed, val.hash());
        flex_vec ret(vector_length);
        for (size_t i = 0; i < vector_length; ++i) {
          double u = double(hash64_combine(valhash, hash64(i))) /
                     double(std::numeric_limits<size_t>::max());
          ret[i] = inv_laplace_cdf(u, laplace_scale);
        }
        return ret;
      }, flex_type_enum::VECTOR);
}

gl_sframe count_featurizer::fit_transform(gl_sframe raw) {
  fit(raw);
  return transform(raw);
}

gl_sframe count_featurizer::transform(gl_sframe raw) {
  auto data = raw.select_columns(feature_columns);

  // select out all the remaining columns
  auto feature_columns_set = std::set<std::string>(feature_columns.begin(), feature_columns.end());
  auto remaining_columns = raw.column_names();
  remaining_columns.erase(
      std::remove_if(remaining_columns.begin(), remaining_columns.end(),
                 [&](const std::string& s)->bool {
                   return feature_columns_set.count(s) > 0;
                 }),
      remaining_columns.end());
  auto remaining_data = raw.select_columns(remaining_columns);

  auto integers = data.select_columns(feature_columns);
  // for each feature column, we generate on sarray of counts
  // Then for each SArray of counts, we use an "apply" to get to the
  // probability column outcome is... everything can be lazy! And I like lazy.
  auto state = m_state;
  size_t num_classes = state->counters.size();


  // if we need laplace smearing, we need to make a sequential array
  // to force seeds per row
  gl_sframe output_frame;
  gl_sarray sequential_array;
  double laplace_smearing = state->laplace_smearing;
  bool perform_laplace_smearing = laplace_smearing > 0.0;

  if (perform_laplace_smearing) {
    sequential_array = gl_sarray::from_sequence(0, data.size());
  }

  for (size_t column_number = 0;
       column_number < feature_columns.size();
       ++column_number) {
    auto column_name = feature_columns[column_number];

    // for each column, generate the count column
    auto count_column = data[column_name].apply(
        [state, column_number](const flexible_type& val)->flexible_type {
          auto& counters = state->counters;
          flex_vec ret(counters.size());
          size_t ctr = 0;
          for (const auto& ycounter: counters) {
            ret[ctr] = ycounter[column_number].estimate(val);
            ++ctr;
          }
          return ret;
        }, flex_type_enum::VECTOR);

    // apply laplace smearing if applicable
    // and add the column to the output frame
    if (perform_laplace_smearing) {
      auto smear_seed = state->seed + 2 * column_number;
      auto smeared_column = count_column +
          make_random_integer_laplace_array(sequential_array,
                                            smear_seed,
                                            laplace_smearing,
                                            num_classes);
      // add the count column to the output sframe
      output_frame[state->count_column_prefix + column_name] = smeared_column;
    } else {
      // add the count column to the output sframe
      output_frame[state->count_column_prefix + column_name] = count_column;
    }
    // to generate the probability column, we need to do a sum
    // and normalize. It is important to note that we are summing and
    // normalizing the UNSMEARED counts.
    auto sum_column = count_column.apply(
        [num_classes](const flexible_type& val)->flexible_type {
          DASSERT_TRUE(val.get_type() == flex_type_enum::VECTOR);
          const auto& count = val.get<flex_vec>();
          DASSERT_EQ(count.size(), num_classes);
          static_cast<void>(num_classes); // Avoid warning (num_classes unused in release mode).
          double sum = 0;
          for (const auto& val: count) sum += std::max<double>(val, 0.0);
          if (sum < 1.0) sum = 1.0;
          return sum;
        }, flex_type_enum::FLOAT);
    auto prob_column = count_column.subslice(0, num_classes - 1, 1) / sum_column;



    // apply laplace smearing if applicable to the probability column
    // and add the column to the output frame
    if (perform_laplace_smearing) {
      auto smear_seed = state->seed + 2 * column_number + 1;
      // note that this time we are scaling the laplace rng by the sum
      auto smeared_column = prob_column +
          make_random_real_laplace_array(sequential_array,
                                         smear_seed,
                                         laplace_smearing,
                                         num_classes - 1) / sum_column;
      output_frame[state->prob_column_prefix + column_name] = smeared_column;
    } else {
      output_frame[state->prob_column_prefix + column_name] = prob_column;
    }
  }
  output_frame.add_columns(remaining_data);
  return output_frame;
}

size_t count_featurizer::get_version() const {
  return count_featurizer_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void count_featurizer::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);
  //// Everything else
  oarc << feature_columns
       << unprocessed_features
       << exclude
       << fitted;
  if (fitted) {
    oarc << m_state->seed
         << m_state->laplace_smearing
         << m_state->count_column_prefix
         << m_state->prob_column_prefix
         << m_state->counters;
  }
}

void count_featurizer::load_version(turi::iarchive& iarc, size_t version){
  ASSERT_EQ(version, count_featurizer_VERSION);
  variant_deep_load(state, iarc);

  iarc >> feature_columns
       >> unprocessed_features
       >> exclude
       >> fitted;

  if (fitted) {
    m_state = std::make_shared<transform_state>();
    iarc >> m_state->seed
         >> m_state->laplace_smearing
         >> m_state->count_column_prefix
         >> m_state->prob_column_prefix
         >> m_state->counters;
  }
}

} // namespace feature_engineering
} // namespace sdk_model
} // namespace turi
