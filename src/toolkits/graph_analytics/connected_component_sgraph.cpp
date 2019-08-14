/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/graph_analytics/connected_component.hpp>
#include <model_server/lib/toolkit_function_macros.hpp>
#include <model_server/lib/toolkit_util.hpp>
#include <model_server/lib/simple_model.hpp>
#include <core/storage/sframe_interface/unity_sgraph.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>
#include <core/storage/sgraph_data/sgraph_fast_triple_apply.hpp>
#include <core/storage/sframe_data/algorithm.hpp>
#include <core/storage/sframe_data/groupby_aggregate.hpp>
#include <core/storage/sframe_data/groupby_aggregate_operators.hpp>
#include <core/parallel/atomic.hpp>
#include <core/parallel/mutex.hpp>
#include <core/logging/table_printer/table_printer.hpp>
#include <core/export.hpp>

namespace turi {
namespace connected_component {

const std::string COMPONENT_ID_COLUMN = "component_id";

/**
 * Standard union find datastructure for connected components.
 *
 * Implements weighted union, and path compression.
 *
 * The \ref union_gorup, and \ref find_root operations are
 * thread safe for finding connected components.
 */
class union_find_cc {
 public:
  union_find_cc(size_t num_vertices) {
    // initialize parents
    parents.reserve(num_vertices);
    parents.resize(num_vertices);
    parallel_for(0, num_vertices, [&](size_t i) { parents[i] = i; });

    // initialize rank
    rank.reserve(num_vertices);
    rank.resize(num_vertices, 1);
  }

  void union_group(size_t group_a, size_t group_b) {
    // update
    if (rank[group_a] > rank[group_b]) {
      parents[group_b] = group_a;
      rank[group_a] += rank[group_b];
    } else {
      parents[group_a] = group_b;
      rank[group_b] += rank[group_a];
    }
  }

  // Merge with another union find datastructure
  union_find_cc& operator+= (const union_find_cc& other) {
    ASSERT_EQ(parents.size(), other.parents.size());

    // Iterate over "edges" from the other
    for (size_t i = 0; i < other.parents.size(); ++i) {
      // Add every "edge" from the other, and perform union if necessary
      size_t src_root = find_root(i);
      size_t dst_root = find_root(other.parents[i]);
      if (src_root != dst_root) {
        union_group(src_root, dst_root);
      }
    }
    return *this;
  }

  size_t find_root(size_t vid) {
    if (parents[vid] == vid) {
      return vid;
    }

    size_t ret = find_root(parents[vid]);

    // path compression
    parents[vid] = ret;
    return ret;
  }

  void clear_rank() {
    rank.clear();
    rank.shrink_to_fit();
  }

  void clear() {
    clear_rank();
    parents.clear();
    parents.shrink_to_fit();
  }


 private:
  /// Length |V|, parents[i] stores the parent vertex id for vertex i.
  /// If i is root, parents[i] = i
  std::vector<size_t> parents;

  /// Length |V|, rank[i] stores the rank for component i.
  /// The rank is approximiate during constructiondue to race condition.
  std::vector<size_t> rank;
};

/**
 * Compute connected component on the graph, add a new column to the vertex
 * with name "component_id".
 *
 * Returns an sframe with component id and component size information.
 */
sframe compute_connected_component(sgraph& g) {

  // Initialize component union find data structure
  size_t nthreads = thread::cpu_count();
  std::vector<union_find_cc> thread_local_union_find(nthreads, g.num_vertices());

  // For each partition, store the first vertex id
  // This is the prefix sum of the partition size.
  std::vector<size_t> partition_base_id(g.get_num_partitions());
  size_t acc = 0;
  for (size_t i = 0; i < partition_base_id.size(); ++i) {
    partition_base_id[i] = acc;
    acc += g.vertex_partition(i).size();
  }

  std::atomic<size_t> num_changed(0);
  sgraph_compute::fast_triple_apply_fn_type apply_fn =
      [&](sgraph_compute::fast_edge_scope& scope) {
        size_t tid = thread::thread_id();
        auto& union_find = thread_local_union_find[tid];
        auto src_addr = scope.source_vertex_address();
        auto dst_addr = scope.target_vertex_address();

        // Get vertex id
        size_t src_vid = partition_base_id[src_addr.partition_id] + src_addr.local_id;
        size_t dst_vid = partition_base_id[dst_addr.partition_id] + dst_addr.local_id;

        // Find component id
        size_t src_component_id = union_find.find_root(src_vid);
        size_t dst_component_id = union_find.find_root(dst_vid);

        // Union two components
        if (src_component_id != dst_component_id) {
          union_find.union_group(src_component_id, dst_component_id);
          ++num_changed;
        }
      };

  table_printer table({{"Number of components merged", 0}});
  table.print_header();
  while(true) {
    if (cppipc::must_cancel()) {
      log_and_throw(std::string("Toolkit canceled by user"));
    }
    num_changed = 0;

    // Compute union find in parallel
    sgraph_compute::fast_triple_apply(g, apply_fn, {}, {});

    // Merge thread local union find structures
    for (size_t i = 1; i < thread_local_union_find.size(); ++i) {
      thread_local_union_find[0] += thread_local_union_find[i];
    }

    // Broadcast the master copy
    for (size_t i = 1; i < thread_local_union_find.size(); ++i) {
      thread_local_union_find[i] = thread_local_union_find[0];
    }

    table.print_row(num_changed);
    if (num_changed == 0) {
      break;
    }
  }
  table.print_footer();

  // Converged!
  // We no longer need the approxmiate rank stored in union_find , clear it and free up some memory
  // We also don't need copies in other threads, only retain the master copy.
  for (size_t i = 1; i < thread_local_union_find.size(); ++i) { thread_local_union_find[i].clear(); }
  auto& union_find = thread_local_union_find[0];
  union_find.clear_rank();

  // Prepare for return results:
  // 1. Vertex data of component id
  // 2. SFrame of component size

  // One more pass to eagerly compute component id for each vertex.
  std::vector<std::vector<size_t>> component_ids(g.get_num_partitions());
  // At the same time we can compute the actual size of each componenets
  std::vector<turi::atomic<size_t>> component_sizes(g.num_vertices());

  parallel_for(0, g.get_num_partitions(), [&](size_t partition_id) {
    size_t begin_vid = partition_base_id[partition_id];
    auto& component_ids_for_this_partition = component_ids[partition_id];
    component_ids_for_this_partition.resize(g.vertex_partition(partition_id).size());
    for (size_t i = 0; i < component_ids_for_this_partition.size(); ++i) {
      size_t vid = begin_vid + i;
      size_t cid = union_find.find_root(vid);
      component_ids_for_this_partition[i] = cid;
      component_sizes[cid].inc();
    }
  });

  // Clear everything in union find
  union_find.clear();

  // store result to graph
  g.add_vertex_field<size_t, flex_int>(component_ids,
                                       COMPONENT_ID_COLUMN,
                                       flex_type_enum::INTEGER);

  // prepare component stats sframe.
  sframe component_info;
  component_info.open_for_write({COMPONENT_ID_COLUMN, "Count"},
                                {flex_type_enum::INTEGER,flex_type_enum::INTEGER},
                                "", 1);
  auto out = component_info.get_output_iterator(0);
  std::vector<flexible_type> row(2, (flex_int)(0));
  for (size_t i = 0; i < component_sizes.size(); ++i) {
    if (component_sizes[i] > 0) {
      row[0] = i;
      row[1] = (flex_int)component_sizes[i];
      *out++ = row;
    }
  }
  component_info.close();

  return component_info;
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

  sframe components = compute_connected_component(g);
  std::shared_ptr<unity_sframe> components_wrapper(new unity_sframe());
  components_wrapper->construct_from_sframe(components);

  std::shared_ptr<unity_sgraph> result_graph(new unity_sgraph(std::make_shared<sgraph>(g)));


  variant_map_type model_params;
  model_params["graph"] = to_variant(result_graph);
  model_params["component_id"] = to_variant(result_graph->get_vertices());
  model_params["training_time"] = mytimer.current_time();
  model_params["component_size"] = to_variant(components_wrapper);

  variant_map_type response;
  response["model"] = to_variant(std::make_shared<simple_model>(model_params));
  return response;
}

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

} // end of namespace connected components
} // end of namespace turi
