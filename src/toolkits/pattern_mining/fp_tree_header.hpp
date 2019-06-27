/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FP_TREE_HEADER_H
#define TURI_FP_TREE_HEADER_H

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

namespace turi {
namespace pattern_mining {

/**
 * Struct for fp_tree_header
 *
 * Vars
 *   support (size_t) - item supports in decreasing or:der
 *   id (size_t) - item ids
 *   pointer (pointers) - pointers to linked list of item id
 */
struct fp_tree_heading {
  public:
    size_t id;
    size_t support;
    fp_node* pointer;
};

/**
 * Header for fp_tree
 *
 * Vars
 *   headings (vector of fp_tree_heading)
 *   index_map (map of id to index) - hash for searching headings by item id
 */
class fp_tree_header {
  public:
    std::vector<fp_tree_heading> headings;
    std::map<size_t, size_t> id_index_map;

    // Constructor
    fp_tree_header();
    fp_tree_header(const std::vector<size_t>& ids, \
        const std::vector<size_t>& supports);
    fp_tree_header(const fp_tree_header& other_header);

    /**
     * Get index of id in headings
     */
    size_t get_index(const size_t& id) const;
    inline const size_t size() const {return headings.size();};

    /**
     * Sort a transaction by heading's order
     */
    std::vector<std::pair<size_t, size_t>> sort_transaction( \
        const std::vector<size_t>& new_transaction) const;

    /**
     * Extract vectors and maps from headings
     */
    std::vector<size_t> get_ids();
    std::vector<size_t> get_supports();
    std::map<size_t, fp_node*> get_pointers();

    /**
     * Extract headings for specific id
     */
    bool has_id(const size_t& id) const;
    const fp_tree_heading get_heading(const size_t& id) const;
};

/**
 * Print header
 */
std::ostream& operator<<(std::ostream& out, const fp_tree_header& header);

} // pattern_mining
} // turicreate
#endif
