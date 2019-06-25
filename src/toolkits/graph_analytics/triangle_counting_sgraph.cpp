/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/graph_analytics/triangle_counting.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <model_server/lib/toolkit_util.hpp>
#include <model_server/lib/simple_model.hpp>
#include <core/storage/sframe_interface/unity_sgraph.hpp>
#include <core/storage/sgraph_data/sgraph_compute.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <core/export.hpp>

namespace turi {

namespace triangle_counting {

const std::string INT_VID_COLUMN = "__int_vid__";
const std::string NEIGHBOR_ID_COLUMN = "__neighbor_ids__";
const std::string EDGE_DELETE_COLUMN = "__deleted__";
const std::string EDGE_COUNT_COLUMN = "__count__";
const std::string VERTEX_COUNT_COLUMN = "triangle_count";

typedef sgraph_compute::sgraph_engine<flexible_type>::graph_data_type graph_data_type;
typedef sgraph::edge_direction edge_direction;


/**************************************************************************/
/*                                                                        */
/*                          Set Helper Functions                          */
/*                                                                        */
/**************************************************************************/
/**
 * Helper function to add a value to flex_vec
 * and keep all values in the flex_vec unique.
 */
void set_insert(flexible_type& set, const flexible_type& value) {
  flex_vec& vec = set.mutable_get<flex_vec>();
  bool is_unique = true;
  for (auto& v : vec) {
    if ((size_t)v == (size_t)(value)) {
      is_unique = false;
      break;
    }
  }
  if (is_unique) {
    vec.push_back((double)(value));
  }
}

/**
 * Helper function to check wheter an element is in the flex_vec.
 * Assuming the flex_vec is sorted.
 */
bool set_contains(const flexible_type& sorted_vec, const flexible_type& value) {
  const flex_vec& vec = sorted_vec.get<flex_vec>();
  return std::binary_search(vec.begin(), vec.end(), value);
}

/**
 * Helper function to count the size of intersection of two sorted
 * vectors.
 */
size_t count_intersection(const flex_vec& sorted_v1, const flex_vec& sorted_v2) {
  size_t ret = 0;
  for (size_t i = 0, j = 0; i < sorted_v1.size() && j < sorted_v2.size();) {
    if (sorted_v1[i] == sorted_v2[j]) {
      ++ret, ++i, ++j;
    } else if (sorted_v1[i] < sorted_v2[j]) {
      ++i;
    } else {
      ++j;
    }
  }
  return ret;
}

/**************************************************************************/
/*                                                                        */
/*                    Triangle Counting Implementation                    */
/*                                                                        */
/**************************************************************************/
/**
 * Initialize a unique integer vertex id from 0 to N-1
 */
void init_vertex_id(sgraph& g) {
  auto& vgroup = g.vertex_group();
  std::vector<size_t> beginids{0};
  size_t acc = 0;
  for (auto& sf: vgroup) {
    acc += sf.size();
    beginids.push_back(acc);
  }
  parallel_for (0, vgroup.size(), [&](size_t partitionid) {
    size_t begin = beginids[partitionid];
    size_t end = beginids[partitionid+1];
    std::shared_ptr<sarray<flexible_type>> id_column =
        std::make_shared<sarray<flexible_type>>();
    id_column->open_for_write(1);
    id_column->set_type(flex_type_enum::INTEGER);
    auto out = id_column->get_output_iterator(0);
    while(begin != end) {
      *out = begin;
      ++begin;
      ++out;
    }
    id_column->close();
    vgroup[partitionid] = vgroup[partitionid].add_column(id_column, INT_VID_COLUMN);
  });
}

/**
 * Make the graph undirected -- so that there is only one edge between two vertices.
 * Also removes the self edges S -> S.
 *
 * This function will Add a new edge data field to the graph "__deleted__".
 * If S->T and T->S both exists in the graph, then S->T[__deleted__]  = 1 if and only if S.id > T.id.
 */
void make_undirect_graph(sgraph& g) {
  sgraph_compute::sgraph_engine<flexible_type> engine;
  flex_vec empty_set{};
  const size_t id_idx = g.get_vertex_field_id(INT_VID_COLUMN);

  // First we gather the incoming neighbor ids into each vertex
  auto ret =
    engine.gather(
        g,
        [=](const graph_data_type& center,
            const graph_data_type& edge,
            const graph_data_type& other,
            edge_direction edgedir,
            flexible_type& combiner) {
          set_insert(combiner, other[id_idx]);
        },
        flexible_type(empty_set),
        edge_direction::IN_EDGE);
  g.add_vertex_field(ret, NEIGHBOR_ID_COLUMN);

  // Here we have to perform an extra "apply" to get the vector sorted.
  // An emmit function in the gather will be nice.
  ret =
    sgraph_compute::vertex_apply(
      g,
      NEIGHBOR_ID_COLUMN,
      flex_type_enum::VECTOR,
      [&](flexible_type& x) {
        flex_vec& vec = x.mutable_get<flex_vec>();
        std::sort(vec.begin(), vec.end());
        return vec;
      });
  g.replace_vertex_field(ret, NEIGHBOR_ID_COLUMN);

  const size_t neighbor_set_idx = g.get_vertex_field_id(NEIGHBOR_ID_COLUMN);
  // For each edge, if src.in_neighbor.contains(dst.id()) and src.id() < dst.id()
  // return delete=1, else return delete=0.
  ret =
    engine.parallel_for_edges(g,
        [=](const graph_data_type& source,
            graph_data_type& edge,
            const graph_data_type& target) {
          bool is_bidirection = (source[id_idx] < target[id_idx]) &&
                                (set_contains(source[neighbor_set_idx], target[id_idx]));
          bool is_self_edges = (source[id_idx] == target[id_idx]);
          if (is_bidirection || is_self_edges) {
            return flexible_type(1);
          } else {
            return flexible_type(0);
          }
        },
        flex_type_enum::INTEGER);
  g.add_edge_field(ret, EDGE_DELETE_COLUMN);
}

/**
 * Compute triangle count for each vertex, and return the total
 * number of triangles in the graph.
 */
size_t compute_triangle_count(sgraph& g) {
  timer mytimer;
  logprogress_stream << "Initializing vertex ids." << std::endl;
  // add a unique integer id to each vertex at column INT_VID_COLUMN
  init_vertex_id(g);

  if(cppipc::must_cancel()) {
    log_and_throw(std::string("Toolkit cancelled by user."));
  }

  logprogress_stream << "Removing duplicate (bidirectional) edges." << std::endl;
  // add an edge column EDGE_DELETE_COLUMN to mark the edges deleted
  // after making the graph undiredcted.
  make_undirect_graph(g);

  if(cppipc::must_cancel()) {
    log_and_throw(std::string("Toolkit cancelled by user."));
  }

  // now we can count the triangles
  // First we gather all neighbor ids into each vertex, only counting for edges
  // not deleted.
  sgraph_compute::sgraph_engine<flexible_type> engine;
  flex_vec empty_set{};
  const size_t id_idx = g.get_vertex_field_id(INT_VID_COLUMN);
  const size_t edge_delete_idx = g.get_edge_field_id(EDGE_DELETE_COLUMN);
  const size_t neighbor_set_idx = g.get_vertex_field_id(NEIGHBOR_ID_COLUMN);

  logprogress_stream << "Counting triangles..." << std::endl;
  auto ret =
  engine.gather(g,
      [=](const graph_data_type& center,
          const graph_data_type& edge,
          const graph_data_type& other,
          edge_direction edgedir,
          flexible_type& combiner) {
          if ((int)edge[edge_delete_idx] == 0) {
            set_insert(combiner, other[id_idx]);
          }
        },
        flexible_type(empty_set),
        edge_direction::ANY_EDGE);
  g.replace_vertex_field(ret, NEIGHBOR_ID_COLUMN);

  if(cppipc::must_cancel()) {
    log_and_throw(std::string("Toolkit cancelled by user."));
  }

  ret =
    sgraph_compute::vertex_apply(
      g,
      NEIGHBOR_ID_COLUMN,
      flex_type_enum::VECTOR,
      [&](flexible_type& x) {
        flex_vec& vec = x.mutable_get<flex_vec>();
        std::sort(vec.begin(), vec.end());
        return vec;
      });
  g.replace_vertex_field(ret, NEIGHBOR_ID_COLUMN);

  if(cppipc::must_cancel()) {
    log_and_throw(std::string("Toolkit cancelled by user."));
  }

  // Next for each edge, we count the size of the intersection of the neighbor ids
  // from each side, and save it to EDGE_TRIANGLE_COUNT. This is how many triangles
  // this edge participates in.
  ret =
    engine.parallel_for_edges(g,
      [=](const graph_data_type& source,
          graph_data_type& edge,
          const graph_data_type& target) {
        if ((int)edge[edge_delete_idx] == 0) {
          return count_intersection(source[neighbor_set_idx].get<flex_vec>(),
                                    target[neighbor_set_idx].get<flex_vec>());
        } else {
          return (size_t)0;
        }
      }, flex_type_enum::INTEGER);
  g.add_edge_field(ret, EDGE_COUNT_COLUMN);

  if(cppipc::must_cancel()) {
    log_and_throw(std::string("Toolkit cancelled by user."));
  }

  // Now, for each vertex, we gather the EDGE_TRIANGLE_COUNT and divide by 2. This is
  // how many triangles this vertex participates in.
  const size_t edge_count_idx = g.get_edge_field_id(EDGE_COUNT_COLUMN);
  ret =
    engine.gather(g,
        [=](const graph_data_type& center,
            const graph_data_type& edge,
            const graph_data_type& other,
            edge_direction dir,
            flexible_type& combiner) {
          combiner += edge[edge_count_idx];
        }, flexible_type(0), edge_direction::ANY_EDGE);
  g.add_vertex_field(ret, VERTEX_COUNT_COLUMN);
  ret =
    sgraph_compute::vertex_apply(
        g,
        VERTEX_COUNT_COLUMN,
        flex_type_enum::INTEGER,
        [&](const flexible_type& x) {
          return (size_t)x / 2;
        });
  g.replace_vertex_field(ret, VERTEX_COUNT_COLUMN);

  if(cppipc::must_cancel()) {
    log_and_throw(std::string("Toolkit cancelled by user."));
  }

  // Finally, the total triangle counts is the sum of all per-vertex count divide by 3.
  size_t total_triangles =
    sgraph_compute::vertex_reduce<size_t>(
        g,
        VERTEX_COUNT_COLUMN,
        [] (const flexible_type& x, size_t& acc) {
          acc += (size_t)x;
        },
        [] (const size_t& v, size_t&  acc) {
          acc += v;
        });

  // clean up;
  g.remove_vertex_field(INT_VID_COLUMN);
  g.remove_vertex_field(NEIGHBOR_ID_COLUMN);
  g.remove_edge_field(EDGE_DELETE_COLUMN);
  g.remove_edge_field(EDGE_COUNT_COLUMN);

  total_triangles /= 3;

  logprogress_stream << "Finished in " << mytimer.current_time() << " secs." << std::endl;
  logprogress_stream << "Total triangles in the graph : " << total_triangles << std::endl;

  return total_triangles;
}

/**************************************************************************/
/*                                                                        */
/*                             Main Function                              */
/*                                                                        */
/**************************************************************************/
variant_map_type exec(variant_map_type& params) {

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

  size_t total_counts = compute_triangle_count(g);

  std::shared_ptr<unity_sgraph> result_graph(new unity_sgraph(std::make_shared<sgraph>(g)));

  variant_map_type model_params;
  model_params["num_triangles"] = total_counts;
  model_params["training_time"]  = mytimer.current_time();
  model_params["graph"]  = to_variant(result_graph);
  model_params["triangle_count"]  = to_variant(result_graph->get_vertices());

  variant_map_type response;
  response["model"] = to_variant(std::make_shared<simple_model>(model_params));
  return response;
}

variant_map_type get_default_options(variant_map_type& params) {
  variant_map_type response;
  return response;
}

variant_map_type get_model_fields(variant_map_type& params) {
  return {
    {"num_triangles", "Total number of triangles in the graph."},
    {"triangle_count", "An SFrame with the triangle count for each vertex."},
    {"graph", "A new SGraph with the triangle count as a vertex property."},
    {"training_time", "Total training time of the model"}
  };
}

BEGIN_FUNCTION_REGISTRATION
REGISTER_NAMED_FUNCTION("create", exec, "params");
REGISTER_FUNCTION(get_model_fields, "params");
END_FUNCTION_REGISTRATION

} // end of namespace triangle counting
} // end of namespace turi
