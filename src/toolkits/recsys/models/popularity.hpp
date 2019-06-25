/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_RECSYS_MODEL_POPULARITY_H_
#define TURI_RECSYS_MODEL_POPULARITY_H_

#include <vector>
#include <string>
#include <toolkits/recsys/recsys_model_base.hpp>
#include <toolkits/nearest_neighbors/nearest_neighbors.hpp>
#include <toolkits/nearest_neighbors/ball_tree_neighbors.hpp>


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
  IMPORT_BASE_CLASS_REGISTRATION(recsys_model_base)
  END_CLASS_MEMBER_REGISTRATION

};

}}

#endif /* TURI_RECSYS_MODEL_POP_COUNT_H_ */
