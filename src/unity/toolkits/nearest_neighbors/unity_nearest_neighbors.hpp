/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_NEAREST_NEIGHBORS_H
#define TURI_UNITY_NEAREST_NEIGHBORS_H

#include <unity/lib/toolkit_function_specification.hpp>

namespace turi {
namespace nearest_neighbors {



/** 
 * Check if the data is empty. 
 * 
 * \param[in] X SFrame. 
 */  
void check_empty_data(const sframe& X);

/** 
* Check that the feature and label types are valid.
*
* \param[in] X SFrame Feature data
* \param[in] y SFrame Labels
* \param[in] model_name std::string name of the model
* \param[in] distance std::string distance option
*/  
void check_column_types(const sframe& X, const sframe& y,
                        std::string model_name, std::string distance);

/*
 * Get the list of options that are relevant ot each model.
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
 * \property[in] model       Regression model object.
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
 * \property[in] model       Regression model object.
 * \property[in] model_name  Name of the model.
 */
toolkit_function_response_type training_stats(toolkit_function_invocation& invoke);

/**
 * Get any value from the model.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary
 *
 * Invocation object properties
 * \property[in] model       Regression model object.
 * \property[in] model_name  Name of the model.
 * \property[out] "value"    Value of the key.
 *
 */
toolkit_function_response_type get_value(toolkit_function_invocation& invoke);

/**
 * List all keys in the model.
 *
 * \param[in] invoke toolkit_function_invocation object
 * \returns Dict with "model" keys but empty values.
 *
 * Invocation object properties
 * \property[in] model       Regression model object.
 * \property[in] model_name  Name of the model.
 *
 */
toolkit_function_response_type list_keys(toolkit_function_invocation& invoke);

/**
 * Creation function for nearest neighbors reference objects. Checks for errors
 * in inputs and makes sure all options provided by the user overwrite default
 * options.
 *
 * \param[in] invoke toolkit invocation object
 * \returns Dictionary with new model.
 *
 * Invocation object properties
 * \property[in,out] model   Regression model object.
 * \property[in] model_name  Name of the model.
 */
// toolkit_function_response_type train(toolkit_function_invocation& invoke);
variant_map_type train(variant_map_type& invoke);

/** 
 * Query function for the nearest neighbors toolkit. 
 *
 * \param[in] invoke tookit_invocation object
 * \returns SFrame with labels of queries, reference points in the nearest
 * neighbors model, distances between the queries and answers, and ranks of the
 * answers for each query.
 *
 * Invocation object properties
 * \property[in, out] model      NearestNeighborsModel object
 * \property[in] model_name      Name of the model
 */
toolkit_function_response_type query(toolkit_function_invocation& invoke);

/** 
 * Similarity graph function for the nearest neighbors toolkit. 
 *
 * \param[in] invoke tookit_invocation object
 * \returns SFrame with labels of queries, reference points in the nearest
 * neighbors model, distances between the queries and answers, and ranks of the
 * answers for each query.
 *
 * Invocation object properties
 * \property[in, out] model      NearestNeighborsModel object
 * \property[in] model_name      Name of the model
 */
toolkit_function_response_type similarity_graph(toolkit_function_invocation& invoke);

/**
 * Obtain registration for the nearest_neighbors toolkit.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace nearest_neighbors
} // namespace turi

#endif 
