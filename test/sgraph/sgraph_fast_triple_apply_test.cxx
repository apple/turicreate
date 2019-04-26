#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <sgraph/sgraph.hpp>
#include <sgraph/sgraph_fast_triple_apply.hpp>

#include "sgraph_test_util.hpp"
#include "sgraph_check_degree_count.hpp"
#include "sgraph_check_pagerank.hpp"

using namespace turi;

// Implement degree count function using triple_apply 
std::vector<std::pair<flexible_type, flexible_type>> triple_apply_degree_count(
  sgraph& g, sgraph::edge_direction dir) {

  sgraph_compute::fast_triple_apply_fn_type fn;
  g.init_vertex_field("__degree__", flex_int(0));
  std::vector<std::string> vertex_fields = g.get_vertex_fields();

  auto vertex_degree_data = sgraph_compute::create_vertex_data<turi::atomic<size_t>>(g);

  if (dir == sgraph::edge_direction::IN_EDGE) {
    fn = [&](sgraph_compute::fast_edge_scope& scope) {
      auto target_addr = scope.target_vertex_address();
      vertex_degree_data[target_addr.partition_id][target_addr.local_id]++;
    };
  } else if (dir == sgraph::edge_direction::OUT_EDGE) {
    fn = [&](sgraph_compute::fast_edge_scope& scope) {
      auto source_addr = scope.source_vertex_address();
      vertex_degree_data[source_addr.partition_id][source_addr.local_id]++;
    };
  } else {
    fn = [&](sgraph_compute::fast_edge_scope& scope) {
      auto target_addr = scope.target_vertex_address();
      vertex_degree_data[target_addr.partition_id][target_addr.local_id]++;
      auto source_addr = scope.source_vertex_address();
      vertex_degree_data[source_addr.partition_id][source_addr.local_id]++;
    };
  }

  sgraph_compute::fast_triple_apply(g, fn, {}, {});

  std::vector<std::pair<flexible_type, flexible_type>> ret;
  auto vertex_ids = g.fetch_vertex_data_field("__id");

  for (size_t i = 0; i < vertex_degree_data.size(); ++i) {
    std::vector<flexible_type> id_vec;
    vertex_ids[i]->get_reader()->read_rows(0, vertex_ids[i]->size(), id_vec);
    TS_ASSERT_EQUALS(id_vec.size(), vertex_degree_data[i].size());
    for (size_t j = 0; j < id_vec.size(); ++j) {
      ret.push_back({id_vec[j], (size_t)vertex_degree_data[i][j]});
    }
  }
  return ret;
}

struct sgraph_triple_apply_test  {

public:

void test_triple_apply_degree_count() {
  check_degree_count(triple_apply_degree_count);
}

void test_triple_apply_edge_data_modification() {
  // Create an edge field, and assign it the value of the sum of source and target ids.
  size_t n_vertex = 10;
  size_t n_partition = 4;
  sgraph g = create_ring_graph(n_vertex, n_partition, false /* one direction */);

  g.init_edge_field("id_sum", flex_int(0));
  size_t field_id = 2;

  std::vector<std::vector<flexible_type>> vdata = g.fetch_vertex_data_field_in_memory("__id");

  sgraph_compute::fast_triple_apply(g,
                               [&](sgraph_compute::fast_edge_scope& scope) {
                                 auto src_addr = scope.source_vertex_address();
                                 auto dst_addr = scope.target_vertex_address();

                                 auto src_id = vdata[src_addr.partition_id][src_addr.local_id];
                                 auto dst_id = vdata[dst_addr.partition_id][dst_addr.local_id];

                                 scope.edge()[field_id] = src_id + dst_id; 
                                                          
                               }, {"id_sum"}, {"id_sum"});

  sframe edge_sframe = g.get_edges();
  std::vector<std::vector<flexible_type>> edge_data_rows;
  edge_sframe.get_reader()->read_rows(0, edge_sframe.size(), edge_data_rows);
  for (auto& row : edge_data_rows) {
    TS_ASSERT_EQUALS(int(row[0] + row[1]), int(row[3]));
  }
  g.remove_edge_field("id_sum");
}

};

BOOST_FIXTURE_TEST_SUITE(_sgraph_triple_apply_test, sgraph_triple_apply_test)
BOOST_AUTO_TEST_CASE(test_triple_apply_degree_count) {
  sgraph_triple_apply_test::test_triple_apply_degree_count();
}
BOOST_AUTO_TEST_CASE(test_triple_apply_edge_data_modification) {
  sgraph_triple_apply_test::test_triple_apply_edge_data_modification();
}
BOOST_AUTO_TEST_SUITE_END()
