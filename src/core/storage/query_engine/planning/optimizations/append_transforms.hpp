/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_OPTIMIZATION_APPEND_TRANSFORMS_H_
#define TURI_SFRAME_QUERY_OPTIMIZATION_APPEND_TRANSFORMS_H_

#include <core/storage/query_engine/planning/optimizations/optimization_transforms.hpp>
#include <core/storage/query_engine/planning/optimization_engine.hpp>
#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/planning/optimization_node_info.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

#include <array>

namespace turi {
namespace query_eval {

class opt_append_transform : public opt_transform {
  bool transform_applies(planner_node_type t) {
    return (t == planner_node_type::APPEND_NODE);
  }
};

/**  Transform append(source, source) --> source
 */
class opt_append_on_source : public opt_append_transform {

  std::string description() { return "append(source, source) -> source"; }

  std::pair<bool, sframe> try_sframe_append(cnode_info_ptr n) {
    sframe new_sf;
    for (const auto& input: n->inputs) {
      if (input->type != planner_node_type::SFRAME_SOURCE_NODE) return {false, new_sf};
      auto begin = input->p("begin_index");
      auto end = input->p("end_index");

      const auto& sf = input->any_p<sframe>("sframe");

      if (begin == 0 && end == sf.size()) {
        // stupidly we need the names to match for the append to work...
        for (size_t i = 0; i < new_sf.num_columns(); ++i) {
          new_sf.set_column_name(i, sf.column_name(i));
        }

        if(begin != end)
          new_sf = new_sf.append(sf);

      } else {
        return {false, new_sf};
      }
    }

    if(new_sf.num_rows() == 0) {
      new_sf = n->inputs[0]->any_p<sframe>("sframe");
    }

    return {true, new_sf};
  }

  std::pair<bool, sarray<flexible_type> > try_sarray_append(cnode_info_ptr n) {
    sarray<flexible_type> new_sa;
    for (const auto& input: n->inputs) {
      if (input->type != planner_node_type::SARRAY_SOURCE_NODE) return {false, new_sa};
      auto begin = input->p("begin_index");
      auto end = input->p("end_index");

      auto sa_ptr = input->any_p<std::shared_ptr<sarray<flexible_type> > >("sarray");

      const auto& sa = *sa_ptr;

      if (begin == 0 && end == sa.size()) {
        if(begin != end)
          new_sa = new_sa.append(sa);
      } else {
        return {false, new_sa};
      }
    }

    if(new_sa.size() == 0) {
      new_sa = *(n->inputs[0]->any_p<std::shared_ptr<sarray<flexible_type> > >("sarray"));
    }

    return {true, new_sa};
  }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {
    // only source nodes accepted
    // and all have the same begin and end positions
    ASSERT_NE(n->inputs.size(), 0);

    // Quickly fail if not dealing with two sframe/sarray sources
    if(! ((n->inputs[0]->type == planner_node_type::SFRAME_SOURCE_NODE
           || n->inputs[0]->type == planner_node_type::SARRAY_SOURCE_NODE)
          &&
          (n->inputs[1]->type == planner_node_type::SFRAME_SOURCE_NODE
           || n->inputs[1]->type == planner_node_type::SARRAY_SOURCE_NODE))) {

      return false;
    }

    // Try append as sframe
    auto sframe_append_result = try_sframe_append(n);

    if (sframe_append_result.first) {
      auto& new_sf = sframe_append_result.second;
      // we can rewrite the current node.
      pnode_ptr new_pnode = op_sframe_source::make_planner_node(new_sf,
                                                                0,
                                                                new_sf.num_rows());
      opt_manager->replace_node(n, new_pnode);
      return true;
    }

    // Try append as sarray
    auto sarray_append_result = try_sarray_append(n);
    if (sarray_append_result.first) {
      auto& new_sa = sarray_append_result.second;
      // we can rewrite the current node.
      pnode_ptr new_pnode = op_sarray_source::make_planner_node(std::make_shared<sarray<flexible_type> >(new_sa),
                                                                0,
                                                                new_sa.size());
      opt_manager->replace_node(n, new_pnode);
      return true;
    }
    return false;
  }
};

/** Eliminated by optimization to prune off an append of an empty
 *  transform.
 */
class opt_eliminate_empty_append : public opt_append_transform {

  std::string description() { return "append(source, empty_source) -> source"; }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    if(n->inputs[1]->length() == 0) {
      opt_manager->replace_node(n, n->inputs[0]->pnode);
      return true;
    }

    if(n->inputs[0]->length() == 0) {
      opt_manager->replace_node(n, n->inputs[1]->pnode);
      return true;
    }

    return false;
  }
};


}}

#endif /* _APPEND_TRANSFORMS_H_ */
