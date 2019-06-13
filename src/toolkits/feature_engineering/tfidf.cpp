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
#include <toolkits/feature_engineering/tfidf.hpp>
#include <toolkits/feature_engineering/topk_indexer.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

/**
 * Custom version of create_topk_index_mapping that handles dictionary (k, v)
 * pairs by incrementing the count for k's index by the value, v. This is
 * reasonable behavior for the tfidf's common use case of bag-of-words dicts.
 */
static void create_topk_index_mapping_for_keys(const gl_sarray& src,
                               std::shared_ptr<topk_indexer> indexer) {

  // Setup the indexer.
  indexer->initialize();
  size_t src_size = src.size();

  // Perform the indexing.
  in_parallel([&](size_t thread_idx, size_t num_threads) {

    // Break the SArray into various sizes.
    size_t start_idx = src_size * thread_idx / num_threads;
    size_t end_idx = src_size * (thread_idx + 1) / num_threads;
    flex_type_enum run_mode;

    for (const auto& v: src.range_iterator(start_idx, end_idx)) {
    flexible_type processed_input;
    if (v.get_type() == flex_type_enum::LIST){
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
        // Categorical cols.
        case flex_type_enum::INTEGER:
        case flex_type_enum::UNDEFINED: {
          indexer->insert_or_update(processed_input, thread_idx);
          break;
        }

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
 * Compute tf-idf score for a given (document, term) pair.
 *
 * For more information, see http://en.wikipedia.org/wiki/Tf%E2%80%93idf
 */
double compute_tfidf(size_t num_documents, size_t term_frequency,
                     size_t doc_frequency) {
  if (term_frequency <= 0) {
    log_and_throw("Found a nonpositive value. Only positive numbers are allowed for numeric dictionary values.");
  }

  // 1 added to denominator to prevent divide-by-0, and added to the
  // log() value to prevent negative idf's. Mimics the behaviour of
  // Lucene.
  const auto& idf = log(num_documents / (double) (doc_frequency));
  return term_frequency * idf;
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
flexible_type tfidf_apply(const flexible_type& input,
            const std::shared_ptr<topk_indexer>& indexer,
            const size_t num_documents) {

  // Go through all the cases.
  flex_dict output = {};
  size_t index = 0;
  size_t term_frequency;
  size_t doc_frequency;
  double tfidf_score;
  flexible_type processed_input;
  flex_type_enum run_mode = input.get_type();
  DASSERT_TRUE(indexer != NULL);
  DASSERT_TRUE(run_mode == flex_type_enum::INTEGER
               || run_mode == flex_type_enum::LIST
               || run_mode == flex_type_enum::UNDEFINED
               || run_mode == flex_type_enum::DICT);

  if (run_mode == flex_type_enum::LIST){
    const flex_list& vv = input.get<flex_list>();
    size_t n_values = vv.size();
    std::unordered_map<flexible_type, flexible_type> temp_input;

    for(size_t k = 0; k < n_values; ++k) {
      temp_input[vv[k]]++;
    }
    processed_input = flex_dict(temp_input.begin(),temp_input.end());
    run_mode = flex_type_enum::DICT;
  } else {
    processed_input = input;
  }

  switch(run_mode) {
    case flex_type_enum::UNDEFINED: {
      index = indexer->lookup(processed_input);
      doc_frequency = indexer->lookup_counts(processed_input);
      term_frequency = 1;
      tfidf_score = compute_tfidf(num_documents, term_frequency, doc_frequency);
      if (index != (size_t)-1) {
        output.push_back(std::make_pair(processed_input, tfidf_score));
      }
      break;
    }

    // Dictionary
    case flex_type_enum::DICT: {
      const flex_dict& dv = processed_input.get<flex_dict>();
      size_t n_values = dv.size();
      for(size_t k = 0; k < n_values; ++k) {
        const std::pair<flexible_type, flexible_type>& kvp = dv[k];
        index = indexer->lookup(kvp.first);
        doc_frequency = indexer->lookup_counts(kvp.first);

        int term_frequency = 0;
        if (kvp.second.get_type() == flex_type_enum::INTEGER) {
          term_frequency = kvp.second.get<flex_int>();
          if (term_frequency <= 0) {
            log_and_throw("Nonpositive dict value found. Only positive numeric values allowed.");
          }
          tfidf_score = compute_tfidf(num_documents, term_frequency, doc_frequency);
        } else if (kvp.second.get_type() == flex_type_enum::FLOAT) {
          // Round down any floats
          term_frequency = (size_t) kvp.second.get<flex_float>();
          if (term_frequency <= 0) {
            log_and_throw("Nonpositive dict value found. Only positive numeric values allowed.");
          }

          tfidf_score = compute_tfidf(num_documents, term_frequency, doc_frequency);
        } else {
          // Non-numeric flexible type values are considered to have a
          // "term frequency" of 0 and thus a tf-idf of 0.0.
          tfidf_score = 0.0;
        }

        // Debugging:
        // logprogress_stream << kvp.first << " : "
        //                    << "numdocs " << num_documents << " : "
        //                    << "docfreq " << doc_frequency << " : "
        //                    << "termfreq " << term_frequency << " : "
        //                    << tfidf_score << std::endl;
        //

        if (index != (size_t)-1) {
          output.push_back(std::make_pair(kvp.first, tfidf_score));
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
  return output;
}


/**
 * Initialize the options
 */
void tfidf::init_options(const std::map<std::string,
                                                   flexible_type>&_options){
  // Can only be called once.
  DASSERT_TRUE(options.get_option_info().size() == 0);


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

 options.create_flexible_type_option(
      "output_column_prefix",
      "The prefix to use for the column name of each transformed column.",
      FLEX_UNDEFINED,
      false);

  // Set options!
  options.set_options(_options);
  add_or_update_state(flexmap_to_varmap(options.current_option_values()));
}

/**
 * Get a version for the object.
 */
size_t tfidf::get_version() const {
  return TFIDF_VERSION;
}

/**
 * Save the object using Turi's oarc.
 */
void tfidf::save_impl(turi::oarchive& oarc) const {
  // Save state
  variant_deep_save(state, oarc);

  //// Everything else
  oarc << options
       << feature_columns
       << feature_types
       << index_map
       << num_documents
       << exclude;
}

/**
 * Load the object using Turi's iarc.
 */
void tfidf::load_version(turi::iarchive& iarc, size_t version){
  // State
  variant_deep_load(state, iarc);

  // Everything else
  iarc >> options
       >> feature_columns
       >> feature_types
       >> index_map
       >> num_documents
       >> exclude;
}


/**
 * Initialize the transformer.
 */
void tfidf::init_transformer(const std::map<std::string,
                      flexible_type>& _options){
  DASSERT_TRUE(options.get_option_info().size() == 0);

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
void tfidf::fit(gl_sframe data){
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

  num_documents = data.size();
  state["num_documents"] = data.size();

  // Store feature types.
  feature_types.clear();
  for (const auto& f: fit_features) {
    feature_types[f] = data.select_column(f).dtype();
  }

  // Learn the index mapping.
  index_map.clear();

  for (const auto& feat: fit_features) {

    size_t max_threshold = variant_get_value<double>(state.at("max_document_frequency")) * num_documents;
    size_t min_threshold = std::ceil(variant_get_value<double>(state.at("min_document_frequency")) * num_documents);


    index_map[feat].reset(new topk_indexer(
                     std::numeric_limits<int>::max(),
                     min_threshold, max_threshold,
                     feat));

    if (feature_types[feat] == flex_type_enum::STRING){
      create_topk_index_mapping_for_keys(data[feat].count_words(), index_map[feat]);
    } else {
      create_topk_index_mapping_for_keys(data[feat], index_map[feat]);
    }
    DASSERT_TRUE(index_map[feat] != NULL);
  }

  gl_sframe_writer feature_encoding({"feature_column", "term", "document_frequency"},
                         {flex_type_enum::STRING, flex_type_enum::STRING,
                           flex_type_enum::STRING}, 1);

  for (const auto& f : fit_features) {
    const auto& ind = index_map[f];
    for (const auto& val: ind->get_values()) {
      const auto& count = ind->lookup_counts(val);
      if (val != FLEX_UNDEFINED) {
        feature_encoding.write({f, flex_string(val), count}, 0);
      } else {
        feature_encoding.write({f, val, count}, 0);
      }
    }
  }
  state["document_frequencies"] = to_variant(feature_encoding.close());

}

/**
 * Transform the given data.
 */
gl_sframe tfidf::transform(gl_sframe data){
  DASSERT_TRUE(options.get_option_info().size() > 0);
  if (index_map.size() == 0) {
    log_and_throw("The TFIDF must be fitted before .transform() is called.");
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
  flexible_type output_column_prefix =
      variant_get_value<flexible_type>(state.at("output_column_prefix"));
  if (output_column_prefix == FLEX_UNDEFINED) {
    output_column_prefix = "";
  } else {
    output_column_prefix = output_column_prefix + ".";
  }

  for (auto &col_name : transform_features){

    gl_sarray feat;

    if (data[col_name].dtype() == flex_type_enum::STRING){
      feat = data[col_name].count_words();
    } else {
      feat = data[col_name];
    }

    const auto& ind = index_map[col_name];

    size_t local_num_documents = num_documents;

    // Get the output column name.
    flex_string output_column_name = output_column_prefix + col_name;

    // Error checking mode.
    feat.head(10).apply([ind, local_num_documents](
      const flexible_type& x){
        return tfidf_apply(x, ind, local_num_documents);
      }, flex_type_enum::DICT).materialize();

    ret_sf[output_column_name] =
          feat.apply([ind, local_num_documents](
      const flexible_type& x){
        return tfidf_apply(x, ind, local_num_documents);
    }, flex_type_enum::DICT);
  }
  return ret_sf;
}

} // namespace feature_engineering
} // namespace sdk_model
} // namespace turi
