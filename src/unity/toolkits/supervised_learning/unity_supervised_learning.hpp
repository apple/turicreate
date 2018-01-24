/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SUPERVISED_LEARNING_H_
#define TURI_UNITY_SUPERVISED_LEARNING_H_

#include <vector>

#include <unity/lib/toolkit_function_specification.hpp>
#include <unity/lib/variant.hpp>

namespace turi {
namespace supervised{


/**
 * Get any value from the model.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary
 *
 * Params dictionary keys
 * \property[in] model       SVM model object.
 * \property[in] model_name  Name of the modeextract_features*/
variant_map_type get_value(variant_map_type& params);



/**
 * Get the value of a particular option.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary
 *
 * Params dictionary keys
 * \property[in] model       SVM model object.
 * \property[in] model_name  name of the model.
 * \property[in] key         Key of the model to query..
 * \property[out] "value"    Value of the key.
 *
 */
variant_map_type get_option_value(variant_map_type& params);

/**
 * Get the current set of options.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with current options.
 *
 * Params dictionary keys
 * \property[in] model       SVM model object.
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
 * \property[in] model       SVM model object.
 * \property[in] model_name  Name of the model.
 */
variant_map_type get_train_stats(variant_map_type& params);


/**
 * Init function for train function. Checks for errors in inputs and makes sure
 * all options provided by the user overwrite default options.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with new model.
 *
 * Params dictionary keys
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 */
variant_map_type train(variant_map_type& params);

/**
 * Init function for prediction.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with prediction stats.
 *
 * Params dictionary keys
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[in] test_response SFrame with only independent vars.
 * \property[out] predictions Sarray with predictions.
 */
variant_map_type predict(variant_map_type& params);

/**
 * Init function for prediction.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with prediction stats.
 *
 * Params dictionary keys
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[in] test_response SFrame with only independent vars.
 * \property[out] predictions Sarray with predictions.
 */
variant_map_type extract_feature(variant_map_type& params);

/**
 * Init function for prediction_topk.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with prediction stats.
 *
 * Params dictionary keys
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[in] test_response SFrame with only independent vars.
 * \property[out] predictions Sarray with predictions.
 */
variant_map_type predict_topk(variant_map_type& params);


/**
 * Init function for classify
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with classifer ssistats.
 *
 * Params dictionary keys
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[in] test_response SFrame with only independent vars.
 * \property[out] pclassify SFrame with classifier columns.
 */
variant_map_type classify(variant_map_type& params);

/**
 * Init function for evaluation.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with evaluation stats.
 *
 * Params dictionary keys
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 *
 */
variant_map_type evaluate(variant_map_type& params);

/**
 * List all keys in the model.
 *
 * \param[in] params dictionary with function arguments
 * \returns Dict with "model" keys but empty values.
 *
 * Params dictionary keys
 * \property[in] model       SVM model object.
 * \property[in] model_name  Name of the model.
 *
 */
variant_map_type list_fields(variant_map_type& params);

/**
 * Is this model trained?
 *
 * \param[in] params dictionary with function arguments
 * \returns response Dictionary
 *
 * Params dictionary keys
 * \property[in] model       SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[out] "is_trained" True if model is trained
 *
 */
variant_map_type is_trained(variant_map_type& params);

/**
 * Append key value pair of the model
 *
 * \param[in] params dictionary with function arguments
 * \returns Dictionary with new model.
 *
 * Params dictionary keys
 * \property[in,out] model   SVM model object.
 * \property[in] model_name  Name of the model.
 * \property[in] key         Key of the model to append.
 * \property[in] value       Value of the model to append.
 *
 */
variant_map_type add_or_update_model(variant_map_type& params);




/**
 * Obtains the registration for the toolkit.
 */
std::vector<toolkit_function_specification> get_toolkit_function_registration();

} // namespace supervised
} // namespace turi
#endif

