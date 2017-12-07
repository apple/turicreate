/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// rpc and distributed
#include <rpc/dc.hpp>
#include <rpc/dc_dist_object.hpp>
#include <rpc/dc_global.hpp>
#include <distributed/distributed_context.hpp>
#include <unity/dml/dml_function_invocation.hpp>
#include <unity/dml/dml_function_wrapper.hpp>

// SGraph
#include <sframe/sframe.hpp>
#include <sgraph/sgraph.hpp>
#include <sgraph/sgraph_compute.hpp>
#include <sgraph/sgraph_fast_triple_apply.hpp>
#include <sgraph/hilbert_curve.hpp>
#include <table_printer/table_printer.hpp>
// Distributed Graph
#include <unity/dml/distributed_graph.hpp>
#include <unity/dml/distributed_graph_compute.hpp>

// Unity data structures
#include <unity/lib/gl_sgraph.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <unity/lib/simple_model.hpp>

// Parallel for
#include <parallel/pthread_tools.hpp>

// EIGTODO
#include <numerics/armadillo.hpp>

namespace turi {

/**************************************************************************/
/*                                                                        */
/*                         Worker Implementation                          */
/*                                                                        */
/**************************************************************************/

std::map<std::string, flexible_type> distributed_pagerank_worker_impl(variant_map_type args)  {

  /// User Input
  std::string graph_path = variant_get_value<flexible_type>(args["__path_of_graph"]);
  size_t max_iterations = variant_get_value<size_t>(args.at("max_iterations"));
  double reset_probability = variant_get_value<double>(args.at("reset_probability"));
  double threshold = variant_get_value<double>(args.at("threshold"));
  std::string output_path = variant_get_value<std::string>(args.at("output_path"));

  if (threshold < 0) {
    throw("Parameter 'threshold' must be positive.");
  } else if (reset_probability < 0 || reset_probability > 1) {
    throw("Parameter 'reset_probability' should be between 0 and 1.");
  } else if ((int)max_iterations <= 0) {
    throw("Max iterations should be positive.");
  }


  // global_logger().set_log_level(LOG_DEBUG);
  auto dc = distributed_control::get_instance();
  distributed_sgraph_compute::distributed_graph graph(graph_path, dc);

  dc->barrier();

  // typedefs
  typedef turi::atomic<size_t> atomic_integer_type;
  typedef turi::atomic<double> atomic_float_type;
  typedef std::vector<atomic_integer_type> atomic_int_vec;
  typedef std::vector<atomic_float_type> atomic_float_vec;

  auto degree_counts =
    distributed_sgraph_compute::create_partition_aligned_vertex_data<atomic_int_vec>(
      graph, [](size_t num_vertices) {
        atomic_int_vec ret(num_vertices, 0.0);
        ret.shrink_to_fit();
        return ret;
      });
  auto prev_pagerank =
    distributed_sgraph_compute::create_partition_aligned_vertex_data<atomic_float_vec>(
      graph, [](size_t num_vertices) {
        atomic_float_vec ret(num_vertices, 0.0);
        ret.shrink_to_fit();
        return ret;
      });

  auto cur_pagerank =
    distributed_sgraph_compute::create_partition_aligned_vertex_data<atomic_float_vec>(
      graph, [](size_t num_vertices) {
        atomic_float_vec ret(num_vertices, 1.0);
        ret.shrink_to_fit();
        return ret;
      });

  auto delta =
    distributed_sgraph_compute::create_partition_aligned_vertex_data<atomic_float_vec>(
      graph, [](size_t num_vertices) {
        atomic_float_vec ret(num_vertices, 0.0);
        ret.shrink_to_fit();
        return ret;
      });


  logprogress_stream << "Counting out degree" << std::endl;

  distributed_sgraph_compute::fast_triple_apply(
      graph,
      [&](sgraph_compute::fast_edge_scope& scope) {
        auto src_addr = scope.source_vertex_address();
        degree_counts[src_addr.partition_id][src_addr.local_id].inc();
      }
  );

  auto degree_combiner_fn = [](atomic_int_vec& a,
                               const atomic_int_vec& b)->void {
    for (size_t i = 0; i < a.size(); ++i) {
      a[i].value += b[i].value;
    }
  };
  distributed_sgraph_compute::combiner<atomic_int_vec, decltype(degree_combiner_fn)>
      degree_combiner(*dc, degree_combiner_fn);
  degree_combiner.perform_combine(graph, degree_counts, distributed_sgraph_compute::combiner_filter::SRC);

  logprogress_stream << "Done" << std::endl;

  sgraph_compute::fast_triple_apply_fn_type apply_fn =
      [&](sgraph_compute::fast_edge_scope& scope) {
        auto source_addr = scope.source_vertex_address();
        auto target_addr = scope.target_vertex_address();
        const auto& source_degree = degree_counts[source_addr.partition_id][source_addr.local_id].value;
        const auto& source_data = prev_pagerank[source_addr.partition_id][source_addr.local_id].value;
        auto& target_data = cur_pagerank[target_addr.partition_id][target_addr.local_id];
        target_data.inc(source_data / source_degree);
    };

  auto pagerank_combiner_fn =
      [](atomic_float_vec& a,
         const atomic_float_vec& b)->void {
        for (size_t i = 0; i < a.size(); ++i) {
          a[i].value += b[i].value;
        }
      };

  distributed_sgraph_compute::combiner<atomic_float_vec, decltype(pagerank_combiner_fn)>
      pagerank_combiner(*dc, pagerank_combiner_fn);

  table_printer table({{"Iteration", 0},
                       {"L1 change in pagerank", 0}});
  table.print_header();

  double sum = 0.0;
  double l1 = 0.0;
  size_t num_iter;

  timer mytimer;
  for (num_iter = 0; num_iter < max_iterations; ++num_iter) {
    sum = 0.0;
    l1 = 0.0;

    std::swap(prev_pagerank, cur_pagerank);
    parallel_for (0, cur_pagerank.size(), [&](size_t i) {
      for (auto& val : cur_pagerank[i]) val.value = 0.0;
    });

    distributed_sgraph_compute::fast_triple_apply(graph, apply_fn);
    pagerank_combiner.perform_combine(graph, cur_pagerank, distributed_sgraph_compute::combiner_filter::DST);

    parallel_for (0, cur_pagerank.size(), [&](size_t i) {
      for (auto& val : cur_pagerank[i]) {
        val.value = reset_probability + (1.0 - reset_probability) * val.value;
      }
    });

    // Computing  the total pagerank and l1 change
    atomic<double> atomic_sum = 0.0;
    atomic<double> atomic_l1 = 0.0;
    for(auto i : graph.my_master_vertex_partitions()) {
      auto& cur_vec = cur_pagerank[i];
      auto& prev_vec = prev_pagerank[i];
      auto& delta_vec = delta[i];
      DASSERT_EQ(cur_vec.size(), prev_vec.size());
      DASSERT_EQ(cur_vec.size(), delta_vec.size());

      parallel_for (0, cur_vec.size(), [&](size_t j) {
        auto diff = std::abs<double>(cur_vec[j] - prev_vec[j]);
        delta_vec[j] = diff;
        atomic_l1.inc(diff);
        atomic_sum.inc(cur_vec[j].value);
      });
    }
    sum = atomic_sum.value;
    l1 = atomic_l1.value;
    dc->all_reduce(sum);
    dc->all_reduce(l1);

    table.print_row(num_iter+1, l1);

    if (l1 < threshold) {
      break;
    }
  } // end of pagerank iterations
  table.print_footer();

  prev_pagerank.clear();

  // Add the pagerank and delta column to graph
  size_t num_partitions = graph.num_partitions();
  auto pagerank_sa = std::vector<std::shared_ptr<sarray<flexible_type>>>(num_partitions);
  auto delta_sa = std::vector<std::shared_ptr<sarray<flexible_type>>>(num_partitions);

  parallel_for(0, num_partitions, [&](size_t i) {
    {
      auto& sa = pagerank_sa[i];
      sa = std::make_shared<sarray<flexible_type>>();
      sa->open_for_write(1);
      sa->set_type(flex_type_enum::FLOAT);
      auto out = sa->get_output_iterator(0);
      for (const auto& v: cur_pagerank[i]) {
        *out++ = v.value;
      }
      sa->close();
    }
    {
      auto& sa = delta_sa[i];
      sa = std::make_shared<sarray<flexible_type>>();
      sa->open_for_write(1);
      sa->set_type(flex_type_enum::FLOAT);
      auto out = sa->get_output_iterator(0);
      for (const auto& v: delta[i]) {
        *out++ = v.value;
      }
      sa->close();
    }
  });

  graph.add_vertex_field(pagerank_sa, "pagerank", flex_type_enum::FLOAT);
  graph.add_vertex_field(delta_sa, "delta", flex_type_enum::FLOAT);

  logprogress_stream << "Saving graph..." << std::endl;
  // Save the graph distributedly
  graph.save_as_sgraph(output_path);
  logprogress_stream << "Done" << std::endl;

  std::map<std::string, flexible_type> return_stats;
  return_stats["l1"] = l1;
  return_stats["num_iter"] = num_iter;
  return_stats["sum"] = sum;
  return return_stats;
}


/**************************************************************************/
/*                                                                        */
/*                        Commander Implementation                        */
/*                                                                        */
/**************************************************************************/
variant_type distributed_pagerank_impl(variant_map_type args) {
  logprogress_stream << "Running distributed pagerank" << std::endl;
  timer mytimer;

  ASSERT_TRUE(args.count("__path_of_graph"));
  std::string path = variant_get_value<flexible_type>(args["__path_of_graph"]);
  // sgraph cannot be passed from commander to worker
  if (args.count("graph")) { args.erase(args.find("graph")); }

  unity_sgraph ug;
  ug.load_graph(path);
  sgraph sg = ug.get_graph();

  std::string output_path = "result_graph";
  if (args.count("__base_path__")) {
    output_path = variant_get_value<std::string>(args["__base_path__"]) + "/" + output_path;
  }
  args["output_path"] = output_path;

  // sgraph cannot be passed from commander to worker
  if (args.find("graph") != args.end()) {
    args.erase(args.find("graph"));
  }

  auto& ctx = turi::get_distributed_context();
  auto worker_ret = ctx.distributed_call(distributed_pagerank_worker_impl, args)[0];
  logstream(LOG_INFO) << "Total: " << worker_ret["sum"] << std::endl;


  double threshold = variant_get_value<double>(args.at("threshold"));
  double reset_probability = variant_get_value<double>(args.at("reset_probability"));
  size_t max_iterations = variant_get_value<size_t>(args.at("max_iterations"));

  variant_map_type ret;
  auto ret_g = std::make_shared<unity_sgraph>();
  ret_g->load_graph(output_path);
  ret["graph"] = to_variant(ret_g);
  ret["pagerank"] = to_variant(ret_g->get_vertices());
  ret["delta"] = worker_ret.at("l1");
  ret["training_time"] = mytimer.current_time();
  ret["num_iterations"] = worker_ret.at("num_iter");
  ret["reset_probability"] = reset_probability;
  ret["threshold"] = threshold;
  ret["max_iterations"] = max_iterations;
  auto model = std::make_shared<simple_model>(ret);

  return to_variant(model);
}


} // end of turi namespace
REGISTER_DML_FUNCTION(distributed_pagerank, turi::distributed_pagerank_impl);
