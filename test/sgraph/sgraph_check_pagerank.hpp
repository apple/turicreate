/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_TEST_PAGERANK_HPP
#define TURI_SGRAPH_TEST_PAGERANK_HPP

#include <core/storage/sgraph_data/sgraph.hpp>
#include "sgraph_test_util.hpp"

using namespace turi;

typedef std::function<void(sgraph&,  size_t)> pagerank_fn_type;

void check_pagerank(pagerank_fn_type compute_pagerank) {
  size_t n_vertex = 10;
  size_t n_partition = 2;
  {
    // for symmetic ring graph, all vertices should have the same pagerank
    sgraph ring_graph  = create_ring_graph(n_vertex, n_partition, false);
    compute_pagerank(ring_graph, 3);
    sframe vdata = ring_graph.get_vertices();
    size_t data_column_index = vdata.column_index("vdata");
    std::vector<std::vector<flexible_type>> vdata_buffer;
    vdata.get_reader()->read_rows(0, ring_graph.num_vertices(), vdata_buffer);
    for (auto& row : vdata_buffer) {
      TS_ASSERT_EQUALS(row[data_column_index], 1.0);
    }
  }
  {
    // for star graph, the center's pagerank = 0.15 + 0.85 * (n-1)) 
    sgraph star_graph = create_star_graph(n_vertex, n_partition);
    // star_graph.get_edges().debug_print();
    compute_pagerank(star_graph, 3);
    sframe vdata = star_graph.get_vertices();
    // vdata.debug_print();
    size_t id_column_index = vdata.column_index("__id");
    size_t data_column_index = vdata.column_index("vdata");
    std::vector<std::vector<flexible_type>> vdata_buffer;
    vdata.get_reader()->read_rows(0, star_graph.num_vertices(), vdata_buffer);
    for (auto& row : vdata_buffer) {
      if (row[id_column_index] == 0) {
        double expected = 0.15 + 0.85 * 0.15 * (n_vertex-1);
        TS_ASSERT_DELTA(row[data_column_index], expected, 0.0001);
      } else {
        TS_ASSERT_DELTA(row[data_column_index], 0.15, 0.0001);
      }
    }
  }
}
#endif
