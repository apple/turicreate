/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/query_engine/planning/optimization_engine.hpp>
#include <core/storage/query_engine/planning/optimizations/union_transforms.hpp>
#include <core/storage/query_engine/planning/optimizations/project_transforms.hpp>
#include <core/storage/query_engine/planning/optimizations/append_transforms.hpp>
#include <core/storage/query_engine/planning/optimizations/logical_filter_transforms.hpp>
#include <core/storage/query_engine/planning/optimizations/general_union_project_transforms.hpp>
#include <core/storage/query_engine/planning/optimizations/source_transforms.hpp>

namespace turi {
namespace query_eval {

////////////////////////////////////////////////////////////////////////////////

/** The query optimizer has a number of different stages.  Each stage
 *  is run until no optimizations apply to any node.
 *
 *  Which stages are run are determined by get_stages_to_run();
 *
 *  Stage 0: Preparation optimizations for the later stages.
 *  E.g. merging unions.
 *
 *  Stage 1: Cheaper optimizations that are run frequently, when
 *  building a graph.
 *
 *  Stage 2: More expensive optimizations, run only when materializing
 *  a graph.
 *
 *  Stage 3: Any "cleanup" optimizations.  The graph must be in a
 *  stage where materialization is valid.
 */


/** Deterime which stages are run, given exec_params.
 */
std::vector<size_t> get_stages_to_run(const materialize_options& exec_params) {
  if(exec_params.only_first_pass_optimizations) {
    return {0, 1, 4, 5, 6};
  } else {
    return {0, 1, 2, 3, 4, 5, 6};
  }
}

void populate_transforms(optimization_transform_registry *otr) {

  // The overall number of stages to set.
  otr->set_num_stages(7);

  ////////////////////////////////////////////////////////////////////////////////
  // Expand out some types of optimized nodes that are not considered
  // in the optimization, and will be merged at the end

  otr->register_optimization({0}, std::make_shared<opt_expand_generalized_union_project>());

  ////////////////////////////////////////////////////////////////////////////////
  // Non-invasive optimizations done at any point.  These don't really
  // change the structure of the graph and may be needed to clean up stuff in the

  otr->register_optimization({1, 2, 3, 4}, std::make_shared<opt_eliminate_identity_project>());

  ////////////////////////////////////////////////////////////////////////////////
  // Cheap and basic optimizations done at any stage.

  otr->register_optimization({1, 2, 3}, std::make_shared<opt_split_contractive_expansive_projection>());
  otr->register_optimization({1, 2, 3}, std::make_shared<opt_union_merge>());
  otr->register_optimization({1, 2, 3}, std::make_shared<opt_union_on_source>());
  otr->register_optimization({1, 2, 3}, std::make_shared<opt_project_on_source>());
  otr->register_optimization({1, 2, 3}, std::make_shared<opt_append_on_source>());
  otr->register_optimization({1, 2, 3}, std::make_shared<opt_merge_projects>());
  otr->register_optimization({1, 2, 3}, std::make_shared<opt_union_project_merge>());
  otr->register_optimization({1, 2, 3}, std::make_shared<opt_eliminate_empty_append>());
  otr->register_optimization({1, 2, 3}, std::make_shared<opt_union_project_exchange>());
  otr->register_optimization({1, 2, 3}, std::make_shared<opt_project_append_exchange>());
  otr->register_optimization({1, 2, 3}, std::make_shared<opt_eliminate_singleton_union>());

  ////////////////////////////////////////////////////////////////////////////////
  // Optimizations that are allowed to turn the graph into a state
  // which cannot be materialized.

  otr->register_optimization({2}, std::make_shared<opt_project_logical_filter_exchange>());
  otr->register_optimization({2}, std::make_shared<opt_logical_filter_linear_transform_exchange>());

  // Better logical filter exchanges
  otr->register_optimization({2, 3}, std::make_shared<opt_logical_filter_expanding_project_exchange>());

  ////////////////////////////////////////////////////////////////////////////////
  // Now, make the logical filters in common.  All the optimizations
  // that can't split the logical filters are allowed here.

  otr->register_optimization({3}, std::make_shared<opt_merge_identical_logical_filters>());

  ////////////////////////////////////////////////////////////////////////////////
  // Cleanup part 1: merge all the same sources into common nodes.

  otr->register_optimization({4}, std::make_shared<opt_merge_all_same_sarrays>());

  ////////////////////////////////////////////////////////////////////////////////
  // Any optimizations needed to clean up the graph to make it
  // materializable.


  otr->register_optimization({5}, std::make_shared<opt_union_to_generalized_union_project>());
  otr->register_optimization({5}, std::make_shared<opt_project_to_generalized_union_project>());
  otr->register_optimization({5}, std::make_shared<opt_merge_generalized_union_projects>());
  // TODO: Merge all logical_filter transforms that have identical masks.

  ////////////////////////////////////////////////////////////////////////////////
  // Adding metadata to nodes

  otr->register_optimization({6}, std::make_shared<opt_project_add_direct_source_tags>());

}

}}
