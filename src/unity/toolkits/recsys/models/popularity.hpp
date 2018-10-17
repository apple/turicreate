/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_RECSYS_MODEL_POPULARITY_H_
#define TURI_RECSYS_MODEL_POPULARITY_H_

#include <vector>
#include <string>
#include <unity/toolkits/recsys/recsys_model_base.hpp>
#include <unity/toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <unity/toolkits/nearest_neighbors/ball_tree_neighbors.hpp>


namespace turi {

namespace v2 {
class ml_data;
}

class sframe;
class flexible_type;

namespace recsys {

class EXPORT recsys_popularity : public recsys_model_base {
 public:

  // Implement the bare minimum of the pure virtual methods
  void init_options(const std::map<std::string, flexible_type>&_options) override;

  bool use_target_column(bool target_is_present) const override {
    return target_is_present;
  }

  /** Creates and trains the model.  Training can be done either
   * through the ml_data version, or the sarray of item-target pairs.
   *
   * At the end of training, the state variable "item_predictions"
   * holds the predicted value of each of the items.
   */
  std::map<std::string, flexible_type> train(const v2::ml_data& data) override;

  #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"  // TODO: fix this issue
  #endif
  std::map<std::string, flexible_type> train(
      std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > > trained_user_item);
  #ifdef __clang__
    #pragma clang diagnostic pop
  #endif

  sframe predict(const v2::ml_data& test_data) const override;

  sframe get_similar_items(std::shared_ptr<sarray<flexible_type>> items, 
                           size_t k=0) const override;

  sframe get_similar_users(std::shared_ptr<sarray<flexible_type>> users,
                           size_t k=0) const override;

  void score_all_items(
      std::vector<std::pair<size_t, double> >& scores,
      const std::vector<v2::ml_data_entry>& query_row,
      size_t top_k,
      const std::vector<std::pair<size_t, double> >& user_item_list,
      const std::vector<std::pair<size_t, double> >& new_user_item_data,
      const std::vector<v2::ml_data_row_reference>& new_observation_data,
      const std::shared_ptr<v2::ml_data_side_features>& known_side_features) const override;

  /////////////////////////////////////////////////////////////////////////////////
  // Save and load stuff
  static constexpr size_t POPULARITY_RECOMMENDER_VERSION = 0;

  inline size_t internal_get_version() const override {
    return POPULARITY_RECOMMENDER_VERSION;
  }
  void internal_save(turi::oarchive& oarc) const override;
  void internal_load(turi::iarchive& iarc, size_t version) override;

 private:
  std::vector<double> item_predictions;
  double unseen_item_prediction;
  std::shared_ptr<nearest_neighbors::ball_tree_neighbors> nearest_items_model;

 public: 
  BEGIN_CLASS_MEMBER_REGISTRATION("popularity")
  REGISTER_CLASS_MEMBER_FUNCTION(recsys_popularity::list_fields)
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_value", recsys_popularity::get_value_from_state, "field");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_similar_items2", recsys_popularity::api_get_similar_items, "items", "k", "verbose", "get_all_items");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_similar_users2", recsys_popularity::api_get_similar_users, "users", "k", "get_all_users");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "predict2", recsys_popularity::api_predict, "data_to_predict", "new_user_data", "new_item_data");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_current_options2", recsys_popularity::api_get_current_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "set_current_options2", recsys_popularity::api_set_current_options, "options");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "train_test_split2", recsys_popularity::api_train_test_split, "dataset", "user_column", "item_column",
    "max_num_users", "item_test_proportion", "random_seed");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "train2", recsys_popularity::api_train, "dataset", "user_data", "item_data", "opts");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "recommend2", recsys_popularity::api_recommend, "query", "exclude", "restrictions", "new_data", "new_user_data",
    "new_item_data", "exclude_training_interactions", "top_k", "diversity", "random_seed");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_popularity_baseline2", recsys_popularity::get_popularity_baseline);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_item_intersection_info2", recsys_model_base::api_get_item_intersection_info, "item_pairs");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "export_to_coreml2", recsys_model_base::export_to_coreml, "model", "filename");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "precision_recall_stats2", recsys_model_base::api_precision_recall_stats, "indexed_validation_data", "recommend_output", "cutoffs");

  //REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
  //    "get_value2", recsys_popularity::api_get_value, "field");
  REGISTER_CLASS_MEMBER_FUNCTION(
    recsys_popularity::recommend_extension_wrapper, "reference_data", "new_observation_data", "top_k")

  END_CLASS_MEMBER_REGISTRATION

};

}}

#endif /* TURI_RECSYS_MODEL_POP_COUNT_H_ */


