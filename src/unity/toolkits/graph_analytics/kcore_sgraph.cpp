/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/graph_analytics/kcore.hpp>
#include <unity/lib/toolkit_util.hpp>
#include <unity/lib/simple_model.hpp>
#include <unity/lib/unity_sgraph.hpp>
#include <sgraph/sgraph_compute.hpp>
#include <sframe/algorithm.hpp>
#include <atomic>
#include <export.hpp>

namespace turi {
namespace kcore {

const std::string CORE_ID_COLUMN = "core_id";
const std::string DEGREE_COLUMN = "degree";
const std::string DELETED_COLUMN = "deleted";

// Global Variables
int CURRENT_K, KMIN, KMAX;

/**************************************************************************/
/*                                                                        */
/*                   Setup and Teardown functions                         */
/*                                                                        */
/**************************************************************************/
void setup(toolkit_function_invocation& invoke) {

  KMIN = safe_varmap_get<flexible_type>(invoke.params, "kmin");
  KMAX = safe_varmap_get<flexible_type>(invoke.params, "kmax");
  CURRENT_K = 1;

  if (KMIN < 0 || KMAX < 0) {
    throw("kmin and kmax should be positive");
  } else if (KMIN >= KMAX) {
    throw("kmin must be smaller than kmax");
  }
}

/**
 * We start with every vertex having core_id = KMAX,
 * Each iteration, while the gather will +1 for neighbors whose core_id > CURRENT_K 
 * If the gather is > 0 and <= CURRENT_K, then we set the core_id to CURRENT_K (indicate its deleted).
 * And repeat...
 */
void triple_apply_kcore(sgraph& g) {
  typedef sgraph_compute::sgraph_engine<flexible_type>::graph_data_type graph_data_type;
  typedef sgraph::edge_direction edge_direction;

  // initialize every vertex with core id kmin
  g.init_vertex_field(CORE_ID_COLUMN, KMIN);
  g.init_vertex_field(DEGREE_COLUMN, 0);
  g.init_vertex_field(DELETED_COLUMN, 0);
  g.init_edge_field(DELETED_COLUMN, 0);

  // Initialize degree count
  sgraph_compute::sgraph_engine<flexible_type> ga;
  auto degrees = ga.gather(
          g,
          [=](const graph_data_type& center,
              const graph_data_type& edge,
              const graph_data_type& other,
              edge_direction edgedir,
              flexible_type& combiner) {
              combiner += 1;
          },
          flexible_type(0),
          edge_direction::ANY_EDGE);
  g.replace_vertex_field(degrees, DEGREE_COLUMN);

  // Initialize fields
  size_t vertices_left = g.num_vertices();
  std::atomic<size_t> num_vertices_changed;
  const size_t core_idx = g.get_vertex_field_id(CORE_ID_COLUMN);
  const size_t degree_idx = g.get_vertex_field_id(DEGREE_COLUMN);
  const size_t v_deleted_idx= g.get_vertex_field_id(DELETED_COLUMN);
  const size_t e_deleted_idx= g.get_edge_field_id(DELETED_COLUMN);

  // Triple apply
  sgraph_compute::triple_apply_fn_type apply_fn =
  [&](sgraph_compute::edge_scope& scope) {
    auto& source = scope.source();
    auto& target = scope.target();
    auto& edge = scope.edge();
    scope.lock_vertices();
    // edge is not deleted
    if (!edge[e_deleted_idx]) {
      // check source degree
      if (!source[v_deleted_idx] && source[degree_idx] <= CURRENT_K) {
        source[core_idx] = CURRENT_K;
        source[v_deleted_idx] = 1;
        num_vertices_changed++;
      }
      // check target degree
      if (!target[v_deleted_idx] && target[degree_idx] <= CURRENT_K) {
        target[core_idx] = CURRENT_K;
        target[v_deleted_idx] = 1;
        num_vertices_changed ++;
      }
      // delete the edge if either side is deleted
      if (source[v_deleted_idx] || target[v_deleted_idx]) {
        edge[e_deleted_idx] = 1;
        --source[degree_idx];
        --target[degree_idx];
        // We need to check again if the deletion of this edge 
        // causing either source or target vertex to be deleted.
        if (!source[v_deleted_idx] && source[degree_idx] <= CURRENT_K) {
          source[core_idx] = CURRENT_K;
          source[v_deleted_idx] = 1;
          num_vertices_changed++;
        }
        // check target degree
        if (!target[v_deleted_idx] && target[degree_idx] <= CURRENT_K) {
          target[core_idx] = CURRENT_K;
          target[v_deleted_idx] = 1;
          num_vertices_changed++;
        }
      }
    }
    scope.unlock_vertices();
  };

  bool requires_vertex_id = false;
  for (CURRENT_K = KMIN; CURRENT_K < KMAX; ++CURRENT_K) {
    while (true) {
      if(cppipc::must_cancel()) {
        log_and_throw(std::string("Toolkit cancelled by user."));
      }
      num_vertices_changed = 0;
      sgraph_compute::triple_apply(g, apply_fn, {CORE_ID_COLUMN, DEGREE_COLUMN,
        DELETED_COLUMN}, {DELETED_COLUMN}, requires_vertex_id);
      if (num_vertices_changed == 0)
        break;
      vertices_left -= num_vertices_changed;
      if (CURRENT_K == 0 || num_vertices_changed == 0 || vertices_left == 0) {
        // we are done with the current core.
        break;
      }
      ASSERT_GT(vertices_left, 0);
    }
    logprogress_stream << "Finish computing core " << CURRENT_K << "\t Vertices left: "
                       << vertices_left << std::endl;
    if (vertices_left == 0) {
      break;
    }
  } // end of kcore iterations

  auto final_core_ids = sgraph_compute::vertex_apply(
            g,
            degrees,
            flex_type_enum::INTEGER,
            [&](const std::vector<flexible_type>& vdata, const flexible_type& actual_degree) -> flexible_type {
              if (!vdata[v_deleted_idx]) {
                // active vertices gets KMAX
                return flexible_type(KMAX);
              } else if (actual_degree == 0) {
                // singleton degree gets KMIN
                return flexible_type(KMIN);
              } else {
                return vdata[core_idx];
              }
            });
  g.replace_vertex_field(final_core_ids, CORE_ID_COLUMN);

  // cleanup
  g.remove_vertex_field(DEGREE_COLUMN);
  g.remove_vertex_field(DELETED_COLUMN);
  g.remove_edge_field(DELETED_COLUMN);
}

/**************************************************************************/
/*                                                                        */
/*                             Main Function                              */
/*                                                                        */
/**************************************************************************/
toolkit_function_response_type exec(toolkit_function_invocation& invoke) {
  timer mytimer;
  setup(invoke);

  std::shared_ptr<unity_sgraph> source_graph =
      safe_varmap_get<std::shared_ptr<unity_sgraph>>(invoke.params, "graph");
  ASSERT_TRUE(source_graph != NULL);
  sgraph& source_sgraph = source_graph->get_graph();
  // Do not support vertex groups yet.
  ASSERT_EQ(source_sgraph.get_num_groups(), 1);

  // Setup the graph we are going to work on. Copying sgraph is cheap.
  sgraph g(source_sgraph);
  g.select_vertex_fields({sgraph::VID_COLUMN_NAME});
  g.select_edge_fields({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME});

  triple_apply_kcore(g);

  std::shared_ptr<unity_sgraph> result_graph(new unity_sgraph(std::make_shared<sgraph>(g)));

  variant_map_type params;
  params["graph"] = to_variant(result_graph);
  params["core_id"] = to_variant(result_graph->get_vertices());
  params["training_time"] = mytimer.current_time();
  params["kmin"] = KMIN;
  params["kmax"] = KMAX;

  toolkit_function_response_type response;
  response.params["model"] = to_variant(std::make_shared<simple_model>(params));
  response.success = true;

  return response;
}

static const variant_map_type DEFAULT_OPTIONS {
  {"kmin", 0}, {"kmax", 10}
};

toolkit_function_response_type get_default_options(toolkit_function_invocation& invoke) {
  toolkit_function_response_type response;
  response.success = true;
  response.params = DEFAULT_OPTIONS;
  return response;
}

static const variant_map_type MODEL_FIELDS{
  {"graph", "A new SGraph with the core id as a vertex property"},
  {"core_id", "An SFrame with each vertex's core id"},
  {"training_time", "Total training time of the model"},
  {"kmin", "The minimun core id assigned to any vertex"},
  {"kmax", "The maximun core id assigned to any vertex"}
};

toolkit_function_response_type get_model_fields(toolkit_function_invocation& invoke) {
  toolkit_function_response_type response;
  response.success = true;
  response.params = MODEL_FIELDS;
  return response;
}
/**************************************************************************/
/*                                                                        */
/*                          Toolkit Registration                          */
/*                                                                        */
/**************************************************************************/


EXPORT std::vector<toolkit_function_specification> get_toolkit_function_registration() {
  toolkit_function_specification main_spec;
  main_spec.name = "kcore";
  main_spec.default_options = DEFAULT_OPTIONS;
  main_spec.toolkit_execute_function = exec;

  toolkit_function_specification option_spec;
  option_spec.name = "kcore_default_options";
  option_spec.toolkit_execute_function = get_default_options;

  toolkit_function_specification model_spec;
  model_spec.name = "kcore_model_fields";
  model_spec.toolkit_execute_function = get_model_fields;
  return {main_spec, option_spec, model_spec};
}

} // end of namespace kcore 
} // end of namespace turi
