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
#include <toolkits/feature_engineering/word_trimmer.hpp>
#include <toolkits/feature_engineering/topk_indexer.hpp>

#include <boost/algorithm/string/join.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {


gl_sframe generate_vocab(std::map<std::string, std::shared_ptr<topk_indexer>> index_map, flex_list stopwords){
  auto names = std::vector<std::string>({"column", "word", "count"});
  auto types = std::vector<flex_type_enum>({flex_type_enum::STRING,
                                            flex_type_enum::STRING,
                                            flex_type_enum::INTEGER});
  gl_sframe_writer writer(names,types,1);

  for (const auto& kvp : index_map){
    std::vector<flexible_type> values = kvp.second->get_values();
    for (const auto& v: values){
      if (std::find(stopwords.begin(), stopwords.end(), v) == stopwords.end()){
        std::vector<flexible_type> row;
        row.push_back(kvp.first);
        row.push_back(v);
        row.push_back(kvp.second->lookup_counts(v));
        writer.write(row, 0);
      }
    }
  }
  return writer.close();
}


/**
 * Ported from original count_words implementation.
 */
flex_list word_count_delimiters(flexible_type f, const flex_list& delimiter_list, bool to_lower) {


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

  flex_list ret;
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
      // find the end of the word, make a substring, and transform to lower case
      word = std::string(str, word_begin, i - word_begin);
      if  (to_lower)
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
      word_flex = std::move(word);

      // add the word to list
      ret.push_back(word_flex);

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
    ret.push_back(word_flex);
  }
  return ret;
}


/**
 * Constructs a top-k indexer.
 *
 * String: The string is tokenized, and each token is inserted into the indexer
 *
 * List of strings: The elements of the list are inserted into the indexer
 *
 * Dictionary of (string, integer) pairs: The string is inserted into the
 * indexer with a count of the integer value
 *
 */

void word_trimmer_topk_index_mapping(const gl_sarray& src,
                               std::shared_ptr<topk_indexer> indexer,
                               bool to_lower, const flexible_type& delimiters) {

  const transform_utils::string_filter_list& string_filters = transform_utils::ptb_filters;

  const bool use_ptb_tokenizer = (delimiters.get_type() == flex_type_enum::UNDEFINED);
  flex_list delimiter_list;

  if (!use_ptb_tokenizer) {
    if (delimiters.get_type() != flex_type_enum::LIST) {
      log_and_throw("Invalid type. "
                    "RareWordTrimmer delimiter must be a list of single-character strings.");
    }
    delimiter_list = delimiters.get<flex_list>();
  }

  // Setup the indexer.
  indexer->initialize();
  size_t src_size = src.size();

  // Perform the indexing.
  in_parallel([&](size_t thread_idx, size_t num_threads) {

    // Break the SArray into various sizes.
    size_t start_idx = src_size * thread_idx / num_threads;
    size_t end_idx = src_size * (thread_idx + 1) / num_threads;

    for (const auto& v: src.range_iterator(start_idx, end_idx)) {
      switch(v.get_type()) {
        // Categorical cols.
        case flex_type_enum::STRING: {
          flex_list tokens_list;
          if (use_ptb_tokenizer){
            tokens_list = transform_utils::tokenize_string(v, string_filters, to_lower);
          } else {
            tokens_list = word_count_delimiters(v, delimiter_list, to_lower);
          }
          for (const auto& token : tokens_list){
            indexer->insert_or_update(token, thread_idx);
          }
          break;
        }

        // Categorical vector
        case flex_type_enum::LIST: {
          const flex_list& vv = v.get<flex_list>();
          size_t n_values = vv.size();
          std::string indexer_key;
          for(size_t k = 0; k < n_values; ++k) {
            if (vv[k].get_type() != flex_type_enum::STRING){
              log_and_throw("Invalid type. List input to RareWordTrimmer must contain only strings.");
            }
            indexer_key = vv[k].get<flex_string>();
            if (to_lower){
              std::transform(indexer_key.begin(), indexer_key.end(), indexer_key.begin(), ::tolower);
            }
            indexer->insert_or_update(indexer_key, thread_idx);
          }
          break;
        }

        // Dictionary
        case flex_type_enum::DICT: {

          const flex_dict& dv = v.get<flex_dict>();
          size_t n_values = dv.size();
          std::string indexer_key;
          for(size_t k = 0; k < n_values; ++k) {
            const std::pair<flexible_type, flexible_type>& kvp = dv[k];
            if (kvp.first.get_type() != flex_type_enum::STRING)
          log_and_throw("Invalid type. Dictionary input to RareWordTrimmer must have string-typed keys.");

            if (kvp.second.get_type() != flex_type_enum::INTEGER)
              log_and_throw("Invalid type. Dictionary input to RareWordTrimmer must have integer or float values.");
            indexer_key = kvp.first.get<flex_string>();
            if (to_lower){
              std::transform(indexer_key.begin(), indexer_key.end(), indexer_key.begin(), ::tolower);
            }

            indexer->insert_or_update(indexer_key, thread_idx, kvp.second);
          }
          break;
        }

        case flex_type_enum::UNDEFINED:
          /* just skip */
          break;

        // Should not be here.
        default:
          DASSERT_TRUE(false);
          break;

      } // End switch
    }  // End range iterator.
  }); // End parallel evaluation

  indexer->finalize();
}


/**
 * Map a collection of categorical types to a sparse indexed representation.
 *
 * \param[in] input          Row of flexible types.
 * \param[in] index_map       index_map for each row.
 * \param[in] start_indices  Start index for each of the columns.
 *
 * \returns  output Indexed flexible type (as dictionary).
 *
 */
flexible_type word_trimmer_apply(const flexible_type& input,
            const std::shared_ptr<topk_indexer>& indexer,
            bool to_lower, const flex_list& stopwords, const flexible_type& delimiters) {

  // Go through all the cases.
  flexible_type output;
  size_t index = 0;
  flex_type_enum run_mode = input.get_type();
  DASSERT_TRUE(indexer != NULL);
  DASSERT_TRUE(run_mode == flex_type_enum::STRING
               || run_mode == flex_type_enum::LIST
               || run_mode == flex_type_enum::UNDEFINED
               || run_mode == flex_type_enum::DICT);

  // do nothing; return input value
  if (run_mode == flex_type_enum::UNDEFINED) return input;

  const transform_utils::string_filter_list& string_filters = transform_utils::ptb_filters;

  const bool use_ptb_tokenizer = (delimiters.get_type() == flex_type_enum::UNDEFINED);
  flex_list delimiter_list;

  if (!use_ptb_tokenizer) {
    if (delimiters.get_type() != flex_type_enum::LIST) {
      log_and_throw("Invalid type. "
                    "RareWordTrimmer delimiter must be a list of single-character strings.");
    }
    delimiter_list = delimiters.get<flex_list>();
  }

  switch(run_mode) {
    // Strings
    case flex_type_enum::STRING: {
      std::vector<std::string> results_list = {};
      flex_list tokens_list;
      if (use_ptb_tokenizer){
        tokens_list = transform_utils::tokenize_string(input.get<flex_string>(), string_filters, to_lower);
      } else {
        tokens_list = word_count_delimiters(input.get<flex_string>(), delimiter_list, to_lower);
      }

      for (const auto& token : tokens_list){
          index = indexer->lookup(token);
          if (index != (size_t)-1 && std::find(stopwords.begin(), stopwords.end(), token) == stopwords.end()) {
            results_list.push_back(token);
          }
      }
      output = boost::algorithm::join(results_list, " ");
      break;
    }

    // Categorical vector
    case flex_type_enum::LIST: {
      flex_list results_list = {};
      std::string indexer_key;
      for (const auto& element : input.get<flex_list>()){
        if (element.get_type() != flex_type_enum::STRING){
          log_and_throw("Invalid type. List input to RareWordTrimmer must contain only strings.");
        }
        indexer_key = element.get<flex_string>();
        if (to_lower){
              std::transform(indexer_key.begin(), indexer_key.end(), indexer_key.begin(), ::tolower);
        }
        index = indexer->lookup(indexer_key);
        if (index != (size_t)-1 && std::find(stopwords.begin(), stopwords.end(), indexer_key) == stopwords.end()) {
          results_list.push_back(indexer_key);
        }
      }
      output = results_list;
      break;
    }

    // Dictionary
    case flex_type_enum::DICT: {
      const flex_dict& dv = input.get<flex_dict>();
      flex_dict results_dict;
      size_t n_values = dv.size();
      std::string indexer_key;
      for(size_t k = 0; k < n_values; ++k) {
        const std::pair<flexible_type, flexible_type>& kvp = dv[k];
        if (kvp.first.get_type() != flex_type_enum::STRING)
          log_and_throw("Invalid type. Dictionary input to RareWordTrimmer must have string-typed keys.");

        if (kvp.second.get_type() != flex_type_enum::INTEGER)
          log_and_throw("Invalid type. Dictionary input to RareWordTrimmer must have integer values.");

        indexer_key = kvp.first.get<flex_string>();
        if (to_lower){
          std::transform(indexer_key.begin(), indexer_key.end(), indexer_key.begin(), ::tolower);
        }

        index = indexer->lookup(indexer_key);
        if (index != (size_t)-1 && std::find(stopwords.begin(), stopwords.end(), indexer_key) == stopwords.end()) {
         results_dict.push_back(std::make_pair(indexer_key, kvp.second));
        }
      }
      output = results_dict;
      break;
    }

    // Should never happen here.
    default:
      log_and_throw("Invalid type. Column must be of type int, string,"
                        " list or dictionary.");
      break;
  }
  return output;
}


/**
 * Initialize the options
 */
void word_trimmer::init_options(const std::map<std::string,
                                                   flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_flexible_type_option(
     "delimiters",
     "List of delimiters for tokenization",
     flex_list({"\r", "\v", "\n", "\f", "\t", " "}),
     false);

  options.create_boolean_option(
      "to_lower",
      "Convert all capitalized letters to lower case",
      true,
      false);

  options.create_string_option(
      "output_column_prefix",
      "The column in the output SFrame where the encoded features are present.",
      FLEX_UNDEFINED);

  options.create_integer_option(
      "threshold",
      "The threshold of occurence counts below which words get trimmed.",
      2,
      1,
      std::numeric_limits<int>::max(),
      false);

  options.create_flexible_type_option(
      "stopwords",
      "A list of manually specified stopwords which are removed from the corpus",
      FLEX_UNDEFINED,
      false         );

  // Set options!
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Get a version for the object.
 */
size_t word_trimmer::get_version() const {
  return WORD_TRIMMER_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void word_trimmer::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  //// Everything else
  oarc << options
       << feature_columns
       << feature_types
       << exclude
       << index_map
       << stopwords
       << delimiters;
}

/**
 * Load the object using Turi's iarc.
 */
void word_trimmer::load_version(turi::iarchive& iarc, size_t version){
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> feature_columns
       >> feature_types
       >> exclude
       >> index_map
       >> stopwords
       >> delimiters;

  to_lower = options.value("to_lower");
}


/**
 * Initialize the transformer.
 */
void word_trimmer::init_transformer(const std::map<std::string,
                      flexible_type>& _options){
  DASSERT_TRUE(options.get_option_info().size() == 0);

  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if (k.first != "features" && k.first != "exclude"){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }

  init_options(opts);

  //Set stopwords
  stopwords =  variant_get_value<flexible_type>(state.at("stopwords"));
  if (stopwords.get_type() == flex_type_enum::UNDEFINED){
    stopwords = flex_list();
  }
  for (const auto& s : stopwords.get<flex_list>()){
    if (s.get_type() != flex_type_enum::STRING) {
      log_and_throw("All elements in the 'stopwords' list must be strings.");
    }
  }


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

  state["vocabulary"] = gl_sframe();
  delimiters = _options.at("delimiters");
  to_lower = _options.at("to_lower");
}

/**
 * Fit the data.
 */
void word_trimmer::fit(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);

  // Get the feature names.
  std::vector<std::string> fit_features = transform_utils::get_column_names(
                            data, exclude, feature_columns);

  // Select features of the right type.
  fit_features = transform_utils::select_valid_features(data, fit_features,
                      {flex_type_enum::STRING,
                       flex_type_enum::LIST,
                       flex_type_enum::DICT});

  // Validate the features.
  transform_utils::validate_feature_columns(data.column_names(), fit_features);
  state["features"] = to_variant(fit_features);

  // Store feature types.
  feature_types.clear();
  for (const auto& f: fit_features) {
    feature_types[f] = data.select_column(f).dtype();
  }

  // Learn the index mapping.
  index_map.clear();

  for (const auto& feat: fit_features) {

    index_map[feat].reset(new topk_indexer(
                     (size_t) (-1),
                     variant_get_value<size_t>(state.at("threshold")),
                     std::numeric_limits<int>::max(),
                     feat));

    word_trimmer_topk_index_mapping(data[feat], index_map[feat], variant_get_value<bool>(state.at("to_lower")), delimiters);
    DASSERT_TRUE(index_map[feat] != NULL);
  }

  state["vocabulary"] = generate_vocab(index_map, stopwords);
}

/**
 * Transform the given data.
 */
gl_sframe word_trimmer::transform(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);
  if (index_map.size() == 0) {
    log_and_throw("The RareWordTrimmer must be fitted before .transform() is called.");
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

  flexible_type local_stopwords = stopwords;
  flexible_type local_delimiters = delimiters;
  bool local_to_lower = to_lower;
  for (auto &f : transform_features){

    flex_type_enum output_type = feature_types[f];
    gl_sarray feat = data[f];
    std::shared_ptr<topk_indexer> indexer = index_map[f];

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
      [indexer, local_stopwords, local_delimiters, local_to_lower](const flexible_type& x ){ return word_trimmer_apply(x, indexer, local_to_lower, local_stopwords, local_delimiters); } ,
      output_type
    ).materialize();

    // Set the transform function for the rest of the apply
    ret_sf[output_column_name] = feat.apply(
      [indexer, local_stopwords, local_delimiters, local_to_lower](const flexible_type& x ){ return word_trimmer_apply(x, indexer, local_to_lower, local_stopwords, local_delimiters); } ,
      output_type);
  }

  return ret_sf;
}


} // namespace feature_engineering
} // namespace sdk_model
} // namespace turi
