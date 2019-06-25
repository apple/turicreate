/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FP_RESULTS_TREE_H
#define TURI_FP_RESULTS_TREE_H

#include <vector>
#include <stack>
#include <map>
#include <string>
#include <memory>
#include <algorithm>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sarray.hpp>

#include <toolkits/feature_engineering/topk_indexer.hpp>
#include <toolkits/feature_engineering/statistics_tracker.hpp>

#include <toolkits/pattern_mining/fp_node.hpp>
#include <toolkits/pattern_mining/fp_tree.hpp>
#include <core/util/dense_bitset.hpp>

namespace turi{
namespace pattern_mining {

/**
 * Tree data structure for keeping track of the frequent 'closed' itemsets
 *   This is a compressed, memory efficient data structure used to store and
 *   during the mining of 'closed' itemsets.
 *   Similar to the fp_tree structure, but stores itemsets, not transactions.
 *
 * Members:
 *   id_order_map (map of size_t to ranking) - global ordering of the ids
 *      (heuristic is to use decreasing order of support)
 *   hash_id_map (map to fp_nodes) - map to the head of linked lists for each id
 *   root_node
 *
 *
 * See: http://web.engr.illinois.edu/~hanj/pdf/icdm02_topk.pdf
 */
class fp_results_tree {
   public:
    std::map<size_t, size_t> id_order_map;
    std::map<size_t, fp_node*> hash_id_map;
    std::shared_ptr<fp_node> root_node = nullptr;

    // Constructors
    fp_results_tree();
    fp_results_tree(const std::vector<size_t>& id_order);
    fp_results_tree(const fp_results_tree& other_tree);

    /**
     * Save the fp_results_tree into a oarc.
     */
    inline void save(oarchive& oarc) const {
      // Save the id_order_map.
      size_t num_transactions = get_num_transactions();
      oarc << id_order_map
           << num_transactions;

      // Save the closed_itemsets as SFrame.
      gl_sframe closed_itemsets = get_closed_itemsets();
      const std::string & prefix = oarc.get_prefix();
      closed_itemsets.save(prefix);
    }

    /**
     * Load the fp_results_tree from an iarc.
     */
    inline void load(iarchive& iarc) {

      // Save the id_order_map.
      size_t num_transactions = 0;
      iarc >> id_order_map
           >> num_transactions;

      // Load the root-node
      root_node = std::make_shared<fp_node>(ROOT_ID, 0);
      root_node->item_count = num_transactions;

      // Load the closed_itemset SFrame.
      const std::string & prefix = iarc.get_prefix();
      gl_sframe closed_itemsets = gl_sframe(prefix);
      build_tree(closed_itemsets);
    }

    /**
     * Check if frequent itemset cannot be closed
     *
     * Checks if the itemset is a subset of an existing itemset in the tree
     * with equal support. Returns true if a subset with equal support exists.
     *
     * Args:
     *   potential_itemset (vector of size_t) - itemset to check
     * Returns:
     *   is_redundant (bool) - whether the itemset passes the test
     */
    bool is_itemset_redundant(const std::vector<size_t>& potential_itemset, \
        const size_t& support) const;

    /**
     * Add a potential closed itemset to the tree
     *
     * Args:
     *   potential_itemset (vector of size_ts) - itemset to add
     *   support (size_t) - support of the itemset
     */
    virtual void add_itemset(const std::vector<size_t>& potential_itemset, \
        const size_t& support);

    /**
     * Build results tree from a collection of closed itemsets
     *
     * Args:
     *   closed_itemsets (gl_sframe) - sframe of closed itemset (ids), support
     *
     */
    void build_tree(const gl_sframe& closed_itemsets);

    /**
     * Return the current collection of closed itemsets
     *
     * Returns:
     *   closed_itemsets (gl_sframe) - sframe of closed itemset (ids), support
     */
    gl_sframe get_closed_itemsets( \
        const std::shared_ptr<topk_indexer>& indexer = nullptr) const;

    /**
     * Return the current collection of closed itemsets as bitsets.
     *
     * Args:
     *   size: Size of each bitset returned.
     *   top_k (size_t) - maximum number of closed itemsets to returns
     *   min_length (size_t) - minimum required length for a closed itemset
     *
     * Returns:
     *   closed_bitsets - Vector of bitsets representing each set.
     */
    std::vector<dense_bitset> get_top_k_closed_bitsets(
                             const size_t& size,
                             const size_t& top_k = TOP_K_MAX,
                             const size_t& min_length = 1) const;

    /**
     * Return the top_k closed itemsets in descending order.
     *
     * Takes longer than get_closed_itemsets() due to sorting, but is faster
     * than trying to sort a gl_sframe.
     *
     * Args:
     *   top_k (size_t) - maximum number of closed itemsets to returns
     *   min_length (size_t) - minimum required length for a closed itemset
     *
     * Returns:
     *   top_k_closed_itemsets (gl_sframe) - sframe of top_k itemsets, support
     */
    gl_sframe get_top_k_closed_itemsets(const size_t& top_k = TOP_K_MAX, \
        const size_t& min_length = 1,
        const std::shared_ptr<topk_indexer>& indexer = nullptr) const;

    /**
     * Sort itemset by id_order_map (last element first)
     * Args:
     *   itemset (vector of size_ts)
     * Returns:
     *   sorted_itemset (vector of size_ts)
     */
    std::vector<size_t> sort_itemset(const std::vector<size_t>& itemset) const;

    /**
     * Get the support for a frequent itemset
     *   Note: the support for the empty set is the total number of transactions
     *
     * Args:
     *   sorted_itemset (vector of size_t) - a sorted frequent itemset (not
     *     necessarily closed) (see sort_itemset).
     *   lower_bound_on_support (size_t) - optional, for efficiency
     * Returns:
     *   support (size_t)
     */
    size_t get_support(const std::vector<size_t>& sorted_itemset, \
      const size_t& lower_bound_on_support = 0) const;

    /**
     * Get the number of transaction
     */
    inline size_t get_num_transactions() const {return root_node->item_count;};
    /**
     * Prune Tree
     * Pass over the tree, removing all nodes (corresponding to itemsets)
     *   with support strictly less than the given (final) min support
     * Rebuilds tree -> Should be rarely called (ideally only at the end)
     *
     * Args:
     *   min_support (size_t)
     */
    void prune_tree(const size_t& min_support);

};

// Helper Functions

/**
 * Check if itemset is a subset on path to root (from node)
 * Args:
 *   sorted_itemset (vector of size_ts) - sorted in id_order
 *   node (pointer to fp_node) - starting point in tree
 * Returns:
 *   is_subset (bool) - whether the sorted_itemset is a subset
 */
bool is_subset_on_path(const std::vector<size_t>& sorted_itemset, \
    fp_node* node);

/**
 * Provides printing of the fp_results_tree
 */
std::ostream& operator<<(std::ostream& out, const fp_results_tree& tree);

/**
 * Converts an itemset (a vector of item_ids) into a flex_list for an sframe.
 *   Uses an indexer to convert from ids to item type
 */
flex_list itemset_to_flex_list(const std::vector<size_t>& itemset, \
    const std::shared_ptr<topk_indexer>& indexer = nullptr);

/**
 * Tree data structure for keeping track of the top_k frequent 'closed' itemsets
 *   of length at least min_length
 *   This is a compressed, memory efficient data structure used to store and
 *   during the mining of 'closed' itemsets.
 *
 *  Extends the fp_results_tree class
 *  Additional Vars:
 *    top_k (size_t) - maximum number of closed itemsets to mine
 *      top_k should be less than TOP_K_MAX
 *    min_length (size_t) - minimum length of a closed itemset
 *    min_support_heap (min_heap) - min heap of top_k supports
 *
 */
class fp_top_k_results_tree : public fp_results_tree {
  public:
    size_t top_k;
    size_t min_length;
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>>
                                                            min_support_heap;

    fp_top_k_results_tree();
    fp_top_k_results_tree(const std::vector<size_t>& id_order, \
        const size_t& k = TOP_K_MAX, const size_t& length = 1);
    fp_top_k_results_tree(const fp_top_k_results_tree & other_tree);

    /**
     * Save the fp_results_tree into a oarc.
     */
    inline void save(oarchive& oarc) const {
      // Save the id_order_map.
      size_t num_transactions = root_node->item_count;
      oarc << id_order_map
           << top_k
           << min_length
           << num_transactions;

      // Save the closed_itemsets as SFrame.
      gl_sframe closed_itemsets = get_top_k_closed_itemsets(TOP_K_MAX, 1);
      const std::string & prefix = oarc.get_prefix();
      closed_itemsets.save(prefix);
    }

    /**
     * Load the fp_results_tree from an iarc.
     */
    inline void load(iarchive& iarc) {
      // Save the id_order_map.
      size_t num_transactions = 0;
      iarc >> id_order_map
           >> top_k
           >> min_length
           >> num_transactions;

      // Load the root-node
      root_node = std::make_shared<fp_node>(ROOT_ID, 0);
      root_node->item_count = num_transactions;

      // Load the closed_itemset SFrame.
      const std::string & prefix = iarc.get_prefix();
      gl_sframe closed_itemsets = gl_sframe(prefix);
      build_tree(closed_itemsets);
    }

    /**
     * Add a potential closed itemset to the tree
     *
     * Args:
     *   potential_itemset (vector of size_ts) - itemset to add
     *   support (size_t) - support of the itemset
     */
    void add_itemset(const std::vector<size_t>& potential_itemset, \
        const size_t& support);

    /**
     * Get a bound for the min_support.
     *
     * Returns:
     *   A current estimate of min support.
     */
    size_t get_min_support_bound();

    /**
     * Insert support into the min_support heap.
     *
     * Args:
     *   support (size_t) - support of the itemset
     */
    void insert_support(const size_t& support);

    /**
     * Overrides fp_result_tree::get_closed_itemsets
     * Return the current collection of closed itemsets
     * Args:
     *   indexer: An indexer to convert from id to the value.
     *
     * Returns:
     *   closed_itemsets (gl_sframe) - sframe of closed itemset (ids), support
     */
    gl_sframe get_closed_itemsets( \
        const std::shared_ptr<topk_indexer>& indexer = nullptr) const;

};




} // pattern_mining
} // turicreate

#endif
