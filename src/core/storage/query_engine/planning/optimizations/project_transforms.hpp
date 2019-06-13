/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_OPTIMIZATION_PROJECTION_TRANSFORMS_H_
#define TURI_SFRAME_QUERY_OPTIMIZATION_PROJECTION_TRANSFORMS_H_

#include <core/storage/query_engine/planning/optimizations/optimization_transforms.hpp>
#include <core/storage/query_engine/planning/optimization_engine.hpp>
#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/planning/optimization_node_info.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

#include <array>

namespace turi {
namespace query_eval {

class opt_project_transform : public opt_transform {
  bool transform_applies(planner_node_type t) {
    return (t == planner_node_type::PROJECT_NODE);
  }
};

/**  Transform the projection on a source node to a source node.
 */
class opt_project_on_source : public opt_project_transform {

  std::string description() { return "project(source) -> source"; }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    if(n->inputs[0]->type != planner_node_type::SFRAME_SOURCE_NODE)
      return false;

    auto flex_indices = n->p("indices").get<flex_list>();

    sframe old_sf = n->inputs[0]->any_p<sframe>("sframe");

    // Only apply this if the projection does not expand the number of
    // columns present.
    if(flex_indices.size() > old_sf.num_columns())
      return false;

    std::vector<std::shared_ptr<sarray<flexible_type> > > columns;

    for(const auto& idx_f : flex_indices) {
      size_t idx = idx_f;
      DASSERT_LT(idx, old_sf.num_columns());
      columns.push_back(old_sf.select_column(idx));
    }

    DASSERT_TRUE(!columns.empty());

    size_t begin_index = n->inputs[0]->p("begin_index");
    size_t end_index = n->inputs[0]->p("end_index");

    pnode_ptr new_pnode = op_sframe_source::make_planner_node(sframe(columns), begin_index, end_index);

    opt_manager->replace_node(n, new_pnode);
    return true;
  }
};


/** Eliminate unneeded projections.
 */
class opt_eliminate_identity_project : public opt_project_transform {

  std::string description() { return "project(a, {0,1,...,num_columns(a)}) -> a"; }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    const auto& flex_indices = n->p("indices").get<flex_list>();

    size_t nc = n->inputs[0]->num_columns();

    if(flex_indices.size() != nc)
      return false;

    for(size_t idx = 0; idx < flex_indices.size(); ++idx) {
      if(flex_indices[idx].get<flex_int>() != flex_int(idx))
        return false;
    }

    opt_manager->replace_node(n, n->inputs[0]->pnode);
    return true;
  }
};


/** Merge subsequent projections.
 */
class opt_merge_projects : public opt_project_transform {

  std::string description() { return "project1(project2(a)) -> project3(a)"; }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    DASSERT_TRUE(n->type == planner_node_type::PROJECT_NODE);

    if(n->inputs[0]->type != planner_node_type::PROJECT_NODE)
      return false;

    const auto& iv_1 = n->inputs[0]->p("indices").get<flex_list>();
    const auto& iv_2 = n->p("indices").get<flex_list>();

    // Do this under three cases -- if both are expansions, or both
    // are contractions.  But ignore them if an expansion is
    // downstream of a contraction.

    bool first_is_contraction = (n->inputs[0]->inputs[0]->num_columns() > iv_1.size());
    bool second_is_expansion = (iv_2.size() > iv_1.size());

    if(first_is_contraction && second_is_expansion)
      return false;

    std::vector<size_t> iv_out;

    for(const auto& idx_f : iv_2) {
      size_t idx = idx_f;
      DASSERT_LT(idx, iv_1.size());
      iv_out.push_back((size_t)(iv_1[idx]));
    }

    pnode_ptr out = op_project::make_planner_node(n->inputs[0]->inputs[0]->pnode, iv_out);

    opt_manager->replace_node(n, out);

    return true;
  }
};

/** Allows a projection to propegate through an append.
 *
 */
class opt_project_append_exchange : public opt_project_transform {

  std::string description() { return "project(append(a,b)) -> append(project(a), project(b))"; }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    DASSERT_TRUE(n->type == planner_node_type::PROJECT_NODE);

    if(n->inputs[0]->type != planner_node_type::APPEND_NODE)
      return false;

    // Determine what kind of transformation we can do.  It's possible that we don't do any.
    const auto& iv = n->p("indices").get<flex_list>();

    // Propegate downstream only if it's not an expansion.
    if(iv.size() > n->inputs[0]->num_columns())
      return false;

    std::vector<size_t> iv_s(iv.begin(), iv.end());

    pnode_ptr out = op_append::make_planner_node(
        op_project::make_planner_node(n->inputs[0]->inputs[0]->pnode, iv_s),
        op_project::make_planner_node(n->inputs[0]->inputs[1]->pnode, iv_s));

    opt_manager->replace_node(n, out);

    return true;
  }

};


class opt_project_logical_filter_exchange : public opt_project_transform {

  std::string description() { return "project(logical_filter(a), mask) -> logical_filter(project(a), mask)"; }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    DASSERT_TRUE(n->type == planner_node_type::PROJECT_NODE);

    if(n->inputs[0]->type != planner_node_type::LOGICAL_FILTER_NODE)
      return false;

    const auto& iv = n->p("indices").get<flex_list>();

    if(iv.size() > n->inputs[0]->num_columns())
      return false;

    std::vector<size_t> iv_s(iv.begin(), iv.end());

    pnode_ptr out = op_logical_filter::make_planner_node(
        op_project::make_planner_node(n->inputs[0]->inputs[0]->pnode, iv_s),
        n->inputs[0]->inputs[1]->pnode);

    opt_manager->replace_node(n, out);

    return true;
  }

};

/**  Selectively pass a project through a union.
 *
 *   The goal of this operator is to pass a project through a union
 *   with the hope of pruning the tree before the union.  However,
 *   order must be preserved.  Therefore, this operator actually has a
 *   number of possible output transformations.
 *
 *   1.  One side is eliminated.  In this case, the union is dropped,
 *   and the projection simply is translated to the pre-union indices.
 *
 *   2.  The projection operator maintains the partitioning between
 *   the two union inputs.  In this case, it is replaced with
 *   a union of two projection nodes.
 *
 *   3.  The projection reduces the number of input columns to the
 *   union.  In this case, it is replaced with a union of two
 *   projections followed by a transposing projection operator.  This
 *   allows eliminations to propegate up the tree.
 *
 */
class opt_union_project_exchange : public opt_project_transform {

  std::string description() { return "partitionable_project(union(a,...)) ?->? union(project1(a), ...)"; }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    DASSERT_TRUE(n->type == planner_node_type::PROJECT_NODE);

    if(n->inputs[0]->type != planner_node_type::UNION_NODE)
      return false;

    cnode_info_ptr u_node = n->inputs[0];

    DASSERT_TRUE(!u_node->inputs.empty());

    // Determine what kind of transformation we can do.  It's possible
    // that we don't do any.
    const auto& out_iv = n->p("indices").get<flex_list>();

    std::vector<int> output_used(u_node->num_columns(), false);

    for(const auto& idx : out_iv) {
      DASSERT_LT(idx, output_used.size());
      output_used[idx.get<flex_int>()] = true;
    }

    // See if there are any that can get eliminated.  If not, we're done.
    if(std::all_of(output_used.begin(), output_used.end(), [](int t) { return t; } ) )
      return false;

    // Remap the indices
    std::vector<size_t> remapped_indices(u_node->num_columns(), size_t(-1));

    std::vector<std::vector<size_t> > projections_by_input(u_node->inputs.size());
    std::vector<bool> input_needs_projection(u_node->inputs.size(), false);

    ////////////////////////////////////////////////////////////////////////////////
    // Build the projection mappings

    size_t current_offset = 0;
    size_t current_input = 0;
    size_t current_input_idx = 0;
    for(size_t i = 0; i < u_node->num_columns(); ++i) {

      if(output_used[i]) {
        projections_by_input[current_input].push_back(current_input_idx);
        remapped_indices[i] = i - current_offset;
      } else {
        ++current_offset;
        input_needs_projection[current_input] = true;
      }

      ++current_input_idx;

      if(current_input_idx == u_node->inputs[current_input]->num_columns()) {
        ++current_input;
        current_input_idx = 0;
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Remap the current inputs. Maybe they need projections, and
    // maybe they don't.

    std::vector<pnode_ptr> inputs(u_node->inputs.size());

    for(size_t i = 0; i < u_node->inputs.size(); ++i) {
      if(projections_by_input[i].empty()) {
        inputs[i].reset();
      } else if(input_needs_projection[i]) {
        inputs[i] = op_project::make_planner_node(u_node->inputs[i]->pnode, projections_by_input[i]);
      } else {
        inputs[i] = u_node->inputs[i]->pnode;
      }
    }

    // Clear out the empty ones.
    auto new_end = std::remove_if(inputs.begin(), inputs.end(),
                                  [](const pnode_ptr& p) { return p == nullptr; });
    inputs.resize(new_end - inputs.begin());

    ////////////////////////////////////////////////////////////////////////////////
    // Rebuilding the new projection

    std::vector<size_t> new_projection_indices(out_iv.size());

    for(size_t i = 0; i < out_iv.size(); ++i) {
      DASSERT_LT(out_iv[i], remapped_indices.size());
      DASSERT_NE(remapped_indices[out_iv[i]], size_t(-1));

      new_projection_indices[i] = remapped_indices[out_iv[i]];
    }

    // If the union isn't needed, don't use it.
    if(inputs.size() == 1) {
      opt_manager->replace_node(
          n, op_project::make_planner_node(inputs[0], new_projection_indices));
    } else {
      // It's done, folks!
      opt_manager->replace_node(
          n, op_project::make_planner_node(
              op_union::make_planner_node(inputs), new_projection_indices));
    }

    return true;
  }
};

/**  If a projection node results in more columns, but it is only
 *  choosing a subset of the columns, split it in to two.  The
 *  optimizations are designed so that expansive projections move
 *  downstream, and contractive expansions move upstream.
 */
class opt_split_contractive_expansive_projection : public opt_project_transform {

  std::string description() {
    return "project(a, ...) ?->? expanding_project(contracting_project(a, ...), ...)";
  }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    DASSERT_TRUE(n->type == planner_node_type::PROJECT_NODE);

    // Determine what kind of transformation we can do.  It's possible
    // that we don't do any.
    const auto& iv = n->p("indices").get<flex_list>();

    std::set<size_t> used_indices(iv.begin(), iv.end());

    size_t n_cols = n->inputs[0]->num_columns();

    if( ! (used_indices.size() < n_cols && iv.size() >= n_cols) )
      return false;

    std::vector<size_t> indices_1;
    indices_1.reserve(used_indices.size());
    std::vector<size_t> indices_2(iv.size());

    std::map<size_t, size_t> idx_map;

    for(size_t i = 0; i < iv.size(); ++i) {
      size_t idx = iv[i];

      auto it = idx_map.lower_bound(idx);

      if(it != idx_map.end() && it->first == idx) {
        indices_2[i] = it->second;
      } else {
        size_t new_idx = indices_1.size();
        idx_map.insert(it, {idx, new_idx});
        indices_1.push_back(idx);
        indices_2[i] = new_idx;
      }
    }

    pnode_ptr p_1 = op_project::make_planner_node(n->inputs[0]->pnode, indices_1);
    pnode_ptr p_2 = op_project::make_planner_node(p_1, indices_2);

    opt_manager->replace_node(n, p_2);
    return true;
  }
};

}}

#endif /* _PROJECTION_TRANSFORMS_H_ */
