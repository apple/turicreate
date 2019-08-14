/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_KMEANS_H
#define TURI_UNITY_KMEANS_H

#include <vector>

#include <model_server/lib/toolkit_function_specification.hpp>
#include <model_server/lib/variant.hpp>

namespace turi {
namespace kmeans {

/**
 * Get the current set of options.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with current options.
 *
 * Params dictionary keys
 * \property[in] model       KMeans model object.
 * \property[in] model_name  Name of the model.
 */
variant_map_type get_current_options(variant_map_type& params);

/**
 * Get any value from the model.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary
 *
 * Params dictionary keys
 * \property[in] model       KMeans model object.
 * \property[in] model_name  Name of the model.
 * \property[out] "value"    Value of the key.
 *
 */
variant_map_type get_value(variant_map_type& params);

/**
 * Creation function for KMeans clustering model. Checks for errors
 * in inputs and makes sure all options provided by the user overwrite default
 * options.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with new model.
 *
 * Params dictionary keys
 * \property[in,out] model   KMeans model object.
 * \property[in] model_name  Name of the model.
 */
variant_map_type train(variant_map_type& params);

/**
 * Predict function for the Kmeans model.
 *
 * \param params dictionary with function arguments
 * \return toolkit function response
 *
 * Params dictionary keys
 * \property[in] model   KMeans model object.
 * \property[in] model_name  Name of the model.
 */
variant_map_type predict(variant_map_type& params);

/**
 * Obtain registration for the KMeans model toolkit.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace kmeans
} // namespace turi

#endif
