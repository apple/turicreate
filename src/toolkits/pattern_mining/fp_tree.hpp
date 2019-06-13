/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FP_TREE_H
#define TURI_FP_TREE_H

#include <vector>
#include <map>
#include <stack>
#include <string>
#include <memory>
#include <algorithm>
#include <iostream>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sarray.hpp>

#include <toolkits/feature_engineering/topk_indexer.hpp>
#include <toolkits/feature_engineering/statistics_tracker.hpp>

#include <toolkits/pattern_mining/fp_node.hpp>
#include <toolkits/pattern_mining/fp_tree_header.hpp>

namespace turi {
namespace pattern_mining {

const size_t TOP_K_MAX = std::numeric_limits<size_t>::max();
// FP-TREE /////////////////////////////////////////
/**
 * FP-Tree data structure
 * Vars:
 *    root_node (pointer fp_node) - the root node of the tree
 *    root_prefix (vector of ids) - the prefix of the fp-tree
 *    header (fp_tree_header) - the header for the fp-tree.
 *    my_closed_node_count - data structure to raise min_support
 *
 * See http://hanj.cs.illinois.edu/pdf/sigmod00.pdf
 *     http://web.engr.illinois.edu/~hanj/pdf/icdm02_topk.pdf and
 *     http://hanj.cs.illinois.edu/pdf/tkde05_tfp.pdf
 */
class fp_tree {
  public:
    std::shared_ptr<fp_node> root_node = nullptr;
    std::vector<size_t> root_prefix;
    fp_tree_header header;

    // Constructors
    fp_tree();
    fp_tree(const fp_tree_header& header, \
        const std::vector<size_t>& prefix = std::vector<size_t>());
    fp_tree(const fp_tree & other_tree);
    // Destructor
    ~fp_tree();

    // Member Functions:
    /**
     * Prune tree of items less than min_support
     * Args:
     *   min_support (size_t) - new min_support to prune tree
     */
    void prune_tree(const size_t& min_support);

    /**
     * Get the support of all items at or below min_depth
     * Args:
     *   min_depth (size_t)
     * Returns:
     *   supports_at_depth (vector of size_t)
     */
    std::vector<size_t> get_supports_at_depth(const size_t& depth) const;

    /**
     * Get the support of the item in transactions below a depth
     *
     * Args:
     *   heading (fp_tree_heading) - the item heading
     *   min_depth (size_t) - minimum depth in fp-tree (default is 1)
     * Returns:
     *   support (size_t) the support (total count) of item id in the fp_tree
     *                                                   (at min_depth)
     */
    size_t get_support(const fp_tree_heading& heading, \
        const size_t& min_depth = 1) const;

    /**
     * Get total number of transactions in the current fp-tree.
     */
    size_t get_num_transactions() const;

    /**
     * Get the supports of descendant node of an anchor node
     * Args:
     *   anchor_node (pointer to fp_node)
     * Returns:
     *   supports (vector of size_t) the supports of the children nodes
     *     This vector is used to raise min_support.
     */
    std::vector<size_t> get_descendant_supports(fp_node* anchor_node);

    /**
     * Add a transaction to the tree
     * Args:
     *   new_transaction (vector of size_ts) - new itemset
     *   count (size_t) - repetitions of the transaction (should be positive)
     */
    virtual void add_transaction(const std::vector<size_t>& new_transaction,\
        const size_t& count);


    /**
     * Get the header for a conditional fp-tree
     *
     *   The conditional fp-tree of item 'alpha' is the fp-tree constructed
     *   from transcations in the current fp-tree that contain more frequent
     *   items than 'alpha'. Note that only items with support greater than
     *   min_support (in the new subtree) are returned.
     *
     * Args:
     *   heading (fp_tree_heading) - the item heading of the conditional fp-tree
     *   min_support (size_t) - the minimum number of transactions required
     *     to be 'frequent'
     * Returns:
     *   cond_header (fp_tree-header) - header of conditional fp-tree
     */
     fp_tree_header get_cond_header(const fp_tree_heading& heading, \
        const size_t& min_support) const;

    /**
     * Get the item frequencies for a conditional fp-tree
     *
     * Args:
     *   heading (fp_tree_heading) - the item heading of the conditional fp-tree
     * Returns:
     *   item_counts (vector of size_t, size_t) - Vector of item id and counts
     *     of the conditional fp-tree
     */
    std::vector<std::pair<size_t, size_t>> get_cond_item_counts(\
        const fp_tree_heading& heading) const;

    /**
     * Construct the conditional fp-tree (fp-tree) for item id at the heading
     *
     *   The conditional fp-tree of item 'id' is the fp-tree constructed
     *   from transcations in the current fp-tree that contain more frequent
     *   items than 'id'.
     *
     * Args:
     *   heading (fp_tree_heading) - the item heading of the conditional fp-tree
     *   min_support (size_t) - the minimum number of transactions required
     *     to be 'frequent'.
     * Returns:
     *   cond_tree (fp_tree) - sorted vector of item ids.
     */
    fp_tree build_cond_tree(const fp_tree_heading& heading, \
        const size_t& min_support) const;

}; // End of fp-tree class

// Other Functions

/**
 * Build the original fp-tree from a database of transactions and increase the
 * minimum support using closed_node_count.
 *
 * Args:
 *   database (gl_sarray) - SArray of transactions (list of item ids)
 *   min_support (size_t) - minimum number of transactions required to be
 *     'frequent'. Increased if possible.
 *   top_k (size_t) - maximum number of closed itemsets to mine
 *   min_length (size_t) - minimum length of closed itemsets to mine
 * Returns:
 *   global_fp_tree (fp_tree) - the data structure for FP-growth
 */
fp_tree build_tree(const gl_sarray& database, const size_t& min_support);

// Helper functions

/**
 * Sorted and filter the header of an fp-tree
 *
 * Args:
 *   item_counts (vector of size_t, size_t) - vector of item id and count
 *   min_support (size_t) - minimum number of transactions required to be
 *     'frequent'.
 * Returns:
 *   header (fp_tree_header) - header for new fp_tree
 */
fp_tree_header build_header( \
    const std::vector<std::pair<size_t,size_t>>& item_counts,\
    const size_t& min_support);

/**
 * Get the item frequencies for the original fp-tree
 *
 * Args:
 *   database (gl_sarray) - SArray of transactions (list of item ids)
 * Returns:
 *   item_counts (vector of size_t, size_t) - vector of item id and counts
 */
std::vector<std::pair<size_t, size_t>> get_item_counts(const gl_sarray& database);

/**
 * Convert row of gl_sarray to vector of item ids
 *   Removes Duplicates/Repeats in the row
 *   FLEX_UNDEFINED are ignored (this allows for empty vectors)
 *
 * Args:
 *   transaction_array (flexible_type) - flex_list of flex_int/double
 * Returns:
 *   id_vector (vector of size_ts) - vector to add to fp_tree
 */
std::vector<size_t> flex_to_id_vector(const flexible_type& transaction_array);

/**
 * Provides printing of the fp_tree.
 */
std::ostream& operator<<(std::ostream& out, const fp_tree& tree);



// FP_TOP_K_TREE /////////////////////////////////////////
/**
 * TOP-K FP-Tree data structure
 *
 * Differs from a regular fp_tree in keeping track of a closed_node_count
 *  data structure to raise min_support
 *
 * Vars:
 *   top_k - number of closed itemsets to mine (MINE_ALL returns all itemsets)
 *   min_length - minimum length of closed itemset
 *   support_count_map - map of support to number of closed nodes with support
 *
 *     http://web.engr.illinois.edu/~hanj/pdf/icdm02_topk.pdf
 *     http://hanj.cs.illinois.edu/pdf/tkde05_tfp.pdf
 */

class fp_top_k_tree : public fp_tree {
  public:
    size_t top_k;
    size_t min_length;
    std::map<size_t, size_t> closed_node_count;

    // Constructors are derived from fp_tree
    fp_top_k_tree();
    fp_top_k_tree(const fp_tree_header& header, \
        const size_t& k = TOP_K_MAX, const size_t& length = 1, \
        const std::vector<size_t>& prefix = std::vector<size_t>());
    fp_top_k_tree(const fp_top_k_tree& other_tree);

    /**
     * Get the min_support bound from closed_node_count
     */
    size_t get_min_support_bound();

    /*
     * Anchor node + Descendent Support-raising-method
     */
    fp_node* get_anchor_node();
    size_t get_anchor_min_support_bound();

    /**
     * Returns min_depth for a node to be in transaction of length min_length
     */
    size_t get_min_depth();

    /**
     * Modified methods to keep track of closed_node_count
     */
    fp_top_k_tree build_cond_tree(const fp_tree_heading& heading, \
        size_t& min_support) const;
    void add_transaction(const std::vector<size_t>& new_transaction,\
        const size_t& count);
    void update_if_closed_node(fp_node* node, const size_t& count);

};

/**
 * Build the global top-k fp-tree from a database of transactions
 *
 * Args:
 *   database (gl_sarray) - SArray of transactions (list of item ids)
 *   min_support (size_t) - minimum number of transactions required to be
 *     'frequent'. Increased if possible.
 *   top_k (size_t) - maximum number of closed itemsets to mine
 *   min_length (size_t) - minimum length of closed itemsets to mine
 * Returns:
 *   global_top_k_tree (fp_top_k_tree) - the data structure for Top-K FP Growth
 */
fp_top_k_tree build_top_k_tree(const gl_sarray& database, size_t& min_support,\
     const size_t& top_k = TOP_K_MAX, const size_t& min_length = 1);

/**
 * Get k-th largest element of vector using min-heap.
 */
size_t get_largest(std::vector<size_t> vec, size_t k);

} // pattern_mining
} // turicreate
#endif
