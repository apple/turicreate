/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <string>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/feature_engineering/sample_transformer.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

/**
 * Initialize the options
 */
void sample_transformer::init_options(const std::map<std::string, flexible_type>&_options){
    options.create_real_option(
        "constant",
        "Constant that you want us to transform all your data to.",
        0.5,
        0,
        1,
        false);

    // Set options!
    options.set_options(_options);
    add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Get a version for the object.
 */
size_t sample_transformer::get_version() const {
  return SAMPLE_TRANSFORMER_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void sample_transformer::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  // Everything else
  oarc << constant
       << options;
}

/**
 * Load the object using Turi's iarc.
 */
void sample_transformer::load_version(turi::iarchive& iarc, size_t version){
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> constant
       >> options;
}


/**
 * Initialize the transformer.
 */
void sample_transformer::init_transformer(const std::map<std::string,
                      flexible_type>& _options){
  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if (k.first != "features"){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }

  // Capture some things as private variables.
  constant = _options.at("constant");
  init_options(opts);

  // Set the features
  flexible_type features = _options.at("features");
  state["features"] = to_variant(features);
  if (features == FLEX_UNDEFINED) {
    state["num_features"] = 1;
  } else {
    state["num_features"] = to_variant(features.size());
  }
}

/**
 * Fit the data.
 */
void sample_transformer::fit(gl_sframe data){
  if (variant_get_value<flexible_type>(state["features"]) == FLEX_UNDEFINED){
    state["features"] = to_variant(data.column_names());
    state["num_features"] = to_variant(data.num_columns());
  }
  return;
}

/**
 * Transform the given data.
 */
gl_sframe sample_transformer::transform(gl_sframe data){
  gl_sframe ret_sf = data;
  for (const std::string& col_name: ret_sf.column_names()){
    auto constant = this->constant;
    ret_sf[col_name] = data[col_name].apply([constant]
                      (const flexible_type& x) {
                          return constant;
                       }, flex_type_enum::FLOAT);
  }
  return ret_sf;
}



} // feature_engineering
} // sdk_model
} // turicreate
