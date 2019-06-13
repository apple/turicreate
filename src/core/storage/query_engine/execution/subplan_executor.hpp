/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_ENGINE_SUBPLAN_EXECUTOR_H_
#define TURI_SFRAME_QUERY_ENGINE_SUBPLAN_EXECUTOR_H_

#include <vector>
#include <memory>
#include <functional>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/query_engine/planning/materialize_options.hpp>

namespace turi { namespace query_eval {

typedef std::function<bool(size_t, const std::shared_ptr<sframe_rows>&)> execution_callback;

struct planner_node;


/**
 * \ingroup sframe_query_engine
 * \addtogroup execution Execution
 * \{
 */

/**
 * The subplan executor executes a restricted class of constant rate query
 * plans.
 *
 * The subplan executor is the last stage of a hierarchy of query
 * executors.
 *
 * The hierarchy is:
 *  - \ref planner::materialize Handles the most general materializations
 *  - \ref planner::partial_materialize Handles the most general materializations
 *                                      but performs all materializations except
 *                                      for the last stage. A private function.
 *  - \ref planner::execute_node Replicates a plan for parallelization.
 *                               A private function.
 *  - \ref subplan_executor Executes a restricted plan.
 *
 * As described in \ref execution_node, to successfully execute a query plan
 * requires certain rate control constraints to be true: i.e. all nodes
 * must read/write data at exactly the same rate.
 *
 * This executor assumes that the query plan to execute is exactly restricted
 * to that. It simply sets up the pipeline of \ref execution_node objects
 * and materializes the results.
 */
class subplan_executor {
 public:

 /**
  * Runs a single job sequentially returning the resultant SFrame.
  *
  * Note that materialize_options may be used to adapt the materialization
  * process.
  */
  sframe run(const std::shared_ptr<planner_node>& run_this,
             const materialize_options& exec_params = materialize_options());

  /** Runs a batch of planner nodes in parallel, returning an SFrame for
   * each of them.
   *
   * Note that materialize_options may be used to adapt the materialization
   * process.
   */
  std::vector<sframe> run(
      const std::vector<std::shared_ptr<planner_node> >& stuff_to_run_in_parallel,
      const materialize_options& exec_params = materialize_options());


  /** Runs a batch of planner nodes in parallel, returning an SFrame comprising
   *  of the concatenation of the output of each of the planner nodes.
   *
   *  All the stuff_to_run_in_parallel must share exactly the same schema.
   *
   *  Note that materialize_options may be used to adapt the materialization
   *  process.
   */
  sframe run_concat(
      const std::vector<std::shared_ptr<planner_node> >& stuff_to_run_in_parallel,
      const materialize_options& exec_params = materialize_options());

 private:

 /**
  * \internal
  * Runs a single job sequentially to a single sframe segment.
  */
  void generate_to_sframe_segment(const std::shared_ptr<planner_node>& run_this,
                                  sframe& out,
                                  size_t output_segment_id);

  /**
   * \internal
   * Runs a single job sequentially, calling the callback on each output.
   */
  void generate_to_callback_function(
    const std::shared_ptr<planner_node>& plan,
    size_t output_segment_id,
    execution_callback out_f);
};

/// \}

}}

#endif /* _SUBPLAN_EXECUTOR_H_ */
