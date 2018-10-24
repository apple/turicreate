/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_RECSYS_FACTORIZATION_MODELS_H_
#define TURI_RECSYS_FACTORIZATION_MODELS_H_

#include <unity/toolkits/recsys/recsys_model_base.hpp>

namespace turi {

namespace v2 {
class ml_data;
}

namespace factorization {
class factorization_model;
}

namespace recsys {

/** Implements all the factorization stuff -- a thin wrapper to the
 *  factorization models.
 */
class EXPORT recsys_factorization_model_base : public recsys_model_base {

 public:
  bool include_columns_beyond_user_item() const { return true; }

  void init_options(const std::map<std::string, flexible_type>& _options); 

  std::map<std::string, flexible_type> train(const v2::ml_data& training_data);

  sframe predict(const v2::ml_data& test_data) const;

  sframe get_similar_items(std::shared_ptr<sarray<flexible_type>> indexed_items, 
                           size_t k=0) const;

  void get_item_similarity_scores(
      size_t item, std::vector<std::pair<size_t, double> >& sim_scores) const;

  sframe get_similar_users(std::shared_ptr<sarray<flexible_type>> indexed_items,
                           size_t k=0) const;

 protected:
  sframe get_similar(size_t column, std::shared_ptr<sarray<flexible_type> > indexed_items, size_t k) const;

 private:
  mutable mutex _get_similar_buffers_lock;
  mutable std::vector<arma::fvec>  _get_similar_buffers;


 public:
  void score_all_items(
      std::vector<std::pair<size_t, double> >& scores,
      const std::vector<v2::ml_data_entry>& query_row,
      size_t top_k,
      const std::vector<std::pair<size_t, double> >& user_item_list,
      const std::vector<std::pair<size_t, double> >& new_user_item_data,
      const std::vector<v2::ml_data_row_reference>& new_observation_data,
      const std::shared_ptr<v2::ml_data_side_features>& known_side_features) const; 
  
  static constexpr size_t RECSYS_FACTORIZATION_MODEL_VERSION = 1;

  inline size_t internal_get_version() const {
    return RECSYS_FACTORIZATION_MODEL_VERSION;
  }
  void internal_save(turi::oarchive& oarc) const;

  void internal_load(turi::iarchive& iarc, size_t version);

 protected:

  /** This term determines whether we work in ranking factorization or
   *  not.
   */
  virtual bool include_ranking_options() const = 0;

  std::map<std::string, flexible_type> train(
                  const v2::ml_data& training_data_by_user, 
                  const v2::ml_data& training_data_by_item);
 private:
  std::shared_ptr<factorization::factorization_model> model;

};

////////////////////////////////////////////////////////////////////////////////
// Now the individual model definitions

/** Implements factorization model.
 */
class recsys_factorization_model : public recsys_factorization_model_base {

 public:
  bool use_target_column(bool target_is_present) const override { return true; }

 private:
  bool include_ranking_options() const override { return false; }

 public: 
   // TODO: convert interface above to use the extensions methods here
  BEGIN_CLASS_MEMBER_REGISTRATION("factorization_recommender")
  REGISTER_CLASS_MEMBER_FUNCTION(recsys_factorization_model::list_fields)
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_value", recsys_factorization_model::get_value_from_state, "field");



  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_value", recsys_factorization_model::get_value_from_state, "field"); //"get_value"
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_similar_items", recsys_factorization_model::api_get_similar_items, "items", "k", "verbose", "get_all_items");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_similar_users", recsys_factorization_model::api_get_similar_users, "users", "k", "get_all_users");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "predict", recsys_factorization_model::api_predict, "data_to_predict", "new_user_data", "new_item_data");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_current_options", recsys_factorization_model::api_get_current_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "set_current_options", recsys_factorization_model::api_set_current_options, "options");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_train_stats", recsys_factorization_model::api_get_train_stats);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "train_test_split", recsys_factorization_model::api_train_test_split, "dataset", "user_column", "item_column",
    "max_num_users", "item_test_proportion", "random_seed");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "train", recsys_factorization_model::api_train, "dataset", "user_data", "item_data", "nearest_items", "opts");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "recommend", recsys_factorization_model::api_recommend, "query", "exclude", "restrictions", "new_data", "new_user_data",
    "new_item_data", "exclude_training_interactions", "top_k", "diversity", "random_seed");
  //REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
  //  "get_popularity_baseline", recsys_factorization_model::get_popularity_baseline);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_item_intersection_info", recsys_factorization_model::api_get_item_intersection_info, "item_pairs");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "export_to_coreml", recsys_factorization_model::export_to_coreml, "model", "filename"); ///
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "precision_recall_stats", recsys_factorization_model::api_precision_recall_stats, "indexed_validation_data", "recommend_output", "cutoffs");

  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_data_schema", recsys_factorization_model::api_get_data_schema);






  REGISTER_CLASS_MEMBER_FUNCTION(
    recsys_factorization_model::recommend_extension_wrapper, 
    "reference_data", "new_observation_data", "top_k")
  END_CLASS_MEMBER_REGISTRATION
};

/** Implements linear_model.
 */
class recsys_ranking_factorization_model : public recsys_factorization_model_base {

 public:
  bool use_target_column(bool target_is_present) const override { return target_is_present; }

 private:
  bool include_ranking_options() const override { return true; }

 public: 
   // TODO: convert interface above to use the extensions methods here
  BEGIN_CLASS_MEMBER_REGISTRATION("ranking_factorization_recommender")
  REGISTER_CLASS_MEMBER_FUNCTION(recsys_ranking_factorization_model::list_fields)
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_value", recsys_ranking_factorization_model::get_value_from_state, "field");


  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_value", recsys_ranking_factorization_model::get_value_from_state, "field"); //"get_value"
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_similar_items", recsys_ranking_factorization_model::api_get_similar_items, "items", "k", "verbose", "get_all_items");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_similar_users", recsys_ranking_factorization_model::api_get_similar_users, "users", "k", "get_all_users");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "predict", recsys_ranking_factorization_model::api_predict, "data_to_predict", "new_user_data", "new_item_data");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
      "get_current_options", recsys_ranking_factorization_model::api_get_current_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "set_current_options", recsys_ranking_factorization_model::api_set_current_options, "options");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_train_stats", recsys_ranking_factorization_model::api_get_train_stats);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "train_test_split", recsys_ranking_factorization_model::api_train_test_split, "dataset", "user_column", "item_column",
    "max_num_users", "item_test_proportion", "random_seed");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "train", recsys_ranking_factorization_model::api_train, "dataset", "user_data", "item_data", "nearest_items", "opts");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "recommend", recsys_ranking_factorization_model::api_recommend, "query", "exclude", "restrictions", "new_data", "new_user_data",
    "new_item_data", "exclude_training_interactions", "top_k", "diversity", "random_seed");
  //REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
  //  "get_popularity_baseline", recsys_ranking_factorization_model::get_popularity_baseline);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_item_intersection_info", recsys_ranking_factorization_model::api_get_item_intersection_info, "item_pairs");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "export_to_coreml", recsys_ranking_factorization_model::export_to_coreml, "model", "filename"); ///
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "precision_recall_stats", recsys_ranking_factorization_model::api_precision_recall_stats, "indexed_validation_data", "recommend_output", "cutoffs");

  REGISTER_NAMED_CLASS_MEMBER_FUNCTION(
    "get_data_schema", recsys_ranking_factorization_model::api_get_data_schema);



  REGISTER_CLASS_MEMBER_FUNCTION(
    recsys_ranking_factorization_model::recommend_extension_wrapper, 
    "reference_data", "new_observation_data", "top_k")
  END_CLASS_MEMBER_REGISTRATION

};


}}

#endif
