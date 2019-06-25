#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/query_engine/execution/execution_node.hpp>
#include <core/storage/query_engine/operators/sarray_source.hpp>
#include <core/storage/query_engine/operators/lambda_transform.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/algorithm.hpp>

#include "check_node.hpp"

using namespace turi;
using namespace turi::query_eval;

// TODO: get a pickle string for simple lambda 
const std::string IDENTITY_LAMBDA_STRING="";
const std::string PLUS_ONE_LAMBDA_STRING="";

struct transform_test {
 public:
  void test_identity_transform() {
    std::vector<flexible_type> expected {0,1,2,3,4,5};
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(expected.begin(), expected.end(), *sa);
    sa->close();
    auto node = make_node(op_sarray_source(sa),
                          IDENTITY_LAMBDA_STRING, flex_type_enum::INTEGER);
    check_node(node, expected);
  }

  void test_plus_one() {
    std::vector<flexible_type> data{0,1,2,3,4,5};
    auto sa = std::make_shared<sarray<flexible_type>>();
    sa->open_for_write();
    turi::copy(data.begin(), data.end(), *sa);
    sa->close();
    std::vector<flexible_type> expected(data.size());
    auto f = [](const flexible_type& val) { return val + 1; };
    std::transform(data.begin(), data.end(), expected.begin(), f);
    auto node = make_node(op_sarray_source(sa), PLUS_ONE_LAMBDA_STRING, flex_type_enum::INTEGER);
    check_node(node, expected);
  }

  std::shared_ptr<execution_node> make_node(const op_sarray_source& source,
                                            const std::string& lambda_str, flex_type_enum type) {
    auto lambda_fn = std::make_shared<lambda::pylambda_function>(lambda_str);
    auto source_node = std::make_shared<execution_node>(std::make_shared<op_sarray_source>(source));
    auto node = std::make_shared<execution_node>(std::make_shared<op_lambda_transform>(lambda_fn, type),
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
