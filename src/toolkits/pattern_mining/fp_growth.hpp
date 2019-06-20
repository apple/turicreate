/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FP_GROWTH_H
#define TURI_FP_GROWTH_H

#include <vector>
#include <map>
#include <stack>
#include <string>
#include <algorithm>
#include <core/export.hpp>
#include <core/util/dense_bitset.hpp>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/extensions/ml_model.hpp>

#include <toolkits/pattern_mining/fp_tree.hpp>
#include <toolkits/pattern_mining/fp_results_tree.hpp>
#include <toolkits/feature_engineering/topk_indexer.hpp>

namespace turi {
namespace pattern_mining {


/**
 * fp_growth model interface.
 * ---------------------------------------
 * TODO: Add some comments hare about what the base class is supposed to be.
 *
 */
class EXPORT fp_growth : public ml_model_base {

  static constexpr size_t FP_GROWTH_VERSION = 0;

  gl_sframe closed_itemset;                  /* Closed itemsets as SFrame. */
  fp_top_k_results_tree closed_itemset_tree; /* Closed itemsets as trees. */
  std::vector<dense_bitset> closed_bitsets;  /* Closed itemsets as bitset. */

  std::vector<std::string> features;         /* Features that make a session id. */
  std::string item;                          /* Set the item column name. */
  std::shared_ptr<topk_indexer> indexer;     /* Indexer for the event map. */

  public:

  virtual inline ~fp_growth() {}

  /**
   * Validate the input data.
   * \param[in] data     gl_sframe of data in stacked format.
   * \param[in] item    Name of the item column.
   * \param[in] features Names of the feature columns.
   */
  void validate(const gl_sframe& data,
                const std::string& item,
                const std::vector<std::string>& features) const;
  /**
   * Set the feature column names.
   *
   * \param[in] _features Names of the "features" column.
   */
  void set_features(const std::vector<std::string>& _features) {
    this->features.resize(_features.size());
    for (size_t i = 0; i < _features.size(); ++i) {
      this->features[i] = _features[i];
    }
    add_or_update_state({{"features", to_variant(this->features)}});
    add_or_update_state({{"num_features", to_variant(_features.size())}});
  }

  /**
   * Set the item column name.
   *
   * \param[in] item Name of the item column.
   */
  void set_item(const std::string& _item) {
     this->item.assign(_item);
     add_or_update_state({{"item", to_variant(this->item)}});
  }

  /**
   * Train FP-Growth on the full dataset.
   *
   * \param[in] data  gl_sframe of data in stacked format.
   */
  void train(const gl_sframe& data);

  /* Preprocess data.
   *
   * \param[in] data  gl_sframe of data in stacked format.
   * \returns Indexed data in a stacked format.
   */
  gl_sframe preprocess(const gl_sframe& data) const;

  /**
   * Extract features from the training data.
   *
   * \param[in] Data in the same format as the input data.
   */
  gl_sframe extract_features(const gl_sframe& data) const;

  /**
   * Predict top-k rules
   *
   * \param[in] Data in the same format as the input data.
   * \param[in] output_type The scoring function used to score the results.
   * \param[in] k The topk rules to predict.
   */
  gl_sframe predict_topk(const gl_sframe& data,
                         const std::string& score_function = "confidence",
                         const size_t& k = 5) const;

  /**
   * Returns the current model version
   */
  size_t get_version() const override;

  /**
   * Serializes the model. Must save the model to the file format version
   * matching that of get_version()
   */
  void save_impl(oarchive& oarc) const override;

  /**
   * Loads a model previously saved at a particular version number.
   * Should raise an exception on failure.
   */
  void load_version(iarchive& iarc, size_t version) override;

  /**
   * Set one of the options in the algorithm. Use the option manager to set
   * these options. If the option does not satisfy the conditions that the
   * option manager has imposed on it. Errors will be thrown.
   *
   * \param[in] options Options to set
   */
  void init_options(const std::map<std::string,flexible_type>& _options) override;

  /**
   * Get frequent itemsets using the FP-Growth algorithm.
   */
  gl_sframe get_frequent_patterns() const;

  // Register function for Python
  // --------------------------------------------------------------------------
  BEGIN_CLASS_MEMBER_REGISTRATION("_FPGrowth")
  REGISTER_CLASS_MEMBER_FUNCTION(fp_growth::init_options, "_options");
  REGISTER_CLASS_MEMBER_FUNCTION(fp_growth::get_current_options);
  REGISTER_CLASS_MEMBER_FUNCTION(fp_growth::get_frequent_patterns);
  REGISTER_CLASS_MEMBER_FUNCTION(fp_growth::list_fields);
  REGISTER_CLASS_MEMBER_FUNCTION(fp_growth::extract_features, "data");
  REGISTER_CLASS_MEMBER_FUNCTION(fp_growth::predict_topk,
                 "data", "score_function", "k");
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("_get_default_options",
                                     fp_growth::get_default_options);
  REGISTER_NAMED_CLASS_MEMBER_FUNCTION("get",
                                     fp_growth::get_value_from_state,
                                     "key");
  END_CLASS_MEMBER_REGISTRATION
};

/**
 * Create a pattern mining model.
 *
 * \param[in] data Data as an SFrame in stacked format.
 * \param[in] item The name of the item/event column.
 * \param[in] features The names of the columns that uniquely identify the session.
 * \param[in] min_support The min support of each pattern.
 * \param[in] max_patterns The maximum number of patterns to mine.
 * \param[in] min_length The min length of each pattern.
 *
 */
EXPORT std::shared_ptr<fp_growth> _pattern_mining_create(
               gl_sframe data,
               std::string item,
               std::vector<std::string> features,
               size_t min_support,
               size_t max_patterns,
               size_t min_length);


// ITEMSET MINING ALGORITHMS /////////////////////////////////////////////////
/**
 * Mine the closed itemset from the transaction database with at least
 * min_support
 *
 * Args:
 *   database (gl_sarray) - SArray of transactions (flex_list of int)
 *   min_support (size_t) - minimum number of transactions required to be
 *     'frequent'.
 *
 * Returns:
 *   closed_itemset_tree (fp_results_tree) - closed itemsets in compressed format
 *
 */
fp_results_tree closet_algorithm(const gl_sarray& database, const size_t& min_support);

void closet_growth(fp_tree& my_tree, fp_results_tree& closed_itemset_tree,\
    const size_t& min_support);

void closet_prune(fp_tree& my_tree, fp_results_tree& closed_itemset_tree,\
    const size_t& min_support);


/**
 * Mine the top_k closed itemsets from the transaction database that have
 * at least min_length.
 *
 * Args:
 *   database (gl_sarray) - SArray of transactions (flex_list of int)
 *   min_support (size_t) - minimum number of transaction required to be
 *     'frequent'.
 *   top_k (size_t) - the maximum number of transactions to mine
 *   min_length (size_t) - the required minimum length of a transaction
 *     increasing min_length mines longer (but less frequent) transactions
 * Returns:
 *   closed_itemset_tree (fp_top_k_results_tree) closed itemset tree in
 *     compressed format.
 *
 *
 */
fp_top_k_results_tree top_k_algorithm(const gl_sarray& database, \
    size_t& min_support, const size_t& top_k = TOP_K_MAX, \
    const size_t& min_length = 1);

/**
 * Helper function. Mines the global fp_tree top down (most frequent first)
 */
void global_top_down_growth(fp_top_k_tree& my_tree,\
    fp_top_k_results_tree& closed_itemset_tree, size_t& min_support);

/**
 * Helper function. Mines local fp_trees bottom up (least frequent first)
 */
void local_bottom_up_growth(fp_top_k_tree& my_tree,\
    fp_top_k_results_tree& closed_itemset_tree, size_t& min_support);


} // pattern_mining
} // turicreate
#endif
