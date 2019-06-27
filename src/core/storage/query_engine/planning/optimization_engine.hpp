/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_OPTIMIZATION_ENGINE_H_
#define TURI_SFRAME_QUERY_OPTIMIZATION_ENGINE_H_

#include <core/storage/query_engine/planning/planner_node.hpp>
#include <core/storage/query_engine/planning/materialize_options.hpp>
#include <core/storage/query_engine/planning/optimization_node_info.hpp>

namespace turi { namespace query_eval {

class opt_transform;

/** A registry of the different transforms in an indexed location.
 *  Built once at the beginning of the program.
 */
struct optimization_transform_registry {

  /** Called first to set the number of stages.
   */
  void set_num_stages(size_t n);

  /** Called by the populate_transforms function in
   *  optimization_transforms.cpp.
   */
  void register_optimization(const std::vector<size_t>& valid_stages, std::shared_ptr<opt_transform> opt);

  /** The number of distinct optimization stages in the model.
   */
  size_t num_stages() const;

  /** Returns a vector of possible transforms for a given stage and
   *  node type.
   */
  inline const std::vector<std::shared_ptr<opt_transform> >& get_transforms(
      size_t stage, planner_node_type t) const;


 private:
  // nested as possible_transforms[stage][type][transform]
  std::vector<std::vector<std::vector<std::shared_ptr<opt_transform> > > > possible_transforms;
};


/**
 * \ingroup sframe_query_engine
 * \addtogroup planning Planning, Optimization and Execution
 * \{
 */

/**
 * The main engine to power the optimizations.
 *
 */
class optimization_engine {
 public:

  /**  The main function to optimize the graph.
   */
  static pnode_ptr optimize_planner_graph(pnode_ptr tip, const materialize_options& exec_params);

 private:

  /** Use should only be through the above optimize_planner_graph
   *  static function.
   */
  optimization_engine(std::shared_ptr<const optimization_transform_registry>);

  ~optimization_engine();

 public:

  ////////////////////////////////////////////////////////////////////////////////
  // Routines used by the transforms to manipulate a graph.

  // Can ensure a particular node goes back on the queue to processed.
  inline void mark_node_as_active(cnode_info_ptr n);

  // All graph manipulations go through here.  Things are marked as
  // active in several contexts.
  void replace_node(cnode_info_ptr old_node, pnode_ptr new_pnode);

 private:

  static constexpr size_t num_types() { return int(planner_node_type::INVALID); }

  std::shared_ptr<const optimization_transform_registry> transform_registry;

  /** Runs the optimization parameters.
   */
  pnode_ptr _run(pnode_ptr tip, const materialize_options& exec_params);

  /** Initialize and run a stage -- i.e. populate the active nodes,
   *  etc.
   *
   */
  void run_stage(size_t stage, node_info_ptr tip, const materialize_options& exec_params);

  /**  Goes through and populates the active node list.
   */
  void populate_active_nodes(cnode_info_ptr tip);

  /** In a given stage, only nodes with applicable types are added to
   *  the active queue.  This cuts down on processing time for each
   *  stage.
   */
  std::vector<int> stage_type_active_mask;

  // Build the active node queue out.
  void _build_active_node_queue(const cnode_info_ptr& tip);
  std::vector<cnode_info_ptr> active_nodes;

  void eliminate_node_and_prune(node_info_ptr n);

  node_info_ptr build_node_info(pnode_ptr);

  std::map<pnode_ptr, node_info_ptr> node_lookups;
  std::vector<node_info_ptr> all_nodes;

  void release_node(const node_info_ptr& ptr);

};

/// \}
//
}}

#endif /* _PLAN_OPTIMIZATIONS_H_ */
