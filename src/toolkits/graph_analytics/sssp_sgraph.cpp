/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/graph_analytics/sssp.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <model_server/lib/toolkit_util.hpp>
#include <model_server/lib/simple_model.hpp>
#include <core/storage/sframe_interface/unity_sgraph.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <core/storage/sgraph_data/sgraph_compute.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <core/logging/table_printer/table_printer.hpp>
#include <atomic>
#include <core/export.hpp>

namespace turi {
namespace sssp {

const std::string DISTANCE_COLUMN = "distance";

/**************************************************************************/
/*                                                                        */
/*        The following variables will be initialized during setup        */
/*                                                                        */
/**************************************************************************/
// Maximum distance allowed.
double MAX_DIST;

// edge field name storing the weights
flexible_type SOURCE_VID;

std::string EDGE_WEIGHT_COLUMN;

// true if edge_attr is not set, using uniform weights instead.
bool UNIFORM_WEIGHTS;


const variant_map_type& get_default_options() {
  static const variant_map_type DEFAULT_OPTIONS {
    {"weight_field", ""}, {"max_distance", 1e30}
  };
  return DEFAULT_OPTIONS;
}

void setup(variant_map_type& params) {
  for (const auto& opt : get_default_options()) {
    params.insert(opt);  // Doesn't overwrite keys already in params
  }

  SOURCE_VID = safe_varmap_get<flexible_type>(params, "source_vid");
  EDGE_WEIGHT_COLUMN =
      (std::string)safe_varmap_get<flexible_type>(params, "weight_field");
  UNIFORM_WEIGHTS = EDGE_WEIGHT_COLUMN == "";
  flexible_type max_dist =
      safe_varmap_get<flexible_type>(params, "max_distance");

  if ((max_dist.get_type() != flex_type_enum::INTEGER &&
       max_dist.get_type() != flex_type_enum::FLOAT) ||
      max_dist <= 0) {
    throw("Invalid max_distance: " + (std::string)(max_dist));
  } else {
    MAX_DIST = (double)max_dist;
  }
}


void check_and_init_graph(sgraph& g) {
  // check edge weight field exists, type correct --> check all weight are non negative.
  if (!UNIFORM_WEIGHTS) {
    size_t id = g.get_edge_field_id(EDGE_WEIGHT_COLUMN); // throw exception if column not found
    flex_type_enum type = g.get_edge_field_types()[id];
    if (type != flex_type_enum::INTEGER && type != flex_type_enum::FLOAT) {
      throw("Weight column " + (std::string)EDGE_WEIGHT_COLUMN + " type must be INTEGER or FLOAT.");
    }
    auto weight_values = g.fetch_edge_data_field(EDGE_WEIGHT_COLUMN);
    parallel_for (0, weight_values.size(), [&](size_t idx) {
      auto& sa = weight_values[idx];
      sarray_reader_buffer<flexible_type> reader(sa->get_reader(), 0, sa->size());
      while (reader.has_next()) {
        double w = (double)reader.next();
        if (w < 0) {
          throw("Detect negative weight: " + std::to_string(w) + "All weight must be non negative.");
        }
      }
    });
  }

  // Clean up unnecessary fields
  g.select_vertex_fields({sgraph::VID_COLUMN_NAME});
  if (!UNIFORM_WEIGHTS) {
    g.select_edge_fields({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME, EDGE_WEIGHT_COLUMN});
  } else {
    g.select_edge_fields({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME});
  }

  bool found_source_vertex = false;
  // check source id exists and initialize the distance column
  auto ret = sgraph_compute::vertex_apply(
      g,
      sgraph::VID_COLUMN_NAME, // first argument in lambda is the vertex column in g
      flex_type_enum::FLOAT,
      [&](const flexible_type& x) {
        if (x == SOURCE_VID) {
          found_source_vertex = true;
          return 0.0;
        } else {
          return MAX_DIST;
        }
      });
  if (!found_source_vertex) {
    throw("Cannot find source vertex: " + (std::string)(SOURCE_VID));
  }
  g.add_vertex_field(ret, DISTANCE_COLUMN);
}

/**
 *  Computes the shortest path distance from all vertices to the source vertex using triple_apply model
 *  Add a new column named DISTANCE_COLUMN to the vertex data.
 */

void triple_apply_sssp(sgraph& g) {
  std::atomic<int64_t> num_changed;
  sgraph_compute::sgraph_engine<flexible_type> ga;
  const size_t dist_idx = g.get_vertex_field_id(DISTANCE_COLUMN);
  const size_t weight_idx = UNIFORM_WEIGHTS ? -1 : g.get_edge_field_id(EDGE_WEIGHT_COLUMN);

  sgraph_compute::triple_apply_fn_type relax_edge_fn =
    [&](sgraph_compute::edge_scope& scope) {
      flexible_type src_cid = scope.source()[dist_idx];
      flexible_type dst_cid = scope.target()[dist_idx];
      double weight = UNIFORM_WEIGHTS ? 1.0 : (double)scope.edge()[weight_idx];

      if(src_cid + weight < dst_cid)  {
        scope.target()[dist_idx] = src_cid + weight;
        ++num_changed;
      }
   };

  table_printer table({{"Number of vertices updated", 0}});
  table.print_header();

  const bool requires_vertex_id = false;
  while(true){
    if(cppipc::must_cancel()) {
      log_and_throw(std::string("Toolkit cancelled by user."));
    }
    num_changed = 0;
    sgraph_compute::triple_apply(g, relax_edge_fn, {DISTANCE_COLUMN},
                                 std::vector<std::string>(),
                                 requires_vertex_id);

    table.print_row(num_changed.load());
    if (num_changed == 0) {
      break;
    }
  }

  table.print_footer();

}

/**
 * Computes the shortest path distance from all vertices to the source vertex.
 * Add a new column named DISTANCE_COLUMN to the vertex data.
 */
void compute_sssp(sgraph& g) {
  typedef sgraph_compute::sgraph_engine<flexible_type>::graph_data_type graph_data_type;
  typedef sgraph::edge_direction edge_direction;

  std::atomic<int64_t> num_changed;
  sgraph_compute::sgraph_engine<flexible_type> ga;
  const size_t dist_idx = g.get_vertex_field_id(DISTANCE_COLUMN);
  const size_t weight_idx = UNIFORM_WEIGHTS ? -1 : g.get_edge_field_id(EDGE_WEIGHT_COLUMN);
  while(true) {
    if(cppipc::must_cancel()) {
      log_and_throw(std::string("Toolkit cancelled by user."));
    }
    num_changed = 0;
    auto ret = ga.gather(
             g,
            [=](const graph_data_type& center,
                const graph_data_type& edge,
                const graph_data_type& other,
                edge_direction edgedir,
                flexible_type& combiner) {
              double weight = UNIFORM_WEIGHTS ? 1.0 : (double)edge[weight_idx];
              combiner = std::min<double>(combiner, other[dist_idx] + weight);
            },
            flexible_type(MAX_DIST),
            edge_direction::IN_EDGE);

    ret = sgraph_compute::vertex_apply(
            g,
            DISTANCE_COLUMN, // first argument in lambda is the distance column in g
            ret,             // second argument in lambda is from here.
            flex_type_enum::FLOAT,
            [&](const flexible_type& x, const flexible_type& y) {
              if (y < x) {
                ++num_changed;
                return y;
              } else {
                return x;
              }
            });
    g.replace_vertex_field(ret, DISTANCE_COLUMN);
    logprogress_stream << "Num vertices updated: "  << num_changed.load() << std::endl;
    if (num_changed == 0) {
      break;
    }
  }
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
  check_and_init_graph(g);
  g.select_vertex_fields({sgraph::VID_COLUMN_NAME, DISTANCE_COLUMN});
  if (!UNIFORM_WEIGHTS) {
    g.select_edge_fields({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME, EDGE_WEIGHT_COLUMN});
  } else {
    g.select_edge_fields({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME});
  }
  //compute_sssp(g);
  triple_apply_sssp(g);

  std::shared_ptr<unity_sgraph> result_graph(new unity_sgraph(std::make_shared<sgraph>(g)));

  variant_map_type model_params;
  model_params["training_time"] = mytimer.current_time();
  model_params["graph"] = to_variant(result_graph);
  model_params["distance"] = to_variant(result_graph->get_vertices());
  model_params["weight_field"] = EDGE_WEIGHT_COLUMN;
  model_params["source_vid"] = SOURCE_VID;
  model_params["max_distance"] = MAX_DIST;

  variant_map_type response;
  response["model"] = to_variant(std::make_shared<simple_model>(model_params));
  return response;
}

variant_map_type get_model_fields(variant_map_type& params) {
  return {
    {"graph", "A new SGraph with the distance as a vertex property"},
    {"distance", "An SFrame with each vertex's distance to the source vertex"},
    {"training_time", "Total training time of the model"},
    {"weight_field", "The edge field for weight"},
    {"source_vid", "The source vertex id"},
    {"max_distance", "Maximum distance between any two vertices"}
  };
}




/**
 * This function is used to backtrack the "parent" for each vertex so that the
 * shortest path for the vertex can be queried.
 */
std::vector<variant_type> _shortest_path_traverse_function(std::map<std::string, flexible_type>& src,
                                                           std::map<std::string, flexible_type>& edge,
                                                           std::map<std::string, flexible_type>& dst,
                                                           const flexible_type& source_vid,
                                                           const std::string& weight_field) {
  if (src["__id"] == source_vid) {
    src["__parent__"] = src["row_id"];
  }
  if (dst["distance"] == src["distance"] + edge[weight_field]) {
    dst["__parent__"] = std::max((flex_int)dst["__parent__"],
                                 (flex_int)src["row_id"]);
  }
  return {to_variant(src), to_variant(edge), to_variant(dst)};
}


/**
 * Compute the shortest path between a set of vertices A, and a set of
 * vertices B. In other words, find the shortest path between any vertex in A
 * and any vertex B. Will return all the shortest paths of the same length.
 */
std::vector<flexible_type> _all_shortest_paths(std::shared_ptr<unity_sgraph> sourcegraph,
                                               std::vector<flexible_type> sources,
                                               std::vector<flexible_type> dests,
                                               std::string weight_column){
  // create fast lookup sets
  std::set<flexible_type> source_set, dest_set;
  for (auto i : sources) source_set.insert(i);
  for (auto i : dests) dest_set.insert(i);
  // lets get a minimal graph to work on
  sgraph g(sourcegraph->get_graph());

  g.select_vertex_fields({sgraph::VID_COLUMN_NAME});
  if (weight_column.empty()) {
    g.select_edge_fields({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME});
  } else {
    g.select_edge_fields({sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME, weight_column});
  }
  // add a numeric vertex ID column
  {
    std::vector<flex_int> vids;
    vids.reserve(g.num_vertices());
    for (size_t i = 0;i < g.num_vertices(); ++i) vids.push_back(i);
    auto numeric_ids = std::make_shared<sarray<flexible_type>>();
    numeric_ids->open_for_write(1); numeric_ids->set_type(flex_type_enum::INTEGER);
    std::copy(vids.begin(), vids.end(), numeric_ids->get_output_iterator(0));
    numeric_ids->close();
    g.add_vertex_field(numeric_ids, "__sssp_numeric_vertex_id__");
  }

  size_t id_idx = g.get_vertex_field_id(sgraph::VID_COLUMN_NAME);
  size_t numeric_id_idx = g.get_vertex_field_id("__sssp_numeric_vertex_id__");
  size_t weight_idx = (size_t)(-1);

  if (!weight_column.empty()) {
    weight_idx = g.get_edge_field_id(weight_column);
  }
  bool found_source_vertex = false;
  bool found_dest_vertex = false;
  /*
   * SOURCE vertices are marked as distance 1
   * DEST vertices are marked as distance -1
   * ALL other vertices are marked as distance 0
   *
   * We expand from SOURCE increasing.
   * We expand from DEST decreasing.
   * When a positive number hits a negative number, we are done.
   */
  struct vertex_data {
    double distance = 0; // if < 0, -(distance+1) is distance to sink
                         // if > 0 distance-1 is distance to source
                         // if 0, is undiscovered
    flexible_type id;    // the ID of the vertex
    flex_int parent = (-1); // if distance < 0, is the next vertex to sink
                            // if distance > 0 is the next vertex to source
                            // if (-1) is undiscovered
    double parent_weight = 0; // edge weight to parent
  };


  std::vector<vertex_data> vertices(g.num_vertices());

  // load all the vertex data into memory
  {
    parallel_for(0, g.get_num_partitions(),
                 [&](size_t segid) {
                  auto& vertex_frame = g.vertex_partition(segid);
                  auto reader = vertex_frame.get_reader(1);
                   for(auto iter = reader->begin(0);
                       iter != reader->end(0);
                       ++iter) {
                     auto numeric_id = (*iter)[numeric_id_idx];
                     auto id = (*iter)[id_idx];
                     vertices[numeric_id].id = id;
                     if (source_set.count(id)) {
                       found_source_vertex = true;
                       vertices[numeric_id].distance = 1;
                     } else if (dest_set.count(id)) {
                       found_dest_vertex = true;
                       vertices[numeric_id].distance = -1;
                     }
                   }
                 });
    if (!found_source_vertex) log_and_throw("Cannot find source vertices");
    if (!found_dest_vertex) log_and_throw("Cannot find destination vertices");
  }
  g.remove_vertex_field(sgraph::VID_COLUMN_NAME);
  g.rename_vertex_fields({"__sssp_numeric_vertex_id__"},{sgraph::VID_COLUMN_NAME});
  id_idx = g.get_vertex_field_id(sgraph::VID_COLUMN_NAME);

  // compute bidirectional sssp

  std::atomic<int64_t> num_changed;
  // this maps cost --> path. This is periodically pruned to always contain
  // the best path
  std::map<double, std::vector<flexible_type> > shortest_paths;
  mutex shortest_paths_accumulated_lock;
  // this contains [vid, vid] --> cost. This enumerates all the edges where
  // source expansion meets the destination expansion.
  std::map<std::pair<flex_int, flex_int>, double> paths_discovered;
  mutex paths_discovered_lock;


  sgraph_compute::sgraph_engine<flexible_type> ga;

  sgraph_compute::triple_apply_fn_type apply_fn =
      [&](sgraph_compute::edge_scope& scope) {
        double weight = (weight_idx == (size_t)(-1)) ? 1.0 : (double)(scope.edge()[weight_idx]);
        scope.lock_vertices();
        flex_int source_id = (flex_int)scope.source()[id_idx];
        flex_int dest_id = (flex_int)scope.target()[id_idx];
        /*
         * ASSERT_LT(source_id, vertices.size());
         * ASSERT_LT(dest_id, vertices.size());
         */
        vertex_data& source = vertices[source_id];
        vertex_data& dest = vertices[dest_id];

        if (source.distance > 0 && dest.distance >= 0) {
          // source propagation
          /*
           * logprogress_stream
           *     << "Source Propagation: "
           *     << source.id << ":" << source.distance
           *     << " --> "
           *     << dest.id << ":" << dest.distance << std::endl;
           */
          double new_cost = source.distance + weight;
          if (dest.distance == 0 ||
              dest.distance > new_cost) {
            dest.distance = new_cost;
            dest.parent = source_id;
            dest.parent_weight = weight;
            ++num_changed;
          }
        }
        else if (dest.distance < 0 && source.distance <= 0) {
          // dest propagation. distances are negated and go downwards
          // be careful of the signs of the compareisons
          double new_cost = dest.distance - weight;
          if (source.distance == 0 ||
              source.distance < new_cost) {
            source.distance = new_cost;
            source.parent = dest_id;
            source.parent_weight = weight;
            ++num_changed;
          }
        } else if (source.distance > 0 && dest.distance < 0) {
          // On this edge, the source expansion meets the dest expansion.
          // i.e. at this point a path has been discovered.
          // compute the cost - 2 because source begins at cost 1, and dest begins begins at cost -1
          double path_cost = source.distance + (-dest.distance) - 2 + weight;
          auto edge_pair = std::make_pair(source_id, dest_id);

          // have we found this exact path before?
          bool path_found_before = false;
          paths_discovered_lock.lock();
          auto paths_discovered_iter = paths_discovered.find(edge_pair);
          if (paths_discovered_iter != paths_discovered.end() &&
              paths_discovered_iter->second <= path_cost) {
            // we found the exact entry with an as good or better path listed.
            path_found_before = true;
          }

          // only perform the rest of the path insertion if we have found a path before
          if (!path_found_before) {
            flex_list outpath;
            vertex_data x = source;
            outpath.push_back(x.id);
            while(1) {
              if (x.parent >= 0) {
                x = vertices[x.parent];
                outpath.push_back(x.id);
              }
              else break;
            }

            flex_list target_path;
            x = dest;
            target_path.push_back(x.id);
            while(1) {
              if (x.parent >= 0) {
                x = vertices[x.parent];
                target_path.push_back(x.id);
              }
              else break;
            }

            std::vector<flexible_type> path(outpath.rbegin(), outpath.rend());
            path.insert(path.end(), target_path.begin(), target_path.end());
            /*
             * logprogress_stream << "Discovering path of cost " << path_cost
             *                    << " meeting at "
             *                    << source.id << " " << dest.id << std::endl;
             */
            shortest_paths_accumulated_lock.lock();
            // only insert if I am better or as good as the best path I have found
            if (shortest_paths.size() == 0 || path_cost <= shortest_paths.begin()->first) {
              shortest_paths[path_cost].push_back(path);
              // trim down. any worse poaths
              if (shortest_paths.size() > 1) {
                auto iter = shortest_paths.begin(); ++iter;
                shortest_paths.erase(iter, shortest_paths.end());
              }
            }
            shortest_paths_accumulated_lock.unlock();
            paths_discovered[edge_pair] = path_cost;
            ++num_changed;
          }
          paths_discovered_lock.unlock();
        }
        scope.unlock_vertices();
      };
  while(true) {
    if(cppipc::must_cancel()) {
      log_and_throw(std::string("Toolkit cancelled by user."));
    }
    num_changed = 0;
    sgraph_compute::triple_apply(g, apply_fn, std::vector<std::string>(),
                                 std::vector<std::string>());
    logprogress_stream << "Num vertices updated: "  << num_changed.load() << std::endl;
    if (num_changed == 0) {
      break;
    }
    // perform path acceleration
    while(1) {
      size_t acceleration_changes = 0;
      for (size_t i = 0;i < vertices.size(); ++i) {
        if (vertices[i].parent != -1) {
          if (vertices[i].distance > 0 &&
              vertices[i].distance > vertices[vertices[i].parent].distance + vertices[i].parent_weight) {
            vertices[i].distance = vertices[vertices[i].parent].distance + vertices[i].parent_weight;
            ++acceleration_changes;
          }
          else if (vertices[i].distance < 0 &&
                   vertices[i].distance < vertices[vertices[i].parent].distance - vertices[i].parent_weight) {
            vertices[i].distance = vertices[vertices[i].parent].distance - vertices[i].parent_weight;
            ++acceleration_changes;
          }
        }
      }
      logprogress_stream << "Num accelerated relaxations: " << acceleration_changes << std::endl;
      if (acceleration_changes == 0) break;
    }
  }
  if (shortest_paths.size() == 0) return std::vector<flexible_type>();
  else return shortest_paths.begin()->second;
}
/**************************************************************************/
/*                                                                        */
/*                          Toolkit Registration                          */
/*                                                                        */
/**************************************************************************/
BEGIN_FUNCTION_REGISTRATION

REGISTER_NAMED_FUNCTION("create", exec, "params");
REGISTER_FUNCTION(get_model_fields, "params");

REGISTER_NAMED_FUNCTION("shortest_path_traverse_function",
                        _shortest_path_traverse_function,
			"src", "edge", "dst", "source_vid", "weight_field");
REGISTER_DOCSTRING(
    _shortest_path_traverse_function,
    "Computes for each vertex, it's parent vertex (i.e. the shortest path "
    "originating from the source vertex must reach the current via the parent "
    "vertex.");

REGISTER_NAMED_FUNCTION("all_shortest_paths",
                        _all_shortest_paths,
			"graph", "sources", "dests", "weight_field");

REGISTER_DOCSTRING(
    _all_shortest_paths,
    "Compute the shortest path between a set of vertices A, and a set of "
    "vertices B. In other words, find the shortest path between any vertex in "
    "A and any vertex B. Will return all the shortest paths of the same length."
    "May return duplicates.")

END_FUNCTION_REGISTRATION


} // end of namespace sssp
} // end of namespace turi
