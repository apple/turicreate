/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/variant_deep_serialize.hpp>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <toolkits/feature_engineering/tokenizer.hpp>
#include <toolkits/feature_engineering/transform_utils.hpp>
#include <core/logging/assertions.hpp>



namespace turi {
namespace sdk_model {
namespace feature_engineering {

/**
 * Initialize the options
 */
void tokenizer::init_options(const std::map<std::string,
                             flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_boolean_option(
      "to_lower",
      "Convert all capitalized letters to lower case",
      false,
      false);

  options.create_string_option(
     "output_column_prefix",
     "Prefix of word_counter output column",
      flex_undefined());

  options.create_flexible_type_option(
     "delimiters",
     "List of delimiters for tokenization",
     flex_list({"\r", "\v", "\n", "\f", "\t", " "}),
     false);

  // Set options!
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Get a version for the object.
 */
size_t tokenizer::get_version() const {
  return TOKENIZER_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void tokenizer::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  //// Everything else
  oarc << options
       << fitted
       << to_lower
       << exclude
       << feature_columns
       << feature_types
       << unprocessed_features
       << delimiters;
}

/**
 * Load the object using Turi's iarc.
 */
void tokenizer::load_version(turi::iarchive& iarc, size_t version){

  if (version == 0) { // version 0 save & load is broken, warn and exit
    log_and_throw("Known issue: Version 0 of Tokenizer cannot be loaded. "
      "Please update the object using the latest version of Turi Create.");
    return;
  }

  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> fitted
       >> to_lower
       >> exclude
       >> feature_columns
       >> feature_types
       >> unprocessed_features
       >> delimiters;
}

/**
 * Fit the data and store the feature column names.
 */
void tokenizer::fit(gl_sframe data){
  DASSERT_TRUE(state.count("features") > 0);
  DASSERT_TRUE(options.get_option_info().size() > 0);

  // Get the set of features to work with.
  feature_columns = transform_utils::get_column_names(
                            data, exclude, unprocessed_features);


  // Select the features of the right type.
  feature_columns = transform_utils::select_valid_features(data, feature_columns,
                      {flex_type_enum::STRING});

  transform_utils::validate_feature_columns(data.column_names(), feature_columns);

  // Store feature types and cols.
  feature_types.clear();
  for (const auto& f: feature_columns) {
    feature_types[f] = data.select_column(f).dtype();
  }

  // Update state based on the new set of features.
  state["features"] = to_variant(feature_columns);

  fitted = true;
}

/**
 * If the delimiter option is set to "None," then use the Penn treebank tokenization.
 * Otherwise, setup the tokenization regex using the specified delimiters.
 * (Default is to conform to the behavior of count_words, which uses a list of
 *  space characters.)
 */
void tokenizer::set_string_filters(){
  DASSERT_TRUE(options.get_option_info().size() > 0);

  switch (delimiters.get_type()) {
    case flex_type_enum::UNDEFINED: {  // Use Penn treebank-style tokenization
      string_filters = transform_utils::ptb_filters;
      break;
    }

    case flex_type_enum::LIST: {
      // Tokenize using custom delimiters list
      flex_list delimiter_list = delimiters.get<flex_list>();
      std::string all_delims = "";
      for (auto elem = delimiter_list.begin(); elem != delimiter_list.end(); ++elem) {
        if (elem->get_type() != flex_type_enum::STRING)
          log_and_throw("Invalid type. Tokenizer delimiters must be strings.");

        all_delims += elem->get<flex_string>().substr(0,1);
      }

      string_filters = transform_utils::string_filter_list({
        std::make_pair(
          boost::regex(std::string("([^" + all_delims + "]+)")),   // a token is any word that does not containing a delimiter
          [](const std::string& current){return true;}
          )
      });
      break;
    }

    default:
      log_and_throw("Invalid type. "
        "Tokenizer delimiter must be a list of single-character strings.");
      break;

  }
}

/**
 * Initialize the transformer.
 */
void tokenizer::init_transformer(
  const std::map<std::string, flexible_type>& _options){

  DASSERT_TRUE(options.get_option_info().size() == 0);

  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if (k.first != "features" && k.first != "exclude"){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }
  init_options(opts);

  // set internal variables according to specified options
  to_lower = _options.at("to_lower");

  unprocessed_features = _options.at("features");
  exclude = _options.at("exclude");
  if (int(exclude) == 1) {
    state["features"] = to_variant(FLEX_UNDEFINED);
    state["excluded_features"] = to_variant(unprocessed_features);
  } else {
    state["features"] = to_variant(unprocessed_features);
    state["excluded_features"] = to_variant(FLEX_UNDEFINED);
  }

  delimiters = _options.at("delimiters");
  set_string_filters();

}

/**
 * Transform the given data.
 */
gl_sframe tokenizer::transform(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);

  //Check if fitting has already ocurred.
  if (!fitted){
    log_and_throw("The Tokenizer must be fitted before .transform() is called.");
  }

  set_string_filters();

  // Validated column names
  std::vector<std::string> subset_columns;
  subset_columns = transform_utils::select_feature_subset(data, feature_columns);
  transform_utils::validate_feature_types(subset_columns, feature_types, data);

  // Get the set of features to transform.
  std::vector<std::string> transform_features = subset_columns;

  // Original data
  gl_sframe ret_sf = data;
  flexible_type output_column_prefix =
      variant_get_value<flexible_type>(state.at("output_column_prefix"));
  if (output_column_prefix == FLEX_UNDEFINED) {
    output_column_prefix = "";
  } else {
    output_column_prefix = output_column_prefix + ".";
  }

  for (auto &f : transform_features){

    gl_sarray feat = data[f];

    // Get the output column name.
    flex_string output_column_name = output_column_prefix + f;

    transform_utils::string_filter_list m_string_filters = string_filters;
    bool m_to_lower = to_lower;
    auto transformfn = [m_string_filters, m_to_lower](const flexible_type& x){
      return transform_utils::tokenize_string(x, m_string_filters, m_to_lower);
    };

    // Error checking mode.
    feat.head(10).apply(
      transformfn,
      flex_type_enum::LIST
    ).materialize();

    ret_sf[output_column_name] = feat.apply(
      transformfn,
      flex_type_enum::LIST);
  }
  return ret_sf;
}

} // feature_engineering
} // sdk_model
} // turicreate
