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
  IMPORT_BASE_CLASS_REGISTRATION(recsys_model_base)
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
  IMPORT_BASE_CLASS_REGISTRATION(recsys_model_base)
  END_CLASS_MEMBER_REGISTRATION
};


}}

#endif
