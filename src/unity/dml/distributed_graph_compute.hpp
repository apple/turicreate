/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef GRAPHALB_DML_DISTRIBUTED_GRAPH_COMPUTE_HPP
#define GRAPHALB_DML_DISTRIBUTED_GRAPH_COMPUTE_HPP

#include <rpc/dc.hpp>
#include <rpc/dc_global.hpp>
#include <rpc/dc_dist_object.hpp>
#include <parallel/pthread_tools.hpp>

#include <unity/dml/distributed_graph.hpp>

namespace turi {
namespace distributed_sgraph_compute {

/**************************************************************************/
/*                                                                        */
/*                             Graph Combiner                             */
/*                                                                        */
/**************************************************************************/
enum class combiner_filter { SRC, DST, ALL };

/**
 * This is a reusable combiner object.
 * It is expensive to create, so you should only make one of it for each
 * combination of T and Combiner type. Essentially:
 *
 * \code
 * combiner combiner(dc);
 *
 * combiner.perform_combine(graph, data, fn);
 *
 * \endcode
 * Combiner must be a function of the form
 * combiner(left, right) and mutates left with const right
 */
template <typename T, typename Combiner>
class combiner {
 public:
  combiner(distributed_control& dc,
           const Combiner& value_combiner)
    : rmi(dc, this), value_combiner(value_combiner) { }

  void perform_combine(distributed_graph& g,
                       std::vector<T>& v,
                       combiner_filter filter=combiner_filter::ALL) {
    ASSERT_EQ(v.size(), g.num_partitions());
    master_locks.resize(v.size());
    data = &v;
    graph = &g;
    rmi.barrier();
    send_to_masters(filter);
    send_to_children();
  }

  void perform_sync(distributed_graph& g,
      std::vector<T>& v) {
    ASSERT_EQ(v.size(), g.num_partitions());
    data = &v;
    graph = &g;
    rmi.barrier();
    send_to_children();
  }

 private:
  void send_to_masters(combiner_filter filter) {
    std::vector<size_t> my_parts;
    if (filter == combiner_filter::SRC) {
      my_parts = graph->my_src_vertex_partitions();
    } else if (filter == combiner_filter::DST) {
      my_parts = graph->my_dst_vertex_partitions();
    } else {
      my_parts = graph->my_vertex_partitions();
    }

    parallel_for(0, my_parts.size(), [&](size_t idx) {
      auto coord = my_parts[idx];
      // send to master if I am not master
      auto master = graph->get_partition_master(coord);
      if (master != rmi.procid()) {
        logstream(LOG_INFO) << "[Proc " << rmi.procid() << "] call to proc "
                            << master << " for partition " << coord
                            << " of size " << (*data)[coord].size()
                            << std::endl;
        rmi.RPC_CALL(remote_call, combiner::receive_from_children)(master, coord, (*data)[coord]);
      }
    });
    rmi.full_barrier();
  }

  void send_to_children() {
    for (size_t coord = 0; coord < graph->num_partitions(); ++coord) {
      auto worker_list  = graph->get_partition_workers(coord);
      // if I am master for this partition and if there are children
      if (graph->is_master_of_partition(coord) && worker_list.size() > 1) {
        rmi.RPC_CALL(broadcast_call, combiner::receive_from_master)(
            worker_list.begin() + 1, // skip the first worker, which is master
            worker_list.end(),
            coord,
            (*data)[coord]);
      }
    }
    rmi.full_barrier();
  }

  void receive_from_children(size_t partition, const T& child_values) {
    logstream(LOG_INFO) << "[Proc " << rmi.procid() << "] receive "
                        << "partition " << partition
                        << " of size " << child_values.size()
                        << std::endl;
    // ASSERT_EQ(child_values.size(), data->at(partition).size());
    ASSERT_LT(partition, data->size());
    std::lock_guard<turi::mutex> guard(master_locks[partition]);
    auto& my_values = (*data)[partition];
    value_combiner(my_values, child_values);
  }

  void receive_from_master(size_t partition, T& master_value) {
    // logstream(LOG_INFO) << "Receiving Call for " << partition << std::endl;
    // ASSERT_EQ(master_value.size(), data->at(partition).size());
    ASSERT_LT(partition, data->size());
    (*data)[partition] = std::move(master_value);
  }

  dc_dist_object<combiner> rmi;
  Combiner value_combiner;
  distributed_graph* graph;
  std::vector<T>* data;
  std::vector<turi::mutex> master_locks;
};

/**************************************************************************/
/*                                                                        */
/*                            Utility Function                            */
/*                                                                        */
/**************************************************************************/
inline void fast_triple_apply(distributed_graph& g,
    sgraph_compute::fast_triple_apply_fn_type apply_fn,
    const std::vector<std::string>& edge_fields = std::vector<std::string>()) {

  // do not support mutate edge fields
  std::vector<std::string> mutated_edge_fields;

  sgraph glocal = g.local_graph();

  // Select edge fields required for computation
  std::vector<std::string> edge_columns{sgraph::SRC_COLUMN_NAME, sgraph::DST_COLUMN_NAME};
  for (auto f: edge_fields) { edge_columns.push_back(f); }
  for (auto f: mutated_edge_fields) { edge_columns.push_back(f); }

  glocal.select_edge_fields(edge_columns);
  sgraph_compute::fast_triple_apply(glocal, apply_fn,
                                    edge_fields,
                                    mutated_edge_fields);
}

template<typename VertexData, typename F>
inline void vertex_apply(distributed_graph& g, std::vector<VertexData>& vdata, F apply_func) {
  for (auto partition_id: g.my_master_vertex_partitions()) {
    apply_func(vdata[partition_id], partition_id);
  }
  auto dc = distributed_control::get_instance();
  auto dummy_combine_fn = [](VertexData& a, const VertexData& b)->void {};
  combiner<VertexData, decltype(dummy_combine_fn)> dummy_combiner(*dc, dummy_combine_fn);
  dummy_combiner.perform_sync(g, vdata);
}

/**
 * Create a vector of size_t num_partition, each of the element
 * is created with BlockAllocator with the input of
 * number of vertices in the local graph.
 *
 * Type T is the type of the vertex partition data to be allocated,
 * it can be std::vector<flexible_type>, or EIGTODO Matrices
 *
 * BlockAllocator is a function that takes a size_t and return
 * data of type T.
 */
template <typename T, typename BlockAllocator>
std::vector<T>
create_partition_aligned_vertex_data(const distributed_graph& graph,
                                     BlockAllocator block_allocator) {
  std::vector<T> ret(graph.num_partitions());
  for (auto coord: graph.my_vertex_partitions()) {
    size_t partition_size = graph.num_vertices(coord);
    ret[coord] = std::move(block_allocator(partition_size));
  }
  return ret;
}

/**
 * Returns a vector of size num_partition, where vec[i] is the
 * vertex data of that partition i, if current machine is the master
 * of partition i. Otherwise, vec[i] is empty.
 */
inline std::vector<std::vector<flexible_type>>
get_vertex_data_of_master_partitions(distributed_graph& graph, std::string field_name) {
  std::vector<std::vector<flexible_type>> ret;
  ret.resize(graph.num_partitions());
  for (auto coord: graph.my_master_vertex_partitions()) {
    std::vector<flexible_type> buffer;
    auto vals = graph.local_graph().vertex_partition(coord).select_column(field_name);
    size_t rows_read = vals->get_reader()->read_rows(0, vals->size(), buffer);
    ret[coord] = buffer;
  }
  return ret;
}

} // end distributed_sgraph_compute
} // end turicreate
#endif
