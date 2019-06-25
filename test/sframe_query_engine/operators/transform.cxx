#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/query_engine/execution/execution_node.hpp>
#include <core/storage/query_engine/operators/sarray_source.hpp>
#include <core/storage/query_engine/operators/transform.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/algorithm.hpp>

#include "check_node.hpp"

using namespace turi;
using namespace turi::query_eval;

struct transform_test {
 public:
  void test_identity_transform() {
    std::vector<flexible_type> expected {0,1,2,3,4,5};
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(expected.begin(), expected.end(), *sa);
    sa->close();
    auto node = make_node(op_sarray_source(sa),
                          [](const sframe_rows::row& row) { return row[0]; },
                          flex_type_enum::INTEGER);
    check_node(node, expected);
  }

  void test_plus_one() {
    std::vector<flexible_type> data{0,1,2,3,4,5};
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(data.begin(), data.end(), *sa);
    sa->close();
    transform_type f = [](const sframe_rows::row& row) { return row[0] + 1; };
    std::vector<flexible_type> expected(data.size());
    auto f2 = [](const flexible_type& val) { return val + 1; };
    std::transform(data.begin(), data.end(), expected.begin(), f2);
    auto node = make_node(op_sarray_source(sa), f, flex_type_enum::INTEGER);
    check_node(node, expected);
  }

  std::shared_ptr<execution_node> make_node(const op_sarray_source& source, transform_type f, flex_type_enum type) {
    auto source_node = std::make_shared<execution_node>(std::make_shared<op_sarray_source>(source));
    auto node = std::make_shared<execution_node>(std::make_shared<op_transform>(f, type),
                                                 std::vector<std::shared_ptr<execution_node>>({source_node}));
    return node;
  }
};

BOOST_FIXTURE_TEST_SUITE(_transform_test, transform_test)
BOOST_AUTO_TEST_CASE(test_identity_transform) {
  transform_test::test_identity_transform();
}
BOOST_AUTO_TEST_CASE(test_plus_one) {
  transform_test::test_plus_one();
}
BOOST_AUTO_TEST_SUITE_END()
