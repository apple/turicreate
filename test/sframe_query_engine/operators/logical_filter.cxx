#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/query_engine/execution/execution_node.hpp>
#include <core/storage/query_engine/operators/sarray_source.hpp>
#include <core/storage/query_engine/operators/logical_filter.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/algorithm.hpp>

#include "check_node.hpp"

using namespace turi;
using namespace turi::query_eval;

struct logical_filter_test {
 public:

  void test_filter_empty_array() {
    auto data_sa = std::make_shared<sarray<flexible_type>>();
    data_sa->open_for_write();
    data_sa->close();

    auto filter_sa = std::make_shared<sarray<flexible_type>>();
    filter_sa->open_for_write();
    filter_sa->close();

    std::vector<flexible_type> expected;

    auto node = make_node(op_sarray_source(data_sa), op_sarray_source(filter_sa));
    check_node(node, expected);
  }

  void test_filter_none() {
    auto data_sa = get_data_sarray();

    std::vector<flexible_type> filter(data_sa->size(), 0);
    auto filter_sa = std::make_shared<sarray<flexible_type>>();
    filter_sa->open_for_write();
    turi::copy(filter.begin(), filter.end(), *filter_sa);
    filter_sa->close();

    std::vector<flexible_type> expected;
    auto node = make_node(op_sarray_source(data_sa), op_sarray_source(filter_sa));
    check_node(node, expected);
  }

  void test_filter_all() {
    auto data_sa = get_data_sarray();

    std::vector<flexible_type> filter(data_sa->size(), 1);
    auto filter_sa = std::make_shared<sarray<flexible_type>>();
    filter_sa->open_for_write();
    turi::copy(filter.begin(), filter.end(), *filter_sa);
    filter_sa->close();

    std::vector<flexible_type> expected;
    data_sa->get_reader()->read_rows(0, data_sa->size(), expected);
    auto node = make_node(op_sarray_source(data_sa), op_sarray_source(filter_sa));
    check_node(node, expected);
  }

  void test_filter_even() {
    auto data_sa = get_data_sarray();

    std::vector<flexible_type> filter;
    for (size_t i = 0; i < data_sa->size(); i++) {
      if (i % 2 == 0) {
        filter.push_back(0);
      } else {
        filter.push_back(1);
      }
    }

    auto filter_sa = std::make_shared<sarray<flexible_type>>();
    filter_sa->open_for_write();
    turi::copy(filter.begin(), filter.end(), *filter_sa);
    filter_sa->close();

    std::vector<flexible_type> data;
    data_sa->get_reader()->read_rows(0, data_sa->size(), data);

    std::vector<flexible_type> expected;
    for (size_t i =0 ; i < data.size(); ++i) {
      if (filter[i]) {
        expected.push_back(data[i]);
      }
    }

    auto node = make_node(op_sarray_source(data_sa), op_sarray_source(filter_sa));
    check_node(node, expected);
  }


  std::shared_ptr<sarray<flexible_type>> get_data_sarray() {
    std::vector<flexible_type> data{0,1,2,3,4,5};
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(data.begin(), data.end(), *sa);
    sa->close();
    return sa;
  }

  template <typename Source>
  std::shared_ptr<execution_node> make_node(const Source& source_left, const Source& source_right) {
    auto left_node = std::make_shared<execution_node>(std::make_shared<Source>(source_left));
    auto right_node = std::make_shared<execution_node>(std::make_shared<Source>(source_right));
    auto node = std::make_shared<execution_node>(std::make_shared<op_logical_filter>(),
                                                 std::vector<std::shared_ptr<execution_node>>({left_node, right_node}));
    return node;
  }
};

BOOST_FIXTURE_TEST_SUITE(_logical_filter_test, logical_filter_test)
BOOST_AUTO_TEST_CASE(test_filter_empty_array) {
  logical_filter_test::test_filter_empty_array();
}
BOOST_AUTO_TEST_CASE(test_filter_none) {
  logical_filter_test::test_filter_none();
}
BOOST_AUTO_TEST_CASE(test_filter_all) {
  logical_filter_test::test_filter_all();
}
BOOST_AUTO_TEST_CASE(test_filter_even) {
  logical_filter_test::test_filter_even();
}
BOOST_AUTO_TEST_SUITE_END()
