/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <util/hash_value.hpp>
#include<algorithm>

#include<unity/lib/variant_deep_serialize.hpp>
#include <unity/toolkits/feature_engineering/feature_hasher.hpp>
#include <unity/toolkits/feature_engineering/transform_utils.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {


/**
 * Hashes key and inserts value. In case of collisions, there is a secondary
 * hash which determines if values should be added or subtracted. This builds
 * an unbiased approximation to the value
 */
void insert_into_map(uint128_t combined_key_name, size_t num_bits,
      const flexible_type& value, std::map<size_t, flexible_type>& hashed_map){

  // Random large prime number
  uint128_t combine_seed = 32416190071LL;
  flex_type_enum value_type = value.get_type();

  // Decides whether collision values are added or subtracted
  bool sign = (hash128_combine(combined_key_name, combine_seed) % 2);
  uint128_t final_key = combined_key_name & ( (1 << num_bits) - 1);

  // For floats, flex_type will cast to the left value, so we have to
  // cast to a float before we do any adding.
  if (value_type == flex_type_enum::FLOAT){
    hashed_map[final_key] = (flex_float) hashed_map[final_key];
  }
  // Add or subtract hashes.
  if (sign){
     hashed_map[final_key] -= (flex_float) value;
  } else {
     hashed_map[final_key] += (flex_float) value;
  }
}

/**
 * Function that takes feature columns and hashes them to a single dictionary
 * per row.
 */
flex_dict hash_apply(const sframe_rows::row& row,
                    const std::vector<uint128_t> hashed_names, size_t num_bits){

  std::map<size_t,flexible_type> hashed_map;
  flex_dict ret;
  for (size_t i = 0; i < row.size(); ++i){
    flex_dict dict_to_hash = transform_utils::flexible_type_to_flex_dict(row[i]);
    for (auto& k : dict_to_hash){
      // Combine the hash value.
      hash_value hashed_key(k.first);
      uint128_t combined_key_name = hash128_combine(
                                    hashed_key.hash(), hashed_names[i]);
      // Hash numerics.
      if (transform_utils::is_numeric_type(k.second.get_type())){
        hash_value hashed_key(k.first);
        insert_into_map(combined_key_name, num_bits, k.second, hashed_map);

      // Add categorical hashes for non-numerics.
      } else {
        hash_value hashed_value(k.second);
        uint128_t super_key = hash128_combine(
                     combined_key_name, hashed_value.hash());
        insert_into_map(super_key, num_bits, 1, hashed_map);
      }
    }
  }
  std::copy(hashed_map.begin(), hashed_map.end(), inserter(ret , ret.begin()));
  return ret;
}

/**
 * Initialize the transformer.
 */
void feature_hasher::init_transformer(
            const std::map<std::string, flexible_type>& _options){

  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if (k.first != "features" && k.first != "exclude"){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }
  init_options(opts);

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
 * Get a version for the object
 */
size_t feature_hasher::get_version() const {
  return FEATURE_HASHER_VERSION;
}

/**
 * Save object using Turi's oarc
 */
void feature_hasher::save_impl(oarchive& oarc) const {

  //Save state
  variant_deep_save(state, oarc);

  // Everything else
  oarc <<
  options <<
  feature_columns <<
  unprocessed_features <<
  fitted <<
  feature_types <<
  exclude;
}

/**
 * Load the object using Turi's iarc
 */
void feature_hasher::load_version(iarchive& iarc, size_t version){
  if (version > FEATURE_HASHER_VERSION){
      log_and_throw("This model version cannot be loaded. Please re-save your model.");
  }
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >>
  options >>
  feature_columns >>
  unprocessed_features >>
  fitted >>
  feature_types >>
  exclude;
}

/**
 * Define and set options manager options.
 */
void feature_hasher::init_options(
                        const std::map<std::string, flexible_type>& _options) {
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_integer_option(
      "num_bits",
      "The number of bits to hash to",
      18,
      1,
      std::numeric_limits<int>::max(),
      false);

  options.create_string_option(
      "output_column_name",
      "The name of the SFrame column features are being hashed to",
      "hashed_features");

  //Set options
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 *Validates input data.
 */
void feature_hasher::fit(gl_sframe data) {
  DASSERT_TRUE(state.count("features") > 0);
  DASSERT_TRUE(options.get_option_info().size() > 0);

  // Get the set of features to work with.
  feature_columns = transform_utils::get_column_names(data, exclude,
                                         unprocessed_features);

  // Select the features of the right type.
  feature_columns = transform_utils::select_valid_features(data, feature_columns,
                      { flex_type_enum::STRING,
                        flex_type_enum::FLOAT,
                        flex_type_enum::INTEGER,
                        flex_type_enum::LIST,
                        flex_type_enum::VECTOR,
                        flex_type_enum::DICT});

  transform_utils::validate_feature_columns(data.column_names(), feature_columns);
  state["features"] = to_variant(feature_columns);

  // Store feature types and cols.
  feature_types.clear();
  for (auto& col_name : feature_columns){
    feature_types[col_name] = data[col_name].dtype();
  }
  fitted = true;
}

/**
 * Perform the hashing
 */
gl_sframe feature_hasher::transform(gl_sframe data) {

  //Check if fitting has already ocurred.
  if (!fitted){
    log_and_throw("The FeatureHasher must be fitted before .transform() is called.");
  }

  // Validated column names
  size_t num_bits = (size_t) options.value("num_bits");
  std::vector<std::string> subset_columns = transform_utils::select_feature_subset(data, feature_columns);
  transform_utils::validate_feature_types(subset_columns, feature_types, data);

  //Extract meta-information from selected columns
  gl_sframe selected_sf = data.select_columns(subset_columns);
  gl_sframe ret_sf = data;
  std::vector<flex_type_enum> types = selected_sf.column_types();

  // Cache hash values of feature names for efficiency.
  std::vector<uint128_t> hashed_names;
  for (auto &i : subset_columns){
    hash_value hashed_name(i);
    hashed_names.push_back(hashed_name.hash());
    ret_sf.remove_column(i);  // Remove column from returned sf.
  }

  // Error checking mode.
  selected_sf.head(10).apply([hashed_names, num_bits](
    const sframe_rows::row& x){
        return hash_apply(x,hashed_names, num_bits);
    }, flex_type_enum::DICT).materialize();

  // Check if output name exists. If not, get a valid one.
  std::string output_name = (std::string) options.value("output_column_name");
  output_name = transform_utils::get_unique_feature_name(ret_sf.column_names(),
                                                             output_name);

  ret_sf[output_name] = selected_sf.apply([hashed_names, num_bits]
    (const sframe_rows::row& x){
        return hash_apply(x, hashed_names, num_bits);
    }, flex_type_enum::DICT);

  return ret_sf;
}
} // namespace feature_engineering
} // namespace sdk_model
} // namespace turi
