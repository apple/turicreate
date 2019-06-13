/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_OPTIMIZATION_GENERALIZED_UNION_PROJECT_TRANSFORMS_HPP_
#define TURI_SFRAME_QUERY_OPTIMIZATION_GENERALIZED_UNION_PROJECT_TRANSFORMS_HPP_

#include <core/storage/query_engine/planning/optimizations/optimization_transforms.hpp>
#include <core/storage/query_engine/planning/optimization_engine.hpp>
#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/planning/optimization_node_info.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

#include <array>

namespace turi {
namespace query_eval {

////////////////////////////////////////////////////////////////////////////////
// These things apply at the beginning

class opt_expand_generalized_union_project : public opt_transform {

  std::string description() { return "gen_union_proj(...) -> project(union(...), ...)"; }

  bool transform_applies(planner_node_type t) {
    return t == planner_node_type::GENERALIZED_UNION_PROJECT_NODE;
  }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    std::vector<size_t> idx_offsets(n->inputs.size());
    for(size_t i = 0, offset = 0; i < n->inputs.size(); offset += n->inputs[i]->num_columns(), ++i) {
      idx_offsets[i] = offset;
    }

    const flex_dict& input_index_maps = n->p("index_map").get<flex_dict>();

    std::vector<size_t> project_map(input_index_maps.size());

    for(size_t i= 0; i < input_index_maps.size(); ++i) {
      const auto& p = input_index_maps[i];

      project_map[i] = idx_offsets[size_t(p.first)] + size_t(p.second);
    }

    pnode_ptr u_node = op_union::make_planner_node(n->pnode->inputs);
    pnode_ptr p_node = op_project::make_planner_node(u_node, project_map);

    opt_manager->replace_node(n, p_node);
    return true;
  }
};

////////////////////////////////////////////////////////////////////////////////
// These things apply to the final stage

class opt_union_to_generalized_union_project : public opt_transform {

  std::string description() { return "union(...) -> gen_union_proj(...)"; }

  bool transform_applies(planner_node_type t) {
    return t == planner_node_type::UNION_NODE;
  }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    DASSERT_TRUE(n->type == planner_node_type::UNION_NODE);

    std::vector<std::pair<size_t, size_t> > index_map;

    for(size_t i = 0; i < n->inputs.size(); ++i) {
      for(size_t j = 0; j < n->inputs[i]->num_columns(); ++j) {
        index_map.push_back({i, j});
      }
    }

    pnode_ptr new_pnode = op_union_project::make_planner_node(n->pnode->inputs, index_map);
    opt_manager->replace_node(n, new_pnode);

    return true;
  }
};

class opt_project_to_generalized_union_project : public opt_transform {

  std::string description() { return "project(...) -> gen_union_proj(...)"; }

  bool transform_applies(planner_node_type t) {
    return t == planner_node_type::PROJECT_NODE;
  }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    DASSERT_TRUE(n->type == planner_node_type::PROJECT_NODE);

    const auto& iv = n->p("indices").get<flex_list>();

    std::vector<std::pair<size_t, size_t> > index_map(iv.size());

    for(size_t i = 0; i < iv.size(); ++i) {
      index_map[i] = {0, iv[i]};
    }

    pnode_ptr new_pnode = op_union_project::make_planner_node({n->inputs[0]->pnode}, index_map);
    opt_manager->replace_node(n, new_pnode);

    return true;
  }
};

class opt_merge_generalized_union_projects : public opt_transform {

  std::string description() { return "gen_union_proj(..., gen_union_proj(...), ...) -> gen_union_proj(...)"; }

  bool transform_applies(planner_node_type t) {
    return t == planner_node_type::GENERALIZED_UNION_PROJECT_NODE;
  }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    if(! n->input_type_present(planner_node_type::GENERALIZED_UNION_PROJECT_NODE))
      return false;

    std::vector<pnode_ptr> inputs;
    std::map<const node_info*, size_t> _input_loc_map;

    ////////////////////////////////////////////////////////////////////////////////
    // Bookkeeping functions

    auto get_input = [&](const cnode_info_ptr& nn) -> size_t {
      auto it = _input_loc_map.lower_bound(nn.get());

      if(it != _input_loc_map.end() && it->first == nn.get()) {
        return it->second;
      } else {
        size_t idx = _input_loc_map.size();

        inputs.push_back(nn->pnode);
        _input_loc_map.insert(it, {nn.get(), idx});
        return idx;
      }
    };

    ////////////////////////////////////////////////////////////////////////////////
    // Bookkeeping functions

    const flex_dict& input_index_maps = n->p("index_map").get<flex_dict>();

    std::vector<std::pair<size_t, size_t> > index_map(input_index_maps.begin(), input_index_maps.end());

    for(auto& idx_p : index_map) {
      size_t old_input_idx = idx_p.first;
      size_t old_input_col = idx_p.second;
      size_t new_input_idx;
      size_t new_input_col;

      DASSERT_LT(old_input_idx, n->inputs.size());
      DASSERT_LT(old_input_col, n->inputs[old_input_idx]->num_columns());

      cnode_info_ptr nn = n->inputs[old_input_idx];

      // Now, we get to the interesting part
      if(nn->type == planner_node_type::GENERALIZED_UNION_PROJECT_NODE) {
        const flex_dict& local_input_index_maps = nn->p("index_map").get<flex_dict>();

        const auto& sub_idx_p = local_input_index_maps[old_input_col];

        new_input_idx = get_input(nn->inputs[sub_idx_p.first]);
        new_input_col = sub_idx_p.second;
      } else {
        // Just stays more or less the same.
        new_input_idx = get_input(nn);
        new_input_col = old_input_col;
      }

      idx_p.first = new_input_idx;
      idx_p.second = new_input_col;
    }

    pnode_ptr new_pnode = op_union_project::make_planner_node(inputs, index_map);
    opt_manager->replace_node(n, new_pnode);

    return true;
  }
};

class opt_project_add_direct_source_tags : public opt_transform {

  std::string description() { return "add_source_metadata(gen_union_proj(...))"; }

  bool transform_applies(planner_node_type t) {
    return t == planner_node_type::GENERALIZED_UNION_PROJECT_NODE;
  }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    bool have_source = false;
    for(const auto& nn : n->inputs) {
      if(nn->type == planner_node_type::SFRAME_SOURCE_NODE
         || nn->type == planner_node_type::SARRAY_SOURCE_NODE) {
        have_source = true;
        break;
      }
    }
    if(!have_source)
      return false;

    if(n->has_any_p("direct_source_mapping"))
      return false;

    std::map<size_t, std::shared_ptr<sarray<flexible_type> > > input_mapping;

    const flex_dict& index_map = n->p("index_map").get<flex_dict>();

    for(size_t i = 0; i < index_map.size(); ++i) {
      size_t idx_1 = index_map[i].first;
      size_t idx_2 = index_map[i].second;

      auto nn = n->inputs[idx_1];

      if(nn->type == planner_node_type::SFRAME_SOURCE_NODE) {
        auto sa = nn->any_p<sframe>("sframe").select_column(idx_2);
        if (nn->p("begin_index") == 0 && nn->p("end_index") == sa->size()) {
          input_mapping[i] = sa;
        }
      } else if (nn->type == planner_node_type::SARRAY_SOURCE_NODE) {
        DASSERT_EQ(idx_2, 0);
        auto sa = nn->any_p<std::shared_ptr<sarray<flexible_type> > >("sarray");
        if (nn->p("begin_index") == 0 && nn->p("end_index") == sa->size()) {
          input_mapping[i] = sa;
        }
      }
    }

    auto new_pnode = pnode_ptr(new planner_node(*n->pnode));
    new_pnode->any_operator_parameters["direct_source_mapping"] = input_mapping;

    opt_manager->replace_node(n, new_pnode);

    return true;
  }
};

}}

#endif /* _PROJECTION_TRANSFORMS_H_ */
