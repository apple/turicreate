#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/query_engine/execution/execution_node.hpp>
#include <core/storage/query_engine/operators/sarray_source.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/algorithm.hpp>

#include "check_node.hpp"

using namespace turi;
using namespace turi::query_eval;

struct sarray_source_test  {
 public:
  void test_empty_source() {
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    sa->close();
    auto node = make_node(sa);
    check_node(node, std::vector<flexible_type>());
  }

  void test_simple_sarray() {
    std::vector<flexible_type> expected {0,1,2,3,4,5};
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(expected.begin(), expected.end(), *sa);
    sa->close();
    auto node = make_node(sa);
    check_node(node, expected);
  }

  std::shared_ptr<execution_node> make_node(std::shared_ptr<sarray<flexible_type>> source) {
    auto node = std::make_shared<execution_node>(std::make_shared<op_sarray_source>(source));
    return node;
  }

};

BOOST_FIXTURE_TEST_SUITE(_sarray_source_test, sarray_source_test)
BOOST_AUTO_TEST_CASE(test_empty_source) {
  sarray_source_test::test_empty_source();
}
BOOST_AUTO_TEST_CASE(test_simple_sarray) {
  sarray_source_test::test_simple_sarray();
}
BOOST_AUTO_TEST_SUITE_END()
