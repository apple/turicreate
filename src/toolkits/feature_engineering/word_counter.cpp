/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/feature_engineering/word_counter.hpp>
#include <core/logging/assertions.hpp>



namespace turi {
namespace sdk_model {
namespace feature_engineering {

/**
 * Ported from original count_words implementation.
 */
void word_count_delimiters_update(flexible_type f, flex_list delimiter_list, bool to_lower,
                                           std::unordered_map<flexible_type, size_t>& ret_count) {


  std::set<char> delimiters;
  for (auto it = delimiter_list.begin(); it != delimiter_list.end(); ++it) {
    // iterate through flexible_types storing the delimiters
    // cast each to a string and take first char in string
    // insert into std::set for quicker look-ups than turi::flex_list
    delimiters.insert(it->to<std::string>().at(0));
  }

  auto is_delimiter = [delimiters](const char c)->bool {
    return delimiters.find(c) != delimiters.end();
  };

  flex_dict ret;
  const std::string& str = f.get<flex_string>();

  // Tokenizing the string by space, and add to mape
  // Here we optimize for speed to reduce the string malloc
  size_t word_begin = 0;
  // skip leading delimiters
  while (word_begin < str.size() && (is_delimiter(str[word_begin])))
    ++word_begin;

  std::string word;
  flexible_type word_flex;

  for (size_t i = word_begin; i < str.size(); ++i) {
    if (is_delimiter(str[i])) {
      // find the end of thw word, make a substring, and transform to lower case
      word = std::string(str, word_begin, i - word_begin);
      if  (to_lower)
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
      word_flex = std::move(word);

      // add the word to map
      ret_count[word_flex]++;

      // keep skipping delimiters, and reset word_begin
      while (i < str.size() && (is_delimiter(str[i])))
        ++i;
      word_begin = i;
    }
  }

  // add the last word
  if (word_begin < str.size()) {
    word = std::string(str, word_begin, str.size() - word_begin);
    if  (to_lower)
      std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    word_flex = std::move(word);
    ret_count[word_flex]++;
  }

  // convert to dictionary
  for(auto& val : ret_count) {
    ret.push_back({val.first, flexible_type(val.second)});
  }
}


/**
 * For a given flexible_type input, create a bag-of-words representation.
 * Handle undefined, strings, lists, and dict.
 *
 * * string: Tokenize and update a count for each unique token.
 * * list: Throws error on non-string elements. For each element in the list,
 *         tokenize the string and update a count for each unique token.
 * * dict: Process the keys as a list of strings. (See above.)
 *
 * Returns a dict of {token: count[token]}.
 */
flexible_type word_counter_apply_with_manual(const flexible_type& input,
                                             flex_list delimiter_list,
                                             bool to_lower) {
  flex_type_enum run_mode = input.get_type();
  DASSERT_TRUE(run_mode == flex_type_enum::STRING
               || run_mode == flex_type_enum::LIST
               || run_mode == flex_type_enum::DICT
               || run_mode == flex_type_enum::UNDEFINED);

  // Tokenize all string inputs according to delimiters and to_lower options,
  // then accumulate counts and return as dictionary
  flexible_type output;
  std::unordered_map<flexible_type, size_t> ret_count;
  switch(run_mode) {
    case flex_type_enum::UNDEFINED: {
      // No transform required
      output = input;
      break;
    }

    case flex_type_enum::STRING: {
      word_count_delimiters_update(input, delimiter_list, to_lower, ret_count);
      break;
    }

    case flex_type_enum::DICT: {
      const flex_dict& dv = input.get<flex_dict>();
      for(const auto& kvp: dv) {
        // Make sure the keys are strings and values are int or float
        if (kvp.first.get_type() != flex_type_enum::STRING)
          log_and_throw("Invalid type. Dictionary input to WordCounter must have string-typed keys.");
        if (kvp.second.get_type() != flex_type_enum::INTEGER
            && kvp.second.get_type() != flex_type_enum::FLOAT)
          log_and_throw("Invalid type. Dictionary input to WordCounter must have integer or float values.");
        word_count_delimiters_update(kvp.first.get<flex_string>(), delimiter_list, to_lower, ret_count);
      }
      break;
    }

    case flex_type_enum::LIST: {
      const flex_list& vv = input.get<flex_list>();
      for(const auto& elem: vv) {
        // Check that each element a string
        if (elem.get_type() != flex_type_enum::STRING)
          log_and_throw("Invalid type. List input to WordCounter must contain only strings.");

        word_count_delimiters_update(elem.get<flex_string>(), delimiter_list, to_lower, ret_count);
      }
      break;
    }

    // Should never happen here.
    default:
      log_and_throw("Invalid type. Column must be of type string, list or dictionary.");
      break;
  } // switch(run_mode)

  return flex_dict(ret_count.begin(), ret_count.end());
}




/**
 * Map a string, dict, or list to a bag-of-words dictionary.
 *
 * \param[in] input           A flexible_type input of type str, dict, or list
 * \param[in] string_filter   A list of regex-condition pairs for tokenizing the string
 * \param[in] to_lower        A boolean indicating whether or not to convert strings to lower case
 *
 * \returns  output bag-of-words sparse dictionary flexible_type.
 *
 */
flexible_type word_counter_apply_with_regex(const flexible_type& input,
                                            bool to_lower) {
  flex_type_enum run_mode = input.get_type();
  DASSERT_TRUE(run_mode == flex_type_enum::STRING
               || run_mode == flex_type_enum::LIST
               || run_mode == flex_type_enum::DICT
               || run_mode == flex_type_enum::UNDEFINED);

  const transform_utils::string_filter_list& string_filters = transform_utils::ptb_filters;

  // Tokenize all string inputs according to delimiters and to_lower options,
  // then accumulate counts and return as dictionary
  flexible_type output;
  std::unordered_map<flexible_type, flexible_type> ret_count;
  switch(run_mode) {
    case flex_type_enum::UNDEFINED: {
      // No transform required
      output = input;
      break;
    }

    case flex_type_enum::STRING: {
      flex_list tokens_list = transform_utils::tokenize_string(input, string_filters, to_lower);
      for(const auto& token : tokens_list) {
        ret_count[token]++;
      }

      output = flex_dict(ret_count.begin(), ret_count.end());
      break;
    }

    case flex_type_enum::DICT: {
      const flex_dict& dv = input.get<flex_dict>();
      for(const auto& kvp: dv) {
        // Make sure the keys are strings and values are int or float
        if (kvp.first.get_type() != flex_type_enum::STRING)
          log_and_throw("Invalid type. Dictionary input to WordCounter must have string-typed keys.");

        if (kvp.second.get_type() != flex_type_enum::INTEGER
            && kvp.second.get_type() != flex_type_enum::FLOAT)
          log_and_throw("Invalid type. Dictionary input to WordCounter must have integer or float values.");

        flex_list tokens_list = transform_utils::tokenize_string(kvp.first.get<flex_string>(), string_filters, to_lower);
        for(const auto& token: tokens_list) {
          ret_count[token]++;
        }
      }

      output = flex_dict(ret_count.begin(), ret_count.end());

      break;
    }

    case flex_type_enum::LIST: {
      const flex_list& vv = input.get<flex_list>();
      for(const auto& elem: vv) {
        // Check that each element a string
        if (elem.get_type() != flex_type_enum::STRING)
          log_and_throw("Invalid type. List input to WordCounter must contain only strings.");

        flex_list tokens_list = transform_utils::tokenize_string(elem.get<flex_string>(), string_filters, to_lower);
        for(const auto& token: tokens_list) {
          ret_count[token]++;
        }
      }

      output = flex_dict(ret_count.begin(), ret_count.end());
      break;
    }

    // Should never happen here.
    default:
      log_and_throw("Invalid type. Column must be of type string, list or dictionary.");
      break;
  } // switch(run_mode)

  return output;
}

/**
 * Initialize the options
 */
void word_counter::init_options(const std::map<std::string,
                             flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_boolean_option(
      "to_lower",
      "Convert all capitalized letters to lower case",
      true,
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
size_t word_counter::get_version() const {
  return WORD_COUNTER_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void word_counter::save_impl(turi::oarchive& oarc) const {
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
void word_counter::load_version(turi::iarchive& iarc, size_t version){
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
 * Initialize the transformer.
 */
void word_counter::init_transformer(
  const std::map<std::string, flexible_type>& _options){

  // make sure this is the first and only time we are setting the options
  DASSERT_TRUE(options.get_option_info().size() == 0);

  // copy over the new values from input _options
  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if (k.first != "features" && k.first != "exclude"){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }

  // initialize options
  init_options(opts);

  // set internal variables according to specified options
  to_lower = _options.at("to_lower");

  unprocessed_features = _options.at("features");
  exclude = _options.at("exclude");
  if ((int) exclude == 1) {
    state["features"] = to_variant(FLEX_UNDEFINED);
    state["excluded_features"] = to_variant(unprocessed_features);
  } else {
    state["features"] = to_variant(unprocessed_features);
    state["excluded_features"] = to_variant(FLEX_UNDEFINED);
  }

  delimiters = _options.at("delimiters");
}

/**
 * Fit the data and store the feature column names.
 */
void word_counter::fit(gl_sframe data){
  DASSERT_TRUE(state.count("features") > 0);
  DASSERT_TRUE(options.get_option_info().size() > 0);

  // Get the set of features to work with.
  feature_columns = transform_utils::get_column_names(
                            data, exclude, unprocessed_features);


  // Select the features of the right type.
  feature_columns = transform_utils::select_valid_features(data, feature_columns,
                      {flex_type_enum::STRING,
                       flex_type_enum::LIST,
                       flex_type_enum::DICT});

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
 * Transform the given data.
 */
gl_sframe word_counter::transform(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);

  //Check if fitting has already ocurred.
  if (!fitted){
    log_and_throw("The WordCounter must be fitted before .transform() is called.");
  }

  // Decide whether or not to use regex
  const bool use_ptb_tokenizer = (delimiters.get_type() == flex_type_enum::UNDEFINED);
  flex_list delimiter_list;

  if (!use_ptb_tokenizer) {
    if (delimiters.get_type() != flex_type_enum::LIST) {
      log_and_throw("Invalid type. "
                    "WordCounter delimiter must be a list of single-character strings.");
    }
    delimiter_list = delimiters.get<flex_list>();
  }

  // Make a single lambda to use in the apply below.
  bool m_to_lower = to_lower;
  std::function<flexible_type(const flexible_type&)> transform_regex =
      [m_to_lower, delimiter_list](const flexible_type& x){
    return word_counter_apply_with_regex(x, m_to_lower);
  };
  std::function<flexible_type(const flexible_type&)> transform_manual =
      [m_to_lower, delimiter_list](const flexible_type& x){
    return word_counter_apply_with_manual(x, delimiter_list, m_to_lower);
  };
  std::function<flexible_type(const flexible_type&)> transform_fn;
  transform_fn = use_ptb_tokenizer ? transform_regex : transform_manual;

  // Validated column names
  std::vector<std::string> subset_columns;
  subset_columns = transform_utils::select_feature_subset(data, feature_columns);
  transform_utils::validate_feature_types(subset_columns, feature_types, data);

  // Get the set of features to transform.
  std::vector<std::string> transform_features = subset_columns;

  // Original data
  gl_sframe ret_sf = data;

  for (auto &f : transform_features){

    gl_sarray feat = data[f];

    // Get a unique output column column name
    flexible_type output_column_prefix_opt = variant_get_value<flexible_type>(state.at("output_column_prefix"));

    std::string output_column_name;

    if (output_column_prefix_opt.get_type() == flex_type_enum::UNDEFINED) {
      output_column_name = f;
    } else {
      output_column_name = output_column_prefix_opt.get<flex_string>() + "." + f;
    }

    // Do the first few transformations to check for errors
    feat.head(10).apply(
      transform_fn,
      flex_type_enum::DICT
    ).materialize();

    // Set the transform function for the rest of the apply
    ret_sf[output_column_name] = feat.apply(
      transform_fn,
      flex_type_enum::DICT);
  }
  return ret_sf;
}

} // feature_engineering
} // sdk_model
} // turicreate
