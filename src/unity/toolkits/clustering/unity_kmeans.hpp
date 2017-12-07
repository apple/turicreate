/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_KMEANS_H
#define TURI_UNITY_KMEANS_H

#include <unity/lib/toolkit_function_specification.hpp>

namespace turi {
namespace kmeans {

/**
 * Check if the data is empty.
 *
 * \param[in] X SFrame.
 */
void check_empty_data(const sframe& X);

/**
* Check that the features are valud
*
* \param[in] X SFrame Feature data
*/
void check_column_types(const sframe& X);

/*
 * Get the list of options that are relevant to each model.
 *
 * \param[in] model_name Name of the model.
 * \returns List of keys of option names relevant to a model.
 */
std::vector<std::string> get_model_option_keys(std::string model_name);

/**
 * Get the current set of options.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with current options.
 *
 * Invocation object properties
 * \property[in] model       KMeans model object.
 * \property[in] model_name  Name of the model.
 */
toolkit_function_response_type get_current_options(toolkit_function_invocation& invoke);

/**
 * Get any value from the model.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary
 *
 * Invocation object properties
 * \property[in] model       KMeans model object.
 * \property[in] model_name  Name of the model.
 * \property[out] "value"    Value of the key.
 *
 */
toolkit_function_response_type get_value(toolkit_function_invocation& invoke);

/**
 * Creation function for KMeans clustering model. Checks for errors
 * in inputs and makes sure all options provided by the user overwrite default
 * options.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with new model.
 *
 * Invocation object properties
 * \property[in,out] model   KMeans model object.
 * \property[in] model_name  Name of the model.
 */
toolkit_function_response_type train(toolkit_function_invocation& invoke);

/**
 * Predict function for the Kmeans model.
 *
 * \param invoke toolkit invocation object
 * \return toolkit function response
 *
 * Invocation object properties
 * \property[in] model   KMeans model object.
 * \property[in] model_name  Name of the model.
 */
toolkit_function_response_type predict(toolkit_function_invocation& invoke);

/**
 * Predict function for a KMeans model.
 *
 * \param[in] invoke tookit_invocation object
 * \returns SFrame with one row per input row, where each row in the output
 * corresponds to a row in the input, with columns indicating the closest
 * cluster (by its ID) and its distance from the input.
 *
 * Invocation object properties
 * \property[in, out] model      KMeans model object
 * \property[in] model_name      Name of the model
 *
 */
//toolkit_function_response_type predict(toolkit_function_invocation& invoke);

/**
 * Obtain registration for the KMeans model toolkit.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace kmeans
} // namespace turi

#endif
