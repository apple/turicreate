/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <toolkits/feature_engineering/ngram_counter.hpp>
#include <core/logging/assertions.hpp>



namespace turi {
namespace sdk_model {
namespace feature_engineering {

/**
 * Update a given ngram dictionary with new ngrams from the input token list
 *
 * \param[in] ngram_dict      A dictionary mapping from string to counts (could be float or int type)
 * \param[in] word_list       A list of words/tokens to count
 * \param[in] n               The length of the ngram
 * \param[in] weight          An integer or float indicating the weight/count of each ngram
 *
 */
void update_ngram_dictionary(std::unordered_map<flex_string, flexible_type>& ngram_dict,
                             const flex_list& word_list,
                             const size_t n,
                             const flexible_type weight) {
  if (word_list.size() < n)
    return;

  for (size_t k = 0; k < word_list.size()-n+1; ++k) {
    // Compute size, preallocate, and form the ngram
    size_t ngram_len = word_list[k].size();
    for(size_t i = 1; i < n; ++i)
       ngram_len += word_list[k+i].get<flex_string>().size() + 1;
    std::string ngram_str;
    ngram_str.reserve(ngram_len);

    ngram_str += word_list[k].get<flex_string>();
    for(size_t i = 1; i < n; ++i) {
      ngram_str += " ";
      ngram_str += word_list[k+i].get<flex_string>();
    }

    // Update ngram dictionary
    if (weight.get_type() == flex_type_enum::FLOAT)
      ngram_dict[ngram_str] = weight.to<flex_float>() + ngram_dict[ngram_str].to<flex_float>();
    else
      ngram_dict[ngram_str] += weight;
  }
}

/**
 * Map a string, dict, or list to a bag-of-ngrams dictionary.
 *
 * \param[in] input           A flexible_type input of type str, dict, or list
 * \param[in] n               An integer specifying the size of the ngram
 * \param[in] string_filter   A list of regex-condition pairs for tokenizing the string
 * \param[in] to_lower        A boolean indicating whether or not to convert strings to lower case
 *
 * \returns  output bag-of-ngrams sparse dictionary flexible_type.
 *
 */
flexible_type word_ngram_counter_apply(const flexible_type& input,
                                       const size_t n,
                                       const transform_utils::string_filter_list& string_filters,
                                       const bool to_lower) {
  flex_type_enum run_mode = input.get_type();
  DASSERT_TRUE(run_mode == flex_type_enum::STRING
               || run_mode == flex_type_enum::LIST
               || run_mode == flex_type_enum::DICT
               || run_mode == flex_type_enum::UNDEFINED);

  // Tokenize all string inputs according to delimiters and to_lower options,
  // then accumulate counts and return as dictionary
  std::unordered_map<flex_string, flexible_type> ret_count;
  flexible_type output;
  switch(run_mode) {
    case flex_type_enum::UNDEFINED: {
      // No transform required
      output = input;
      break;
    }

    case flex_type_enum::STRING: {
      flex_list tokens_list = transform_utils::tokenize_string(input, string_filters, to_lower);

      update_ngram_dictionary(ret_count, tokens_list, n, 1);

      output = flex_dict(ret_count.begin(), ret_count.end());
      break;
    }

    case flex_type_enum::DICT: {
      const flex_dict& dv = input.get<flex_dict>();
      for(const auto& kvp: dv) {
        // Make sure the keys are strings and values are int or float
        if (kvp.first.get_type() != flex_type_enum::STRING)
          log_and_throw("Invalid type. Dictionary input to NGramCounter must have string-typed keys.");

        if (kvp.second.get_type() != flex_type_enum::INTEGER
            && kvp.second.get_type() != flex_type_enum::FLOAT)
          log_and_throw("Invalid type. Dictionary input to NGramCounter must have integer or float values.");

        flex_list tokens_list = transform_utils::tokenize_string(kvp.first.get<flex_string>(), string_filters, to_lower);

        update_ngram_dictionary(ret_count, tokens_list, n, kvp.second);
      }

      output = flex_dict(ret_count.begin(), ret_count.end());

      break;
    }

    case flex_type_enum::LIST: {
      const flex_list& vv = input.get<flex_list>();
      for(const auto& elem: vv) {
        // Check that each element a string
        if (elem.get_type() != flex_type_enum::STRING)
          log_and_throw("Invalid type. List input to NGramCounter must contain only strings.");

        flex_list tokens_list = transform_utils::tokenize_string(elem.get<flex_string>(), string_filters, to_lower);

        update_ngram_dictionary(ret_count, tokens_list, n, 1);
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
 * Update a given ngram dictionary with new ngrams from the input token list
 *
 * \param[in] ngram_dict      A dictionary mapping from string to counts (could be float or int type)
 * \param[in] n               The length of the ngram
 * \param[in] weight          An integer or float indicating the weight/count of each ngram
 *
 */
void update_character_ngram_dictionary(std::unordered_map<flex_string, flexible_type>& ngram_dict,
                                       const flex_string& input_str,
                                       const size_t n,
                                       const bool ignore_punct,
                                       const bool ignore_space,
                                       const bool to_lower,
                                       const flexible_type weight) {
  // Do a string copy if need to convert to lowercase
  std::string lower;
  if (to_lower){
    lower = std::string(input_str);
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  }
  const std::string& str =  (to_lower) ? lower : input_str;

  if (str.size() < n)
    return;

  std::string ngram_str;
  ngram_str.resize(n);

  for (size_t k = 0; k < str.size()-n+1; ++k) {
    // skip ahead to next acceptable character
    while (k < str.size()
      && ((std::ispunct(str[k]) && ignore_punct)
          || (std::isspace(str[k]) && ignore_space))) {
      ++k;
    }

    // accumulate n acceptable characters into the string
    size_t ind = k;
    size_t cur_len = 0;
    while (cur_len < n) {
      while (ind < str.size()
        && ((std::ispunct(str[ind]) && ignore_punct)
            || (std::isspace(str[ind]) && ignore_space))) { // find next char
        ++ind;
      }
      if (ind >= str.size()) // Done - reached the end of the string
        break;

      ngram_str[cur_len] = str[ind];
      ++cur_len;
      ++ind;
    }

    // if successfully found n characters, then update count for the ngram
    if (cur_len == n) {
      // Update ngram dictionary
      if (weight.get_type() == flex_type_enum::FLOAT)
        ngram_dict[ngram_str] = weight.to<flex_float>() + ngram_dict[ngram_str].to<flex_float>();
      else
        ngram_dict[ngram_str] += weight;
    }
  }
}

/**
 * Map a string, dict, or list to a bag-of-character-ngrams dictionary.
 *
 * \param[in] input           A flexible_type input of type str, dict, or list
 * \param[in] n               An integer specifying the size of the ngram
 * \param[in] ignore_space    A boolean indicating whether or not to ignore space characters
 * \param[in] to_lower        A boolean indicating whether or not to convert strings to lower case
 *
 * \returns  output bag-of-ngrams sparse dictionary flexible_type.
 *
 */
flexible_type character_ngram_counter_apply(const flexible_type& input,
                                            const size_t n,
                                            const bool ignore_punct,
                                            const bool ignore_space,
                                            const bool to_lower) {
  flex_type_enum run_mode = input.get_type();
  DASSERT_TRUE(run_mode == flex_type_enum::STRING
               || run_mode == flex_type_enum::LIST
               || run_mode == flex_type_enum::DICT
               || run_mode == flex_type_enum::UNDEFINED);

  // Tokenize all string inputs according to delimiters and to_lower options,
  // then accumulate counts and return as dictionary
  std::unordered_map<flex_string, flexible_type> ret_count;
  flexible_type output;
  switch(run_mode) {
    case flex_type_enum::UNDEFINED: {
      // No transform required
      output = input;
      break;
    }

    case flex_type_enum::STRING: {

      update_character_ngram_dictionary(
        ret_count, input.get<flex_string>(), n, ignore_punct, ignore_space, to_lower, 1);

      output = flex_dict(ret_count.begin(), ret_count.end());
      break;
    }

    case flex_type_enum::DICT: {
      const flex_dict& dv = input.get<flex_dict>();
      for(const auto& kvp: dv) {
        // Make sure the keys are strings and values are int or float
        if (kvp.first.get_type() != flex_type_enum::STRING)
          log_and_throw("Invalid type. Dictionary input to NGramCounter must have string-typed keys.");

        if (kvp.second.get_type() != flex_type_enum::INTEGER
            && kvp.second.get_type() != flex_type_enum::FLOAT)
          log_and_throw("Invalid type. Dictionary input to NGramCounter must have integer or float values.");

        update_character_ngram_dictionary(
          ret_count, kvp.first.get<flex_string>(), n, ignore_punct, ignore_space, to_lower, kvp.second);
      }

      output = flex_dict(ret_count.begin(), ret_count.end());

      break;
    }

    case flex_type_enum::LIST: {
      std::unordered_map<flex_string, flexible_type> ret_count;
      const flex_list& vv = input.get<flex_list>();
      for(const auto& elem: vv) {
        // Check that each element a string
        if (elem.get_type() != flex_type_enum::STRING)
          log_and_throw("Invalid type. List input to NGramCounter must contain only strings.");

        update_character_ngram_dictionary(
          ret_count, elem.get<flex_string>(), n, ignore_punct, ignore_space, to_lower, 1);
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
void ngram_counter::init_options(const std::map<std::string,
                             flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_integer_option(
      "n",
      "N",
      2,
      1,
      std::numeric_limits<int>::max(),
      false);

  options.create_boolean_option(
      "to_lower",
      "Convert all capitalized letters to lower case",
      true,
      false);

  options.create_string_option(
      "ngram_type",
      "Type of ngram (word or character)",
      "word",
      false);

  options.create_boolean_option(
      "ignore_punct",
      "Ignore punctuation characters in character ngrams",
      true,
      false);

  options.create_boolean_option(
      "ignore_space",
      "Ignore space characters in character ngrams",
      true,
      false);

  options.create_string_option(
     "output_column_prefix",
     "Prefix of ngram_counter output column",
      flex_undefined());

  options.create_flexible_type_option(
     "delimiters",
     "List of delimiters for tokenization",
     flex_list({"\r", "\v", "\n", "\f", "\t", " ",
                "!", "#", "$", "%", "&", "'", "(", ")",
                "*", "+", ",", "-", ".", "/", ":", ";",
                "<", "=", ">", "?", "@", "[", "\\", "]",
                "^", "_", "`", "{", "|", "}", "~"}),
     false);

  // Set options!
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Get a version for the object.
 */
size_t ngram_counter::get_version() const {
  return NGRAM_COUNTER_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void ngram_counter::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  //// Everything else
  oarc << options
       << n
       << fitted
       << to_lower
       << ngram_type
       << ignore_punct
       << ignore_space
       << exclude
       << feature_columns
       << feature_types
       << unprocessed_features
       << delimiters;
}

/**
 * Load the object using Turi's iarc.
 */
void ngram_counter::load_version(turi::iarchive& iarc, size_t version){
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> n
       >> fitted
       >> to_lower
       >> ngram_type
       >> ignore_punct
       >> ignore_space
       >> exclude
       >> feature_columns
       >> feature_types
       >> unprocessed_features
       >> delimiters;
}

/**
 * If the delimiter option is set to "None," then use the Penn treebank tokenization.
 * Otherwise, setup the tokenization regex using the specified delimiters.
 * (Default is to conform to the behavior of count_words, which uses a list of
 *  space characters.)
 */
void ngram_counter::set_string_filters(){

  if (delimiters.get_type() == flex_type_enum::UNDEFINED) {  // Use Penn treebank-style tokenization
    string_filters = transform_utils::ptb_filters;
    return;
  }

  if (delimiters.get_type() != flex_type_enum::LIST) {
    log_and_throw("Invalid type. "
        "NGramCounter delimiter must be a list of single-character strings.");
  }

  // Tokenize using custom delimiters list
  flex_list delimiter_list = delimiters.get<flex_list>();
  std::string all_delims = "";
  for (auto elem = delimiter_list.begin(); elem != delimiter_list.end(); ++elem) {
    if (elem->get_type() != flex_type_enum::STRING)
      log_and_throw("Invalid type. NGramCounter delimiters must be strings.");

    all_delims += elem->get<flex_string>().substr(0,1);
  }

  string_filters = transform_utils::string_filter_list({
    std::make_pair(
      boost::regex(std::string("([^" + all_delims + "]+)")),   // a token is any word that does not containing a delimiter
      [](const std::string& current){return true;}
      )
  });
}

/**
 * Initialize the transformer.
 */
void ngram_counter::init_transformer(
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
  n = _options.at("n");
  to_lower = _options.at("to_lower");
  ignore_punct = _options.at("ignore_punct");
  ignore_space = _options.at("ignore_space");
  ngram_type = _options.at("ngram_type").get<flex_string>();

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
  set_string_filters();
}

/**
 * Fit the data and store the feature column names.
 */
void ngram_counter::fit(gl_sframe data){
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
gl_sframe ngram_counter::transform(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);

  //Check if fitting has already ocurred.
  if (!fitted){
    log_and_throw("The NGramCounter must be fitted before .transform() is called.");
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

    std::function<flexible_type(const flexible_type&)> transformfn;
    transform_utils::string_filter_list m_string_filters = string_filters;
    bool m_to_lower = to_lower;
    bool m_ignore_punct = ignore_punct;
    bool m_ignore_space = ignore_space;
    size_t m_n = n;

    if (ngram_type == "word") {
      transformfn = [m_n, m_string_filters, m_to_lower](const flexible_type& x){
        return word_ngram_counter_apply(x, m_n, m_string_filters, m_to_lower);
      };
    }
    else {
      transformfn = [m_n, m_ignore_punct, m_ignore_space, m_to_lower](const flexible_type& x){
        return character_ngram_counter_apply(x, m_n, m_ignore_punct, m_ignore_space, m_to_lower);
      };
    }

    // Do the first few transformations to check for errors
    feat.head(10).apply(
      transformfn,
      flex_type_enum::DICT
    ).materialize();

    // Set the transform function for the rest of the apply
    ret_sf[output_column_name] = feat.apply(
      transformfn,
      flex_type_enum::DICT);
  }
  return ret_sf;
}

} // feature_engineering
} // sdk_model
} // turicreate
