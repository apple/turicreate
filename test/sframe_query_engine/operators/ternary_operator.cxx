#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/storage/query_engine/execution/execution_node.hpp>
#include <core/storage/query_engine/operators/sarray_source.hpp>
#include <core/storage/query_engine/operators/ternary_operator.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/algorithm.hpp>

#include "check_node.hpp"

using namespace turi;
using namespace turi::query_eval;

struct ternary_operator_test {
 public:
  void test_ternary() {
    std::vector<flexible_type> condition{0,1,0,1,0};
    std::vector<flexible_type> istrue{2,2,2,2,2};
    std::vector<flexible_type> isfalse{0,0,0,0,0};

    auto sa_condition = std::make_shared<sarray<flexible_type>>();
    sa_condition->open_for_write();
    turi::copy(condition.begin(), condition.end(), *sa_condition);
    sa_condition->close();

    auto sa_true = std::make_shared<sarray<flexible_type>>();
    sa_true->open_for_write();
    turi::copy(istrue.begin(), istrue.end(), *sa_true);
    sa_true->close();

    auto sa_false = std::make_shared<sarray<flexible_type>>();
    sa_false->open_for_write();
    turi::copy(isfalse.begin(), isfalse.end(), *sa_false);
    sa_false->close();

    std::vector<flexible_type> expected{0,2,0,2,0};
    auto node = make_node(op_sarray_source(sa_condition),
                          op_sarray_source(sa_true),
                          op_sarray_source(sa_false));
    check_node(node, expected);
  }

  std::shared_ptr<execution_node> make_node(const op_sarray_source& condition,
                                            const op_sarray_source& source_true,
                                            const op_sarray_source& source_false) {
    auto condition_node = std::make_shared<execution_node>(std::make_shared<op_sarray_source>(condition));
    auto true_node = std::make_shared<execution_node>(std::make_shared<op_sarray_source>(source_true));
    auto false_node = std::make_shared<execution_node>(std::make_shared<op_sarray_source>(source_false));
    auto node = std::make_shared<execution_node>(std::make_shared<op_ternary_operator>(),
                             std::vector<std::shared_ptr<execution_node>>({condition_node, true_node, false_node}));
    return node;
  }
};

BOOST_FIXTURE_TEST_SUITE(_ternary_operator_test, ternary_operator_test)
BOOST_AUTO_TEST_CASE(test_ternary) {
  ternary_operator_test::test_ternary();
}
BOOST_AUTO_TEST_SUITE_END()
