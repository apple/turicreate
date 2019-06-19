/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/util/dot_graph_printer/dot_graph.hpp>
#include <core/storage/query_engine/execution/execution_node.hpp>
#include <core/storage/query_engine/planning/planner_node.hpp>
#include <core/storage/query_engine/execution/subplan_executor.hpp>
#include <core/storage/query_engine/operators/operator_transformations.hpp>
#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/planning/planner.hpp>
#include <core/storage/query_engine/planning/optimization_engine.hpp>
#include <core/storage/query_engine/query_engine_lock.hpp>
#include <core/globals/globals.hpp>
#include <core/storage/sframe_data/sframe.hpp>

namespace turi { namespace query_eval {

size_t SFRAME_MAX_LAZY_NODE_SIZE = 10000;

REGISTER_GLOBAL(int64_t, SFRAME_MAX_LAZY_NODE_SIZE, true);


/**
 * Directly executes a linear query plan potentially parallelizing it if possible.
 * No fast path optimizations. You should use execute_node.
 */
static sframe execute_node_impl(pnode_ptr input_n, const materialize_options& exec_params) {
  // Either run directly, or split it up into a parallel section
  if(is_parallel_slicable(input_n) && (exec_params.num_segments != 0)) {
    size_t num_segments = exec_params.num_segments;

    std::vector<pnode_ptr> segments(num_segments);

    for(size_t segment_idx = 0; segment_idx < num_segments; ++segment_idx) {
      std::map<pnode_ptr, pnode_ptr> memo;
      segments[segment_idx] = make_segmented_graph(input_n, segment_idx, num_segments, memo);
    }

    return subplan_executor().run_concat(segments, exec_params);
  } else {
    return subplan_executor().run(input_n, exec_params);
  }
}


/**
 * Executes a query plan, potentially parallelizing it if possible.
 * Also implements fast paths in the event the input node is a source node.
 */
static sframe execute_node(pnode_ptr input_n, const materialize_options& exec_params) {
  // fast path for SFRAME_SOURCE. If I am not streaming into
  // a callback, I can just call save
  if (exec_params.write_callback == nullptr &&
      input_n->operator_type == planner_node_type::SFRAME_SOURCE_NODE) {
    auto sf = input_n->any_operator_parameters.at("sframe").as<sframe>();
    if (input_n->operator_parameters.at("begin_index") == 0 &&
        input_n->operator_parameters.at("end_index") == sf.num_rows()) {
      if (!exec_params.output_index_file.empty()) {
        if (!exec_params.output_column_names.empty()) {
          ASSERT_EQ(sf.num_columns(), exec_params.output_column_names.size());
        }
        for (size_t i = 0;i < sf.num_columns(); ++i) {
          sf.set_column_name(i, exec_params.output_column_names[i]);
        }
        sf.save(exec_params.output_index_file);
      }
      return sf;
    }
    // fast path for SARRAY_SOURCE . If I am not streaming into
    // a callback, I can just call save
  } else if (exec_params.write_callback == nullptr &&
             input_n->operator_type == planner_node_type::SARRAY_SOURCE_NODE) {
    const auto& sa = input_n->any_operator_parameters.at("sarray").as<std::shared_ptr<sarray<flexible_type>>>();
    if (input_n->operator_parameters.at("begin_index") == 0 &&
        input_n->operator_parameters.at("end_index") == sa->size()) {
      sframe sf({sa},{"X1"});
      if (!exec_params.output_index_file.empty()) {
        if (!exec_params.output_column_names.empty()) {
          ASSERT_EQ(1, exec_params.output_column_names.size());
        }
        sf.set_column_name(0, exec_params.output_column_names.at(0));
        sf.save(exec_params.output_index_file);
      }
      return sf;
    }
    // if last node is a generalized union project and some come direct from sources
    // we can take advantage that sarray columns are "moveable" and
    // just materialize the modified columns
  } else if (exec_params.write_callback == nullptr &&
             input_n->operator_type == planner_node_type::GENERALIZED_UNION_PROJECT_NODE) {
    if (input_n->any_operator_parameters.count("direct_source_mapping")) {
      // ok we have a list of direct sources we don't need to rematerialize in the
      // geneneralized union project.
      auto existing_columns = input_n->any_operator_parameters
          .at("direct_source_mapping")
          .as<std::map<size_t, std::shared_ptr<sarray<flexible_type>>>>();

      // if there are no existing columns, there is nothing to optimize
      if (!existing_columns.empty()) {
        auto ncolumns = infer_planner_node_num_output_columns(input_n);

        // the indices the columns to materialize. we will project this out
        // and materialize them
        std::vector<size_t> columns_to_materialize;
        // The final set of sframe columns;
        // we fill in what we know first from existing_columns
        std::vector<std::shared_ptr<sarray<flexible_type>>> resulting_sframe_columns(ncolumns);
        for (size_t i = 0;i < ncolumns; ++i) {
          auto iter = existing_columns.find(i);
          if (iter == existing_columns.end()) {
            // leave a gap in resulting_sframe_columns we will fill these in later
            columns_to_materialize.push_back(i);
          } else {
            resulting_sframe_columns[i] = iter->second;
          }
        }
        if (!columns_to_materialize.empty()) {
          // add a project to the end selecting just this set of columns
          // clear the column names if any. don't need them
          auto new_exec_params = exec_params;
          new_exec_params.output_column_names.clear();
          input_n = op_project::make_planner_node(input_n, columns_to_materialize);
          input_n = optimization_engine::optimize_planner_graph(input_n, new_exec_params);
          logstream(LOG_INFO) << "Materializing only column subset: " << input_n << std::endl;

          sframe new_columns = execute_node_impl(input_n, new_exec_params);
          // rebuild an sframe
          // fill in the gaps in resulting_sframe_columns
          // these are the columns we just materialized
          for (size_t i = 0; i < columns_to_materialize.size(); ++i) {
            resulting_sframe_columns[columns_to_materialize[i]] = new_columns.select_column(i);
          }
        }
        return sframe(resulting_sframe_columns,
                      exec_params.output_column_names);
      }
    }
  }
  return execute_node_impl(input_n, exec_params);
}
////////////////////////////////////////////////////////////////////////////////

/**
 * Materializes deeper nodes, leaving with just a single linearly executable
 * execution node.
 *
 * For instance:
 * Source  --> Transform  ------|
 *                              v
 * Source' --> Transform' ---> Reduce --> Transform
 *
 * Since, (Source --> Transform) and (Source' --> Transform') are linearly
 * executable segments, but Reduce is not, This will trigger materialization on
 * the append, leaving with just
 *
 * Source --> Transform.
 *
 * Since this is now completely linear, this will return.
 *
 * For the final round, ends up with a source node that can just be passed to
 * the executor to run. This node will be parallel slicable.
 */
static pnode_ptr partial_materialize_impl(pnode_ptr n,
                                          const materialize_options& exec_params,
                                          std::map<pnode_ptr, pnode_ptr>& memo
                                         ) {
  if (memo.count(n)) return memo[n];
  for(size_t i = 0; i < n->inputs.size(); ++i) {
    n->inputs[i] = partial_materialize_impl(n->inputs[i], exec_params, memo);
  }

  // If we are just a source node of some sort,
  if(n->inputs.empty()) {
    DASSERT_TRUE(is_source_node(n));
    memo[n] = n;
    return n;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // In some cases we just pass through things.

  // Make sure that the inputs are all parallel sliceable.

  if(!consumes_inputs_at_same_rates(n)) {
    // consumes inputs at different rates.
    // materialize all inputs into this node
    for(auto& i: n->inputs) {
      // logprogress_stream << "Partial Materializing: " << i << std::endl;
      auto optimized_i = optimization_engine::optimize_planner_graph(i, exec_params);
      (*i) = (*op_sframe_source::make_planner_node(execute_node(optimized_i, exec_params)));
    }
    // logprogress_stream << "Reduced Plan: " << n << std::endl;
  }

  if(is_linear_transform(n) || is_sublinear_transform(n)) {
    memo[n] = n;
    return n;
  }

  // logprogress_stream << "Partial Materializing: " << n << std::endl;
  // Otherwise, instantiate this node.
  auto optimized_n = optimization_engine::optimize_planner_graph(n, exec_params);
  (*n) = (*op_sframe_source::make_planner_node(execute_node(optimized_n, exec_params)));
  memo[n] = n;
  return memo[n];
}

pnode_ptr naive_partial_materialize(pnode_ptr n, const materialize_options& exec_params) {

  // Recursively call materialize on all parent nodes, replacing them
  // with source nodes in this graph.  If our node simply joins a
  // number of source nodes together, then go and execute each one.

  for(size_t i = 0; i < n->inputs.size(); ++i) {

    if(! (planner_node_type_to_attributes(n->inputs[i]->operator_type).attribute_bitfield
          & query_operator_attributes::SOURCE)) {

      // For now, ignore other possible downstream nodes attached to
      // this input.
      pnode_ptr p = naive_partial_materialize(n->inputs[i], exec_params);
      sframe sf = execute_node(p, exec_params);
      n->inputs[i] = op_sframe_source::make_planner_node(sf);
    }
  }
  return n;
}

static pnode_ptr partial_materialize(pnode_ptr ptip,
                                     const materialize_options& exec_params) {
  // Naive mode is for error checking.
  if(exec_params.naive_mode) {
    return naive_partial_materialize(ptip, exec_params);
  } else {
    std::map<pnode_ptr, pnode_ptr> memo;
    return partial_materialize_impl(ptip, exec_params, memo);
  }
}

sframe planner::materialize(pnode_ptr ptip,
                            materialize_options exec_params) {
  std::lock_guard<recursive_mutex> GLOBAL_LOCK(global_query_lock);
  if (exec_params.num_segments == 0) {
    exec_params.num_segments = thread::cpu_count();
  }
  auto original_ptip = ptip;
  // Optimize Query Plan
  if (!is_source_node(ptip)) {
    logstream(LOG_INFO) << "Materializing: " << ptip << std::endl;
  }
  if(!exec_params.disable_optimization) {
    ptip = optimization_engine::optimize_planner_graph(ptip, exec_params);
    if (!is_source_node(ptip)) {
      logstream(LOG_INFO) << "Optimized As: " << ptip << std::endl;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Execute stuff.
  pnode_ptr final_node = ptip;


  // Partially materialize the graph first
  // Only a subset of execution paramets matter to the partial materialization calls.
  if (exec_params.partial_materialize) {
    materialize_options recursive_exec_params = exec_params;
    recursive_exec_params.num_segments = thread::cpu_count();
    recursive_exec_params.output_index_file = ""; // no forced output location
    recursive_exec_params.write_callback = nullptr;
    final_node = partial_materialize(ptip, recursive_exec_params);
  }
  logstream(LOG_INFO) << "Reduced plan: " << final_node << std::endl;

  if (exec_params.write_callback == nullptr) {
    // no write callback
    // Rewrite the query node to be materialized source node
    auto ret_sf = execute_node(final_node, exec_params);
    (*original_ptip) = (*(op_sframe_source::make_planner_node(ret_sf)));
    return ret_sf;
  } else {
    // there is a callback. push it through to execute parameters.
    return execute_node(final_node, exec_params);
  }
}

void planner::materialize(std::shared_ptr<planner_node> tip,
                          write_callback_type callback,
                          size_t num_segments,
                          materialize_options args) {
  args.num_segments = num_segments;
  args.write_callback = callback;
  materialize(tip, args);
};

  /** If this returns true, it is recommended to go ahead and
   *  materialize the sframe operations on the fly to prevent memory
   *  issues.
   */
bool planner::online_materialization_recommended(std::shared_ptr<planner_node> tip) {
  size_t lazy_node_size = infer_planner_node_num_dependency_nodes(tip);
  return lazy_node_size >= SFRAME_MAX_LAZY_NODE_SIZE;
}

/**
 * Materialize the output, returning the result as a planner node.
 */
std::shared_ptr<planner_node>  planner::materialize_as_planner_node(
    std::shared_ptr<planner_node> tip, materialize_options exec_params) {

  sframe res = materialize(tip, exec_params);
  return op_sframe_source::make_planner_node(res);
}

/**
 * Materialize the output, returning the result as a planner node.
 */
std::shared_ptr<planner_node> planner::slice(
    std::shared_ptr<planner_node>& tip, size_t begin, size_t end) {
  std::map<pnode_ptr, pnode_ptr> memo;
  if (!is_linear_graph(tip)) {
    // Try partial materialize first
    materialize_options exec_params;
    tip = partial_materialize(tip, exec_params);
    if (!is_linear_graph(tip)) {
      tip = materialize_as_planner_node(tip);
    }
  }
  ASSERT_TRUE(is_linear_graph(tip));
  return make_sliced_graph(tip, begin, end, memo);
}


bool planner::test_equal_length(std::shared_ptr<planner_node> a,
                                std::shared_ptr<planner_node> b) {
  // Checking the size of index array is the same
  auto prove_equal = prove_equal_length(a, b);

  if (!prove_equal.first && infer_planner_node_length(b) == -1) {
    logstream(LOG_INFO) << "Unable to prove equi-length. Materializing RHS" << std::endl;
    materialize(b);
    prove_equal = prove_equal_length(a, b);
  }
  if (!prove_equal.first && infer_planner_node_length(a) == -1) {
    logstream(LOG_INFO) << "Still unable to prove equi-length. Materializing LHS" << std::endl;
    materialize(a);
    prove_equal = prove_equal_length(a, b);
  }
  DASSERT_TRUE(prove_equal.first);
  return prove_equal.second;
}
}}
