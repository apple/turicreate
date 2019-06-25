/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/graph_analytics/kcore.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <model_server/lib/toolkit_util.hpp>
#include <model_server/lib/simple_model.hpp>
#include <core/storage/sframe_interface/unity_sgraph.hpp>
#include <core/storage/sgraph_data/sgraph_compute.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <atomic>
#include <core/export.hpp>

namespace turi {
namespace kcore {

namespace {

const std::string CORE_ID_COLUMN = "core_id";
const std::string DEGREE_COLUMN = "degree";
const std::string DELETED_COLUMN = "deleted";

// Global Variables
int CURRENT_K, KMIN, KMAX;

const variant_map_type& get_default_options() {
  static const variant_map_type DEFAULT_OPTIONS {
    {"kmin", 0},
    {"kmax", 10}
  };
  return DEFAULT_OPTIONS;
}

}  // namespace

/**************************************************************************/
/*                                                                        */
/*                   Setup and Teardown functions                         */
/*                                                                        */
/**************************************************************************/
void setup(variant_map_type& params) {
  for (const auto& opt : get_default_options()) {
    params.insert(opt);  // Doesn't overwrite keys already in params
  }
  KMIN = safe_varmap_get<flexible_type>(params, "kmin");
  KMAX = safe_varmap_get<flexible_type>(params, "kmax");
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

  triple_apply_kcore(g);

  std::shared_ptr<unity_sgraph> result_graph(new unity_sgraph(std::make_shared<sgraph>(g)));

  variant_map_type model_params;
  model_params["graph"] = to_variant(result_graph);
  model_params["core_id"] = to_variant(result_graph->get_vertices());
  model_params["training_time"] = mytimer.current_time();
  model_params["kmin"] = KMIN;
  model_params["kmax"] = KMAX;

  variant_map_type response;
  response["model"] = to_variant(std::make_shared<simple_model>(model_params));

  return response;
}

variant_map_type get_model_fields(variant_map_type& params) {
  return {
    {"graph", "A new SGraph with the core id as a vertex property"},
    {"core_id", "An SFrame with each vertex's core id"},
    {"training_time", "Total training time of the model"},
    {"kmin", "The minimun core id assigned to any vertex"},
    {"kmax", "The maximun core id assigned to any vertex"}
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

} // end of namespace kcore
} // end of namespace turi
