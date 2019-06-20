/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_NEAREST_NEIGHBORS_H
#define TURI_UNITY_NEAREST_NEIGHBORS_H

#include <vector>

#include <model_server/lib/toolkit_function_specification.hpp>
#include <model_server/lib/variant.hpp>

namespace turi {
namespace nearest_neighbors {

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
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with current options.
 *
 * Params dictionary keys
 * \property[in] model       Regression model object.
 * \property[in] model_name  Name of the model.
 */
variant_map_type get_current_options(variant_map_type& params);

/**
 * Get training stats.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with train stats.
 *
 * Params dictionary keys
 * \property[in] model       Regression model object.
 * \property[in] model_name  Name of the model.
 */
variant_map_type training_stats(variant_map_type& params);

/**
 * Get any value from the model.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary
 *
 * Params dictionary keys
 * \property[in] model       Regression model object.
 * \property[in] model_name  Name of the model.
 * \property[out] "value"    Value of the key.
 *
 */
variant_map_type get_value(variant_map_type& params);

/**
 * List all keys in the model.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dict with "model" keys but empty values.
 *
 * Params dictionary keys
 * \property[in] model       Regression model object.
 * \property[in] model_name  Name of the model.
 *
 */
variant_map_type list_fields(variant_map_type& params);

/**
 * Creation function for nearest neighbors reference objects. Checks for errors
 * in inputs and makes sure all options provided by the user overwrite default
 * options.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with new model.
 *
 * Params dictionary keys
 * \property[in,out] model   Regression model object.
 * \property[in] model_name  Name of the model.
 */
variant_map_type train(variant_map_type& params);

/**
 * Query function for the nearest neighbors toolkit.
 *
 * \param[in] params dictionary with function arguments
 * \returns SFrame with labels of queries, reference points in the nearest
 * neighbors model, distances between the queries and answers, and ranks of the
 * answers for each query.
 *
 * Params dictionary keys
 * \property[in, out] model      NearestNeighborsModel object
 * \property[in] model_name      Name of the model
 */
variant_map_type query(variant_map_type& params);

/**
 * Similarity graph function for the nearest neighbors toolkit.
 *
 * \param[in] params dictionary with function arguments
 * \returns SFrame with labels of queries, reference points in the nearest
 * neighbors model, distances between the queries and answers, and ranks of the
 * answers for each query.
 *
 * Params dictionary keys
 * \property[in, out] model      NearestNeighborsModel object
 * \property[in] model_name      Name of the model
 */
variant_map_type similarity_graph(variant_map_type& params);

/**
 * Obtain registration for the nearest_neighbors toolkit.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace nearest_neighbors
} // namespace turi

#endif
