/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/parallel/lambda_omp.hpp>
#include <core/storage/query_engine/execution/subplan_executor.hpp>
#include <core/storage/query_engine/execution/execution_node.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>

namespace turi { namespace query_eval {

////////////////////////////////////////////////////////////////////////////////

static std::shared_ptr<execution_node> get_executor(
    const std::shared_ptr<planner_node>& p,
    std::map<std::shared_ptr<planner_node>,
             std::shared_ptr<execution_node> >& memo) {
  // See if things are cached; if so, just return that.
  if(memo.count(p)) return memo[p];

  // First, get all the inputs
  std::vector<std::shared_ptr<execution_node> > inputs(p->inputs.size());

  for(size_t i = 0; i < p->inputs.size(); ++i) {
    inputs[i] = get_executor(p->inputs[i], memo);
  }
  // Make the operator.
  std::shared_ptr<query_operator> op = planner_node_to_operator(p);
  memo[p] = std::make_shared<execution_node>(op, inputs);
  return memo[p];
}

// Returns an output sframe which can hold the generated output of the
// planner node. The output sframe has been opened for write and must be
// written to and closed before it can be read.
static sframe get_output_sframe_schema(
    const std::shared_ptr<planner_node>& pnode,
    size_t nsegments = 1,
    std::string target_index_file_location = "",
    std::vector<std::string> column_names = std::vector<std::string>()) {
  sframe out;
  // get schema
  auto column_types = infer_planner_node_type(pnode);
  if (column_names.size() == 0) {
    for (size_t i = 0;i < column_types.size(); ++i) {
      column_names.push_back(std::string("X") + std::to_string(i + 1));
    }
  }
  ASSERT_EQ(column_names.size(), column_types.size());
  out.open_for_write(column_names, column_types, target_index_file_location, nsegments);
  return out;
}


/**
 * Finds the earliest exception that occured up the execution node graph.
 * (strictly speaking this does not necessarily find the earliest exception.
 *  This find the earliest, left-most exception. if simultanous exception
 *  occurs in multiple locations, it will only find the left-most exception)
 */
static std::exception_ptr
find_earliest_exception(std::shared_ptr<execution_node> tip,
                        std::set<std::shared_ptr<execution_node>>& visited) {
  if (visited.count(tip)) return std::exception_ptr();
  std::exception_ptr ret;
  for (size_t i = 0;i < tip->num_inputs(); ++i) {
    auto input = tip->get_input_node(i);
    auto input_exception = find_earliest_exception(input, visited);
    if (input_exception != nullptr) {
      ret = input_exception;
      break;
    }
  }
  visited.insert(tip);
  if (ret == nullptr && tip->exception_occurred()) {
    return tip->get_exception();
  } else {
    return ret;
  }
}

void subplan_executor::generate_to_callback_function(
    const std::shared_ptr<planner_node>& plan,
    size_t output_segment_id,
    execution_callback out_function) {

  std::map<std::shared_ptr<planner_node>, std::shared_ptr<execution_node> > memo;
  std::shared_ptr<execution_node> ex_op = get_executor(plan, memo);

  size_t consumer_id = ex_op->register_consumer();

  while(1) {
    auto rows = ex_op->get_next(consumer_id);
    if (rows == nullptr)
      break;

    bool done = out_function(output_segment_id, rows);
    if(done)
      break;
  }

  // look through the list of all nodes for exceptions
  bool has_exception = false;
  for(auto& nodes: memo) {
    if (nodes.second->exception_occurred()) {
      has_exception = true;
    }
  }
      // find the earliest exception which occured.
  if (has_exception) {
    std::set<std::shared_ptr<execution_node>> memo;
    auto earliest_exception = find_earliest_exception(ex_op, memo);
    std::rethrow_exception(earliest_exception);
  }
}

void subplan_executor::generate_to_sframe_segment(const std::shared_ptr<planner_node>& plan,
                                          sframe& out,
                                          size_t output_segment_id) {

  auto outiter = out.get_output_iterator(output_segment_id);

  generate_to_callback_function(
      plan, output_segment_id,
      [&](size_t segment_idx, const std::shared_ptr<sframe_rows>& rows) {
        (*outiter) = *rows;
        return false;
      });
}


sframe subplan_executor::run(const std::shared_ptr<planner_node>& pnode,
                             const materialize_options& exec_params) {

  if(exec_params.write_callback != nullptr) {
    generate_to_callback_function(pnode, 0, exec_params.write_callback);

    sframe ret;
    return ret;
  } else {
    sframe out = get_output_sframe_schema(pnode,
                                          1, // just 1 segment will do
                                          exec_params.output_index_file);
    generate_to_sframe_segment(pnode, out, 0);
    out.close();
    return out;
  }
}

std::vector<sframe> subplan_executor::run(
    const std::vector<std::shared_ptr<planner_node>>& stuff_to_run_in_parallel,
    const materialize_options& exec_params) {

  std::vector<sframe> ret(stuff_to_run_in_parallel.size());

  parallel_for(0, stuff_to_run_in_parallel.size(), [&](const size_t i) {
      ret[i] = run(stuff_to_run_in_parallel[i], exec_params);
  });

  return ret;
}

sframe subplan_executor::run_concat(
    const std::vector<std::shared_ptr<planner_node> >& stuff_to_run_in_parallel,
    const materialize_options& exec_params) {

  if (stuff_to_run_in_parallel.empty()) {
    // make an empty sframe and return
    sframe ret;
    return ret;
  }

  if(exec_params.write_callback != nullptr) {
    execution_callback exec_f = exec_params.write_callback;

    parallel_for(0, stuff_to_run_in_parallel.size(), [&](size_t i) {
        generate_to_callback_function(stuff_to_run_in_parallel[i], i, exec_f);
      });

    // make an empty sframe and return
    sframe ret;
    return ret;
  } else {

    sframe ret = get_output_sframe_schema(stuff_to_run_in_parallel[0],
                                          stuff_to_run_in_parallel.size(),
                                          exec_params.output_index_file,
                                          exec_params.output_column_names);

    parallel_for(0, stuff_to_run_in_parallel.size(), [&](size_t i) {
        generate_to_sframe_segment(stuff_to_run_in_parallel[i], ret, i);
      });

    ret.close();
    return ret;
  }
}

}}
