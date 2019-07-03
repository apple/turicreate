/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_OPTIMIZATION_SOURCE_TRANSFORMS_H_
#define TURI_SFRAME_QUERY_OPTIMIZATION_SOURCE_TRANSFORMS_H_

#include <core/storage/query_engine/planning/optimizations/optimization_transforms.hpp>
#include <core/storage/query_engine/planning/optimization_engine.hpp>
#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/planning/optimization_node_info.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/data/flexible_type/flexible_type.hpp>

#include <array>

namespace turi {
namespace query_eval {

/**  This optimization scans the entire graph for duplicate sarrays,
 *   including inside of sframes, and then merges them, using projects
 *   to keep things consistent.
 *
 *   THis works by:
 *
 *   1.  If it's the tip node of the tree to be optimized, it goes
 *   through and makes a list of all source nodes. sframe sources are
 *   assumed to be a collection of sarray sources.
 *
 *   2. Identical sarrays are merged.  If they are part of sframes,
 *   the two sframes are merged together, with a project transform
 *   following them for each of the outputs.
 *
 *   3. Other identical source nodes (e.g. range nodes) are merged.
 *
 */
class opt_merge_all_same_sarrays : public opt_transform {

  std::string description() { return "source_a, ..., source_a -> source_a"; }

  // Only apply this to the node at the head of the graph
  bool transform_applies(planner_node_type t) {
    return (t == planner_node_type::IDENTITY_NODE);
  }

  void fill_source_sets(const cnode_info_ptr& n,
                        std::set<const node_info*>& seen, std::vector<cnode_info_ptr>& source_nodes) {

    auto it = seen.lower_bound(n.get());

    if(it != seen.end() && *it == n.get())
      return;
    else
      seen.insert(it, n.get());

    if(n->is_source_node())
      source_nodes.push_back(n);

    for(const auto& nn : n->inputs) {
      fill_source_sets(nn, seen, source_nodes);
    }
  }

  // Only apply this to the node at the head of the graph
  bool apply_transform(optimization_engine *opt_manager, cnode_info_ptr n) {

    // First, go through and get all the source_nodes present.
    std::vector<cnode_info_ptr> source_nodes;
    std::set<const node_info*> seen_nodes;
    fill_source_sets(n, seen_nodes, source_nodes);

    // What identifies a unique source.
    struct source_id {
      ptrdiff_t ptr_key;
      size_t begin_index;
      size_t end_index;

      bool operator<(const source_id& s) const {
        return (std::make_pair(ptr_key, std::make_pair(begin_index, end_index))
                < std::make_pair(s.ptr_key, std::make_pair(s.begin_index, s.end_index)));
      }
    };

    struct source_out {
      cnode_info_ptr src_node;
      size_t column_index;
    };

    std::map<source_id, std::vector<source_out> > all_sources;

    for(const cnode_info_ptr& sn : source_nodes) {

      switch(sn->type) {
        case planner_node_type::SFRAME_SOURCE_NODE: {
          size_t begin_index = sn->p("begin_index");
          size_t end_index = sn->p("end_index");
          sframe sf = sn->any_p<sframe>("sframe");

          for(size_t i = 0; i < sf.num_columns(); ++i) {
            source_id id;
            id.ptr_key = ptrdiff_t(sf.select_column(i).get());
            id.begin_index = begin_index;
            id.end_index = end_index;

            source_out out;
            out.src_node = sn;
            out.column_index = i;

            all_sources[id].push_back(out);
          }

          break;
        }

        case planner_node_type::SARRAY_SOURCE_NODE: {
          auto sa = sn->any_p<std::shared_ptr<sarray<flexible_type> > >("sarray");

          source_id id;
          id.ptr_key = ptrdiff_t(sa.get());
          id.begin_index = sn->p("begin_index");
          id.end_index = sn->p("end_index");

          source_out out;
          out.src_node = sn;
          out.column_index = 0;

          all_sources[id].push_back(out);

          break;
        }

        case planner_node_type::RANGE_NODE: {
          source_id id;
          // Use this as the key since any range nodes with the same
          // begin and end indices can be merged.
          id.ptr_key = int(planner_node_type::RANGE_NODE);
          id.begin_index = sn->p("start") + sn->p("begin_index");
          id.end_index = sn->p("start") + sn->p("end_index");

          source_out out;
          out.src_node = sn;
          out.column_index = 0;

          all_sources[id].push_back(out);

          break;
        }

        default: break;
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Merge all the similar nodes.
    bool change_occured = false;
    for(auto p : all_sources) {

      const source_id& id = p.first;
      const std::vector<source_out>& out_v = p.second;

      if(out_v.size() == 1) {
        continue;
      }

      // Based on what sources are present, we should go through and
      // see what's the best way to merge things.  This can be tricky
      // depending on the different cases.

      size_t sarray_count = 0;
      size_t sframe_count = 0;
      size_t range_node_count = 0;

      for(const source_out& out : out_v) {
        switch(out.src_node->type) {

          // Count sframe sources with one column as sarray sources.
          case planner_node_type::SFRAME_SOURCE_NODE: {
            if(out.src_node->num_columns() == 1)
              ++sarray_count;
            else
              ++sframe_count;
            break;
          }

          case planner_node_type::SARRAY_SOURCE_NODE:
            ++sarray_count;
            break;

          case planner_node_type::RANGE_NODE:
            ++range_node_count;
            break;

          default: DASSERT_TRUE(false); break;
        }
      }

      if(range_node_count > 0) {
        // All range nodes.
        DASSERT_EQ(sarray_count, 0);
        DASSERT_EQ(sframe_count, 0);

        for(size_t i = 1; i < out_v.size(); ++i) {
          opt_manager->replace_node(out_v[i].src_node, out_v[0].src_node->pnode);
          return true;
        }

      } else if(sframe_count == 0) {
        // All sarrays.
        DASSERT_EQ(sarray_count, out_v.size());

        for(size_t i = 1; i < out_v.size(); ++i) {
          opt_manager->replace_node(out_v[i].src_node, out_v[0].src_node->pnode);
        }
        change_occured = true;

      } else if(sframe_count == 1) {
        // One sframe, the rest sarrays.  Another easy case.

        size_t sframe_index = 0;

        for(size_t i = 1; i < out_v.size(); ++i) {
          if(out_v[i].src_node->type == planner_node_type::SFRAME_SOURCE_NODE
             && out_v[i].src_node->num_columns() > 1) {
            sframe_index = i;
            break;
          }
        }

        pnode_ptr out_project = op_project::make_planner_node(
            out_v[sframe_index].src_node->pnode, {out_v[sframe_index].column_index});

        for(size_t i = 0; i < out_v.size(); ++i) {
          if(i != sframe_index)
            opt_manager->replace_node(out_v[i].src_node, out_project);
        }

        change_occured = true;

      } else {
        // The hardest.  Merge all of these into one sframe, slap
        // projections on it afterwords, and then exit as we've
        // possibly invalidated the rest of the lookup tables since
        // they will possibly refer to the other components of the
        // replaced sframe.

        std::map<void*, size_t> idx_map;

        std::vector<std::vector<size_t> > projections(out_v.size());
        std::vector<std::shared_ptr<sarray<flexible_type> > > new_columns;

        auto get_index = [&](const std::shared_ptr<sarray<flexible_type> >& s) -> size_t {
          auto it = idx_map.lower_bound(s.get());
          if(it != idx_map.end() && it->first == s.get()) {
            return it->second;
          } else {
            size_t idx = idx_map.size();
            idx_map.insert(it, {s.get(), idx});
            new_columns.push_back(s);
            return idx;
          }
        };

        for(size_t i = 0; i < out_v.size(); ++i) {
          if(out_v[i].src_node->type == planner_node_type::SFRAME_SOURCE_NODE) {

            sframe sf = out_v[i].src_node->any_p<sframe>("sframe");
            projections[i].resize(sf.num_columns());

            for(size_t j = 0; j < sf.num_columns(); ++j) {
              projections[i][j] = get_index(sf.select_column(j));
            }

          } else if(out_v[i].src_node->type == planner_node_type::SARRAY_SOURCE_NODE) {
            auto sa = out_v[i].src_node->any_p<std::shared_ptr<sarray<flexible_type> > >("sarray");

            projections[i] = {get_index(sa)};

          } else {
            DASSERT_TRUE(false);
          }
        }

        // Now, make a new sframe
        pnode_ptr sf_src = op_sframe_source::make_planner_node(sframe(new_columns), id.begin_index, id.end_index);

        for(size_t i = 0; i < out_v.size(); ++i) {
          pnode_ptr rep_node = op_project::make_planner_node(sf_src, projections[i]);
          opt_manager->replace_node(out_v[i].src_node, rep_node);
        }

        // This is needed as the rest of the cache needs to be rebuilt.
        return true;
      }
    }

    return change_occured;
  }


};

}}

#endif
