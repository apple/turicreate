/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_OPTIMIZATION_UNION_TRANSFORMS_H_
#define TURI_SFRAME_QUERY_OPTIMIZATION_UNION_TRANSFORMS_H_

#include <core/storage/query_engine/planning/optimizations/optimization_transforms.hpp>
#include <core/storage/query_engine/planning/optimization_engine.hpp>
#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/planning/optimization_node_info.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

#include <array>

namespace turi {
namespace query_eval {

// This class applies to the union

class opt_union_transform : public opt_transform {
  bool transform_applies(planner_node_type t) {
    return t == planner_node_type::UNION_NODE;
  }
};

class opt_union_merge : public opt_union_transform {

  std::string description() { return "union(a, union(b,c)) -> union(a,b,c)"; }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    DASSERT_TRUE(n->type == planner_node_type::UNION_NODE);

    if(! n->input_type_present(planner_node_type::UNION_NODE))
      return false;

    std::vector<pnode_ptr> inputs;

    std::function<void(const cnode_info_ptr&)> add;

    add = [&](const cnode_info_ptr& nn) {
      if(nn->type == planner_node_type::UNION_NODE) {
        for(const auto& nnn : nn->inputs) add(nnn);
      } else {
        inputs.push_back(nn->pnode);
      }
    };

    add(n);

    pnode_ptr new_pnode = op_union::make_planner_node(inputs);
    opt_manager->replace_node(n, new_pnode);

    return true;
  }
};

/**  Transform a union of two source nodes into a source node.
 *
 */
class opt_union_on_source : public opt_union_transform {

  std::string description() { return "union(source, source) -> source"; }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    DASSERT_TRUE(n->type == planner_node_type::UNION_NODE);

    // Do a quick check to see if this transformation is applicable.
    size_t num_sources_present = 0;
    for(size_t i = 0; i < n->inputs.size(); ++i) {
      auto t = n->inputs[i]->type;
      if(t == planner_node_type::SFRAME_SOURCE_NODE
         || t == planner_node_type::SARRAY_SOURCE_NODE) {
        ++num_sources_present;
      }
    }

    if(num_sources_present < 2)
      return false;

    ////////////////////////////////////////////////////////////////////////////////
    // Now, do a second pass to make sure that at least some of the
    // sources can be merged.  This isn't always the case.

    typedef std::array<size_t, 3> key_type;

    std::vector<key_type> input_keys(n->inputs.size());

    // Go though and extract the keys, and test for uniqueness.
    {
      std::set<key_type> distinct_input_ranges;

      for(size_t i = 0; i < n->inputs.size(); ++i) {
        auto t = n->inputs[i]->type;
        if(t == planner_node_type::SFRAME_SOURCE_NODE
           || t == planner_node_type::SARRAY_SOURCE_NODE) {

          size_t begin_index = n->inputs[i]->p("begin_index");
          size_t end_index   = n->inputs[i]->p("end_index");

          size_t size = 0;

          if(t == planner_node_type::SFRAME_SOURCE_NODE) {
            // Get the size of the sframe
            size = n->inputs[i]->any_p<sframe>("sframe").num_rows();
          } else {
            // Get the size of the sarray
            size = n->inputs[i]->any_p<std::shared_ptr<sarray<flexible_type> > >("sarray")->size();
          }

          // Store the key, since it's somewhat expensive to extract.
          input_keys[i] = key_type{{begin_index, end_index, size}};

          distinct_input_ranges.insert(input_keys[i]);
        }
      }

      if(distinct_input_ranges.size() == num_sources_present) {
        return false;
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Now, we know we'll end up doing something.   Let's go for it.

    struct merge_info {
      bool is_sarray = false;

      // Non sarray inputs get mapped to this.
      size_t input_index;
      size_t output_index_start;
      size_t output_index_end;

      // If it's an sarray, then this is the one.
      size_t begin_index = 0;
      size_t end_index = 0;

      // Pair is (original output index, sarray).
      std::vector<std::pair<size_t, std::shared_ptr<sarray<flexible_type> > > > sa_v;
    };

    std::vector<merge_info> map_info;
    std::map<key_type, size_t> merge_groups;

    size_t current_output_idx = 0;
    for(size_t i = 0; i < n->inputs.size(); ++i) {
      auto t = n->inputs[i]->type;
      if(t == planner_node_type::SFRAME_SOURCE_NODE
         || t == planner_node_type::SARRAY_SOURCE_NODE) {

        size_t begin_index = n->inputs[i]->p("begin_index");
        size_t end_index   = n->inputs[i]->p("end_index");

        const auto& key = input_keys[i];

        auto it = merge_groups.find(key);

        size_t map_idx;
        if(it == merge_groups.end()) {
          merge_groups[key] = map_idx = map_info.size();
          map_info.emplace_back();
          map_info.back().is_sarray = true;
          map_info.back().begin_index = begin_index;
          map_info.back().end_index   = end_index;
        } else {
          map_idx = it->second;
        }

        // Add this to the appropriate info struct

        if(t == planner_node_type::SFRAME_SOURCE_NODE) {

          sframe sf = n->inputs[i]->any_p<sframe>("sframe");

          for(size_t i = 0; i < sf.num_columns(); ++i) {
            map_info[map_idx].sa_v.push_back({current_output_idx, sf.select_column(i)});
            ++current_output_idx;
          }
        } else {
          map_info[map_idx].sa_v.push_back(
              {current_output_idx,
                    n->inputs[i]->any_p<std::shared_ptr<sarray<flexible_type> > >("sarray")});
          ++current_output_idx;
        }

      } else {
        map_info.emplace_back();
        map_info.back().is_sarray = false;
        map_info.back().input_index = i;
        map_info.back().output_index_start = current_output_idx;
        current_output_idx += n->inputs[i]->num_columns();
        map_info.back().output_index_end = current_output_idx;
      }
    }

    size_t num_outputs = current_output_idx;

    ////////////////////////////////////////////////////////////////////////////////
    // First, take care of the case where it's only one output.

    if(map_info.size() == 1) {
      const auto& m = map_info.front();
      DASSERT_TRUE(m.is_sarray);

      std::vector<std::shared_ptr<sarray<flexible_type> > > columns(m.sa_v.size());
      for(size_t i = 0; i < columns.size(); ++i) {
        columns[i] = m.sa_v[i].second;
        if(i + 1 < columns.size()) {
          DASSERT_EQ(m.sa_v[i].first + 1, m.sa_v[i+1].first);
        }
      }

      pnode_ptr rep = op_sframe_source::make_planner_node(sframe(columns), m.begin_index, m.end_index);
      opt_manager->replace_node(n, rep);
      return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Now, it's possible we have to actually follow this with a
    // projection operator to reorder the columns.  Thus keep track of
    // these output indices.

    std::vector<size_t> output_projection_indices(num_outputs, size_t(-1));

    std::vector<pnode_ptr> inputs;
    inputs.reserve(map_info.size());

    current_output_idx = 0;

    for(const auto& m : map_info) {
      if(m.is_sarray) {
        size_t n_columns = m.sa_v.size();

        std::vector<std::shared_ptr<sarray<flexible_type> > > columns(n_columns);

        for(size_t i = 0; i < n_columns; ++i) {
          output_projection_indices[m.sa_v[i].first] = current_output_idx;
          columns[i] = m.sa_v[i].second;
          ++current_output_idx;
        }

        inputs.push_back(op_sframe_source::make_planner_node(sframe(columns), m.begin_index, m.end_index));
      } else {

        inputs.push_back(n->inputs[m.input_index]->pnode);

        for(size_t i = m.output_index_start; i < m.output_index_end; ++i) {
          output_projection_indices[i] = current_output_idx;
          ++current_output_idx;
        }
      }
    }

#ifndef NDEBUG
    for(size_t i : output_projection_indices) {
      DASSERT_NE(i, size_t(-1));
      DASSERT_LT(i, num_outputs);
    }
#endif

    ////////////////////////////////////////////////////////////////////////////////
    // Construct the union node

    pnode_ptr new_pnode = op_union::make_planner_node(inputs);

    ////////////////////////////////////////////////////////////////////////////////
    // Do we need to follow it by a project node?

    bool all_consecutive = true;
    for(size_t i = 0; i + 1 < output_projection_indices.size(); ++i) {
      if(output_projection_indices[i] + 1 != output_projection_indices[i + 1]) {
        all_consecutive = false;
        break;
      }
    }

    if(!all_consecutive)
      new_pnode = op_project::make_planner_node(new_pnode, output_projection_indices);

    opt_manager->replace_node(n, new_pnode);
    return true;
  }
};

class opt_eliminate_singleton_union : public opt_union_transform {

  std::string description() { return "union(a) -> a"; }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    DASSERT_TRUE(n->type == planner_node_type::UNION_NODE);

    if(n->inputs.size() == 1) {
      opt_manager->replace_node(n, n->inputs[0]->pnode);
      return true;
    } else {
      return false;
    }
  }
};


/**
 * Merges a connected subtree of unions into a minimal set of unions.
 * For instance:
 *
 * U(P(A, [1,2]), U(P(A, [3,4]), B)) --> U(P(A, [1,2,3,4]), B)
 *
 * It does so by collapsing to a list of sources. Then rebuilds the list
 * with a minimal set of unions.
 */
class opt_union_project_merge : public opt_union_transform {

  std::string description() { return "union(project1(a), ..., project2(a)) -> union(project3(a...), ...)"; }

  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    // Must be at least 2 projects here for this to apply.
    if(! n->input_type_present(planner_node_type::PROJECT_NODE, 2))
      return false;

    {
      // Do a quick check to see if any inputs can be merged.
      std::set<cnode_info_ptr> project_inputs;
      bool mergable_projection_present = false;
      for(const auto& nn : n->inputs) {
        if(nn->type == planner_node_type::PROJECT_NODE) {
          if(project_inputs.count(nn->inputs[0])) {
            mergable_projection_present = true;
            break;
          } else {
            project_inputs.insert(nn->inputs[0]);
          }
        }
      }

      if(!mergable_projection_present)
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Now, go through and build out the projection points

    struct input_info {
      // If a projection, this is the node it's projecting from.  If
      // not, it's just that node.
      cnode_info_ptr in;

      // If projection
      std::vector<size_t> indices;
      std::vector<size_t> output_indices;
    };

    std::vector<input_info> input_v;
    input_v.reserve(n->inputs.size());

    // Collect a list of the project nodes that can be merged.
    std::map<cnode_info_ptr, size_t> mergable_inputs;

    size_t current_output_index_start = 0;
    for(const cnode_info_ptr& nn : n->inputs) {

      size_t current_output_index_end = current_output_index_start + nn->num_columns();

      if(nn->type == planner_node_type::PROJECT_NODE) {
        auto it = mergable_inputs.find(nn->inputs[0]);

        if(it != mergable_inputs.end()) {
          auto& ii = input_v[it->second];
          DASSERT_TRUE(nn->inputs[0] == ii.in);

          const auto& p_iv = nn->p("indices").get<flex_list>();
          ii.indices.insert(ii.indices.end(), p_iv.begin(), p_iv.end());

          for(size_t out_idx = current_output_index_start; out_idx < current_output_index_end; ++out_idx) {
            ii.output_indices.push_back(out_idx);
          }
        } else {
          input_info ii;
          ii.in = nn->inputs[0];

          const auto& p_iv = nn->p("indices").get<flex_list>();
          ii.indices.assign(p_iv.begin(), p_iv.end());

          ii.output_indices.resize(nn->num_columns());
          std::iota(ii.output_indices.begin(), ii.output_indices.end(), current_output_index_start);

          mergable_inputs[nn->inputs[0]] = input_v.size();
          input_v.push_back(std::move(ii));
        }
      } else {
        auto it = mergable_inputs.find(nn);

        if(it != mergable_inputs.end()) {
          auto& ii = input_v[it->second];
          DASSERT_TRUE(nn == ii.in);

          for(size_t i = 0; i < nn->num_columns(); ++i) {
            ii.indices.push_back(i);
            ii.output_indices.push_back(current_output_index_start + i);
          }
        } else {
          input_info ii;
          ii.in = nn;

          ii.indices.resize(nn->num_columns());
          std::iota(ii.indices.begin(), ii.indices.end(), 0);

          ii.output_indices.resize(nn->num_columns());
          std::iota(ii.output_indices.begin(), ii.output_indices.end(), current_output_index_start);

          mergable_inputs[nn] = input_v.size();
          input_v.push_back(std::move(ii));
        }
      }

      current_output_index_start = current_output_index_end;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Now that we have all of the info, remap all of the inputs

    std::vector<pnode_ptr> new_inputs;
    new_inputs.reserve(input_v.size());

    std::vector<size_t> final_projection(n->num_columns(), size_t(-1));

    size_t mapped_idx = 0;
    for(const auto& ii : input_v) {
      for(size_t out_idx : ii.output_indices) {
        DASSERT_EQ(final_projection[out_idx], size_t(-1));
        final_projection[out_idx] = mapped_idx;
        ++mapped_idx;
      }

      bool is_contiguous = true;
      if(ii.indices.size() == ii.in->num_columns()) {
        for(size_t i = 0; i + 1 < ii.indices.size(); ++i) {
          if(ii.indices[i] + 1 != ii.indices[i+1]) {
            is_contiguous = false;
            break;
          }
        }
      } else {
        is_contiguous = false;
      }

      if(is_contiguous) {
        new_inputs.push_back(ii.in->pnode);
      } else {
        new_inputs.push_back(op_project::make_planner_node(ii.in->pnode, ii.indices));
      }
    }

#ifndef NDEBUG
    {
      DASSERT_FALSE(new_inputs.empty());
      std::set<size_t> idx_set;
      for(size_t idx : final_projection) {
        DASSERT_NE(idx, size_t(-1));
        idx_set.insert(idx);
      }

      {
        // Do a quick check to see if any inputs can be merged.
        std::set<pnode_ptr> project_inputs;
        bool mergable_projection_present = false;
        for(const auto& nn : new_inputs) {
          if(nn->operator_type == planner_node_type::PROJECT_NODE) {
            if(project_inputs.count(nn->inputs[0])) {
              mergable_projection_present = true;
              break;
            } else {
              project_inputs.insert(nn->inputs[0]);
            }
          }
        }

        DASSERT_FALSE(mergable_projection_present);
      }
    }
#endif

    ////////////////////////////////////////////////////////////////////////////////
    // Now, it's time to dump it out.

    pnode_ptr in_node;
    if(new_inputs.size() == 1)
      in_node = new_inputs[0];
    else
      in_node = op_union::make_planner_node(new_inputs);

    opt_manager->replace_node(n, op_project::make_planner_node(in_node, final_projection));
    return true;
  }
};

}}

#endif /* _PROJECTION_TRANSFORMS_H_ */
