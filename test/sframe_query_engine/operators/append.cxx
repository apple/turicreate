#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/query_engine/execution/execution_node.hpp>
#include <core/storage/query_engine/operators/sarray_source.hpp>
#include <core/storage/query_engine/operators/append.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/algorithm.hpp>

#include "check_node.hpp"

using namespace turi;
using namespace turi::query_eval;

struct append_test {
 public:

  void test_self_append() {
    std::vector<flexible_type> data{0,1,2,3,4,5};
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(data.begin(), data.end(), *sa);
    sa->close();
    auto sa_source = op_sarray_source(sa);
    auto node = make_node(sa_source, sa_source);

    std::vector<flexible_type> expected = data;
    expected.insert(expected.begin(), data.begin(), data.end());
    check_node(node, expected);
  }

  void test_empty_append() {
    std::vector<flexible_type> data{0,1,2,3,4,5};
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(data.begin(), data.end(), *sa);
    sa->close();
    auto sa_empty = std::make_shared<sarray<flexible_type>>();
    sa_empty->open_for_write();
    sa_empty->close();
    auto sa_source = op_sarray_source(sa);
    auto empty_sa_source = op_sarray_source(sa_empty);

    {
      auto node = make_node(sa_source, empty_sa_source);
      std::vector<flexible_type> expected = data;
      check_node(node, expected);
    }

    {
      auto node = make_node(empty_sa_source, sa_source);
      std::vector<flexible_type> expected = data;
      check_node(node, expected);
    }

    {
      auto node = make_node(empty_sa_source, empty_sa_source);
      std::vector<flexible_type> expected;
      check_node(node, expected);
    }
  }

  void test_regular_append() {
    std::vector<flexible_type> data1{0,1,2,3,4,5};
    std::vector<flexible_type> data2{0,1,2,3,4,5};

    auto sa1 = std::make_shared<sarray<flexible_type>>();
    sa1->open_for_write();
    turi::copy(data1.begin(), data1.end(), *sa1);
    sa1->close();

    auto sa2 = std::make_shared<sarray<flexible_type>>();
    sa2->open_for_write();
    turi::copy(data2.begin(), data2.end(), *sa2);
    sa2->close();

    auto sa1_source = op_sarray_source(sa1);
    auto sa2_source = op_sarray_source(sa2);

    auto node = make_node(sa1_source, sa2_source);
    std::vector<flexible_type> expected = data1;
    expected.insert(expected.begin(), data1.begin(), data1.end());
    check_node(node, expected);
  }

  template <typename Source>
  std::shared_ptr<execution_node> make_node(const Source& first, const Source& second) {
    auto source_first = std::make_shared<execution_node>(std::make_shared<Source>(first));
    auto source_second = std::make_shared<execution_node>(std::make_shared<Source>(second));
    auto node = std::make_shared<execution_node>(std::make_shared<op_append>(),
                                                 std::vector<std::shared_ptr<execution_node>>({source_first, source_second}));
    return node;
  }
};

BOOST_FIXTURE_TEST_SUITE(_append_test, append_test)
BOOST_AUTO_TEST_CASE(test_self_append) {
  append_test::test_self_append();
}
BOOST_AUTO_TEST_CASE(test_empty_append) {
  append_test::test_empty_append();
}
BOOST_AUTO_TEST_CASE(test_regular_append) {
  append_test::test_regular_append();
}
BOOST_AUTO_TEST_SUITE_END()
