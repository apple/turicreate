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
#include <toolkits/feature_engineering/mean_imputer.hpp>
#include <toolkits/feature_engineering/statistics_tracker.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

/**
 * Replace None variables with mean calculated during fit phase.
 *
 * \param[in] input    A flexible type
 * \param[in] indexer  An already created statistics_tracker.
 * \param[in] column_type The column type being operator on.
 *
 * \returns  output Indexed flexible type.
 *
 */
flexible_type mean_imputer_apply(const flexible_type& input,
            const std::shared_ptr<statistics_tracker>& tracker,
            flex_type_enum column_type) {

  flex_type_enum input_type = input.get_type();
  DASSERT_TRUE(tracker != NULL);
  DASSERT_TRUE(input_type == flex_type_enum::INTEGER
               || input_type == flex_type_enum::UNDEFINED
               || input_type == flex_type_enum::LIST
               || input_type == flex_type_enum::VECTOR
               || input_type == flex_type_enum::FLOAT
               || input_type == flex_type_enum::DICT);

  // Go through all the cases.
  flexible_type output;
  switch(column_type) {
    // Numerical cols
    case flex_type_enum::INTEGER:
    case flex_type_enum::FLOAT: {
      if (input_type == flex_type_enum::UNDEFINED ) {
        flexible_type key = 0;
        flex_float mean = tracker->lookup_means(key);
        output = mean;
      } else {
        output = input;
      }
      break;
    }

    // Categorical vector
    case flex_type_enum::LIST: {
      flex_list out_list;
      flex_list vv = flex_list();
      if (input_type != flex_type_enum::UNDEFINED){
        vv = input.get<flex_list>();
      }
      size_t n_values = tracker->size();
      if (n_values != vv.size() && input_type != flex_type_enum::UNDEFINED){
        log_and_throw("All vectors must be of same size for imputation.");
      }
      out_list.resize(n_values);
      for(size_t k = 0; k < n_values; ++k) {
        if (input_type == flex_type_enum::UNDEFINED || vv[k].get_type() == flex_type_enum::UNDEFINED) {
          flex_float mean = tracker->lookup_means(k);
          out_list[k] = mean;
        } else {
          if (!transform_utils::is_numeric_type(vv[k].get_type())){
            log_and_throw("All elements of list must be numeric for imputation.");
          }
          out_list[k] = vv[k];
        }
      }
      output = out_list;
      break;
    }

    // Numerical vector
    case flex_type_enum::VECTOR: {
      flex_vec out_vec;
      flex_vec vv = flex_vec();
      if(input_type != flex_type_enum::UNDEFINED){
        vv = input.get<flex_vec>();
      }
      size_t n_values = tracker->size();
      if (n_values != vv.size() && input_type != flex_type_enum::UNDEFINED){
        log_and_throw("All vectors must be of same size for imputation.");
      }
      out_vec.resize(n_values);
      for(size_t k = 0; k < n_values; ++k) {
        if (input_type == flex_type_enum::UNDEFINED) {
          flex_float mean = tracker->lookup_means(k);
          out_vec[k] = mean;
        } else {
          out_vec[k] = vv[k];
        }
      }
      output = out_vec;
      break;
    }

    // Dictionary
    case flex_type_enum::DICT: {
      flex_dict out_dict = {};
      if (input_type == flex_type_enum::UNDEFINED){
        std::vector<flexible_type> keys = tracker->get_keys();
        for (auto &k : keys){
          std::pair<flexible_type,flexible_type> pair = std::make_pair(k, tracker->lookup_means(k));
          out_dict.push_back(pair);
        }
      } else {
        const flex_dict& dv = input.get<flex_dict>();
        size_t n_values = dv.size();
        out_dict.resize(n_values);
        for(size_t k = 0; k < n_values; ++k) {
          const std::pair<flexible_type, flexible_type>& kvp = dv[k];
          flex_float mean = tracker->lookup_means(kvp.first);
          if (!transform_utils::is_numeric_type(kvp.second.get_type()) && !(kvp.second.get_type() == flex_type_enum::UNDEFINED)){
            log_and_throw("Dictionaries must only contain numerical values");
          } else if (kvp.second.get_type() == flex_type_enum::UNDEFINED) {
            out_dict[k] = std::make_pair(kvp.first, mean);
          } else {
            out_dict[k] = kvp;
          }
      }
      }
      output = out_dict;
      break;
    }

    // Should never happen here.
    default:
      log_and_throw("Invalid type. Column must be of type int, float,"
                        " list, vec, or dictionary.");
      break;
  }


  return output;
}


/**
 * Initialize the options
 */
void mean_imputer::init_options(const std::map<std::string,
                                                   flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_flexible_type_option(
      "output_column_prefix",
      "The prefix to use for the column name of each transformed column.",
      FLEX_UNDEFINED,
      false);

  options.create_categorical_option(
      "strategy",
      "The strategy with which to fill in missing values",
      "auto",
      {"auto","mean"}
      );

  // Set options!
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Get a version for the object.
 */
size_t mean_imputer::get_version() const {
  return MEAN_IMPUTER_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void mean_imputer::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  // Everything else
  oarc << options
       << feature_columns
       << feature_types
       << means_map
       << exclude;
}

/**
 * Load the object using Turi's iarc.
 */
void mean_imputer::load_version(turi::iarchive& iarc, size_t version){
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> feature_columns
       >> feature_types
       >> means_map
       >> exclude;
}


/**
 * Initialize the transformer.
 */
void mean_imputer::init_transformer(const std::map<std::string,
                      flexible_type>& _options){
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
  feature_columns = _options.at("features");
  exclude = _options.at("exclude");
  if ((int) exclude == 1) {
    state["features"] = to_variant(FLEX_UNDEFINED);
    state["excluded_features"] = to_variant(feature_columns);
  } else {
    state["features"] = to_variant(feature_columns);
    state["excluded_features"] = to_variant(FLEX_UNDEFINED);
  }
}

/**
 * Fit the data.
 */
void mean_imputer::fit(gl_sframe data){
  DASSERT_TRUE(state.count("features") > 0);
  DASSERT_TRUE(options.get_option_info().size() > 0);

  // Get the set of features to work with.
  std::vector<std::string> fit_features = transform_utils::get_column_names(
                            data, exclude, feature_columns);
  transform_utils::validate_feature_columns(data.column_names(), fit_features);

  // Select the features of the right type.
  fit_features = transform_utils::select_valid_features(data, fit_features,
                      {flex_type_enum::FLOAT,
                       flex_type_enum::VECTOR,
                       flex_type_enum::INTEGER,
                       flex_type_enum::LIST,
                       flex_type_enum::DICT});

  // Validate the features.
  transform_utils::validate_feature_columns(data.column_names(), fit_features);
  feature_types.clear();
  for (const auto& f: fit_features) {
    feature_types[f] = data.select_column(f).dtype();
  }
  state["features"] = to_variant(fit_features);

  // Learn the index mapping.
  means_map.clear();
  for (const auto& feat: fit_features) {
    means_map[feat].reset(new statistics_tracker(feat));
    transform_utils::create_mean_mapping(data[feat], feat, means_map[feat]);
    DASSERT_TRUE(means_map[feat] != NULL);
  }

  std::map<std::string, gl_sarray> calculated_means_map;

  for (const auto& feat: fit_features){
    calculated_means_map[feat] = gl_sarray{flex_undefined()};
  }

  gl_sframe calculated_means(calculated_means_map);


  // Make sure this works for int
  for (const auto& feat: fit_features){
    const auto& mean = means_map[feat];
    flex_type_enum output_type = feature_types[feat];
    flex_type_enum feature_type = feature_types[feat];
    if (output_type == flex_type_enum::INTEGER){
      output_type = flex_type_enum::FLOAT;
    }
    calculated_means[feat] = calculated_means[feat].apply([=]
    (const flexible_type& x) {
      return  mean_imputer_apply(x, mean, feature_type);
    }, output_type, false);
  }

  state["means"] = to_variant(calculated_means);


}

/**
 * Transform the given data.
 */
gl_sframe mean_imputer::transform(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);
  if (means_map.size() == 0) {
    log_and_throw("The MeanImputer must be fitted before .transform() is called.");
  }

  // Select and validate features.
  std::vector<std::string> transform_features =
            variant_get_value<std::vector<std::string>>(state.at("features"));
  transform_features = transform_utils::select_feature_subset(
                                   data, transform_features);
  transform_utils::validate_feature_types(
                                   transform_features, feature_types, data);

  // Original data
  gl_sframe ret_sf = data;
  std::vector<std::string> cols = data.column_names();

  flexible_type output_column_prefix =
      variant_get_value<flexible_type>(state.at("output_column_prefix"));
  if (output_column_prefix == FLEX_UNDEFINED) {
    output_column_prefix = "";
  } else {
    output_column_prefix = output_column_prefix + ".";
  }

  // Loop through features.
  for (const auto& feat: transform_features) {

    // Get the output column name.
    flex_string output_column_name = output_column_prefix + feat;

    // Get the output type. Cast to string if not castable.
    flex_type_enum output_type = feature_types[feat];
    if (output_type == flex_type_enum::INTEGER){
      output_type = flex_type_enum::FLOAT;
    }
    const auto& mean = means_map[feat];
    flex_type_enum feature_type = feature_types[feat];

    // Error throwing mode.
    data[feat].head(10).apply([mean, feature_type]
      (const flexible_type& x) {
          return mean_imputer_apply(x, mean, feature_type);
      }, output_type, false).materialize();

    // Tranform mode.
    ret_sf[output_column_name] = data[feat].apply([mean, feature_type]
      (const flexible_type& x) {
          return mean_imputer_apply(x, mean, feature_type);
      }, output_type, false);
  }
  return ret_sf;
}

} // feature_engineering
} // sdk_model
} // turicreate
