/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/graph_analytics/pagerank.hpp>
#include <unity/lib/toolkit_function_macros.hpp>
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/simple_model.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <sgraph/sgraph_fast_triple_apply.hpp>
#include <sframe/algorithm.hpp>
#include <table_printer/table_printer.hpp>
#include <export.hpp>

namespace turi {
  namespace pagerank {

  const std::string PAGERANK_COLUMN = "pagerank";
  const std::string DELTA_COLUMN = "delta";

  double reset_probability = 0.15;
  double threshold = 1e-2;
  int max_iterations = 20;

  bool single_precision = false;

  const variant_map_type& get_default_options() {
    static const variant_map_type DEFAULT_OPTIONS {
      {"threshold", 1E-2}, {"reset_probability", 0.15}, {"max_iterations", 20}
    };
    return DEFAULT_OPTIONS;
  }

  /**************************************************************************/
  /*                                                                        */
  /*                   Setup and Teardown functions                         */
  /*                                                                        */
  /**************************************************************************/
  void setup(variant_map_type& params) {
    for (const auto& opt: get_default_options()) {
      params.insert(opt);  // Doesn't overwrite keys already in params
    }

    threshold = safe_varmap_get<flexible_type>(params, "threshold");
    reset_probability =
        safe_varmap_get<flexible_type>(params, "reset_probability");
    max_iterations = safe_varmap_get<flexible_type>(params, "max_iterations");
    if (params.count("single_precision")) {
      single_precision =
	  safe_varmap_get<flexible_type>(params, "single_precision");
      if (single_precision) {
        logprogress_stream << "Running pagerank using single precision" << std::endl;
      }
    }

    if (threshold < 0) {
      throw("Parameter 'threshold' must be positive.");
    } else if (reset_probability < 0 || reset_probability > 1) {
      throw("Parameter 'reset_probability' should be between 0 and 1.");
    } else if (max_iterations <= 0) {
      throw("Max iterations should be positive.");
    }
  }

template<typename FLOAT_TYPE>
void triple_apply_pagerank(sgraph& g, size_t& num_iter, double& total_pagerank, double& total_delta) {

  logprogress_stream << "Counting out degree" << std::endl;
  // Degree count
  // std::vector<std::vector<atmoic<size_t>>>
  auto degree_counts = sgraph_compute::create_vertex_data<turi::atomic<size_t>>(g);
  sgraph_compute::fast_triple_apply(g,
                                    [&](sgraph_compute::fast_edge_scope& scope) {
                                      auto src_addr = scope.source_vertex_address();
                                      degree_counts[src_addr.partition_id][src_addr.local_id]++;
                                    }, {}, {});
  logprogress_stream << "Done counting out degree" << std::endl;
  // End of degree count

  // Initialize data structures:
  typedef std::vector<std::vector<turi::atomic<FLOAT_TYPE>>> atomic_float_column_type;
  typedef std::vector<std::vector<FLOAT_TYPE>> float_column_type;

  atomic_float_column_type cur_pagerank = sgraph_compute::create_vertex_data_from_const<turi::atomic<FLOAT_TYPE>>(g, 1.0);
  atomic_float_column_type prev_pagerank = sgraph_compute::create_vertex_data_from_const<turi::atomic<FLOAT_TYPE>>(g, reset_probability);
  float_column_type delta = sgraph_compute::create_vertex_data_from_const<FLOAT_TYPE>(g, 0.0);

  auto cur_pagerank_ptr = &cur_pagerank;
  auto prev_pagerank_ptr = &prev_pagerank;

  num_iter = 0;
  total_delta = 0.0;
  total_pagerank = 0.0;

  // Triple apply
  double w = (1 - reset_probability);
  // const size_t degree_idx = g.get_vertex_field_id(OUT_DEGREE_COLUMN);
  // const size_t pr_idx = g.get_vertex_field_id(PAGERANK_COLUMN);
  // const size_t old_pr_idx = g.get_vertex_field_id(PREV_PAGERANK_COLUMN);

  sgraph_compute::fast_triple_apply_fn_type apply_fn =
    [&](sgraph_compute::fast_edge_scope& scope) {
      auto source_addr = scope.source_vertex_address();
      auto target_addr = scope.target_vertex_address();

      auto source_data = (FLOAT_TYPE)(*prev_pagerank_ptr)[source_addr.partition_id][source_addr.local_id];
      const auto& source_degree = degree_counts[source_addr.partition_id][source_addr.local_id];
      auto& target_data = (*cur_pagerank_ptr)[target_addr.partition_id][target_addr.local_id];

      target_data.inc(w * source_data / source_degree);
    };

  table_printer table({{"Iteration", 0}, 
                                {"L1 change in pagerank", 0}});
  table.print_header();


  for (size_t iter = 0; iter < (size_t)max_iterations; ++iter) {

    ++num_iter;
    if(cppipc::must_cancel()) {
      log_and_throw(std::string("Toolkit cancelled by user."));
    }
    // swap the current pagerank and the prev pagerank
    std::swap(cur_pagerank_ptr, prev_pagerank_ptr);

    // reset the current pagerank value to reset_probability
    parallel_for (0, cur_pagerank_ptr->size(), [&](size_t i) {
      (*cur_pagerank_ptr)[i].assign((*cur_pagerank_ptr)[i].size(), reset_probability);
    });

    // Pagerank iteration
    sgraph_compute::fast_triple_apply(g, apply_fn, {}, {});

    // compute the change in pagerank
    total_delta = 0.0;
    std::vector<double> deleta_per_partition(cur_pagerank_ptr->size(), 0.0);
    parallel_for (0, cur_pagerank_ptr->size(), [&](size_t i) {
      auto& cur_vec = (*cur_pagerank_ptr)[i];
      auto& prev_vec = (*prev_pagerank_ptr)[i];
      auto& delta_vec = delta[i];

      DASSERT_EQ(cur_vec.size(), prev_vec.size());
      DASSERT_EQ(cur_vec.size(), delta_vec.size());

      for (size_t j = 0; j < cur_vec.size(); ++j) {
        auto diff = std::abs<FLOAT_TYPE>((FLOAT_TYPE)cur_vec[j] - (FLOAT_TYPE)prev_vec[j]);
        delta_vec[j] = diff;
        deleta_per_partition[i] += diff;
      }
    });
    total_delta = std::accumulate(deleta_per_partition.begin(), deleta_per_partition.end(), 0.0);
    table.print_row(iter+1, total_delta);

    // check convergence
    if (total_delta < threshold) {
      break;
    }
  } // end of pagerank iterations

  table.print_footer();

  // Compute total pagerank
  total_pagerank = 0.0;
  parallel_for (0, cur_pagerank_ptr->size(), [&](size_t i) {
    auto& cur_vec = (*cur_pagerank_ptr)[i];
    for (size_t j = 0; j < cur_vec.size(); ++j) {
      total_pagerank += cur_vec[j];
    }
  });


  // Store result to graph
  g.add_vertex_field<turi::atomic<FLOAT_TYPE>, flex_float>(*cur_pagerank_ptr,
                                                               PAGERANK_COLUMN,
                                                               flex_type_enum::FLOAT);

  g.add_vertex_field<FLOAT_TYPE, flex_float>(delta,
                                             DELTA_COLUMN,
                                             flex_type_enum::FLOAT);
}




  /**************************************************************************/
  /*                                                                        */
  /*                             Main Function                              */
  /*                                                                        */
  /**************************************************************************/
  variant_map_type exec(variant_map_type& params) {

    timer mytimer;
    setup(params);

    std::shared_ptr<unity_sgraph> source_graph =
        safe_varmap_get<std::shared_ptr<unity_sgraph>>(params, "graph");
    ASSERT_TRUE(source_graph != NULL);
    sgraph& source_sgraph = source_graph->get_graph();
    // Do not support vertex groups yet.
    ASSERT_EQ(source_sgraph.get_num_groups(), 1);

    // Setup the graph we are going to work on. Copying sgraph is cheap.
    sgraph g(source_sgraph);
    g.select_vertex_fields({sgraph::VID_COLUMN_NAME});
    g.select_edge_fields({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME});
    double total_pagerank;
    double delta;
    size_t num_iter;

    if (single_precision) {
      triple_apply_pagerank<float>(g, num_iter, total_pagerank, delta);
    } else {
      triple_apply_pagerank<double>(g, num_iter, total_pagerank, delta);
    }

    std::shared_ptr<unity_sgraph> result_graph(new unity_sgraph(std::make_shared<sgraph>(g)));
    variant_map_type model_params;
    model_params["graph"] = to_variant(result_graph);
    model_params["pagerank"] = to_variant(result_graph->get_vertices());
    model_params["delta"] = delta;
    model_params["training_time"] = mytimer.current_time();
    model_params["num_iterations"] = num_iter;
    model_params["reset_probability"] = reset_probability;
    model_params["threshold"] = threshold;
    model_params["max_iterations"] = max_iterations;

    variant_map_type response;
    response["model"]= to_variant(std::make_shared<simple_model>(model_params));

    return response;
  }

  variant_map_type get_model_fields(variant_map_type& params) {
    return {
      {"graph", "A new SGraph with the pagerank as a vertex property"},
      {"pagerank", "An SFrame with each vertex's pagerank"},
      {"delta", "Change in pagerank for the last iteration in L1 norm"},
      {"training_time", "Total training time of the model"},
      {"num_iterations", "Number of iterations"},
      {"reset_probability",
       "The probablity of randomly jumps to any node in the graph"},
      {"threshold", "The convergence threshold in L1 norm"},
      {"max_iterations", "The maximun number of iterations to run"},
    };
  }

  /**************************************************************************/
  /*                                                                        */
  /*                          Toolkit Registration                          */
  /*                                                                        */
  /**************************************************************************/
BEGIN_FUNCTION_REGISTRATION
REGISTER_NAMED_FUNCTION("create", exec, "params");
REGISTER_FUNCTION(get_model_fields, "params");
END_FUNCTION_REGISTRATION

} // end of namespace connected components
} // end of namespace turi
