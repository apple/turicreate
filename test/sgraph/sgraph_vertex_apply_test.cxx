#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/sgraph_data/sgraph.hpp>
#include <core/storage/sgraph_data/sgraph_vertex_apply.hpp>
#include "sgraph_test_util.hpp"

using namespace turi;

struct sgraph_vertex_apply_test {

public:

void check_vertex_apply_result(std::vector<std::shared_ptr<sarray<flexible_type>>>& val) {
  for(auto& iter: val) {
    std::vector<flexible_type> ret;
    turi::copy(*iter, std::inserter(ret, ret.end()));
    for (auto& retval: ret) {
      TS_ASSERT_EQUALS((int)retval.get_type(), (int)flex_type_enum::FLOAT);
      TS_ASSERT_EQUALS(retval.get<double>(), 2.0);
    }
  }
}

void test_vertex_apply() {
  // there are 4 overloads
  size_t n_vertex = 10;
  size_t n_partition = 2;
  sgraph ring_graph  = create_ring_graph(n_vertex, n_partition, false);
  size_t data_index = ring_graph.vertex_group(0)[0].column_index("vdata");
  // map data + 1 = 2.0
  auto ret = sgraph_compute::vertex_apply(ring_graph,
                                          flex_type_enum::FLOAT,
                                          [=](const std::vector<flexible_type>& val){
                                            TS_ASSERT_LESS_THAN(data_index, val.size());
                                            return val[data_index] + 1.0;
                                          });
  check_vertex_apply_result(ret);


  // map data + prevret / 2 = 2.0
  ret = sgraph_compute::vertex_apply(ring_graph,
                                     ret,
                                     flex_type_enum::FLOAT,
                                     [=](const std::vector<flexible_type>& val, flexible_type prev_ret){
                                       TS_ASSERT_LESS_THAN(data_index, val.size());
                                       return val[data_index] + prev_ret / 2; 
                                     });
  check_vertex_apply_result(ret);



  // map data + prevret / 2 = 2.0
  ret = sgraph_compute::vertex_apply(ring_graph,
                                     "vdata",
                                     ret,
                                     flex_type_enum::FLOAT,
                                     [=](const flexible_type& val, flexible_type prev_ret){
                                       return val + prev_ret / 2; 
                                     });
  check_vertex_apply_result(ret);


  // map data + 1 = 2.0
  ret = sgraph_compute::vertex_apply(ring_graph,
                                     "vdata",
                                     flex_type_enum::FLOAT,
                                     [=](const flexible_type& val){
                                       return val + 1.0;
                                     });
  check_vertex_apply_result(ret);

  double vsum = sgraph_compute::vertex_reduce<double>(ring_graph,
                                      [=](const std::vector<flexible_type>& val, double& sum) {
                                        TS_ASSERT_LESS_THAN(data_index, val.size());
                                        sum += (double)val[data_index];
                                      }, 
                                      [=](const double& val, double& sum) {
                                        sum += val; 
                                      });
  TS_ASSERT_EQUALS(vsum, n_vertex);


  flexible_type vsum2 = sgraph_compute::vertex_reduce<flexible_type>(ring_graph,
                                               "vdata",
                                               [=](const flexible_type& val, flexible_type& sum) {
                                                 sum += val;
                                               },
                                               [=](const flexible_type& val, flexible_type& sum) {
                                                 sum += val;
                                               });
  TS_ASSERT_EQUALS(vsum2, n_vertex);

}
};

BOOST_FIXTURE_TEST_SUITE(_sgraph_vertex_apply_test, sgraph_vertex_apply_test)
BOOST_AUTO_TEST_CASE(test_vertex_apply) {
  sgraph_vertex_apply_test::test_vertex_apply();
}
BOOST_AUTO_TEST_SUITE_END()
