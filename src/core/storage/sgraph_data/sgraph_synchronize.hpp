/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_SGRAPH_SYNCHRONIZE_HPP
#define TURI_SGRAPH_SGRAPH_SYNCHRONIZE_HPP

#include<core/storage/sgraph_data/sgraph_synchronize_interface.hpp>
#include<core/logging/assertions.hpp>

namespace turi {
/**
 * \internal
 * \ingroup sgraph_physical
 * \addtogroup sgraph_compute_internal SGraph Compute Internal
 * \{
 */

/**
 * Graph Computation Functions
 */
namespace sgraph_compute {

/**
 * An implementation of \ref sgraph_synchronize_interface to exchange
 * information about an sgraph.
 *
 * The main application of this is for communication of graph information.
 *
 * \ref vertex_partition_exchange is the struct that can hold
 * a subset data of a subset of vertices from a sgraph partition.
 *
 * The choice for sparse vertices packing is motivated by the
 * "triple_apply" computation pattern:
 * as we process each edge partition, the associated vertex partition
 * are sparsely visited and updated.
 *
 * \ref sgraph_synchronize_interface is used for both ends of the communication
 * to deal with initialization, sending and receiving the vertex and edge exchange data.
 *
 * Example:
 * \code
 *
 * ////  Worker side ////
 * sgraph_synchronize_interface* worker_graph_sync; // a client side implementation
 *
 * // initialize vertex data in partition 0, using all_vertices sent from server.
 * worker_graph_sync.load_vertex_partition(0, all_vertices_from_server);
 *
 * // recevie a vertex_exchange from server, let's update the local vertices
 * worker_graph_sync->update_vertex_partition(vexchange_from_server);
 *
 * // do some work to update vertex data in partition 0, and obtain a set of changed vertex data..
 * vertex_exchange updated_vertices = worker_graph_sync->get_vertex_partition_exchange(0);
 *
 * // now we can send updated_vertices to the server.
 *
 * //// Server side ////
 * sgraph_synchronize_interface* server_graph_sync; // a server side implementation
 *
 * // call to worker to initialize vertex partition.
 *
 * // recevie a vertex_exchange server, let's update the local vertices
 * server_graph_sync->update_vertex_partition(vexchange_from_client);
 *
 * // do some work to update vertex data in partition 0, and obtain a set of changed vertex data..
 * vertex_exchange updated_vertices = server_graph_sync->get_vertex_partition_exchange(0);
 *
 * // now we can send updated_vertices to the worker.
 *
 * \endcode
 *
 * \note This really is implementation detail and is used to allow graph
 * computation methods that are implemented in Python
 */
class sgraph_synchronize : public sgraph_synchronize_interface {
 public:

  sgraph_synchronize() { }

  sgraph_synchronize(size_t num_partitions) {
    init(num_partitions);
  }

  void init(size_t num_partitions) {
    m_vertex_partitions.clear();
    m_is_partition_loaded.clear();

    m_num_partitions = num_partitions;
    m_vertex_partitions.resize(num_partitions);
    m_is_partition_loaded.resize(num_partitions, false);
  }

  inline void load_vertex_partition(size_t partition_id, std::vector<sgraph_vertex_data>& vertices) {
    DASSERT_LT(partition_id, m_num_partitions);
    DASSERT_FALSE(m_is_partition_loaded[partition_id]);
    m_vertex_partitions[partition_id] = &vertices;
    m_is_partition_loaded[partition_id] = true;
  }

  inline void update_vertex_partition(vertex_partition_exchange& vpartition_exchange) {
    DASSERT_TRUE(m_is_partition_loaded[vpartition_exchange.partition_id]);

    auto& vertex_partition = *(m_vertex_partitions[vpartition_exchange.partition_id]);
    auto& update_field_index = vpartition_exchange.field_ids;

    for (auto& vid_data_pair : vpartition_exchange.vertices) {
      size_t id = vid_data_pair.first;
      sgraph_vertex_data& vdata = vid_data_pair.second;
      for (size_t i = 0; i < update_field_index.size(); ++i) {
        size_t fid = vpartition_exchange.field_ids[i];
        vertex_partition[id][fid] = vdata[i];
      }
    }
  }

  inline vertex_partition_exchange get_vertex_partition_exchange(size_t partition_id, const std::unordered_set<size_t>& vertex_ids, const std::vector<size_t>& field_ids) {
    DASSERT_TRUE(m_is_partition_loaded[partition_id]);
    vertex_partition_exchange ret;
    ret.partition_id = partition_id;
    ret.field_ids = field_ids;
    auto& vertex_partition = *(m_vertex_partitions[partition_id]);
    for (size_t vid:  vertex_ids) {
      auto& vdata = vertex_partition[vid];
      sgraph_vertex_data vdata_subset;
      for (auto fid: field_ids)  vdata_subset.push_back(vdata[fid]);
      ret.vertices.push_back({vid, std::move(vdata_subset)});
    }
    return ret;
  }

 private:
  std::vector<std::vector<sgraph_vertex_data>*> m_vertex_partitions;
  std::vector<bool> m_is_partition_loaded;
  size_t m_num_partitions;
};

} // end sgraph_compute
} // end turicreate

#endif
