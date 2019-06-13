/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/query_engine/operators/operator_transformations.hpp>
#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/planning/planner_node.hpp>
#include <core/logging/assertions.hpp>

namespace turi { namespace query_eval {

/** Turns a node graph into one with all the source nodes segmented.
 *  Used to run a section in parallel.
 */
pnode_ptr make_segmented_graph(pnode_ptr n, size_t segment_idx,
    size_t num_segments, std::map<pnode_ptr, pnode_ptr>& memo) {
  if (memo.count(n)) return memo[n];
  if(num_segments == 0) {
    memo[n] = n;
    return n;
  }

  pnode_ptr ret(new planner_node(*n));

  if(is_source_node(n)) {

    // First, if it's a source node, then it should have begin_index,
    // and end_index in the operarator_parameters.

    DASSERT_TRUE(n->operator_parameters.count("begin_index"));
    DASSERT_TRUE(n->operator_parameters.count("end_index"));

    size_t old_begin_index = n->operator_parameters.at("begin_index");
    size_t old_end_index = n->operator_parameters.at("end_index");

    size_t old_length = old_end_index - old_begin_index;

    size_t new_begin_index = old_begin_index + (segment_idx * old_length) / num_segments;
    size_t new_end_index = old_begin_index + ((segment_idx + 1) * old_length) / num_segments;

    DASSERT_LE(old_begin_index, new_begin_index);
    DASSERT_LE(new_end_index, old_end_index);

    ret->operator_parameters["begin_index"] = new_begin_index;
    ret->operator_parameters["end_index"] = new_end_index;

  } else {
    for(size_t i = 0; i < ret->inputs.size(); ++i) {
      ret->inputs[i] = make_segmented_graph(ret->inputs[i], segment_idx, num_segments, memo);
    }
  }
  memo[n] = ret;
  return ret;
}

pnode_ptr make_sliced_graph(pnode_ptr n, size_t begin_index, size_t end_index,
                            std::map<pnode_ptr, pnode_ptr>& memo) {
  // only forward slice
  ASSERT_LE(begin_index, end_index);

  if (memo.count(n)) return memo[n];
  pnode_ptr ret(new planner_node(*n));
  if(is_source_node(n)) {
    DASSERT_TRUE(n->operator_parameters.count("begin_index"));
    DASSERT_TRUE(n->operator_parameters.count("end_index"));
    size_t old_begin_index = n->operator_parameters.at("begin_index");
    size_t old_end_index = n->operator_parameters.at("end_index");

    // slice range can be recursive
    size_t new_length = (end_index - begin_index);
    begin_index = old_begin_index + begin_index;
    end_index = begin_index + new_length;

    // cannot slice beyond current range
    ASSERT_LE(end_index, old_end_index);
    ret->operator_parameters["begin_index"] = begin_index;
    ret->operator_parameters["end_index"] = end_index;
  } else {
    for(size_t i = 0; i < ret->inputs.size(); ++i) {
      ret->inputs[i] = make_sliced_graph(ret->inputs[i], begin_index, end_index, memo);
    }
  }
  // forget any length  memoized
  ret->any_operator_parameters.erase("__length_memo__");
  memo[n] = ret;
  return ret;
}

}}
