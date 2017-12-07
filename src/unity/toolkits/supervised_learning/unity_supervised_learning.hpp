/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SUPERVISED_LEARNING_H_
#define TURI_UNITY_SUPERVISED_LEARNING_H_

#include <unity/lib/toolkit_function_specification.hpp>

namespace turi {
namespace supervised{


/**
 * Check if the data is empty!
 *
 * \param[in] X  Feature SFrame.
 *
 */
void check_empty_data(sframe X);

/**
 * Check the feature type for each task.
 *
 * \param[in] model_name Name of the model.
 * \param[in] X          Feature SFrame.
 *
 */
void check_feature_column_types(sframe X);

/**
 * Check the target type for each task.
 *
 * \param[in] model_name Name of the model.
 * \param[in] y          Target SFrame.
 *
 * \note Separate out the checking for various target types i.e
 *        classifier vs regression.
 *
 */
void check_target_column_type(std::string model_name, sframe y);


/**
 * Get the list of options that are relevant ot each model.
 *
 * \param[in] model_name Name of the model.
 * \returns List of keys of option names relevant to a model.
 *
 */
std::vector<std::string> get_model_option_keys(std::string model_name);

/**
 * Get any value from the model.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary
 *
 * invocation object properties
 * \property[in] model       SVM model object.
 * \property[in] model_name  Name of the modeextract_features*/
toolkit_function_response_type get_value(toolkit_function_invocation& invoke);



/**
 * Get the value of a particular option.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary
 *
 * invocation object properties
 * \property[in] model       SVM model object.
 * \property[in] model_name  name of the model.
 * \property[in] key         Key of the model to query..
 * \property[out] "value"    Value of the key.
 *
 */
toolkit_function_response_type get_option_value(toolkit_function_invocation& invoke);

/**
 * Get the default option dictionary.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with default options
 *
 * Invocation object properties
 * \property[in] model       SVM model object.
 * \property[in] model_name  Name of the model.
 */
toolkit_function_response_type get_default_options(toolkit_function_invocation& invoke);

/**
 * Get the current set of options.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with current options.
 *
 * Invocation object properties
 * \property[in] model       SVM model object.
 * \property[in] model_name  Name of the model.
 */
toolkit_function_response_type get_current_options(toolkit_function_invocation& invoke);


/**
 * Get training stats.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with train stats.
 *
 * Invocation object properties
 * \property[in] model       SVM model object.
 * \property[in] model_name  Name of the model.
 */
toolkit_function_response_type get_train_stats(toolkit_function_invocation& invoke);


/**
 * Init function for train function. Checks for errors in inputs and makes sure
 * all options provided by the user overwrite default options.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with new model.
 *
 * Invocation object properties
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 */
toolkit_function_response_type train(toolkit_function_invocation& invoke);

/**
 * Init function for prediction.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with prediction stats.
 *
 * Invocation object properties
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[in] test_response SFrame with only independent vars.
 * \property[out] predictions Sarray with predictions.
 */
toolkit_function_response_type predict(toolkit_function_invocation& invoke);

/**
 * Init function for prediction.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with prediction stats.
 *
 * Invocation object properties
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[in] test_response SFrame with only independent vars.
 * \property[out] predictions Sarray with predictions.
 */
toolkit_function_response_type extract_feature(toolkit_function_invocation& invoke);

/**
 * Init function for prediction_topk.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with prediction stats.
 *
 * Invocation object properties
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[in] test_response SFrame with only independent vars.
 * \property[out] predictions Sarray with predictions.
 */
toolkit_function_response_type predict_topk(toolkit_function_invocation& invoke);


/**
 * Init function for classify
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with classifer ssistats.
 *
 * Invocation object properties
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[in] test_response SFrame with only independent vars.
 * \property[out] pclassify SFrame with classifier columns.
 */
toolkit_function_response_type classify(toolkit_function_invocation& invoke);

/**
 * Init function for evaluation.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with evaluation stats.
 *
 * Invocation object properties
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 *
 */
toolkit_function_response_type evaluate(toolkit_function_invocation& invoke);

/**
 * List all keys in the model.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dict with "model" keys but empty values.
 *
 * Invocation object properties
 * \property[in] model       SVM model object.
 * \property[in] model_name  Name of the model.
 *
 */
toolkit_function_response_type list_keys(toolkit_function_invocation& invoke);

/**
 * Is this model trained?
 *
 * \param[in] invoke toolkit invocation object
 * \returns response Dictionary
 *
 * Invocation object properties
 * \property[in] model       SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[out] "is_trained" True if model is trained
 *
 */
toolkit_function_response_type is_trained(toolkit_function_invocation& invoke);

/**
 * Append key value pair of the model
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with new model.
 *
 * Invocation object properties
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[in] key         Key of the model to append.
 * \property[in] value       Value of the model to append.
 *
 */
toolkit_function_response_type add_or_update_model(toolkit_function_invocation& invoke);




/**
 * Obtains the registration for the toolkit.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace supervised
} // namespace turi
#endif

