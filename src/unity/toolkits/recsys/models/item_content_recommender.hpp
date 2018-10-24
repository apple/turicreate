/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_RECSYS_MODEL_ITEM_CONTENT_RECOMMENDER_H_
#define TURI_RECSYS_MODEL_ITEM_CONTENT_RECOMMENDER_H_

#include <vector>
#include <string>
#include <unity/toolkits/recsys/models/itemcf.hpp>

namespace turi {

namespace v2 {
class ml_data;
}

class sframe;
class sgraph;
class flexible_type;
class sparse_similarity_lookup;

namespace recsys {

class EXPORT recsys_item_content_recommender : public recsys_itemcf {
 public:
  BEGIN_CLASS_MEMBER_REGISTRATION("item_content_recommender")
  REGISTER_CLASS_MEMBER_FUNCTION(recsys_item_content_recommender::list_fields)


  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_value", recsys_item_content_recommender::get_value_from_state, "field"); //"get_value"
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_similar_items", recsys_item_content_recommender::api_get_similar_items, "items", "k", "verbose", "get_all_items");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_similar_users", recsys_item_content_recommender::api_get_similar_users, "users", "k", "get_all_users");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "predict", recsys_item_content_recommender::api_predict, "data_to_predict", "new_user_data", "new_item_data");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_current_options", recsys_item_content_recommender::api_get_current_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "set_current_options", recsys_item_content_recommender::api_set_current_options, "options");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_train_stats", recsys_item_content_recommender::api_get_train_stats);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "train_test_split", recsys_item_content_recommender::api_train_test_split, "dataset", "user_column", "item_column",
    "max_num_users", "item_test_proportion", "random_seed");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "train", recsys_item_content_recommender::api_train, "dataset", "user_data", "item_data", "nearest_items", "opts");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "recommend", recsys_item_content_recommender::api_recommend, "query", "exclude", "restrictions", "new_data", "new_user_data",
    "new_item_data", "exclude_training_interactions", "top_k", "diversity", "random_seed");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_popularity_baseline", recsys_item_content_recommender::get_popularity_baseline);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_item_intersection_info", recsys_item_content_recommender::api_get_item_intersection_info, "item_pairs");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "export_to_coreml", recsys_item_content_recommender::export_to_coreml, "model", "filename"); ///
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "precision_recall_stats", recsys_item_content_recommender::api_precision_recall_stats, "indexed_validation_data", "recommend_output", "cutoffs");

  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_data_schema", recsys_item_content_recommender::api_get_data_schema);



  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_value", recsys_item_content_recommender::get_value_from_state, "field");
  REGISTER_CLASS_MEMBER_FUNCTION(recsys_item_content_recommender::recommend_extension_wrapper, 
    "reference_data", "new_observation_data", "top_k")
  END_CLASS_MEMBER_REGISTRATION
};

}}

#endif
