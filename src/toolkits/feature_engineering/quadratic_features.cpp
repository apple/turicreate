/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/util/hash_value.hpp>
#include <math.h>
#include <algorithm>
#include <set>

#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/feature_engineering/quadratic_features.hpp>
#include <toolkits/feature_engineering/transform_utils.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

std::vector<std::string> subset(const gl_sframe& data,
    const std::vector<std::string>& candidates){
  std::vector<std::string> result;
  auto colnames = data.column_names();
  std::set<std::string> column_name_set(colnames.begin(), colnames.end());

  for (auto& k : candidates ){
    if (column_name_set.find(k) != column_name_set.end()){
      result.push_back(k);
    }
  }
  return result;
}

std::vector<std::vector<std::string>> select_pair_subset(const gl_sframe& data,
    const std::vector<std::vector<std::string>>& feature_pairs ){
  std::vector<std::vector<std::string>> subset_pairs;
  for (auto k = feature_pairs.begin(); k != feature_pairs.end(); ++k){
    std::vector<std::string> feature_subset =
      subset(data, *k);
    if (feature_subset.size() == 2){
      subset_pairs.push_back(*k);
    }
  }
  if (feature_pairs.size() != subset_pairs.size()){
        logprogress_stream << "Warning: The model was fit with "
         << feature_pairs.size() << " pairs of feature columns but only "
         << subset_pairs.size() << " were present during transform(). "
         << "Proceeding with transform by ignoring the missing columns."
         << std::endl;
  }
  return subset_pairs;
}

void parse_input_features(const flexible_type& options_at_features,
    std::vector<std::vector<std::string>>& unprocessed_features){
  if (options_at_features.get_type() != flex_type_enum::UNDEFINED) {
    ASSERT_TRUE(options_at_features.get_type() == flex_type_enum::LIST);
    flex_list options_list = options_at_features;
    if (options_list.size() > 0 && options_list[0].get_type() == flex_type_enum::STRING ){
      for (auto& k: options_list){
        unprocessed_features.push_back(std::vector<std::string> {k.get<flex_string>()});
      }
    } else {
    unprocessed_features = variant_get_value<std::vector<std::vector<std::string>>>
                        (to_variant(options_at_features));
    }
  }
}


void sort_pairs(std::vector<std::vector<std::string>>& unsorted_pairs){
  for (auto& p : unsorted_pairs) {
     std::sort(p.begin(), p.end());
  }
}

/**
 * If there are no feature pairs submitted by user, generate all pairs.
 * Takes column names as input, and mutates feature_pairs
 */
void generate_all_pairs(const std::vector<std::string>& column_names,
  std::vector<std::vector<std::string>> exclude ,std::vector<std::vector<std::string>>& feature_pairs){
  std::set<std::string> excluded_features;
  // Columns to exclude
  for (auto&i : exclude) {
    for (auto&k : i){
      excluded_features.insert(k);
    }
  }
  for(size_t i = 0; i < column_names.size(); ++i){
    if (excluded_features.find(column_names[i]) == excluded_features.end()){
      for (size_t k = i; k < column_names.size(); ++k){
        if ( excluded_features.find(column_names[k]) == excluded_features.end()  ){
          feature_pairs.push_back(std::vector<std::string>{column_names[i], column_names[k]});
        }
      }
    }
  }
}

void exclude_pairs(gl_sframe& training_data, std::vector<std::vector<std::string>>& unprocessed_features,
                  std::vector<std::vector<std::string>>& feature_pairs){
    if (unprocessed_features[0].size() == 2){
      std::vector<std::vector<std::string>> all_pairs;
      generate_all_pairs(training_data.column_names(),{} , all_pairs);
      sort_pairs(all_pairs);
      sort_pairs(unprocessed_features);
      std::set<std::vector<std::string>> all_pairs_set(all_pairs.begin(), all_pairs.end());
      std::set<std::vector<std::string>> exclude_pairs_set(unprocessed_features.begin(), unprocessed_features.end());
      std::set<std::vector<std::string>> result;
      std::set_difference(all_pairs_set.begin(), all_pairs_set.end(),
                          exclude_pairs_set.begin(), exclude_pairs_set.end(),
                          inserter(result,result.begin()));
      feature_pairs = std::vector<std::vector<std::string>>(result.begin(), result.end());
    } else if (unprocessed_features[0].size() == 1) {
      generate_all_pairs(training_data.column_names(), unprocessed_features, feature_pairs);
    }
}

/**
 * Takes column names and generates map between names and indeces in
 * SFrame.
 */
void generate_index_map(const std::vector<std::string>& column_names,
    std::map<std::string,size_t>& index_map){
  for (size_t k=0 ; k < column_names.size(); ++k){
    index_map[column_names[k]] = k;
  }
}

/**
 * Generates string that becomes the key for the quadratic feature.
 * For input feature of type that are List, Vector, or Dict the key/index
 * of that feature is an important part of the key. Otherwise, it is ignored.
 */
std::string generate_key_string(const std::string& column_name,
    const flexible_type& dict_key, flex_type_enum column_type ) {
  if (column_type != flex_type_enum::LIST &&
      column_type != flex_type_enum::VECTOR &&
      column_type != flex_type_enum::STRING &&
      column_type != flex_type_enum::DICT){
    return column_name;
  } else {
    return column_name + ":" +  (std::string)dict_key;
  }
}

std::pair<std::string, flexible_type> generate_key_value_pair(const std::vector<std::string>& column_names,
    const std::vector<flexible_type>& dict_keys, const std::vector<flexible_type>&  dict_values,
    const std::vector<flex_type_enum>& column_types){
  std::vector<std::string> key_strings(2);
  std::vector<flexible_type> values(2);
  for (size_t i=0 ; i<2; ++i){
    key_strings[i] = generate_key_string(column_names[i], dict_keys[i], column_types[i]);
    if (!transform_utils::is_numeric_type(dict_values[i].get_type()) && !transform_utils::is_numeric_type(column_types[i])){
      key_strings[i] = key_strings[i] + ":" + (std::string)dict_values[i];
      values[i] = 1;
    } else {
      values[i] = dict_values[i];
    }
  }
  flexible_type result;
  if (values[0].get_type() == flex_type_enum::UNDEFINED || values[1].get_type() == flex_type_enum::UNDEFINED){
    result = flex_undefined();
  } else {
    result = values[0] * values[1];
  }
  return std::pair<std::string, flexible_type>(key_strings[0] + ", " + key_strings[1], result);
}

/**
 * Takes two term(dictionaries), the column_names associate with those
 * dictionaires, concatenates keys and column names to form a new name
 * and multiplies value. Adds new key + value to interaction_map.
 */
void add_interaction_terms(const flex_dict& term_1, const flex_dict& term_2,
    const std::string& column_name1, const std::string& column_name2,
    flex_type_enum type_1, flex_type_enum type_2, std::map<std::string, flexible_type>& interaction_map ){
  for (auto& k : term_1) {
    for (auto& i : term_2){
        std::pair<std::string,flexible_type > kv = generate_key_value_pair({column_name1,column_name2}, {k.first, i.first}, {k.second,i.second},{type_1, type_2});
          interaction_map[kv.first] = kv.second;
    }
  }
}

/**
 * Apply function
 */
flex_dict interaction_apply(const sframe_rows::row& row,
    const std::vector<flex_type_enum>& types,
    const std::vector<std::vector<std::string>>& feature_pairs,
    const std::map<std::string,size_t>& index_map){


  std::map<std::string, flexible_type> interaction_map;
  flex_dict ret;

  for (auto &k : feature_pairs){
    flex_dict term_1 = transform_utils::flexible_type_to_flex_dict(row[index_map.find(k[0])->second]);
    flex_dict term_2 = transform_utils::flexible_type_to_flex_dict(row[index_map.find(k[1])->second]);
    flex_type_enum type_1 = types[index_map.find(k[0])->second];
    flex_type_enum type_2 = types[index_map.find(k[1])->second] ;
    add_interaction_terms(term_1, term_2, k[0], k[1] ,type_1, type_2, interaction_map);
  }

  std::copy(interaction_map.begin(), interaction_map.end(), inserter(ret, ret.begin()));
  return ret;
}

/**
 * Makes sure all feature pairs contain valid feature_columns
 */
void validate_pairs(const gl_sframe& data ,
    std::vector<std::vector<std::string>>& feature_pairs ){
  // Validate features present within pairs
  std::set<std::string> feature_set;

  std::vector<flex_type_enum> valid_feature_types =  {
          flex_type_enum::FLOAT,
          flex_type_enum::LIST,
          flex_type_enum::STRING,
          flex_type_enum::INTEGER,
          flex_type_enum::VECTOR,
          flex_type_enum::DICT};

  for (auto& k : feature_pairs){
    for (auto& i : k){
      feature_set.insert(i);
    }
  }

  std::vector<std::string> valid_features =
      transform_utils::select_valid_features(data,std::vector<std::string>(feature_set.begin(), feature_set.end()), valid_feature_types);

  // Extract valid pairs
  std::vector<std::vector<std::string>> valid_pairs;

  for (auto k = feature_pairs.begin(); k != feature_pairs.end(); ++k){
    std::vector<std::string> valid_features =
      transform_utils::select_valid_features(data,std::vector<std::string>(*k),
          valid_feature_types, false);
    if (valid_features.size() == 2){
      transform_utils::validate_feature_columns(data.column_names(), valid_features, false);
      valid_pairs.push_back(valid_features);
    }
  }
  if (valid_pairs.size() == 0) {
    std::string err_msg = "None of the specified feature pairs mactch a valid feature column."
         " Valid column types include ";
    for (size_t k = 0; k < valid_feature_types.size() - 1; ++k){
      err_msg += std::string(flex_type_enum_to_name(valid_feature_types[k])) + ",";
    }
    err_msg += std::string(flex_type_enum_to_name(valid_feature_types.back())) + ".";
    log_and_throw(err_msg);

  }
  feature_pairs = valid_pairs;
}

/**
 * Initialize the transfor
 */
void quadratic_features::init_transformer(const std::map<std::string, flexible_type>& _options){
  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if (k.first != "features" && k.first != "exclude"
        && k.first != "feature_pairs"){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }
  init_options(opts);

  flexible_type options_at_feature_pairs = _options.at("feature_pairs");

  parse_input_features(options_at_feature_pairs, unprocessed_features);


  exclude = _options.at("exclude");
  if ((int) exclude == 1) {
    state["features"] = to_variant(FLEX_UNDEFINED);
    state["excluded_features"] = to_variant(options_at_feature_pairs);
  } else {
    state["features"] = to_variant(_options.at("features"));
    state["excluded_features"] = to_variant(FLEX_UNDEFINED);
  }

}


/**
 * Get a version for the object
 */
size_t quadratic_features::get_version() const {
  return QUADRATIC_FEATURES_VERSION;
}

/**
 * Save object using Turi's oarc
 */
void quadratic_features::save_impl(oarchive& oarc) const {

  //Save state
  variant_deep_save(state, oarc);

  // Everything else
  oarc <<
  options <<
  unprocessed_features <<
  feature_pairs <<
  feature_types <<
  fitted <<
  exclude;
}

/**
 * Load the object using Turi's iarc
 */
void quadratic_features::load_version(iarchive& iarc, size_t version){
  if(version > QUADRATIC_FEATURES_VERSION){
      log_and_throw("This model version cannot be loaded. Please re-save your model.");
  }
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >>
  options >>
  unprocessed_features >>
  feature_pairs >>
  feature_types >>
  fitted >>
  exclude;
}

/**
 * Define and set options manager options.
 */
void quadratic_features::init_options(const std::map<std::string, flexible_type>& _options) {

  options.create_string_option(
      "output_column_name" ,
      "The name of the SFrame column where interaction terms are"
      " stored",
      "quadratic_features");

  //Set options
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Fit the data
 */
void quadratic_features::fit(gl_sframe training_data) {

  feature_pairs = {};

  if (exclude){
    exclude_pairs(training_data, unprocessed_features, feature_pairs);
  } else if (!unprocessed_features.size()){
    generate_all_pairs(training_data.column_names(), {},feature_pairs);
  } else {
    feature_pairs = unprocessed_features;
  }

  validate_pairs(training_data, feature_pairs);


  for (auto& k : feature_pairs){
    for (auto& col_name : k){
      feature_types[col_name] = training_data[col_name].dtype();
      }
    }

  fitted = true;

}

/**
 * Transforms the data.
 */
gl_sframe quadratic_features::transform(gl_sframe training_data) {

  std::map<std::string,size_t> index_map;


  if (!fitted){
    log_and_throw("Transformer must be fitted before .transform() is called");
  }



  gl_sframe ret_sf = training_data;

  std::vector<std::vector<std::string>> subset_pairs;

  std::vector<flex_type_enum> types = training_data.column_types();

  subset_pairs = select_pair_subset(training_data, feature_pairs);

  for (auto& k : subset_pairs){
    transform_utils::validate_feature_types(k, feature_types, training_data);
  }

  generate_index_map(training_data.column_names(), index_map);

  training_data.head(10).apply([subset_pairs, types, index_map] (const sframe_rows::row& x) {
      return interaction_apply(x, types, subset_pairs, index_map);}, flex_type_enum::DICT).materialize();

  std::string output_name = (std::string) options.value("output_column_name");
  output_name = transform_utils::get_unique_feature_name(ret_sf.column_names(),
                                                                  output_name);

  ret_sf[output_name]  =  training_data.apply([subset_pairs, types, index_map] (const sframe_rows::row& x) {
      return interaction_apply(x, types, subset_pairs, index_map);}, flex_type_enum::DICT);

  return ret_sf;
}





} // namespace feature_engineering
} // namespace sdk_model
} // namespace turi
