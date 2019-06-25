/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_SGRAPH_TRIPLE_APPLY
#define TURI_SGRAPH_SGRAPH_TRIPLE_APPLY

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

typedef std::vector<flexible_type> vertex_data;
typedef std::vector<flexible_type> edge_data;
typedef sgraph::vertex_partition_address vertex_partition_address;
typedef sgraph::edge_partition_address edge_partition_address;

/**
 * Provide access to an edge scope (Vertex, Edge, Vertex);
 * The scope object permits read, modify both vertex data
 * and the edge data. See \ref triple_apply
 */
class edge_scope {
 public:
  /// Provide vertex data access
  vertex_data& source() { return *m_source; }

  const vertex_data& source() const { return *m_source; }

  vertex_data& target() { return *m_target; }

  const vertex_data& target() const { return *m_target; }

  /// Provide edge data access
  edge_data& edge() { return *m_edge; }

  const edge_data& edge() const { return *m_edge; }

  /// Lock both source and target vertices
  // The lock pointers can be null in which case the function has no effect.
  void lock_vertices() {
    if (m_lock_0 && m_lock_1) {
      if (m_lock_0 == m_lock_1) {
        m_lock_0->lock();
      } else {
        m_lock_0->lock();
        m_lock_1->lock();
      }
    }
  };

  /// Unlock both source and target vertices
  void unlock_vertices() {
    if (m_lock_0 && m_lock_1) {
      if (m_lock_0 == m_lock_1) {
        m_lock_0->unlock();
      } else {
        m_lock_0->unlock();
        m_lock_1->unlock();
      }
    }
  };

  /// Do not construct edge_scope directly. Used by triple_apply_impl.
  edge_scope(vertex_data* source, vertex_data* target, edge_data* edge,
             turi::mutex* lock_0=NULL,
             turi::mutex* lock_1=NULL) :
      m_source(source), m_target(target), m_edge(edge),
      m_lock_0(lock_0), m_lock_1(lock_1) { }

 private:
  vertex_data* m_source;
  vertex_data* m_target;
  edge_data* m_edge;
  // On construction, the lock ordering is gauanteed: lock_0 < lock_1.
  turi::mutex* m_lock_0;
  turi::mutex* m_lock_1;
};

typedef std::function<void(edge_scope&)> triple_apply_fn_type;

typedef std::function<void(std::vector<edge_scope>&)> batch_triple_apply_fn_type;

/**
 * Apply a transform function on each edge and its associated source and target vertices in parallel.
 * Each edge is visited once and in parallel. The modification to vertex data will be protected by lock.
 *
 * The effect of the function is equivalent to the following pesudo-code:
 * \code
 * parallel_for (edge in g) {
 *  lock(edge.source(), edge.target())
 *  apply_fn(edge.source().data(), edge.data(), edge.target().data());
 *  unlock(edge.source(), edge.target())
 * }
 * \endcode
 *
 * \param g The target graph to perform the transformation.
 * \param apply_fn The user defined function that will be applied on each edge scope.
 * \param mutated_vertex_fields A subset of vertex data columns that the apply_fn will modify.
 * \param mutated_edge_fields A subset of edge data columns that the apply_fn will modify.
 * \param requires_vertex_id Set to false for optimization when vertex id is not required
 *        for triple_apply computation.
 *
 * The behavior is undefined when mutated_vertex_fields, and mutated_edge_fields are
 * inconsistent with the apply_fn function.
 */
void triple_apply(sgraph& g,
                  triple_apply_fn_type apply_fn,
                  const std::vector<std::string>& mutated_vertex_fields,
                  const std::vector<std::string>& mutated_edge_fields = {},
                  bool requires_vertex_id = true);


#ifdef TC_HAS_PYTHON
/**
 * Overload. Uses python lambda function.
 */
void triple_apply(sgraph& g, const std::string& lambda_str,
                  const std::vector<std::string>& mutated_vertex_fields,
                  const std::vector<std::string>& mutated_edge_fields = {});

#endif


/**************************************************************************/
/*                                                                        */
/*                        Internal. Test Only API                         */
/*                                                                        */
/**************************************************************************/

/**
 * Overload. Take the apply function that processes a batch of edges at once.
 * Used for testing the building block of lambda triple apply.
 */
void triple_apply(sgraph& g,
                  batch_triple_apply_fn_type batch_apply_fn,
                  const std::vector<std::string>& mutated_vertex_fields,
                  const std::vector<std::string>& mutated_edge_fields = {});

/**
 * Mock the single triple apply using batch_triple_apply implementation.
 * Used for testing only.
 */
void batch_triple_apply_mock(sgraph& g,
                  triple_apply_fn_type apply_fn,
                  const std::vector<std::string>& mutated_vertex_fields,
                  const std::vector<std::string>& mutated_edge_fields = {});
}

/// \}
}

#endif
