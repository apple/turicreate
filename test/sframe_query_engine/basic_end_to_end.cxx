#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/query_engine/planning/planner.hpp>
#include <core/storage/query_engine/planning/planner_node.hpp>
#include <core/storage/query_engine/operators/all_operators.hpp>
#include <core/storage/query_engine/util/aggregates.hpp>
#include <core/storage/sframe_data/sarray.hpp>

using namespace turi;
using namespace turi::query_eval;

struct basic_end_to_end {
 public:

  void test_basic_linear() {
    const size_t TEST_LENGTH = 128 + 64;
    std::vector<flexible_type> data;
    for (size_t i = 0;i < TEST_LENGTH; ++i) data.push_back(i);
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(data.begin(), data.end(), *sa);
    sa->close();

    auto root = op_sarray_source::make_planner_node(sa);

    // add_one = root + 1
    auto add_one = 
        op_transform::make_planner_node(
            root, 
            [](const sframe_rows::row& a)->flexible_type {
              return a[0] + 1;
            },
            flex_type_enum::INTEGER);

    // sum_both = add_one + root
    //
    auto sum_both = 
        op_binary_transform::make_planner_node(
            root, 
            add_one,
            [](const sframe_rows::row& a, 
               const sframe_rows::row& b)->flexible_type {
              return a[0] + b[0];
            },
            flex_type_enum::INTEGER);

    auto res = planner().materialize(sum_both);
    std::vector<flexible_type> all_rows;
    res.select_column(0)->get_reader()->read_rows(0, res.size(), all_rows);

    TS_ASSERT_EQUALS(all_rows.size(), TEST_LENGTH);
    for (flex_int i = 0;i < truncate_check<int64_t>(TEST_LENGTH); ++i) {
      flex_int j = (flex_int)(all_rows[i]);
      TS_ASSERT_EQUALS(2*i+1, j);
    }
  }

  void test_sub_linear() {
    const size_t TEST_LENGTH = 1000000;
    std::vector<flexible_type> data;
    for (size_t i = 0;i < TEST_LENGTH; ++i) data.push_back(i);
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(data.begin(), data.end(), *sa);
    sa->close();

    auto root = op_sarray_source::make_planner_node(sa);

    // even_selector = root % 2 == 0
    auto even_selector = 
        op_transform::make_planner_node(
            root, 
            [](const sframe_rows::row& a)->flexible_type {
              return (flex_int)(a[0]) % 2 == 0;
            },
            flex_type_enum::INTEGER);

    // filter = root[even_selector]
    auto filter = op_logical_filter::make_planner_node(root, even_selector);

    auto res = planner().materialize(filter);
    std::vector<flexible_type> all_rows;
    res.select_column(0)->get_reader()->read_rows(0, res.size(), all_rows);

    TS_ASSERT_EQUALS(all_rows.size(), TEST_LENGTH / 2);
    for (flex_int i = 0;i < truncate_check<int64_t>(TEST_LENGTH) / 2; ++i) {
      TS_ASSERT_EQUALS(2*i, all_rows[i]);
    }
  }


  void test_diamond() {
    const size_t TEST_LENGTH = 1000;
    std::vector<flexible_type> data;
    for (size_t i = 0;i < TEST_LENGTH; ++i) data.push_back(i);
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(data.begin(), data.end(), *sa);
    sa->close();

    auto root = op_sarray_source::make_planner_node(sa);

    // even_selector = root % 2 == 0
    auto even_selector = 
        op_transform::make_planner_node(
            root, 
            [](const sframe_rows::row& a)->flexible_type {
              return (flex_int)(a[0]) % 2 == 0;
            },
            flex_type_enum::INTEGER);

    // add_one = root + 1
    auto add_one = 
        op_transform::make_planner_node(
            root, 
            [](const sframe_rows::row& a)->flexible_type {
              return a[0] + 1;
            },
            flex_type_enum::INTEGER);

    // filter = add_one[even_selector]
    auto filter = op_logical_filter::make_planner_node(add_one, even_selector);

    auto res = planner().materialize(filter);
    std::vector<flexible_type> all_rows;
    res.select_column(0)->get_reader()->read_rows(0, res.size(), all_rows);

    TS_ASSERT_EQUALS(all_rows.size(), TEST_LENGTH / 2);
    for (flex_int i = 0;i < truncate_check<int64_t>(TEST_LENGTH) / 2; ++i) {
      TS_ASSERT_EQUALS(2*i + 1, all_rows[i]);
    }
  }
  void test_reduction_aggregate() {
    const size_t TEST_LENGTH = 1000000;
    std::vector<flexible_type> data;
    for (size_t i = 0;i < TEST_LENGTH; ++i) data.push_back(i);
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(data.begin(), data.end(), *sa);
    sa->close();
    auto root = op_sarray_source::make_planner_node(sa);
    flex_int m = query_eval::reduce<flex_int>(root, 
                                 [](const flexible_type& f, 
                                    flex_int& val) {
                                      if (f > val) val = f;
                                    },
                                  [](const flex_int& f,
                                     flex_int& val) {
                                       if (f > val) val = f;
                                   },
                                   0);
    TS_ASSERT_EQUALS(m, TEST_LENGTH - 1);
  }

  void test_range_slice() {
    const size_t TEST_LENGTH = 1000;
    global_logger().set_log_level(LOG_INFO);

    std::vector<flexible_type> data;
    for (size_t i = 0;i < TEST_LENGTH; ++i) data.push_back(i);
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    sa->set_type(flex_type_enum::INTEGER);
    turi::copy(data.begin(), data.end(), *sa);
    sa->close();

    // Direct slice source node
    {
      const size_t SLICE_LENGTH = TEST_LENGTH / 4;
      materialize_options opts;
      size_t begin = SLICE_LENGTH;
      size_t end  = begin  + SLICE_LENGTH;
      auto root = op_sarray_source::make_planner_node(sa);
      root = planner().slice(root, begin, end);
      auto res = planner().materialize(root, opts);
      std::vector<flexible_type> all_rows;
      res.select_column(0)->get_reader()->read_rows(0, res.size(), all_rows);
      TS_ASSERT_EQUALS(all_rows.size(), SLICE_LENGTH);
      for (flex_int i = 0; i < truncate_check<int64_t>(SLICE_LENGTH); ++i) {
        flex_int j = (flex_int)(all_rows[i]);
        TS_ASSERT_EQUALS(i + begin, j);
      }
    }

    // Slice linear plan
    {
      const size_t SLICE_LENGTH = TEST_LENGTH / 4;
      materialize_options opts;
      size_t begin = SLICE_LENGTH;
      size_t end = begin + SLICE_LENGTH;
      auto root = op_sarray_source::make_planner_node(sa);
      auto add_one =
        op_transform::make_planner_node(
            root,
            [](const sframe_rows::row& a)->flexible_type {
              return a[0] + 1;
            },
            flex_type_enum::INTEGER);
      add_one = planner().slice(add_one, begin, end);
      auto res = planner().materialize(add_one, opts);
      std::vector<flexible_type> all_rows;
      res.select_column(0)->get_reader()->read_rows(0, res.size(), all_rows);
      TS_ASSERT_EQUALS(all_rows.size(), SLICE_LENGTH);
      for (flex_int i = 0; i < truncate_check<int64_t>(SLICE_LENGTH); ++i) {
        flex_int j = (flex_int)(all_rows[i]);
        TS_ASSERT_EQUALS(1 + i + begin, j);
      }
    }

    // Slice sublinear plan
    {
      const size_t SLICE_LENGTH = TEST_LENGTH / 4;
      materialize_options opts;
      size_t begin = SLICE_LENGTH;
      size_t end = begin + SLICE_LENGTH;
      auto root = op_sarray_source::make_planner_node(sa);
      // even_selector = root % 2 == 0
      auto even_selector =
          op_transform::make_planner_node(
              root,
              [](const sframe_rows::row& a)->flexible_type {
                return (flex_int)(a[0]) % 2 == 0;
              },
              flex_type_enum::INTEGER);
      auto filter = op_logical_filter::make_planner_node(root, even_selector);
      filter = planner().slice(filter, begin, end);
      auto res = planner().materialize(filter, opts);
      std::vector<flexible_type> all_rows;
      res.select_column(0)->get_reader()->read_rows(0, res.size(), all_rows);
      TS_ASSERT_EQUALS(all_rows.size(), SLICE_LENGTH);
      for (flex_int i = 0; i < truncate_check<int64_t>(SLICE_LENGTH); ++i) {
        flex_int j = (flex_int)(all_rows[i]);
        TS_ASSERT_EQUALS((i + begin) * 2, j);
      }
    }

    // Non linear Plan
    {
      auto root = op_sarray_source::make_planner_node(sa);
      auto add_one =
          op_transform::make_planner_node(
              root,
              [](const sframe_rows::row& a)->flexible_type {
                return a[0] + 1;
              },
              flex_type_enum::INTEGER);
      auto append = op_append::make_planner_node(add_one, add_one);

      // Slice the first half
      const size_t SLICE_LENGTH = TEST_LENGTH;
      auto sliced = planner().slice(append, 0, SLICE_LENGTH);
      auto res = planner().materialize(sliced);
      std::vector<flexible_type> all_rows;
      res.select_column(0)->get_reader()->read_rows(0, res.size(), all_rows);
      TS_ASSERT_EQUALS(all_rows.size(), SLICE_LENGTH);
      for (flex_int i = 0; i < truncate_check<int64_t>(SLICE_LENGTH); ++i) {
        flex_int j = (flex_int)(all_rows[i]);
        TS_ASSERT_EQUALS(i + 1, j);
      }
      // Slice the second half
      sliced = planner().slice(append, SLICE_LENGTH, TEST_LENGTH * 2);
      res = planner().materialize(sliced);
      res.select_column(0)->get_reader()->read_rows(0, res.size(), all_rows);
      TS_ASSERT_EQUALS(all_rows.size(), SLICE_LENGTH);
      for (flex_int i = 0; i < truncate_check<int64_t>(SLICE_LENGTH); ++i) {
        flex_int j = (flex_int)(all_rows[i]);
        TS_ASSERT_EQUALS(i + 1, j);
      }
    }
  }
};

BOOST_FIXTURE_TEST_SUITE(_basic_end_to_end, basic_end_to_end)
BOOST_AUTO_TEST_CASE(test_basic_linear) {
  basic_end_to_end::test_basic_linear();
}
BOOST_AUTO_TEST_CASE(test_sub_linear) {
  basic_end_to_end::test_sub_linear();
}
BOOST_AUTO_TEST_CASE(test_diamond) {
  basic_end_to_end::test_diamond();
}
BOOST_AUTO_TEST_CASE(test_reduction_aggregate) {
  basic_end_to_end::test_reduction_aggregate();
}
BOOST_AUTO_TEST_CASE(test_range_slice) {
  basic_end_to_end::test_range_slice();
}
BOOST_AUTO_TEST_SUITE_END()
