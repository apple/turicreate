/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_RECSYS_TOOLKIT_MODEL_FUNCTIONS_H_
#define TURI_RECSYS_TOOLKIT_MODEL_FUNCTIONS_H_

#include <unity/toolkits/recsys/recsys_model_base.hpp>
#include <unity/toolkits/recsys/models.hpp>
#include <unity/lib/toolkit_function_specification.hpp>
#include <unity/lib/unity_base_types.hpp>
#include <unity/lib/api/model_interface.hpp>
#include <serialization/serialization_includes.hpp>
#include <unity/lib/unity_sframe.hpp>
#include <unity/toolkits/ml_data_2/sframe_index_mapping.hpp>

namespace turi {

class iarchive;
class oarchive;
class sframe;
class flexible_type;
class column_metadata;
class ml_data;

namespace recsys {

////////////////////////////////////////////////////////////////////////////////

/*
 * TOOLKIT INTERACTION
 */

/**
 * Toolkit function used for initializing a recsys_model.
 *
 * \param model_name name of the model.
 * \param opts options to initialize the model.
 */
toolkit_function_response_type init(toolkit_function_invocation& invoke);

/**
 * Toolkit function used for training a recsys_model.
 *
 * \param training_data dataframe_t a dataframe containing the data to use
 * for fitting the model
 * \param user_id flexible_type the column name containing user ids
 * \param item_id flexible_type the column name containing item ids
 * \param target flexible_type the column name containing the target
 * \param model_name flexible_type the name of the model to fit
 *
 * \return toolkit_function_response_type with a param named "model" containing a
 * model_base* pointer.
 */
toolkit_function_response_type train(toolkit_function_invocation& invoke);

/**
 * Toolkit function for training a recsys_model object
 *
 * Requires the following from Python:
 * \param data_to_predict dataframe_t a DataFrame object in the same format as the one used for training
 * \param model recsys_model_base* the model object.
 *
 * \return toolkit_function_response_type including a param called "data" containing a
 * dataframe with a single column named predictions.
 */
toolkit_function_response_type predict(toolkit_function_invocation& invoke);

/**
 * Toolkit function for obtaining the top-k recommended items for a given user.
 *
 * Requires the following objects from Python:
 * - model recsys_model_base*
 * - users dataframe_t a DataFrame with a single column named "users" containing
 * top_k flexible_type
 *
 * \return toolkit_function_response_type with a parameter named "data" containing a
 * dataframe with four columns: user, item, score, and rank.
 */
toolkit_function_response_type recommend(toolkit_function_invocation& invoke);

/**
 * Tookit function for computing precision and recall.
 *
 * Requires the following objects are passed from unity:
 *
 *  model  model_base*  the model object
 *
 *  validation_data dataframe_t  a DataFrame with the same format as the
 *  one used during training
 *
 *  users dataframe_t  a DataFrame with a column named "users" containing
 *  user ids for which precision and recall should be computed
 *
 *  cutoffs dataframe_t a DataFrame with a column named "cutoff" containing
 *  positive integers k for which we want precision@k and recall@k.
 */
toolkit_function_response_type precision_recall(toolkit_function_invocation& invoke);

toolkit_function_response_type get_value(toolkit_function_invocation& invoke);

toolkit_function_response_type list_fields(toolkit_function_invocation& invoke);

toolkit_function_response_type get_current_options(toolkit_function_invocation& invoke);

toolkit_function_response_type set_current_options(toolkit_function_invocation& invoke);

toolkit_function_response_type get_stats(toolkit_function_invocation& invoke);

toolkit_function_response_type data_summary(toolkit_function_invocation& invoke);

toolkit_function_response_type train_test_split(toolkit_function_invocation& invoke);

/**
 * Toolkit function for getting the top-k similar users for a list of users.
 *
 * Requires the following objects from Python:
 * - model recsys_itemcf* A trained Item Similarity Model
 * - users dataframe_t a DataFrame with a single column named "users" containing flexible_type
 * - topk int number of users returned for each item
 *
 * \return toolkit_function_response_type with a parameter named "data" containing a
 * dataframe with four columns: item, similar item, score and rank.
 */
toolkit_function_response_type get_similar_users(toolkit_function_invocation& invoke);

/**
 * Toolkit function for getting the top-k similar items for a list of items.
 *
 * Requires the following objects from Python:
 * - model recsys_itemcf* A trained Item Similarity Model
 * - users dataframe_t a DataFrame with a single column named "items" containing flexible_type
 * - topk int number of items returned for each item
 *
 * \return toolkit_function_response_type with a parameter named "data" containing a
 * dataframe with four columns: item, similar item, score and rank.
 */
toolkit_function_response_type get_similar_items(toolkit_function_invocation& invoke);

/** Get a popularity baseline model trained according to the current dataset.
 *
 * \return toolkit_function_response_type with a parameter named
 * "popularity_model" containing a full recommender popularity model.
 */
toolkit_function_response_type get_popularity_baseline(toolkit_function_invocation& invoke);

/** Returns information about the intersection of users between two items. 
 */
toolkit_function_response_type get_item_intersection_info(toolkit_function_invocation& invoke);


std::vector<toolkit_function_specification> get_toolkit_function_registration();

}}

#endif /* _TOOLKIT_MODEL_FUNCTIONS_H_ */
