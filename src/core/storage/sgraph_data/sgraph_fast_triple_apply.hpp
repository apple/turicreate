/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_SGRAPH_FAST_TRIPLE_APPLY
#define TURI_SGRAPH_SGRAPH_FAST_TRIPLE_APPLY

#include<core/data/flexible_type/flexible_type.hpp>
#include<core/storage/sgraph_data/sgraph.hpp>
#include<core/storage/sgraph_data/sgraph_compute_vertex_block.hpp>

namespace turi {


/**
 * \ingroup sgraph_physical
 * \addtogroup sgraph_compute SGraph Compute
 * \{
 */

/**
 * Graph Computation Functions
 */
namespace sgraph_compute {

// Vertex address is represented by its partition id, and local index in the partition.
struct vertex_address {
  size_t partition_id;
  size_t local_id;
};

typedef std::vector<flexible_type> edge_data;

/**
 * Provide access to an edge scope (Vertex, Edge, Vertex);
 * The scope object permits read, modify both vertex data
 * and the edge data. See \ref fast_triple_apply
 */
class fast_edge_scope {
 public:
  /// Provide edge data access
  edge_data& edge() { return *m_edge; }

  const edge_data& edge() const { return *m_edge; }

  vertex_address source_vertex_address() { return m_source_addr; }

  vertex_address target_vertex_address() { return m_target_addr; }

  /// Do not construct edge_scope directly. Used by triple_apply_impl.
  fast_edge_scope(const vertex_address& source_addr,
                  const vertex_address&  target_addr,
                  edge_data* edge) :
      m_source_addr(source_addr), m_target_addr(target_addr), m_edge(edge) { }

 private:
  vertex_address m_source_addr;
  vertex_address m_target_addr;
  edge_data* m_edge;
};

typedef std::function<void(fast_edge_scope&)> fast_triple_apply_fn_type;

/**
 * A faster and simplified version of triple_apply.
 *
 * The "faster" assumption is based on that vertex data can be loaded entirey
 * into memory and accessed by the apply function through addressing.
 *
 * The interface made it possible for vertex data to stay in memory *across*
 * multiple triple applies before commiting to the disk.
 *
 * Main interface difference:
 * 1. Vertex data are provided as vertex address, allowing user to specify
 * their own vertex data storage.
 * 2. Allowing user to explicitly specify which edge fields are required
 * to read and mutate.
 * 3. Vertex locking is ommited for simplification. (we can add it later if needed).
 *
 * \param g The target graph to perform the transformation.
 * \param apply_fn The user defined function that will be applied on each edge scope.
 * \param vertex_fields A subset of vertex data columns that the apply_fn will access.
 * \param mutated_vertex_fields A subset of columns in \ref vertex_fields that the apply_fn will modify.
 */
void fast_triple_apply(sgraph& g,
                       fast_triple_apply_fn_type apply_fn,
                       const std::vector<std::string>& edge_fields,
                       const std::vector<std::string>& mutated_edge_fields);


/**
 * Utility function
 */
template<typename T>
std::vector<std::vector<T>> create_vertex_data(const sgraph& g) {
  std::vector<std::vector<T>> ret(g.get_num_partitions());
  for (size_t i = 0; i < g.get_num_partitions(); ++i) {
    ret[i] = std::vector<T>(g.vertex_partition(i).size());
  }
  return ret;
}

template<typename T>
std::vector<std::vector<T>> create_vertex_data_from_const(const sgraph& g, const T& init) {
  std::vector<std::vector<T>> ret(g.get_num_partitions());
  for (size_t i = 0; i < g.get_num_partitions(); ++i) {
    ret[i] = std::vector<T>(g.vertex_partition(i).size(), init);
  }
  return ret;
}


}

/// \}
}

#endif
