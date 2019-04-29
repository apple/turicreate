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
#include <platform/parallel/atomic.hpp>
#include <export.hpp>

namespace turi {
namespace degree_count {

const std::string IN_DEGREE_COLUMN = "in_degree";
const std::string OUT_DEGREE_COLUMN = "out_degree";
const std::string ALL_DEGREE_COLUMN = "total_degree";

/**
 * Compute in_degree, out_degree and total_degree for each vertex in the graph,
 * add three new columns to the vertices.
 */
void compute_degree_count(sgraph& g) {

  // Initialize component ids
  typedef std::vector<std::vector<size_t>> size_t_column_type;
  typedef std::vector<std::vector<turi::atomic<size_t>>> atomic_size_t_column_type;
  atomic_size_t_column_type in_degree_data(sgraph_compute::create_vertex_data<turi::atomic<size_t>>(g));
  atomic_size_t_column_type out_degree_data(sgraph_compute::create_vertex_data<turi::atomic<size_t>>(g));
  size_t_column_type all_degree_data = sgraph_compute::create_vertex_data<size_t>(g);

  sgraph_compute::fast_triple_apply_fn_type apply_fn =
      [&](sgraph_compute::fast_edge_scope& scope) {
        auto src_addr = scope.source_vertex_address();
        auto dst_addr = scope.target_vertex_address();
        out_degree_data[src_addr.partition_id][src_addr.local_id]++;
        in_degree_data[dst_addr.partition_id][dst_addr.local_id]++;
      };

  sgraph_compute::fast_triple_apply(g, apply_fn, {}, {});

  parallel_for(0, all_degree_data.size(), [&](size_t i) {
    size_t n = all_degree_data[i].size();
    const auto& in_vec = in_degree_data[i];
    const auto& out_vec = out_degree_data[i];
    auto& all_vec = all_degree_data[i];
    for (size_t j = 0; j < n; ++j) all_vec[j] = in_vec[j] + out_vec[j];
  });

  // store result to graph
  g.add_vertex_field<turi::atomic<size_t>, flex_int>(in_degree_data,
                                                    IN_DEGREE_COLUMN,
                                                    flex_type_enum::INTEGER);

  g.add_vertex_field<turi::atomic<size_t>, flex_int>(out_degree_data,
                                                    OUT_DEGREE_COLUMN,
                                                    flex_type_enum::INTEGER);

  g.add_vertex_field<size_t, flex_int>(all_degree_data,
                                       ALL_DEGREE_COLUMN,
                                       flex_type_enum::INTEGER);
}


/**************************************************************************/
/*                                                                        */
/*                             Main Function                              */
/*                                                                        */
/**************************************************************************/
variant_map_type exec(variant_map_type& params) {
  // no setup needed
  timer mytimer;
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

  compute_degree_count(g);

  std::shared_ptr<unity_sgraph> result_graph(new unity_sgraph(std::make_shared<sgraph>(g)));

  variant_map_type model_params;
  model_params["graph"] = to_variant(result_graph);
  model_params["training_time"] = mytimer.current_time();

  variant_map_type response;
  response["model"] = to_variant(std::make_shared<simple_model>(model_params));
  return response;
}

static const variant_map_type MODEL_FIELDS{
};

variant_map_type get_model_fields(variant_map_type& params) {
  return {
    {"graph", "A new SGraph with the color id as a vertex property"},
    {"component_id", "An SFrame with each vertex's component id"},
    {"component_size", "An SFrame with the size of each component"},
    {"training_time", "Total training time of the model"}
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

} // end of namespace degree_count
} // end of namespace turi
