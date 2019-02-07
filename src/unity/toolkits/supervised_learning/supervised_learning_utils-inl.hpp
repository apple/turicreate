/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SUPERVISED_LEARNING_UTILS_H_
#define TURI_SUPERVISED_LEARNING_UTILS_H_

#include <Eigen/LU>
// SFrame
#include <sframe/sarray.hpp>
#include <sframe/sframe.hpp>

// ML-Data Utils
#include <ml_data/ml_data.hpp>
#include <ml_data/metadata.hpp>
#include <util/testing_utils.hpp>
// Supervised learning includes. 
#include <unity/toolkits/supervised_learning/supervised_learning.hpp>

// Types
#include <unity/lib/variant.hpp>
#include <unity/lib/unity_base_types.hpp>
#include <unity/lib/variant_deep_serialize.hpp>
#include <unity/lib/flex_dict_view.hpp>

/// SKD
#include <unity/lib/toolkit_function_macros.hpp>
#include <serialization/serialization_includes.hpp>
 

namespace turi {
namespace supervised {


/**
 * Get standard errors from hessian. 
 */

inline Eigen::Matrix<double, Eigen::Dynamic,1> get_stderr_from_hessian(
    const Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>& hessian) {
  DASSERT_EQ(hessian.rows(), hessian.cols());
  return hessian.inverse().diagonal().cwiseSqrt();
}

/**
 * Is this model a classifier?
 *
 * \param[in] model_name  Name of the model
 *
 * if the model_key is empty, then an empty model is created.
 */
inline bool is_classifier(std::string model_name){
  if(model_name.find("classifier") != std::string::npos) {
    return true;
  }
  return false;
}

/**
 * Setup the ml_data for prediction.
 */
inline ml_data setup_ml_data_for_prediction(
                const sframe& X,
                const std::shared_ptr<supervised_learning_model_base>& model,
                ml_missing_value_action missing_value_action) {

  ml_data data;
  data = model->construct_ml_data_using_current_metadata(X, missing_value_action);
  return data;
}

/**
 * Setup the ml_data for evaluation.
 */
inline ml_data setup_ml_data_for_evaluation(const sframe& X, const sframe& y,
                const std::shared_ptr<supervised_learning_model_base>& model,
                ml_missing_value_action missing_value_action) {
  ml_data data;
  data = model->construct_ml_data_using_current_metadata(X, y, missing_value_action);
  return data;
}

/**
 * Check if the data is empty!
 *
 * \param[in] X  Feature names.
 */
inline void check_empty_data(sframe X){
  if (X.num_rows() == 0){
    log_and_throw("Input data does not contain any rows.");
  }

  if (X.num_columns() == 0){
    log_and_throw("Input data does not contain any features.");
  }
}

/**
 * Check that the target types are right.
 *
 * Regression vs classifier:
 *
 * One user in our forum complained that he got an error message for logistic
 * regression suggesting that his columns was not of numeric type. He
 * should have gotten a message that said. Column not if integer type.
 *
 * Let us separate our (for the purposes of error messages) logging
 * for classifier vs regression tasks.
 *
 */
inline void check_target_column_type(std::string model_name, sframe y){
  DASSERT_TRUE(y.num_columns() == 1);

  std::stringstream ss;
  std::string model_name_for_display = "";

  if (model_name == "classifier_svm"){
    model_name_for_display = "SVM";
  } else if (model_name == "classifier_logistic_regression"){
    model_name_for_display = "Logistic Regression";
  }

  // classifier tasks.
  if(model_name == "classifier_svm" || 
     model_name == "classifier_logistic_regression" || 
     model_name == "random_forest_classifier" || 
     model_name == "decision_tree_classifier" || 
     model_name == "boosted_trees_classifier"){

    flex_type_enum ctype = y.column_type(0);
    if (ctype != flex_type_enum::INTEGER && ctype != flex_type_enum::STRING){
      ss.str("");
      ss << "Column type of target '" << y.column_name(0) 
         << "' must be int or str." 
         << std::endl;
      log_and_throw(ss.str());
    }

  } else {
    
    flex_type_enum ctype = y.column_type(0);
    if ((ctype != flex_type_enum::INTEGER) && (ctype !=
          flex_type_enum::FLOAT)){
      ss.str("");
      ss << "Column type of target '" << y.column_name(0) 
         << "' must be int or float." 
         << std::endl;
      log_and_throw(ss.str());
    }
  } 
}

/**
 * Setup an SFrame as test input to predict, predict_topk, or classify function. 
 */
inline sframe setup_test_data_sframe(const sframe& sf,
                                     std::shared_ptr<supervised_learning_model_base> model,
                                     ml_missing_value_action missing_value_action) {
  sframe ret;
  check_empty_data(sf);

  auto expected_columns = model->get_feature_names();
  switch (missing_value_action) {
    case ml_missing_value_action::IMPUTE:
      ret = model->impute_missing_columns_using_current_metadata(sf);
      break;
    case ml_missing_value_action::USE_NAN:
      if (model->support_missing_value()) {
        ret = model->impute_missing_columns_using_current_metadata(sf);
      } else {
        log_and_throw("Model doesn't support missing value, please set missing_value_action to \"impute\"");
      }
      break;
    case ml_missing_value_action::ERROR:
      ret = sf;
      break;
    default:
      log_and_throw("Invalid missing value action");
  }
  ret = ret.select_columns(expected_columns);
  return ret;
}


/**
 * Fill the ml_data_row with an EigenVector using reference encoding for 
 * categorical variables. Here, the 0"th" category is used as the reference
 * category. 
 *
 * [in,out] An ml_data_row_reference object from which we are reading. 
 * [in,out] An eigen expression (could be a sparse, dense, or row of a matrix) 
 */
template <typename EigenExpr>
GL_HOT_INLINE_FLATTEN
inline void fill_reference_encoding(
    const ml_data_row_reference& row_ref, 
    EigenExpr && x) {

  x.setZero();
  size_t offset = 0;

  row_ref.unpack(
      
      // The function to write out the data to x.
      [&](ml_column_mode mode, size_t column_index,
          size_t feature_index, double value,
          size_t index_size, size_t index_offset) {
        
        if(UNLIKELY(feature_index >= index_size))
          return;

        // Decrement if it isn't the reference category.
        size_t idx = offset + feature_index;
        if(mode_is_categorical(mode)) {
          if (feature_index != 0) {
            idx -= 1;
          } else {
            return;
          }
        }

        DASSERT_GE(idx,  0);
        DASSERT_LT(idx, size_t(x.size()));
        x.coeffRef(idx) = value;

      },

      /** 
       * The function to advance the offset, called after each column
       *  is finished.
       */
      [&](ml_column_mode mode, size_t column_index, 
                      size_t index_size) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {
           offset += (index_size - (mode_is_categorical(mode) ? 1 : 0));
      });
}


/**
 * Warn the user for features with low variance.
 *
 * [in] metadata A copy of the ml_metadata.
 */
inline void check_feature_means_and_variances(
                const std::shared_ptr<ml_metadata> metadata,
                bool display_warnings = true) {

  std::stringstream ss;
  std::vector<std::string> error_columns;

  // Get out the features with low variance
  for(size_t cid = 0; cid < metadata->num_columns(); cid++){
    const auto stats = metadata->statistics(cid);
    size_t index_size = metadata->index_size(cid);
    std::string col = metadata->column_name(cid);
    for(size_t i = 0; i < index_size; i++) {
      if (std::abs(stats->stdev(i)) < 1e-20) {
        error_columns.push_back(col);
        break;
      }
    }
  }

  if (error_columns.size() && display_warnings) {
    ss << "WARNING: Detected extremely low variance for feature(s) ";
    for(size_t i=0; i < error_columns.size()-1; i++){
      ss << "'" << error_columns[i] << "', ";
    }
    ss << "'" << error_columns[error_columns.size()-1] << "'"
       << " because all entries are nearly the same.\n"
       << "Proceeding with model training using all features. "
       << "If the model does not provide results of adequate quality, "
       << "exclude the above mentioned feature(s) from the input dataset.";
    logprogress_stream << ss.str() << std::endl;
  }

  // Get out the features with inf or nan
  error_columns.clear();
  bool column_with_nan = false;
  for(size_t cid = 0; cid < metadata->num_columns(); cid++){
    const auto stats = metadata->statistics(cid);
    size_t index_size = metadata->index_size(cid);
    std::string col = metadata->column_name(cid);
    for(size_t i = 0; i < index_size; i++) {
      if (!std::isfinite(stats->mean(i))) {
        error_columns.push_back(col);
        column_with_nan = true;
        break;
      }
    }
  }

  // Throw an error if a column contains NAN/INF.
  if (column_with_nan == true) {
    ss << "Detected inf/nan values in feature(s) ";
    for(size_t i=0; i < error_columns.size()-1; i++){
      ss << "'" << error_columns[i] << "', ";
    }
    ss << "'" << error_columns[error_columns.size()-1] << "'. "
       << "Cannot proceed with model training.";
    log_and_throw(ss.str());
  }
}


/**
 * For each of the provided keys, get a string of the corresponding value.
 */
inline std::vector<std::string> make_evaluation_progress(
    const std::map<std::string, float>& eval_map,
    const std::vector<std::string> keys) {
  std::vector<std::string> ret;
  if (!eval_map.empty()) {
    for (auto& k : keys)
      // TODO: Check that k exists in eval_map.
      ret.push_back(std::to_string(eval_map.at(k)));
  }
  return ret;
}

inline std::vector<std::string> make_progress_string(
    size_t iter, size_t examples, double time,
    const std::vector<std::string>& train_eval,
    const std::vector<std::string>& valid_eval,
    float speed, bool padding_valid_eval) {

  std::vector<std::string> ret; 
  ret.push_back(std::to_string(iter));
  ret.push_back(std::to_string(examples));
  ret.push_back(std::to_string(time));
  for (size_t i = 0 ; i < train_eval.size(); ++i) {
    ret.push_back(train_eval[i]);
    if (!valid_eval.empty()) {
      ret.push_back(valid_eval[i]);
    } else if(padding_valid_eval) {
      ret.push_back("");
    }
  }
  ret.push_back(std::to_string(speed));
  return ret;
}

/**
 * For the provided model, print all of its desired metrics using
 * the provided headers.
 */
inline std::vector<std::pair<std::string, size_t>> make_progress_header(
    supervised_learning_model_base& smodel, 
    const std::vector<std::string>& stat_headers, 
    bool has_validation_data) {

  auto header = std::vector<std::pair<std::string, size_t>>();
  for (const auto& s : stat_headers) {
    header.push_back({s, 8});
  }

  auto metrics = std::vector<std::string>();
  for (const auto& metric: smodel.get_tracking_metrics()) {
    metrics.push_back(metric);
  }

  for (const auto& m: metrics) {
    std::string dm = smodel.get_metric_display_name(m);
    header.push_back({std::string("Training ") + dm, 6});
    if (has_validation_data) 
      header.push_back({std::string("Validation ") + dm, 6});
  }

  return header;
}

inline std::vector<std::string> make_progress_row_string(
    supervised_learning_model_base& smodel,
    const ml_data& data,
    const ml_data& valid_data,
    const std::vector<std::string>& stats) {

  auto train_eval = std::vector<std::string>();
  for (auto& kv : smodel.evaluate(data, "train")) {
    train_eval.push_back(std::to_string(variant_get_value<double>(kv.second)));
  }

  auto valid_eval = std::vector<std::string>();
  bool has_validation_data = valid_data.num_rows() > 0;
  if (has_validation_data) {
    for (auto& kv : smodel.evaluate(valid_data, "train")) {
      valid_eval.push_back(std::to_string(variant_get_value<double>(kv.second)));
    }
  }

  auto ret = std::vector<std::string>();
  for (const auto& s : stats)
    ret.push_back(s);

  for (size_t i = 0 ; i < train_eval.size(); ++i) {
    ret.push_back(train_eval[i]);
    if (!valid_eval.empty()) {
      ret.push_back(valid_eval[i]);
    } else if(has_validation_data) {
      ret.push_back("");
    }
  }

  return ret;
}

/**
 * Get the class weights based on the user options and target metadata.
 * 
 * \param[in] options
 * \param[in] metadata
 * \returns Class weights
 */
inline flexible_type get_class_weights_from_options( 
                const option_manager& options, 
                const std::shared_ptr<ml_metadata>& metadata){

  size_t num_classes = 2;
  num_classes = metadata->target_index_size();
  auto indexer = metadata->target_indexer();
  auto stats = metadata->target_statistics();

  flex_dict class_weights(num_classes);
  flexible_type class_weights_option = options.value("class_weights");

  // Case 1 (None): Uniform weights
  if (class_weights_option.get_type() == flex_type_enum::UNDEFINED) {

    for(size_t i = 0; i < num_classes; i++){
      class_weights[i] = {indexer->map_index_to_value(i), 1.0};
    }

  // Case 2 ('auto'): Sample inversely proportional to class frequency.
  } else if (class_weights_option == "auto") {

    // Weight inversely proportional to class frequency 
    // w_c = (1/n_c) / (sum(i in C, 1/n_i)) 
    float total = 0;
    for(size_t i = 0; i < num_classes; i++){
      DASSERT_TRUE(stats->count(i) > 0);
      total += 1.0 / stats->count(i);
    }
    for(size_t i = 0; i < num_classes; i++){
      class_weights[i] = {indexer->map_index_to_value(i), 
                   1.0 / (total * stats->count(i))};
    }

  // Case 3 (dict): User provided weights.
  } else if (class_weights_option.get_type() == flex_type_enum::DICT) {

    // Check that all weights were provided.
    flex_dict_view class_weights_view(class_weights_option);
    for(size_t i = 0; i < num_classes; i++){
      if (!class_weights_view.has_key(indexer->map_index_to_value(i))){
        std::stringstream ss;
        ss << "The parameter class_weight does not contain a weight for the "
           << "class " << indexer->map_index_to_value(i) << "."
           << " Make sure that the types of the keys in the class_weight "
           << "dictionary are the same as the type of the target column." 
           << std::endl;
        log_and_throw(ss.str());
      } 
    }

    // Save those weights. (Can't save flexible_type to flex_dict)
    size_t i = 0;
    for(const auto& kvp: class_weights_option.get<flex_dict>()){

      // Weights must be numeric 
      bool error = false;
      if (kvp.second.get_type() != flex_type_enum::INTEGER &&
          kvp.second.get_type() != flex_type_enum::FLOAT) {
        error = true;

      // Weights must be positive
      } else {
        float weight = (float)kvp.second;
        if (weight > 1e-20){
          class_weights[i++] = {kvp.first, weight};
        } else {
          error = true;
        }
      }
      // Throw an error message if not numeric and not in range.
      if (error == true){
        std::stringstream ss;
        ss << "The class_weight parameter for the class " << kvp.first 
           << " must be a positive numeric value."
           << std::endl;
        log_and_throw(ss.str());
      } 
    }

  // Error: Weights are not of dictioanry, None, or 'auto' type.
  } else {
    std::stringstream ss;
    ss << "The class_weights parameter cannot be of type " 
       << flex_type_enum_to_name(class_weights_option.get_type()) << "."
       << " Class weights must be a dictionary, None or 'auto'" << std::endl;
    log_and_throw(ss.str());
  }


  return class_weights;
 
} 

/**
 * Get number of examples per class
 * 
 * \param[in] metadata
 * \returns Break down of examples per class.
 *
 * \warning For now, this only does it for binary classificaiton problems.
 *
 */
inline std::map<flexible_type, size_t> get_num_examples_per_class( 
                std::shared_ptr<ml_metadata> metadata){

  std::map<flexible_type, size_t> examples_per_class;
  for(size_t k = 0; k < metadata->target_index_size(); k++){
    examples_per_class[metadata->target_indexer()->map_index_to_value(k)] = 
                      metadata->target_statistics()->count(k);
  }
  return examples_per_class;
} 

/**
 * Get the set of classes.
 * 
 * \param[in] ml_metadata
 * \returns Get the set of all classes in the model.
 *
 */
inline std::vector<flexible_type> get_class_names( 
                std::shared_ptr<ml_metadata> metadata){

  std::vector<flexible_type> classes;
  classes.resize(metadata->target_index_size());
  for(size_t k = 0; k < classes.size(); k++){
    classes[k] = metadata->target_indexer()->map_index_to_value(k);
  }
  return classes;
}

/**
 * Get the number of coefficients from meta_data.
 * \param[in] metadata
 * \returns Number of coefficients.
 */

inline size_t get_number_of_coefficients(std::shared_ptr<ml_metadata> metadata){

  size_t num_coefficients = 1;
  for(size_t i = 0; i < metadata->num_columns(); i++) {
    if (metadata->is_categorical(i)) {
      num_coefficients += metadata->index_size(i) - 1;
    } else {
      num_coefficients += metadata->index_size(i);
    }
  }
  return num_coefficients;
}


/**
* Add a column of None values to the SFrame of coefficients.
*
* \returns coefs (as SFrame)
*/
inline sframe add_na_std_err_to_coef(const sframe& sf_coef) {
  auto sa = std::make_shared<sarray<flexible_type>>(
                   sarray<flexible_type>(FLEX_UNDEFINED, sf_coef.size(), 1,
                   flex_type_enum::FLOAT));
  return sf_coef.add_column(sa, std::string("stderr"));
}

/**
* Get one-hot-coefficients
*
* \params[in] coefs     Coefficients as EigenVector
* \params[in] metadata  Metadata
*
* \returns coefs (as SFrame)
*/
inline void get_one_hot_encoded_coefs(const Eigen::Matrix<double, Eigen::Dynamic, 1>&
    coefs, std::shared_ptr<ml_metadata> metadata,
    std::vector<double>& one_hot_coefs) {

  size_t idx = 0;
  size_t num_classes = metadata->target_index_size();
  bool is_classifier = metadata->target_is_categorical();
  if (is_classifier) {
    num_classes -= 1;  // reference class
  }

  for (size_t c = 0; c < num_classes; c++) {
    for (size_t i = 0; i < metadata->num_columns(); ++i) {
      // Categorical
      size_t start_idx = 0;
      if (metadata->is_categorical(i)) {
        // 0 is the reference
        one_hot_coefs.push_back(0.0);
        start_idx = 1;
      }

      for (size_t j = start_idx; j < metadata->index_size(i); ++j) {
        one_hot_coefs.push_back(coefs[idx]);
        ++idx;
      }
    }

    // Intercept
    one_hot_coefs.push_back(coefs[idx++]);
  }
}

/**
* Save coefficients to an SFrame, retrievable in Python
*
* \params[in] coefs     Coefficients as EigenVector
* \params[in] metadata  Metadata
*
* \returns coefs (as SFrame)
*/
inline sframe get_coefficients_as_sframe(
         const Eigen::Matrix<double, Eigen::Dynamic, 1>& coefs,
         std::shared_ptr<ml_metadata> metadata, 
         const Eigen::Matrix<double, Eigen::Dynamic, 1>& std_err) {

  DASSERT_TRUE(coefs.size() > 0);
  DASSERT_TRUE(metadata);

  // Classifiers need to provide target_metadata to print out the class in
  // the coefficients.
  bool is_classifier = metadata->target_is_categorical();
  bool has_stderr = std_err.size() > 0;
  DASSERT_EQ(std_err.size(), has_stderr * coefs.size());

  sframe sf_coef;
  std::vector<std::string> coef_names;
  coef_names.push_back("name");
  coef_names.push_back("index");
  if (is_classifier) coef_names.push_back("class");
  coef_names.push_back("value");
  if (has_stderr) coef_names.push_back("stderr");

  std::vector<flex_type_enum> coef_types;
  coef_types.push_back(flex_type_enum::STRING);
  coef_types.push_back(flex_type_enum::STRING);
  if (is_classifier) coef_types.push_back(metadata->target_column_type());
  coef_types.push_back(flex_type_enum::FLOAT);
  if (has_stderr) coef_types.push_back(flex_type_enum::FLOAT);

  sf_coef.open_for_write(coef_names, coef_types, "", 1);
  auto it_sf_coef = sf_coef.get_output_iterator(0);

  // Get feature names
  std::vector<flexible_type> feature_names;
  std::vector<flexible_type> feature_index;

  feature_names.reserve(metadata->num_dimensions());
  feature_index.reserve(metadata->num_dimensions());

  for (size_t i = 0; i < metadata->num_columns(); ++i) {
    bool skip_zero = metadata->is_categorical(i);

    for (size_t j = skip_zero ? 1 : 0; j < metadata->index_size(i); ++j) {
      feature_names.push_back(metadata->column_name(i));

      if (metadata->is_indexed(i)) {
        feature_index.push_back(
            metadata->indexer(i)->map_index_to_value(j).to<flex_string>());
      } else if (metadata->column_mode(i) == ml_column_mode::NUMERIC) {
        feature_index.push_back(FLEX_UNDEFINED);
      } else {
        feature_index.push_back(std::to_string(j));
      }
    }
  }

  // Classification
  if (is_classifier) {

    // GLC 1.0.1- did not save things as categorical variables.
    size_t num_classes = metadata->target_index_size();
    size_t variables_per_class = coefs.size() / (num_classes - 1);
    for(size_t k = 1; k < num_classes; k++){

      // Intercept
      std::vector<flexible_type> x(4 + has_stderr);
      x[0] = "(intercept)";
      x[1] = FLEX_UNDEFINED;
      x[2] = (metadata->target_indexer())->map_index_to_value(k);
      x[3] = coefs(variables_per_class * k - 1);
      if (has_stderr) x[4] = std_err(variables_per_class * k - 1);
      *it_sf_coef = x;
      ++it_sf_coef;

      // Write feature coefficients
      for (size_t i = 0; i < feature_names.size(); ++i) {
        x[0] = feature_names[i];
        x[1] = feature_index[i];
        x[2] = (metadata->target_indexer())->map_index_to_value(k);
        x[3] = coefs(variables_per_class * (k-1) + i);
        if (has_stderr) x[4] = std_err(variables_per_class * (k-1) + i);
        *it_sf_coef = x;
        ++it_sf_coef;
      }

    }

  // Regression
  } else {

    // Intercept
    std::vector<flexible_type> x(3 + has_stderr);
    x[0] = "(intercept)";
    x[1] = FLEX_UNDEFINED;
    x[2] = coefs(coefs.size() - 1);
    if (has_stderr) x[3] = std_err(std_err.size() - 1);
    *it_sf_coef = x;
    ++it_sf_coef;

    // Write feature coefficients
    for (size_t i = 0; i < feature_names.size(); ++i) {
      x[0] = feature_names[i];
      x[1] = feature_index[i];
      x[2] = coefs(i);
      if (has_stderr) x[3] = std_err(i);
      *it_sf_coef = x;
      ++it_sf_coef;
    }
  }
  sf_coef.close();
  return sf_coef;
}
inline sframe get_coefficients_as_sframe(
         const Eigen::Matrix<double, Eigen::Dynamic, 1>& coefs,
         std::shared_ptr<ml_metadata> metadata) {
  Eigen::Matrix<double, Eigen::Dynamic, 1> EMPTY;
  return get_coefficients_as_sframe(coefs, metadata, EMPTY);
}

/**
 * Get number of examples per class
 *
 * \param[in] target sarray
 * \returns Break down of examples per class.
 */
inline std::map<flexible_type, size_t>get_num_examples_per_class_from_sarray(
                                   std::shared_ptr<sarray<flexible_type>> sa){
  auto reader = sa->get_reader();
  std::map<flexible_type, size_t> unique_values;
  for(size_t seg_id = 0; seg_id < sa->num_segments(); seg_id++){
    auto iter = reader->begin(seg_id);
    auto enditer = reader->end(seg_id);
    while(iter != enditer) {
      if(unique_values.find(*iter) == unique_values.end()){
        unique_values.insert({*iter,0});
      } else {
        ++unique_values[*iter];
      }
      ++iter;
    }
  }
  return unique_values;
}

} // supervised
} // turicreate
#endif
