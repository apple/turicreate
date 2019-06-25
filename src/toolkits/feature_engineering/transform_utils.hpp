/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TRANSFORM_UTILS_H_
#define TURI_TRANSFORM_UTILS_H_

#include <boost/regex.hpp>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/util/try_finally.hpp>
#include <core/parallel/lambda_omp.hpp>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sarray.hpp>

#include <toolkits/feature_engineering/topk_indexer.hpp>
#include <toolkits/feature_engineering/statistics_tracker.hpp>

namespace turi{
namespace transform_utils{

/**
 * Validate if the set of columns provided by the user is present in the
 * input SFrame.
 *
 *  \param[in] data_column_names    Columns in the dataset.
 *  \param[in] feature_column_names Features provided by user.
 *
 *  \notes Check if set(data_column_names) - set(feature_column_names) is
 *  non empty.
 */
inline void validate_feature_columns(
          const std::vector<std::string>& data_column_names,
          const std::vector<std::string>& feature_column_names,
          bool verbose = true){

  if(feature_column_names.empty()) {
    log_and_throw("No input features are specified.");
  }

  std::set<std::string> data_column_set(data_column_names.begin(),
                                        data_column_names.end());
  std::set<std::string> feature_column_set(feature_column_names.begin(),
                                           feature_column_names.end());

  std::vector<std::string> result;
  std::set_difference(feature_column_set.begin(), feature_column_set.end(),
      data_column_set.begin(), data_column_set.end(),
      inserter(result, result.end()));

  if (result.size() > 0 && verbose){
    std::stringstream err_msg;
    err_msg << "Feature(s) ";
    for (size_t i=0; i < result.size()-1; i++) {
      err_msg << result[i] << ", ";
    }
    err_msg << result[result.size()-1]
            << " are missing from the dataset." << std::endl;
    log_and_throw(err_msg.str());
  }
}


/**
 * Get a unique output feature name based on already existing features.
 *
 *  \param[in] feature_columns    A list of feature_column names to check.
 *  \param[in] output_column_name User's proposed output column name.
 *
 *  \returns A valid output column name that is not present in feature_columns
 *
 *  \note Assumes that feature_names is a subset of feature_types.keys().
 */
inline std::string get_unique_feature_name(
    const std::vector<std::string>& feature_columns,
    const std::string& output_column_name) {

  std::string output_name = output_column_name;
  int counter = 0;
  while (std::find(feature_columns.begin(), feature_columns.end(),
                          output_name) != feature_columns.end()) {
    counter++;
    output_name = output_column_name + "." + std::to_string(counter);
  }
  return output_name;
}

/**
 * Validate if the types of the features are compatible during fit and
 * transform mode.
 *
 *  \param[in] feature_columns  A list of feature_column names to check.
 *  \param[in] feature_types    Fit mode feature types.
 *  \param[in] data             Dataset during transform mode.
 *
 *  \note Assumes that feature_names is a subset of feature_types.keys().
 */
inline void validate_feature_types(
  const std::vector<std::string>& feature_names,
  const std::map<std::string, flex_type_enum>& feature_types,
  const gl_sframe& data) {

  for (auto& col_name : feature_names){
    DASSERT_TRUE(feature_types.count(col_name) > 0);
    auto fit_type = feature_types.at(col_name);
    auto transform_type = data[col_name].dtype();

    if (fit_type != transform_type) {
     log_and_throw("Column '" + col_name + "' was of type " +
      flex_type_enum_to_name(fit_type) + " when fitted using .fit(), but is of type " +
      flex_type_enum_to_name(transform_type) + "during .transform()");
    }
  }
}

/**
 * Checks if particular type is numeric
 */
inline bool is_numeric_type(flex_type_enum type) {
  return (type == flex_type_enum::INTEGER || type == flex_type_enum::FLOAT);
}
/**
 * Checks if particular type is categorical.
 */
inline bool is_categorical_type(flex_type_enum type) {
  return (type == flex_type_enum::INTEGER || type == flex_type_enum::STRING);
}

/**
 * Returns string vector of column names to perform transformation on.
 *
 * \param[in] data      SFrame
 * \param[in] exclude   Flag which determines if feature_columns is an exclude set.
 * \param[in] feature_columns  Include/Excluded features.
 *
 * \returns Set of features to work with.
 *
 */
inline std::vector<std::string> get_column_names(const gl_sframe& data,
                   bool exclude,
                   const flexible_type& feature_columns) {

  std::vector<std::string> feature_columns_vector;
  if (feature_columns.get_type() == flex_type_enum::UNDEFINED){
    feature_columns_vector = data.column_names();
  } else if (!feature_columns.get<flex_list>().size()) {
    feature_columns_vector = data.column_names();
  } else {
    feature_columns_vector = variant_get_value<std::vector<std::string>>(
        to_variant(feature_columns));
  }

  if (exclude){
    std::vector<std::string> data_column_names = data.column_names();
    std::set<std::string> total_set(data_column_names.begin(),
                                                    data_column_names.end());
    std::set<std::string> exclude_set(feature_columns_vector.begin(),
        feature_columns_vector.end());
    std::set<std::string> result;
    std::set_difference(total_set.begin(), total_set.end(),
                        exclude_set.begin(), exclude_set.end(),
                        inserter(result, result.begin()));
    return std::vector<std::string>(result.begin(),result.end());
  } else {
    return feature_columns_vector;
  }
}

/**
 * Subselect features based on input features.
 *
 * \param[in] data      SFrame
 * \param[in] feature_columns  Include/Excluded features.
 *
 * \returns feature_columns \intersect data.column_names()
 *
 */
inline std::vector<std::string> select_feature_subset(const gl_sframe& data,
                   const std::vector<std::string>& feature_columns) {

  std::vector<std::string> data_column_names = data.column_names();
  std::set<std::string> total_set(data_column_names.begin(),
                                  data_column_names.end());
  std::set<std::string> feature_set(feature_columns.begin(),
                                    feature_columns.end());
  std::set<std::string> result;
  std::set_intersection(total_set.begin(), total_set.end(),
                        feature_set.begin(), feature_set.end(),
                        inserter(result, result.begin()));

  if (result.size() != feature_columns.size()) {
    logprogress_stream << "Warning: The model was fit with "
         << feature_columns.size() << " feature columns but only "
         << result.size() << " were present during transform(). "
         << "Proceeding with transform by ignoring the missing columns."
         << std::endl;
  }

  // Need to preserve order.
  std::vector<std::string> ret;
  ret.reserve(result.size());

  for(const auto& s : feature_columns) {
    if(result.count(s)) {
      ret.push_back(s);
    }
  }

  return ret;
}

/**
 * Takes any flexible type and turns it into a flex dict
 * In flex_dict -> out same flex_dict
 * Flex dict output = input
 * In String "x" -> {"x":1}
 * String becomes key, 1 becomes value
 * In list/vec [1,2,3] -> {0:1, 1:2, 2:3}
 * Index becomes key, element becomes value
 * In numeric type ie. 5 -> {0:5}
 * 0 becomes key, numeric value becomes value
 */

inline flex_dict flexible_type_to_flex_dict(const flexible_type& in){
  flex_dict out = {};
  if (in.get_type() == flex_type_enum::DICT){
    out = in;
  } else if (in.get_type() == flex_type_enum::UNDEFINED){
    out.resize(1);
    out[0] = std::make_pair(0,in);
  } else if (in.get_type() == flex_type_enum::STRING){
    out.resize(1);
    out[0] = std::make_pair(in,1);
  } else if (in.get_type() == flex_type_enum::LIST){
    flex_list list = in.get<flex_list>();
    out.resize(list.size());
    for (size_t i = 0 ; i < list.size(); i++){
      out[i] = std::make_pair(i,list[i]);
    }
  } else if (in.get_type() == flex_type_enum::VECTOR) {
    flex_vec vec = in.get<flex_vec>();
    out.resize(vec.size());
    for (size_t i = 0 ; i < vec.size(); i++){
      out[i] = std::make_pair(i,vec[i]);
    }
  } else if (is_numeric_type(in.get_type())){
    out.resize(1);
    out[0] = std::make_pair(0, in);
  }
  return out;
}

/**
 * Takes training_data, feature columns to include/exclude, and exclude bool.
 * Returns columns to perform transformations on.
 */
inline gl_sframe extract_columns(const gl_sframe& training_data,
                                 std::vector<std::string>& feature_columns,
                                 bool exclude) {
  if (!feature_columns.size()) {
    feature_columns = training_data.column_names();
  }
  if (exclude){
    std::vector<std::string> training_data_column_names =
                                            training_data.column_names();
    std::set<std::string> total_set(training_data_column_names.begin(),
                                    training_data_column_names.end());
    std::set<std::string> exclude_set(feature_columns.begin(),
                                      feature_columns.end());

    std::set<std::string> result;
    std::set_difference(total_set.begin(), total_set.end(),
                        exclude_set.begin(), exclude_set.end(),
                        inserter(result, result.begin()));
    return training_data.select_columns(
                    std::vector<std::string>(result.begin(),result.end()));
  } else {
    return training_data.select_columns(feature_columns);
  }
}


/**
 * Utility function for selecting columns of only valid feature types.
 *
 * \param[in] dataset
 *     The input SFrame containing columns of potential features.
 *
 * \param[in] features
 *     List of feature column names. The list cannot be empty.
 *
 * \param[in] valid_feature_types
 *     List of Python types that represent valid features.  If type is array.array,
 *     then an extra check is done to ensure that the individual elements of the array
 *     are of numeric type.  If type is dict, then an extra check is done to ensure
 *     that dictionary values are numeric.
 *
 * \returns
 *     List of valid feature column names.  Warnings are given for each candidate
 *     feature column that is excluded.
 *
 */
inline std::vector<std::string> select_valid_features_nothrow(const gl_sframe&
    dataset,
                const std::vector<std::string>& features,
                const std::vector<flex_type_enum>& valid_feature_types,
                bool verbose = true){

  // Create a map of col types from the col-names.
  std::vector<flex_type_enum> col_types;
  std::map<std::string, flex_type_enum> col_type_map;
  for (size_t i=0; i < features.size(); i++) {
    col_types.push_back(dataset[features[i]].dtype());
    col_type_map[features[i]] = dataset[features[i]].dtype();
  }

  // Check types for valid features.
  std::vector<std::string> valid_features;
  for (size_t i=0; i < features.size(); i++) {
    auto col = features[i];
    auto coltype = col_types[i];

    // Not a valid type. Warn the user.
    if (std::find(valid_feature_types.begin(), valid_feature_types.end(),
                                   coltype) == valid_feature_types.end()) {
      if (verbose){
        logprogress_stream << "WARNING: Column '" << col
           << "' is excluded due to invalid column type ("
           << flex_type_enum_to_name(coltype) <<")." << std::endl;
      }
    // Valid type. Include.
    } else {
      valid_features.push_back(col);
    }
  }

  return valid_features;
}

/**
 * Utility function for selecting columns of only valid feature types. Throws an
 * exception if no features match.
 *
 * \param[in] dataset
 *     The input SFrame containing columns of potential features.
 *
 * \param[in] features
 *     List of feature column names. The list cannot be empty.
 *
 * \param[in] valid_feature_types
 *     List of Python types that represent valid features.  If type is array.array,
 *     then an extra check is done to ensure that the individual elements of the array
 *     are of numeric type.  If type is dict, then an extra check is done to ensure
 *     that dictionary values are numeric.
 *
 * \returns
 *     List of valid feature column names.  Warnings are given for each candidate
 *     feature column that is excluded.
 *
 */
inline std::vector<std::string> select_valid_features(const gl_sframe& dataset,
                const std::vector<std::string>& features,
                const std::vector<flex_type_enum>& valid_feature_types,
                bool verbose = true){

  // Check types for valid features.
  std::vector<std::string> valid_features =
      select_valid_features_nothrow(
          dataset, features, valid_feature_types, verbose);

  // Throw an error if nothing was found to be valid.
  if (valid_features.size() == 0 && verbose ) {
    std::string err_msg = "The input data does not contain any usable feature"
      " columns. This model only supports features of type: ";
    for (size_t k = 0; k < valid_feature_types.size() - 1; ++k){
      err_msg += std::string(
          flex_type_enum_to_name(valid_feature_types[k])) + ", ";
    }
    err_msg += std::string(
        flex_type_enum_to_name(valid_feature_types.back())) + ".";
    log_and_throw(err_msg);
  }
  return valid_features;
}


/**
 * Indexes an SArray of categorical types into an indexed representation.
 *
 * Categorical: If a column is categorical, each unique value is mapped to
 * a unique index in the range 0, ..., n-1, where n is the number of unique
 * values.
 *
 * Numeric: The column type is checked to be INT/FLOAT, then
 * returned as-is.
 *
 * Numeric Vector: If the dictated column type is VECTOR, it is
 * checked to make sure it is numeric and of homogeneous size.
 *
 * Categorical Vector: If the dictated column type is VECTOR, it is
 * checked to make sure it is numeric and of homogeneous size.
 *
 * Dictionary : If the dictated column type is DICT, it is checked to make
 * sure the values are numeric. The keys are then translated to 0..n-1
 * where n is the number of unique keys.
 *
 * \param[in]      src    The SArray to map to indices.
 * \param[in,out] indexer Unique column indexer.
 *
 */
inline void create_topk_index_mapping(const gl_sarray& src,
                               std::shared_ptr<topk_indexer> indexer) {

  // Get the column mode from the dtype.
  flex_type_enum run_mode = src.dtype();

  // Setup the indexer.
  indexer->initialize();
  size_t src_size = src.size();

  // Perform the indexing.
  in_parallel([&](size_t thread_idx, size_t num_threads) {

    // Break the SArray into various sizes.
    size_t start_idx = src_size * thread_idx / num_threads;
    size_t end_idx = src_size * (thread_idx + 1) / num_threads;

    for (const auto& v: src.range_iterator(start_idx, end_idx)) {
      switch(run_mode) {
        // Categorical cols.
        case flex_type_enum::INTEGER:
        case flex_type_enum::UNDEFINED:
        case flex_type_enum::STRING: {
          indexer->insert_or_update(v, thread_idx);
          break;
        }

        // Categorical vector
        case flex_type_enum::LIST: {
          const flex_list& vv = v.get<flex_list>();
          size_t n_values = vv.size();

          for(size_t k = 0; k < n_values; ++k) {
            indexer->insert_or_update(vv[k], thread_idx);
          }
          break;
        }

        // Dictionary
        case flex_type_enum::DICT: {

          const flex_dict& dv = v.get<flex_dict>();
          size_t n_values = dv.size();

          for(size_t k = 0; k < n_values; ++k) {
            const std::pair<flexible_type, flexible_type>& kvp = dv[k];
            flexible_type out_key =
                    flex_string(kvp.first) + ":" + flex_string(kvp.second);
            indexer->insert_or_update(out_key, thread_idx);
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
 * Calculates length of list/vectors in a column src. If not constant length,
 * errors out.
 *
 * \param[in]      src    The SArray to computer mean of.
 * \param[in] column_name Name of column
 *
 */
inline size_t validate_list_vec_length(const gl_sarray& src,const std::string& column_name){

  size_t src_size = src.size();
  flex_list length_list;
  length_list.resize(thread::cpu_count());

  in_parallel([&](size_t thread_idx, size_t num_threads) {

    size_t start_idx = src_size * thread_idx / num_threads;
    size_t end_idx = src_size * (thread_idx + 1) / num_threads;

    flexible_type length = flex_undefined();
    flexible_type old_length = flex_undefined();

    for (const auto& v: src.range_iterator(start_idx, end_idx)){
      if(v.get_type() == flex_type_enum::LIST){
        length = v.get<flex_list>().size();
      }  else if (v.get_type() == flex_type_enum::VECTOR){
        length = v.get<flex_vec>().size();
      }
      if (old_length.get_type() != flex_type_enum::UNDEFINED && old_length != length){
        log_and_throw("All list/vectors in column" + column_name + "must be of same length or None.");
      } else {
        old_length = length;
      }

    }

    length_list[thread_idx] = length;
  });

  flexible_type total_length = flex_undefined();
  flexible_type old_total_length = flex_undefined();
  for (const auto& l: length_list){
    if (l.get_type() != flex_type_enum::UNDEFINED){
      total_length = l;
    }
    if (old_total_length.get_type() != flex_type_enum::UNDEFINED && old_total_length != total_length){
        log_and_throw("All list/vectors in column" + column_name + "must be of same length or None.");
      } else {
        old_total_length = total_length;
      }
  }

  if (total_length.get_type() == flex_type_enum::UNDEFINED){
    log_and_throw("At least one value in column_name" + column_name + "must have"
        " a non-None value");
  }
  return total_length;

}

/**
 * Computes set of all features in a sparse dictionary column
 *
 * \param[in]      src    The SArray to computer mean of.
 * \param[in] column_name Name of column
 * \param[out] out_set The set that will contain all features
 *
 */

inline void num_sparse_features(const gl_sarray& src,const std::string& column_name, std::set<flexible_type>& out_set){

  out_set.clear();
  size_t src_size = src.size();
  std::vector<std::set<flexible_type>> threadlocal_key_set;
  threadlocal_key_set.resize(thread::cpu_count());

  in_parallel([&](size_t thread_idx, size_t num_threads) {

    size_t start_idx = src_size * thread_idx / num_threads;
    size_t end_idx = src_size * (thread_idx + 1) / num_threads;



    for (const auto& d: src.range_iterator(start_idx, end_idx)){
      if(d.get_type() == flex_type_enum::DICT){
        flex_dict dd = d.get<flex_dict>();
        for (const auto& kvp: dd){
          threadlocal_key_set[thread_idx].insert(kvp.first);
        }
      }
    }
  });

  for (const auto& t: threadlocal_key_set){
    for (const auto& k: t){
      out_set.insert(k);
    }
  }

  if(out_set.size() == 0 ){
    log_and_throw("There must be at least one non-None value in dictionary"
        " column for mean imputation");
  }

}

 /**
 * Computes mean of a column. Columns of recursive types have behaviour that
 * is equivalent to unpacking, computing means, then repacking(while preserving
 * sparse interpretation of dictionary columns).
 *
 * \param[in]      src    The SArray to computer mean of.
 * \param[in,out] tracker Unique statistics tracker.
 * \param[in] column_name Name of column
 *
 */
inline void create_mean_mapping(const gl_sarray& src,
                               const std::string& column_name,
                               std::shared_ptr<statistics_tracker> tracker) {

  // Get the column mode from the dtype.
  flex_type_enum run_mode = src.dtype();

  // Setup the indexer.
  tracker->initialize();
  size_t src_size = src.size();
  size_t vec_list_length;
  std::set<flexible_type> sparse_features;



  if (run_mode == flex_type_enum::LIST || run_mode == flex_type_enum::VECTOR){
    vec_list_length = validate_list_vec_length(src, column_name);
  } else if(run_mode == flex_type_enum::DICT){
    num_sparse_features(src, column_name, sparse_features);
  }

  // Perform the indexing.
  in_parallel([&](size_t thread_idx, size_t num_threads) {

    // Break the SArray into various sizes.
    size_t start_idx = src_size * thread_idx / num_threads;
    size_t end_idx = src_size * (thread_idx + 1) / num_threads;

    for (const auto& v: src.range_iterator(start_idx, end_idx)) {
      switch(run_mode) {
        // Numerical cols.
        case flex_type_enum::INTEGER:
        case flex_type_enum::FLOAT: {
          flexible_type key = 0;
          tracker->insert_or_update(key, v, thread_idx);
          break;
        }

        // Categorical vector
        case flex_type_enum::LIST: {
          flex_list vv;
          if (v.get_type() != flex_type_enum::UNDEFINED){
            vv = v.get<flex_list>();
          }
          size_t n_values = vec_list_length;
          for(size_t k = 0; k < n_values; ++k) {
            if (v.get_type() != flex_type_enum::UNDEFINED){
              if (!transform_utils::is_numeric_type(vv[k].get_type())
                  && vv[k].get_type() != flex_type_enum::UNDEFINED){
                  log_and_throw("All list elements must be numeric for mean"
                  " imputation");
              }
              tracker->insert_or_update(k, vv[k], thread_idx);
            } else {
              tracker->insert_or_update(k, flex_undefined(), thread_idx);
            }
          }
          break;
        }

        // Numerical vector
        case flex_type_enum::VECTOR: {
          flex_vec vv = flex_vec();
          if (v.get_type() != flex_type_enum::UNDEFINED){
            vv = v.get<flex_vec>();
          }
          size_t n_values = vec_list_length;
          for(size_t k = 0; k < n_values; ++k) {
            if(v.get_type() != flex_type_enum::UNDEFINED){
              tracker->insert_or_update(k, vv[k], thread_idx);
            } else{
              tracker->insert_or_update(k, flex_undefined(), thread_idx);
            }
          }
          break;
        }


        // Dictionary
        case flex_type_enum::DICT: {

          if (v.get_type() != flex_type_enum::UNDEFINED) {
            const flex_dict& dv = v.get<flex_dict>();
            size_t n_values = dv.size();

            for(size_t k = 0; k < n_values; ++k) {
              const std::pair<flexible_type, flexible_type>& kvp = dv[k];
              if (!transform_utils::is_numeric_type(kvp.second.get_type())
                  && kvp.second.get_type() != flex_type_enum::UNDEFINED){
                log_and_throw("All dictionary entries must be numeric for mean"
                    "imputation");
              }
              tracker->insert_or_update(kvp.first,kvp.second, thread_idx);
            }
          } else {

            for(const auto& v : sparse_features){
              tracker->insert_or_update(v, flex_undefined(), thread_idx);
            }

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

  tracker->finalize(src_size);
}

/////////////////////////////////////
// Utilities for string tokenization
/////////////////////////////////////

// A funtion that checks whether or not the input string should be filtered.
typedef std::function<bool(const std::string&)> string_filter_condition;
// A list of filtering conditions and regex filtering patterns.
typedef std::vector<std::pair<boost::regex, string_filter_condition> > string_filter_list;

/**
 * An approximate Penn Tree Bank tokenization filter.
 *
 * TODO: this should
 * 1) account for multi-word proper nouns
 * 2) separate scientific units from values
 * 3) keep periods at the end of abbreviations
 * 4) keep slashes at the end of urls
 * 5) capture sequences of punctuation like emoticons, ellipses, and "?!?!?!?!"
 * 6) bug: children's is being tokenized as "childre n's"
 */
static const string_filter_list ptb_filters = {
  std::make_pair(
    boost::regex(
      std::string("([+.-]?[0-9]+([.,()-]+[0-9]+)*)|") +    // positive and negative real numbers, and phone numbers with no spaces
      std::string("([^\\w\\s])|") +                        // separates individual punctuation marks
      std::string("(\\b[^\\w\\s]+)|") +                    // leading punctuation (e.g. leading quotation or dollar sign)
      std::string("([\\w]([^\\s]*[\\w])?)|") +             // sequences of non-space characters with leading and trailing letters/numbers (e.g. urls, emails)
      std::string("([^\\w\\s]+\\b)")),                     // trailing punctuation (e.g. trailing quotations, sentence-final punctuation)
    [](const std::string& current){return true;}),
  std::make_pair(
    boost::regex(
      std::string("([nN]?'\\w*)|([^\\s']*[^nN\\s'])")),   // separate contractions and possessives
    [](const std::string& current){return current.find("'") != std::string::npos;})
};

/**
 * Tokenizes the input string according to the input filtering patterns,
 * returns a flex_list of the token strings.
 *
 * \param[in]  to_tokenize    The string to tokenize.
 * \param[in]  filter_list    A list of regex patterns and string_filter_conditions.
 * \param[out] A list of string tokens, tokenized according to the tokenization patterns.
 *
 * The filter_list offers a way to take some logic that might overcomplicate a
 * regex and export it to a filter list comprehension. For each filter,
 * for each item in the current token list (at the beginning, a singleton
 * list with the full doc string), the filter condition is checked. If the
 * condition is satisfied, the regex is applied, and the resulting list is
 * inserted in place of the original token. Otherwise, the original token
 * is placed back into the list.
 *
 */
inline flex_list tokenize_string(const std::string& to_tokenize,
                                 const string_filter_list& filter_list,
                                 const bool to_lower) {
  flex_list previous = {to_tokenize};
  boost::sregex_token_iterator end;

  if (to_lower) {
    std::string str_copy = std::string(to_tokenize);
    std::transform(str_copy.begin(), str_copy.end(), str_copy.begin(), ::tolower);
    previous = {str_copy};
  }

  // The algorithm operates recursively:
  //   - start with the original single string to be tokenized,
  //   - tokenize that string according to the first pattern in the filter list,
  //   - further tokenize each token according to subsequent patterns in the filter list.
  //
  // For each filter pattern in the given filter_list:
  //   - Check the condition on the current token
  //   - if condition is satisfied, tokenize the token using the regex pattern
  //   - otherwise, place the original token back into the list
  //for (auto filter = filter_list.begin(); filter != filter_list.end(); ++filter)
  for (const auto& filter : filter_list)
  {
    std::vector<std::string> current;
    boost::smatch matches;
    const boost::regex& expr = filter.first;
    string_filter_condition condition = filter.second;

    for (const auto& token : previous)
    {
      const std::string& token_string = token.to<std::string>();

      if ( condition(token_string) )
      {
        boost::sregex_token_iterator match(token_string.begin(), token_string.end(), expr, 0);

        for ( ; match != end; ++match)
        {
          current.push_back(*match);
        }
      }
      else
      {
        current.push_back(token);
      }
    }

    previous = flex_list(current.begin(), current.end());
  }

  return previous;
}


}// transform_utils
}//turicreate
#endif
