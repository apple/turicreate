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
#include <toolkits/feature_engineering/bm25.hpp>
#include <toolkits/feature_engineering/topk_indexer.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

/**
 * Calculates the document-term frequencies (in parallel).
 * Custom version of create_topk_index_mapping over that keeps all indices.
 * Creates a dictionary of (k, v) pairs where v is the number of documents with
 * term k.
 *
 * \param[in] src - a gl_sarray input (list or dictionary)
 * \param[in] indexer - pointer to topk_indexer (dictionary with (k,v) pairs)
 */
static void create_topk_index_mapping_for_keys(const gl_sarray& src,
                               std::shared_ptr<topk_indexer> indexer) {

  //Setup the indexer.
  indexer->initialize();
  size_t src_size = src.size();

  // Perform the indexing.
  in_parallel([&](size_t thread_idx, size_t num_threads) {

    // Break the SArray into various sizes.
    size_t start_idx = src_size * thread_idx / num_threads;
    size_t end_idx = src_size * (thread_idx + 1) / num_threads;
    flex_type_enum run_mode;

    for (const auto& v: src.range_iterator(start_idx, end_idx)) {
      // Process each document
      flexible_type processed_input;

      if (v.get_type() == flex_type_enum::LIST){
        // Convert List to Dict
        const flex_list& vv = v.get<flex_list>();
        size_t n_values = vv.size();
        std::unordered_map<flexible_type, flexible_type> temp_input;
        for(size_t k = 0; k < n_values; ++k) {
            temp_input[vv[k]]++;
        }
        processed_input = flex_dict(temp_input.begin(),temp_input.end());
        run_mode = flex_type_enum::DICT;
      } else  {
        processed_input = v;
        run_mode = v.get_type();
      }


      switch(run_mode) {
        // Dictionary
        case flex_type_enum::DICT: {

          const flex_dict& dv = processed_input.get<flex_dict>();
          size_t n_values = dv.size();

          for(size_t k = 0; k < n_values; ++k) {
            const std::pair<flexible_type, flexible_type>& kvp = dv[k];
            indexer->insert_or_update(kvp.first, thread_idx);
          }
          break;
        }

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
 * Calculate the average number of words per document in the corpus.
 * \param[in] src  - a gl_sarray input (list or dictionary)
 * \returns  avg_document_length
 */

flexible_type calc_avg_document_lengths(const gl_sarray& src){
  size_t num_documents = src.size();
  gl_sarray doc_lengths;

  switch(src.dtype()){
    // Handle Lists
    case flex_type_enum::LIST:{
      doc_lengths = src.apply([](const flexible_type& x){
        flexible_type document_length = x.get<flex_list>().size();
        return document_length;
      }, flex_type_enum::FLOAT);
      break;
    }
    // Handle Dictionaries
    case flex_type_enum::DICT: {
      doc_lengths = src.apply([](const flexible_type& x){
        flexible_type document_length = 0.0;
        for(const auto& kvp : x.get<flex_dict>()) {
          document_length += kvp.second;
        }
        return document_length;
      }, flex_type_enum::FLOAT);
      break;
    }
    // Should never occur
    default:
      DASSERT_TRUE(false);
      break;
  }
  flexible_type avg_document_size = doc_lengths.sum() / num_documents;
  return avg_document_size;
}

/**
 * Compute summand of a given word for the BM25 score of a test document.
 * \param[in] term_frequency - number of times word appears in test document ~ f(q_i)
 * \param[in] document_length - number of words in the test document ~ |D|
 * \param[in] num_documents - number of documents in the training corpus ~ N
 * \param[in] document_frequency - number of documents in the training corpus with given word ~ n(q_i)
 * \param[in] avg_document_length - average number of words per document in the training corpus ~ d_avg
 * \param[in] k1 - parameter for relative importance of term frequencies
 * \param[in] b - parameter to downweight scores of long documents
 *
 * For more information, see http://en.wikipedia.org/wiki/Okapi_BM25
 * and see src/python/turicreate/toolkits/feature_engineering/_bm25.py
 */
double compute_bm25(const int term_frequency,
                    const size_t document_length,
                    const size_t num_documents,
                    const size_t document_frequency,
                    const double avg_document_length,
                    const double k1,
                    const double b) {
    if (term_frequency <= 0) {
        log_and_throw("Found a nonpositive value. "
          "Only positive numbers are allowed for numeric dictionary values.");
    }
    double idf = log((num_documents - document_frequency + 0.5)/(document_frequency + 0.5));
    double adjusted_tf_numerator = term_frequency * (k1 + 1);
    double adjusted_tf_denominator = term_frequency + k1 * (1 - b + b * document_length/avg_document_length);

    double bm_summand = idf * adjusted_tf_numerator / adjusted_tf_denominator;
    return bm_summand;
}

/**
 *
 * Score a single document
 * \param[in] input          Row of flexible types representing the document.
 * \param[in] indexer        Pointer to document frequencies for each term.
 * \param[in] num_documents
 * \param[in] avg_document_length
 * \param[in] query          List of query terms.
 * \param[in] k1             bm25 parameter
 * \param[in] b              bm25 parameter
 * \param[in] min_t          minimum document frequency threshold
 * \param[in] max_t          maximum document frequency threshold
 *
 * \returns  bm25_score          bm25 score.
 *
 */
double bm25_apply(const flexible_type& input,
            const std::shared_ptr<topk_indexer>& indexer,
            const size_t num_documents,
            const double avg_document_length,
            const std::vector<flexible_type>& query,
            const double k1,
            const double b,
            const size_t min_t,
            const size_t max_t) {
  size_t document_frequency;
  double bm25_score = 0.0;
  flexible_type processed_input;
  flex_type_enum run_mode = input.get_type();
  DASSERT_TRUE(indexer != NULL);
  DASSERT_TRUE((run_mode == flex_type_enum::LIST)
               || (run_mode == flex_type_enum::DICT));

  // Process input (document)
  if (run_mode == flex_type_enum::LIST){
    // Convert input to Dict if List
    const flex_list& vv = input.get<flex_list>();
    size_t n_values = vv.size();
    std::unordered_map<flexible_type, flexible_type> temp_input;
    for(size_t k = 0; k < n_values; ++k) {
      temp_input[vv[k]]++;
    }
    processed_input = flex_dict(temp_input.begin(),temp_input.end());
    run_mode = flex_type_enum::DICT;
  } else {
    // Otherwise input is Dict
    processed_input = input;
  }

  // Calculate bm25_score
  switch(run_mode) {
    case flex_type_enum::DICT: {
      const flex_dict& dv = processed_input.get<flex_dict>();
      size_t n_values = dv.size();
      int term_frequency = 0;

      // Calculate Document Length
      size_t document_length = 0;
      for(size_t k = 0; k < n_values; ++k) {
        const std::pair<flexible_type, flexible_type>& kvp = dv[k];
        term_frequency = 0;
        if (kvp.second.get_type() == flex_type_enum::INTEGER) {
          term_frequency = kvp.second.get<flex_int>();
          if (term_frequency <= 0) {
            log_and_throw("Nonpositive dict value found. Only positive numeric values allowed.");
          }
          document_length += term_frequency;
        } else if (kvp.second.get_type() == flex_type_enum::FLOAT) {
          // Round down any floats
          term_frequency = (size_t) kvp.second.get<flex_float>();
          if (term_frequency <= 0) {
            log_and_throw("Nonpositive dict value found. Only positive numeric values allowed.");
          }
          document_length += term_frequency;
        } else {
          // Do nothing
        }
      }

      // Calculate BM25 Score
      for(size_t k = 0; k < n_values; ++k) {
        const std::pair<flexible_type, flexible_type>& kvp = dv[k];
        term_frequency = 0;
        if(std::find(query.begin(), query.end(), kvp.first) != query.end()){
          document_frequency = indexer->lookup_counts(kvp.first);
          if ((document_frequency >= min_t) && (document_frequency <= max_t)){
            if (kvp.second.get_type() == flex_type_enum::INTEGER) {
              term_frequency = kvp.second.get<flex_int>();
              if (term_frequency <= 0) {
                log_and_throw("Nonpositive dict value found. Only positive numeric values allowed.");
              }
              bm25_score += compute_bm25(term_frequency, document_length, num_documents,
                                         document_frequency, avg_document_length, k1, b);
            } else if (kvp.second.get_type() == flex_type_enum::FLOAT) {
              // Round down any floats
              term_frequency = (size_t) kvp.second.get<flex_float>();
              if (term_frequency <= 0) {
                log_and_throw("Nonpositive dict value found. Only positive numeric values allowed.");
              }
              bm25_score += compute_bm25(term_frequency, document_length, num_documents,
                                         document_frequency, avg_document_length, k1, b);
            } else {
              // Do nothing for non-numeric values.
            }
          }
        }
      }

      break;
    }

    // Should never happen here.
    default:
      log_and_throw("Invalid type. Column must be of type int, string,"
                        " list or dictionary.");
      break;
  }
  return bm25_score;
}



/**
 * Initialize the options
 */
void bm25::init_options(const std::map<std::string,flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().size() == 0);

  options.create_real_option(
      "k1",
      "Relative importance of term frequencies",
      1.5,
      0.0,
      100000000000, // infinity
      false);

  options.create_real_option(
      "b",
      "How much to downweight long document scores",
      0.75,
      0.0,
      1.0,
      false);

  options.create_real_option(
      "max_document_frequency",
      "Ignore terms that have document frequency higher than this",
      1.0,
      0.0,
      1.0,
      false);

 options.create_real_option(
     "min_document_frequency",
     "Ignore terms that have document frequency lower than this",
      0.0,
      0.0,
      1.0,
      false);

  options.create_string_option(
     "output_column_name",
     "Name of bm25 output column",
      flex_undefined());

  // Set options!
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Get a version for the object.
 */
size_t bm25::get_version() const {
  return BM25_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void bm25::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  //// Everything else
  oarc << options
       << feature_columns
       << feature_types
       << index_map;
}

/**
 * Load the object using Turi's iarc.
 */
void bm25::load_version(turi::iarchive& iarc, size_t version){
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> feature_columns
       >> feature_types
       >> index_map;
}


/**
 * Initialize the transformer.
 */
void bm25::init_transformer(const std::map<std::string,flexible_type>& _options){
  DASSERT_TRUE(options.get_option_info().size() == 0);

  std::map<std::string, flexible_type> opts;
  for(const auto& k: _options){
    if ((k.first != "features") && (k.first != "query")){
      opts[k.first] = variant_get_value<flexible_type>(k.second);
    }
  }

  init_options(opts);

  // Set features
  feature_columns = _options.at("features");
  DASSERT_TRUE(feature_columns.size() <= 1);

  // Set State Variables to None
  state["features"] = flex_undefined();
  state["num_documents"] = flex_undefined();
  state["document_frequencies"] = flex_undefined();
  state["query"] = _options.at("query");

}


/**
 * Fit the data.
 */
void bm25::fit(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);

  // Get the feature name (Note it is only 1 string)
  std::vector<std::string> fit_features = transform_utils::get_column_names(
                            data, exclude, feature_columns);

  // Select features of the right type.
  fit_features = transform_utils::select_valid_features(data, fit_features,
                      {flex_type_enum::STRING,
                       flex_type_enum::LIST,
                       flex_type_enum::DICT});

  DASSERT_TRUE(feature_columns.size() <= 1);

  // Validate the features.
  transform_utils::validate_feature_columns(data.column_names(), fit_features);
  state["features"] = to_variant(fit_features);
  state["num_documents"] = data.size();

  // Store feature types.
  feature_types.clear();
  for (const auto& f : fit_features) {
    feature_types[f] = data.select_column(f).dtype();
  }

  // Learn the document query-term frequencies
  index_map.clear();
  std::vector<flexible_type> query = variant_get_value<std::vector<flexible_type>>(state["query"]);

  for (const auto& f : fit_features) {

    index_map[f].reset(new topk_indexer(
                     std::numeric_limits<int>::max(),
                     0, (size_t) - 1,
                     f));

    if (feature_types[f] == flex_type_enum::STRING){
      create_topk_index_mapping_for_keys(data[f].count_words(), index_map[f]);
    } else {
      create_topk_index_mapping_for_keys(data[f], index_map[f]);
    }
    DASSERT_TRUE(index_map[f] != NULL);
  }

  gl_sframe_writer feature_encoding({"feature_column", "term", "document_frequency"},
                         {flex_type_enum::STRING, flex_type_enum::STRING,
                           flex_type_enum::STRING}, 1);
  for (const auto& f : fit_features) {
    const auto& ind = index_map[f];
    for (const auto& val: ind->get_values()) {
      const auto& count = ind->lookup_counts(val);
      // Only show terms in the query set
      if(std::find(query.begin(), query.end(), val) != query.end()){
        if (val != FLEX_UNDEFINED) {
          feature_encoding.write({f, flex_string(val), count}, 0);
        } else {
          feature_encoding.write({f, val, count}, 0);
        }
      }
    }
  }
  state["document_frequencies"] = to_variant(feature_encoding.close());


  // Learn the average document lengths
  flexible_type avg_document_length = 0.0;
  for (const auto& f : fit_features){
    if (feature_types[f] == flex_type_enum::STRING){
      avg_document_length += calc_avg_document_lengths(data[f].count_words());
    } else {
      avg_document_length += calc_avg_document_lengths(data[f]);
    }
  }
  state["average_doc_length"] = avg_document_length;

}

/**
 * Transform the given data.
 */
gl_sframe bm25::transform(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);
  if (index_map.size() == 0) {
    log_and_throw("The BM25 must be fitted before .transform() is called.");
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

  DASSERT_TRUE(transform_features.size() <= 1);
  for (auto &f : transform_features){

    gl_sarray feat;

    if (data[f].dtype() == flex_type_enum::STRING){
      feat = data[f].count_words();
    } else {
      feat = data[f];
    }

    const auto& ind = index_map[f];


    flexible_type output_column_name_opt = variant_get_value<flexible_type>(state.at("output_column_name"));

    std::string output_column_name;

    if (output_column_name_opt.get_type() == flex_type_enum::UNDEFINED) {
      output_column_name = f;
    } else {
      output_column_name = transform_utils::get_unique_feature_name(
        ret_sf.column_names(),output_column_name_opt.get<flex_string>());
    }

    // Apply Local Parameters
    size_t local_num_documents = variant_get_value<flex_int>(state["num_documents"]);
    size_t max_t = variant_get_value<double>(state.at("max_document_frequency")) * local_num_documents;
    size_t min_t = std::ceil(variant_get_value<double>(state.at("min_document_frequency")) * local_num_documents);
    double local_avg_document_length = variant_get_value<double>(state["average_doc_length"]);
    std::vector<flexible_type> local_query = variant_get_value<std::vector<flexible_type>>(state["query"]);
    double k1 = variant_get_value<double>(state["k1"]);//options.value("k1").to<float>();
    double b = variant_get_value<double>(state["b"]);//options.value("b").to<float>();

    // Error checking mode.
    feat.head(10).apply([ind, local_num_documents, local_avg_document_length,
      local_query, k1, b, min_t, max_t](const flexible_type& x){
        return bm25_apply(x, ind, local_num_documents, local_avg_document_length,
                          local_query, k1, b, min_t, max_t);
      }, flex_type_enum::FLOAT).materialize();

    ret_sf[output_column_name] = feat.apply([ind, local_num_documents,
      local_avg_document_length, local_query, k1, b, min_t, max_t](const flexible_type& x){
        return bm25_apply(x, ind, local_num_documents, local_avg_document_length,
                          local_query, k1, b, min_t, max_t);
    }, flex_type_enum::FLOAT);
  }
  return ret_sf;
}

} // namespace feature_engineering
} // namespace sdk_model
} // namespace turi
