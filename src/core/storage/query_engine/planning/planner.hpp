/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_ENGINE_PLANNER_HPP_
#define TURI_SFRAME_QUERY_ENGINE_PLANNER_HPP_

#include <vector>
#include <string>
#include <memory>

#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/query_engine/planning/materialize_options.hpp>
#include <core/storage/query_engine/planning/planner_node.hpp>

namespace turi {
namespace query_eval {

class query_planner;

/**
 * \ingroup sframe_query_engine
 * \addtogroup planning Planning, Optimization and Execution
 * \{
 */

/**
 * The main query plan call.
 */
class planner {
 public:
  typedef std::function<bool(size_t, const std::shared_ptr<sframe_rows>&)>
    write_callback_type;

  planner() {}

  /**
   * Materialize the output from a node on a graph as an SFrame.
   *
   * Note that exec_params allows some control over the execution of the
   * materialization.
   *
   *
   * This function is the tip of the materialization pipeline,
   * everything materialization operation should come through here, and the
   * objective here is to correctly handle all query plans.
   *
   * Internally, the materialization hierarchy is:
   *  - \ref planner::materialize Handles the most general materializations
   *  - \ref planner::execute_node Replicates a plan for parallelization.
   *                               A private function.
   *  - \ref subplan_executor Executes a restricted plan.
   */
  sframe materialize(std::shared_ptr<planner_node> tip,
                     materialize_options exec_params = materialize_options());

  /**
   * \overload
   * Convenience overload for a very common use case which is to just
   * materialize to a callback function.
   *
   * See the \ref materialize_options for details on what the arguments achieve.
   *
   * But most notably, if partial_materialize is false, the materialization
   * may fail. See \ref materialize_options for details.
   */
  void materialize(std::shared_ptr<planner_node> tip,
                   write_callback_type callback,
                   size_t num_segments,
                   materialize_options exec_params = materialize_options());


  /** If this returns true, it is recommended to go ahead and
   *  materialize the sframe operations on the fly to prevent memory
   *  issues.
   */
  bool online_materialization_recommended(std::shared_ptr<planner_node> tip);

  /**
   * Materialize the output, returning the result as a planner node.
   */
  std::shared_ptr<planner_node>  materialize_as_planner_node(
      std::shared_ptr<planner_node> tip,
      materialize_options exec_params = materialize_options());

  /**
   * Returns a new planner node which is a range slice of the input node.
   *
   * The operation may modify (materialize) input node.
   */
  std::shared_ptr<planner_node> slice(std::shared_ptr<planner_node>& tip, size_t begin, size_t end);

  /**
   * Try to test that both a and b have equal length and to materialize
   * them if necessary to prove that this is the case.
   */
  bool test_equal_length(std::shared_ptr<planner_node> a,
                         std::shared_ptr<planner_node> b);
};



/// \}

} // namespace query_eval
} // namespace turi

#endif /* TURI_SFRAME_QUERY_ENGINE_PLANNER_HPP_ */
