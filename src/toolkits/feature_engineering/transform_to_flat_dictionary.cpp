/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

#include <toolkits/feature_engineering/transform_utils.hpp>
#include <toolkits/feature_engineering/transform_to_flat_dictionary.hpp>
#include <toolkits/feature_engineering/topk_indexer.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {


/**
 * Initialize the options
 */
void transform_to_flat_dictionary::init_options(const std::map<std::string,flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_string_option(
      "separator",
      "The string used to seperate keys in nested dictionaries and lists."
      ".",
      false);

  options.create_string_option(
      "none_tag",
      "The string used to denote a None value."
      "__none__",
      false);

  options.create_string_option(
      "output_column_prefix",
      "The string prepended to the output column names.",
      "",
      false);

  // Set options!
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Get a version for the object.
 */
size_t transform_to_flat_dictionary::get_version() const {
  return TRANSFORM_TO_FLAT_DICTIONARY_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void transform_to_flat_dictionary::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  //// Everything else
  oarc << options
       << feature_columns
       << feature_types
       << exclude
       << unprocessed_features
       << fitted;
}

/**
 * Load the object using Turi's iarc.
 */
void transform_to_flat_dictionary::load_version(turi::iarchive& iarc, size_t version){
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> feature_columns
       >> feature_types
       >> exclude
       >> unprocessed_features
       >> fitted;
}


/**
 * Initialize the transformer.
 */
void transform_to_flat_dictionary::init_transformer(const std::map<std::string,flexible_type>& _options){
  DASSERT_TRUE(options.get_option_info().size() == 0);

  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if (k.first != "features" && k.first != "exclude") {
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

  fitted = false;
}


/**
 * Fit the data.
 */
void transform_to_flat_dictionary::fit(gl_sframe data){

  DASSERT_TRUE(options.get_option_info().size() > 0);

  // Get the feature name (Note it is only 1 string)
  feature_columns = transform_utils::get_column_names(
                            data, exclude, unprocessed_features);

  // Validate the features.
  transform_utils::validate_feature_columns(data.column_names(), feature_columns);

  state["features"] = to_variant(feature_columns);

  // Store feature types.
  feature_types.clear();
  for (const auto& f : feature_columns) {
    feature_types[f] = data.select_column(f).dtype();
  }

  fitted = true;
}

/**
 * Transform the given data.
 */
gl_sframe transform_to_flat_dictionary::transform(gl_sframe data) {

  if(!fitted) {
    log_and_throw("`transform` called before `fit` or `fit_transform`.");
  }

  DASSERT_TRUE(options.get_option_info().size() > 0);

  flex_string separator = options.value("separator").get<flex_string>();
  flex_string undefined_tag = options.value("none_tag").get<flex_string>();
  flex_string output_column_prefix =  options.value("output_column_prefix").get<flex_string>();

  // Select and validate features.
  std::vector<std::string> transform_features;

  transform_features =
        variant_get_value<std::vector<std::string> >(state.at("features"));

  transform_features = transform_utils::select_feature_subset(data, transform_features);

  transform_utils::validate_feature_types(transform_features, feature_types, data);

  gl_sframe ret_sf = data;

  // Do the actual transformations.
  for(const auto& s : transform_features) {
    std::string out_c = (output_column_prefix.empty() ? s : (output_column_prefix + "." + s));

    ret_sf[out_c] = to_sarray_of_flat_dictionaries(
        data[s], separator, undefined_tag, "error", "error");
  }

  // Do the actual transformations.
  return ret_sf;
}

} // namespace feature_engineering
} // namespace sdk_model
} // namespace turi
