/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_OPTIMIZATION_LOGICAL_FILTER_TRANSFORMS_H_
#define TURI_SFRAME_QUERY_OPTIMIZATION_LOGICAL_FILTER_TRANSFORMS_H_

#include <core/storage/query_engine/planning/optimizations/optimization_transforms.hpp>
#include <core/storage/query_engine/planning/optimization_engine.hpp>
#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/planning/optimization_node_info.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

#include <array>

namespace turi {
namespace query_eval {

class opt_logical_filter_transform : public opt_transform {
  bool transform_applies(planner_node_type t) {
    return (t == planner_node_type::LOGICAL_FILTER_NODE);
  }
};

class opt_logical_filter_linear_transform_exchange
    : public opt_logical_filter_transform {

  std::string description() {
    return "logical_filter(linear_transform(a), b) -> linear_transform(logical_filter(a, b))";
  }

  // Should include union nodes as well.
  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {
    DASSERT_TRUE(n->type == planner_node_type::LOGICAL_FILTER_NODE);

    // Activated only if it's a linear transform, but not a project
    // node, and only if there is one output.

    if(!n->inputs[0]->is_linear_transform()
       || n->inputs[0]->outputs.size() > 1
       || n->inputs[0]->type == planner_node_type::PROJECT_NODE)
      return false;

    pnode_ptr ret = n->inputs[0]->pnode->clone();
    ret->inputs.resize(n->inputs[0]->pnode->inputs.size());

    pnode_ptr logical_filter_mask = n->inputs[1]->pnode;

    for(size_t i = 0; i < n->inputs[0]->pnode->inputs.size(); ++i) {
      ret->inputs[i] = op_logical_filter::make_planner_node(n->inputs[0]->pnode->inputs[i], logical_filter_mask);
    }

    opt_manager->replace_node(n, ret);
    return true;
  }
};

class opt_logical_filter_expanding_project_exchange
    : public opt_logical_filter_transform {

  std::string description() {
    return "logical_filter(expanding_project(a), b) -> expanding_project(logical_filter(a, b))";
  }

  // Move expanding projects through the logical filter.
  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {
    DASSERT_TRUE(n->type == planner_node_type::LOGICAL_FILTER_NODE);

    // Activated only if it's a linear transform, but not a project
    // node, and only if there is one output.

    if(n->inputs[0]->type != planner_node_type::PROJECT_NODE)
      return false;

    // Determine what kind of transformation we can do.
    const auto& iv = n->inputs[0]->p("indices").get<flex_list>();

    // If it doesn't expand things, then don't do an exchange.
    if(iv.size() <= n->inputs[0]->inputs[0]->num_columns())
      return false;

    pnode_ptr new_fltr = op_logical_filter::make_planner_node(n->inputs[0]->inputs[0]->pnode, n->inputs[1]->pnode);

    pnode_ptr new_proj = n->inputs[0]->pnode->clone();
    new_proj->inputs = {new_fltr};

    opt_manager->replace_node(n, new_proj);
    return true;
  }
};

class opt_merge_identical_logical_filters
    : public opt_logical_filter_transform {

  std::string description() {
    return "logical_filter(b, a), ..., logical_filter(c, a) -> logical_filter(..., a)";
  }

  // Should include union nodes as well.
  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {
    DASSERT_TRUE(n->type == planner_node_type::LOGICAL_FILTER_NODE);

    // Activated only if it's a linear transform, but not a project
    // node, and only if there is one output.

    size_t n_logical_filter_outs = 0;
    for(const cnode_info_ptr& nn : n->inputs[1]->outputs) {
      if(nn->type == planner_node_type::LOGICAL_FILTER_NODE
         && nn->inputs[1] == n->inputs[1]) {

        ++n_logical_filter_outs;
      }
    }

    if(n_logical_filter_outs <= 1)
      return false;

    std::vector<pnode_ptr> union_inputs(n_logical_filter_outs);
    std::vector<std::vector<size_t> > projection_outputs(n_logical_filter_outs);
    std::vector<cnode_info_ptr> rep_nodes(n_logical_filter_outs);

    size_t idx = 0;
    size_t out_idx = 0;
    for(const cnode_info_ptr& nn : n->inputs[1]->outputs) {
      if(nn->type == planner_node_type::LOGICAL_FILTER_NODE
         && nn->inputs[1] == n->inputs[1]) {

        union_inputs[idx] = nn->inputs[0]->pnode;

        projection_outputs[idx].resize(nn->num_columns());
        std::iota(projection_outputs[idx].begin(), projection_outputs[idx].end(), out_idx);
        out_idx += nn->num_columns();

        rep_nodes[idx] = nn;

        ++idx;
      }
    }

    // Should always be true, as there can't really be duplicates here
#ifndef NDEBUG
    {
      size_t n_unique = std::set<cnode_info_ptr>(rep_nodes.begin(), rep_nodes.end()).size();
      DASSERT_EQ(n_unique, n_logical_filter_outs);
    }
#endif

    pnode_ptr main_in = op_union::make_planner_node(union_inputs);
    pnode_ptr filter = op_logical_filter::make_planner_node(main_in, n->inputs[1]->pnode);

    for(size_t i = 0; i < n_logical_filter_outs; ++i) {
      pnode_ptr new_pr_out = op_project::make_planner_node(filter, projection_outputs[i]);
      opt_manager->replace_node(rep_nodes[i], new_pr_out);
    }

    return true;
  }
};


}}
#endif
