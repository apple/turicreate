/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FP_NODE_H
#define TURI_FP_NODE_H

#include <vector>
#include <list>
#include <map>
#include <string>
#include <memory>
#include <algorithm>

#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sarray.hpp>

#include <toolkits/feature_engineering/topk_indexer.hpp>
#include <toolkits/feature_engineering/statistics_tracker.hpp>

namespace turi{
namespace pattern_mining {

const size_t ROOT_ID = -1; // item id reserved for root node

/**
 * Nodes for the FP-Tree data structure
 *
 * Note: FP-Nodes should only be created via shared pointers
 *   std::make_shared<fp_node>(size_t id)
 *
 * Members:
 *   item_id is the size_teger index of items in the tree (e.g. 0 -> Dog, 1 -> Cat)
 *        Note: m_id = -1 should be reserved for the root node.
 *   item_count is the frequency of the index (e.g. 'Dog' occurs m_count times)
 *   depth is the depth of the node in the tree
 *   parent_node is the node's parent in the tree
 *   children_node are the node's children in the tree
 *   next_node is the next location of a node with the same item_id
 */
class fp_node {
   public:
    size_t item_id;
    size_t item_count;
    size_t depth;
    bool is_closed_node;
    fp_node* next_node = nullptr;
    fp_node* parent_node = nullptr;
    std::vector<std::shared_ptr<fp_node>> children_nodes;

    // Constructor
    /**
     * Construct New Node
     * Args:
     *   id (size_t) - item id for node (Note: ROOT_ID is reserved for root)
     *   depth (size_t) - depth of node in tree
     */
    fp_node(const size_t& id = ROOT_ID, const size_t& node_depth = 0);

    // Set Function
    /**
     * Add Child Node
     * Adds a new child node with id if one does not currently exist.
     * Current node will be set as child node's parent.
     * Args:
     *   child_id (size_t) - id for child node (Note: ROOT_ID is reserved)
     * Returns:
     *   A pointer to the child node
     */
    fp_node* add_child(const size_t& child_id);

    /**
     * Get Child Node
     * Gets a pointer to the child node with id
     * Args:
     *  child_id (size_t) - id for child node (Note: ROOT_ID is reserved)
     * Returns:
     *   A pointer to the child node, nullptr if no child exists
     */
    fp_node* get_child(const size_t& child_id) const;

    /**
     * Get set on path to root from current node.
     * Returns:
     *   set (vector of size_t) - set of item ids on path to root
     */
    std::vector<size_t> get_path_to_root();

    /**
     * Check if node is a closed node.
     * A node is closed if it's support is greater than any of its children.
     * Once closed, a node will remain closed.
     *
     * Returns:
     *   is_closed (bool) - whether the node is closed
     */
    bool is_closed() const;

    /**
     * Delete pointer to self from parent (if a parent exists)
     */
    void erase();

};

} // pattern_mining
} // turicreate
#endif
