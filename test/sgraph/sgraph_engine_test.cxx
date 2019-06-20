#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/sgraph_data/sgraph.hpp>
#include <core/storage/sgraph_data/sgraph_engine.hpp>

#include "sgraph_test_util.hpp"
#include "sgraph_check_degree_count.hpp"
#include "sgraph_check_pagerank.hpp"

using namespace turi;

// Implement degree count function using sgraph_engine
std::vector<std::pair<flexible_type, flexible_type>> degree_count_fn (
  sgraph& g, sgraph::edge_direction dir) {

  sgraph_compute::sgraph_engine<flexible_type> ga;
  typedef sgraph_compute::sgraph_engine<flexible_type>::graph_data_type graph_data_type;
  typedef sgraph::edge_direction edge_direction;
  std::vector<std::shared_ptr<sarray<flexible_type>>> 
      gather_results = ga.gather(g,
                                 [](const graph_data_type& center, 
                                    const graph_data_type& edge, 
                                    const graph_data_type& other, 
                                    edge_direction edgedir,
                                    flexible_type& combiner) {
                                   combiner = combiner + 1;
                                 },
                                 flexible_type(0),
                                 dir);
  std::vector<std::shared_ptr<sarray<flexible_type>>> vertex_ids 
      = g.fetch_vertex_data_field(sgraph::VID_COLUMN_NAME);

  TS_ASSERT_EQUALS(gather_results.size(), vertex_ids.size());
  std::vector<std::pair<flexible_type, flexible_type>> ret;

  for (size_t i = 0;i < gather_results.size(); ++i) {
    std::vector<flexible_type> degree_vec;
    std::vector<flexible_type> id_vec;
    gather_results[i]->get_reader()->read_rows(0, g.num_vertices(), degree_vec);
    vertex_ids[i]->get_reader()->read_rows(0, g.num_vertices(), id_vec);
    TS_ASSERT_EQUALS(degree_vec.size(), id_vec.size());
    for (size_t j = 0; j < degree_vec.size(); ++j) {
      ret.push_back({id_vec[j], degree_vec[j]});
    }
  }
  return ret;
}

// Implement degree count function using sgraph_engine
void pagerank_fn(sgraph& g,  size_t num_iterations) {
  sgraph_compute::sgraph_engine<flexible_type> ga;
  typedef sgraph_compute::sgraph_engine<flexible_type>::graph_data_type graph_data_type;
  typedef sgraph::edge_direction edge_direction;
  // count the outgoing degree
  std::vector<std::shared_ptr<sarray<flexible_type>>> vertex_combine = ga.gather(g,
                                                      [](const graph_data_type& center, 
                                                         const graph_data_type& edge, 
                                                         const graph_data_type& other, 
                                                         edge_direction edgedir,
                                                         flexible_type& combiner) {
                                                      combiner = combiner + 1;
                                                      },
                                                      flexible_type(0),
                                                      edge_direction::OUT_EDGE);
  // merge the outgoing degree to graph
  std::vector<sframe>& vdata = g.vertex_group();
  for (size_t i = 0; i < g.get_num_partitions(); ++i) {
    ASSERT_LT(i, vdata.size());
    ASSERT_LT(i, vertex_combine.size());
    vdata[i] = vdata[i].add_column(vertex_combine[i], "__out_degree__");
  }

  size_t degree_idx = vdata[0].column_index("__out_degree__");
  size_t data_idx = vdata[0].column_index("vdata");

  // now we compute the pagerank
  for (size_t iter = 0; iter < num_iterations; ++iter) {
    vertex_combine = ga.gather(g,
        [=](const graph_data_type& center,
            const graph_data_type& edge,
            const graph_data_type& other,
            edge_direction edgedir,
            flexible_type& combiner) {
           combiner = combiner + 0.85 * (other[data_idx] / other[degree_idx]);
        },
        flexible_type(0.15),
        edge_direction::IN_EDGE);
    for (size_t i = 0; i < g.get_num_partitions(); ++i) {
      vdata[i] = vdata[i].replace_column(vertex_combine[i], "vdata");
    }
    // g.get_vertices().debug_print();
  }
}

struct sgraph_engine_test {

public:
 void test_degree_count() {
   check_degree_count(degree_count_fn);
 }

 void test_pagerank() {
   check_pagerank(pagerank_fn);
 }

};

BOOST_FIXTURE_TEST_SUITE(_sgraph_engine_test, sgraph_engine_test)
BOOST_AUTO_TEST_CASE(test_degree_count) {
  sgraph_engine_test::test_degree_count();
}
BOOST_AUTO_TEST_CASE(test_pagerank) {
  sgraph_engine_test::test_pagerank();
}
BOOST_AUTO_TEST_SUITE_END()
