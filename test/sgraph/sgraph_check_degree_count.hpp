/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SGRAPH_TEST_DEGREE_COUNT_HPP
#define TURI_SGRAPH_TEST_DEGREE_COUNT_HPP

#include <core/storage/sgraph_data/sgraph.hpp>
#include "sgraph_test_util.hpp"

using namespace turi;

typedef std::function<
  std::vector<std::pair<flexible_type, flexible_type>>(sgraph&,
                                                       sgraph::edge_direction)>
  degree_count_fn_type;

// Takes a degree count function (graph, DIR) -> [(id, degree), (id_degree)]
// Check it computes the right degree on various of graphs
void check_degree_count(degree_count_fn_type degree_count_fn) {
  size_t n_vertex = 1000;
  size_t n_partition = 4;

  typedef sgraph::edge_direction edge_direction;
  {
    // for single directional ring graph
    sgraph g = create_ring_graph(n_vertex, n_partition, false /* one direction */);
    std::vector<std::pair<flexible_type, flexible_type>> in_degree = degree_count_fn(g, edge_direction::IN_EDGE);
    std::vector<std::pair<flexible_type, flexible_type>> out_degree = degree_count_fn(g, edge_direction::OUT_EDGE);
    std::vector<std::pair<flexible_type, flexible_type>> total_degree = degree_count_fn(g, edge_direction::ANY_EDGE);
    TS_ASSERT_EQUALS(in_degree.size(), g.num_vertices());
    TS_ASSERT_EQUALS(out_degree.size(), g.num_vertices());
    TS_ASSERT_EQUALS(total_degree.size(), g.num_vertices());
    for (size_t i = 0; i < g.num_vertices(); ++i) {
      TS_ASSERT_EQUALS((int)out_degree[i].second, 1);
      TS_ASSERT_EQUALS((int)in_degree[i].second, 1);
      TS_ASSERT_EQUALS((int)total_degree[i].second, 2);
      TS_ASSERT_EQUALS((int)out_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS((int)in_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS((int)total_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
    }
  }

  {
    // for bi-directional ring graph
    sgraph g = create_ring_graph(n_vertex, n_partition, true /* bi direction */);
    std::vector<std::pair<flexible_type, flexible_type>> in_degree = degree_count_fn(g, edge_direction::IN_EDGE);
    std::vector<std::pair<flexible_type, flexible_type>> out_degree = degree_count_fn(g, edge_direction::OUT_EDGE);
    std::vector<std::pair<flexible_type, flexible_type>> total_degree = degree_count_fn(g, edge_direction::ANY_EDGE);
    TS_ASSERT_EQUALS(in_degree.size(), g.num_vertices());
    TS_ASSERT_EQUALS(out_degree.size(), g.num_vertices());
    TS_ASSERT_EQUALS(total_degree.size(), g.num_vertices());
    for (size_t i = 0; i < g.num_vertices(); ++i) {
      TS_ASSERT_EQUALS((int)out_degree[i].second, 2);
      TS_ASSERT_EQUALS((int)in_degree[i].second, 2);
      TS_ASSERT_EQUALS((int)total_degree[i].second, 4);
      TS_ASSERT_EQUALS((int)out_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS((int)in_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS((int)total_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
    }
  }

  {
    // for star graph
    sgraph g = create_star_graph(n_vertex, n_partition);
    std::vector<std::pair<flexible_type, flexible_type>> in_degree = degree_count_fn(g, edge_direction::IN_EDGE);
    std::vector<std::pair<flexible_type, flexible_type>> out_degree = degree_count_fn(g, edge_direction::OUT_EDGE);
    std::vector<std::pair<flexible_type, flexible_type>> total_degree = degree_count_fn(g, edge_direction::ANY_EDGE);
    TS_ASSERT_EQUALS(in_degree.size(), g.num_vertices());
    TS_ASSERT_EQUALS(out_degree.size(), g.num_vertices());
    TS_ASSERT_EQUALS(total_degree.size(), g.num_vertices());
    for (size_t i = 0; i < g.num_vertices(); ++i) {
      TS_ASSERT_EQUALS((int)in_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS((int)out_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
      TS_ASSERT_EQUALS((int)total_degree[i].second.get_type(), (int)flex_type_enum::INTEGER);
      if (in_degree[i].first == 0) {
        TS_ASSERT_EQUALS((int)in_degree[i].second, n_vertex -1);
      } else {
        TS_ASSERT_EQUALS((int)in_degree[i].second, 0);
      }
      if (out_degree[i].first == 0) {
        TS_ASSERT_EQUALS((int)out_degree[i].second, 0);
      } else {
        TS_ASSERT_EQUALS((int)out_degree[i].second, 1);
      }
      if (total_degree[i].first == 0) {
        TS_ASSERT_EQUALS((int)total_degree[i].second, n_vertex -1);
      } else {
        TS_ASSERT_EQUALS((int)total_degree[i].second, 1);
      }
      }
    }
}

#endif
