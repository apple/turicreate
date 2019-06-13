#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/query_engine/execution/execution_node.hpp>
#include <core/storage/query_engine/operators/sframe_source.hpp>
#include <core/storage/query_engine/operators/project.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/algorithm.hpp>

#include "check_node.hpp"

using namespace turi;
using namespace turi::query_eval;

struct project_test  {
 public:
  void test_simple_case() {
    std::vector<std::vector<flexible_type>> data {
      {0, "s0"}, {1, "s1"}, {2, "s2"}, {3, "s3"}, {4, "s4"}, {5, "s5"}
    };
    std::vector<std::string> column_names{"int", "string"};
    std::vector<flex_type_enum> column_types{flex_type_enum::INTEGER, flex_type_enum::STRING};
    auto sf = make_sframe(column_names, column_types, data);

    std::vector<std::vector<size_t>> test_cases{{0}, {1}, {0, 1}, {1, 0}};
    for (auto& project_indices : test_cases) {
      std::vector<std::vector<flexible_type>> expected(data.size());
      std::transform(data.begin(), data.end(), expected.begin(),
                     [=](const std::vector<flexible_type>& row) {
                        std::vector<flexible_type> ret;
                        for (size_t i : project_indices)
                          ret.push_back(row[i]);
                        return ret;
                     });
      auto node = make_node(sf, project_indices);
      check_node(node, expected);
    }
  }

  void test_empty_sframe() {
    std::vector<std::vector<flexible_type>> data;
    std::vector<std::string> column_names{"int", "string"};
    std::vector<flex_type_enum> column_types{flex_type_enum::INTEGER, flex_type_enum::STRING};
    auto sf = make_sframe(column_names, column_types, data);

    std::vector<std::vector<size_t>> test_cases{{0}, {1}, {0, 1}, {1, 0}};
    for (auto& project_indices : test_cases) {
      std::vector<std::vector<flexible_type>> expected;
      auto node = make_node(sf, project_indices);
      check_node(node, expected);
    }
  }

  void test_project_out_of_bound() {
    // std::vector<std::vector<flexible_type>> data {
    //   {0, "s0"}, {1, "s1"}, {2, "s2"}, {3, "s3"}, {4, "s4"}, {5, "s5"}
    // };
    //
    // std::vector<std::string> column_names{"int", "string"};
    // std::vector<flex_type_enum> column_types{flex_type_enum::INTEGER, flex_type_enum::STRING};
    // auto sf = make_sframe(column_names, column_types, data);
    // auto node = make_node(sf, {2});
    // check_node_throws(node);
  }

  sframe make_sframe(const std::vector<std::string>& column_names,
                     const std::vector<flex_type_enum>& column_types,
                     const std::vector<std::vector<flexible_type>>& rows) {
    sframe sf;
    sf.open_for_write(column_names, column_types);
    turi::copy(rows.begin(), rows.end(), sf);
    sf.close();
    return sf;
  }

  std::shared_ptr<execution_node> make_node(sframe source, std::vector<size_t> project_indices) {
    auto source_node = std::make_shared<execution_node>(std::make_shared<op_sframe_source>(source));
    auto node = std::make_shared<execution_node>(std::make_shared<op_project>(project_indices),
                                                 std::vector<std::shared_ptr<execution_node>>({source_node}));
    return node;
  }
};

BOOST_FIXTURE_TEST_SUITE(_project_test, project_test)
BOOST_AUTO_TEST_CASE(test_simple_case) {
  project_test::test_simple_case();
}
BOOST_AUTO_TEST_CASE(test_empty_sframe) {
  project_test::test_empty_sframe();
}
BOOST_AUTO_TEST_CASE(test_project_out_of_bound) {
  project_test::test_project_out_of_bound();
}
BOOST_AUTO_TEST_SUITE_END()
