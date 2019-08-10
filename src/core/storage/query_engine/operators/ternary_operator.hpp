/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_TERNARY_OPERATOR_HPP
#define TURI_SFRAME_QUERY_MANAGER_TERNARY_OPERATOR_HPP

#include <functional>
#include <core/logging/assertions.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/util/coro.hpp>

namespace turi {
namespace query_eval {

/**
 * \ingroup sframe_query_engine
 * \addtogroup operators Logical Operators
 * \{
 */

/**
 * An element-wise "ternary operator".
 * Takes 3 columns: condition, istrue, isfalse.
 * For each row,
 *    if condition == True, the corresponding row is selected from istrue
 *    if condition == False, the corresponding row is selected from isfalse
 */
template<>
class operator_impl<planner_node_type::TERNARY_OPERATOR> : public query_operator {
 public:
  DECL_CORO_STATE(execute);

  planner_node_type type() const { return planner_node_type::TERNARY_OPERATOR; }

  static std::string name() { return "ternary"; }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::LINEAR;
    ret.num_inputs = 3;
    return ret;
  }

  inline operator_impl() { }

  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>(*this);
  }

  inline bool coro_running() const {
    return CORO_RUNNING(execute);
  }
  inline void execute(query_context& context) {
    constexpr size_t CONDITION_INPUT = 0;
    constexpr size_t ISTRUE_INPUT = 1;
    constexpr size_t ISFALSE_INPUT = 2;
    CORO_BEGIN(execute)
    while(1) {
      {
      auto condition = context.get_next(CONDITION_INPUT);

      if (condition == nullptr) break;
      ASSERT_EQ(condition->num_columns(), 1);
      const auto& condition_column = *(condition->cget_columns()[0]);
      // sum the number of non-zeros
      size_t num_non_zero = std::accumulate(condition_column.begin(),
                                            condition_column.end(),
                                            (size_t)0,
                                            [](size_t acc, const flexible_type& f) {
                                              return acc + (! f.is_zero());
                                            });

      auto output_buffer = context.get_output_buffer();

      // fast path. All true, or all false
      if (num_non_zero == 0 || num_non_zero == condition_column.size()) {
        size_t input_number;
        size_t skip_number;
        if (num_non_zero == 0) {
          skip_number = ISTRUE_INPUT;
          input_number = ISFALSE_INPUT;
        } else {
          skip_number = ISFALSE_INPUT;
          input_number = ISTRUE_INPUT;
        }

        // all is false
        context.skip_next(skip_number);

        auto& out_columns = output_buffer->get_columns();

        auto input = context.get_next(input_number);
        ASSERT_EQ(input->num_rows(), condition_column.size());
        ASSERT_EQ(input->num_columns(), 1);

        out_columns.clear();
        out_columns.push_back(input->cget_columns()[0]);
      } else {

        auto isfalse = context.get_next(ISFALSE_INPUT);
        auto istrue = context.get_next(ISTRUE_INPUT);
        ASSERT_TRUE(istrue != nullptr);
        ASSERT_TRUE(isfalse != nullptr);
        ASSERT_EQ(isfalse->num_rows(), condition_column.size());
        ASSERT_EQ(istrue->num_rows(), condition_column.size());
        ASSERT_EQ(isfalse->num_columns(), 1);
        ASSERT_EQ(istrue->num_columns(), 1);

        output_buffer->resize(1, condition_column.size());

        auto istrue_iter = istrue->cbegin();
        auto isfalse_iter = isfalse->cbegin();
        auto out_iter = output_buffer->begin();
        for (auto& cval : condition_column) {
          if (cval.is_zero()) {
              (*out_iter)[0] = (*isfalse_iter)[0];
          } else {
              (*out_iter)[0] = (*istrue_iter)[0];
          }
          ++istrue_iter;
          ++isfalse_iter;
          ++out_iter;
        }
      }
      context.emit(output_buffer);
      }
      CORO_YIELD();
    }
    CORO_END
  }

  static std::shared_ptr<planner_node> make_planner_node(
      std::shared_ptr<planner_node> condition,
      std::shared_ptr<planner_node> istrue,
      std::shared_ptr<planner_node> isfalse) {

    return planner_node::make_shared(planner_node_type::TERNARY_OPERATOR,
                                     std::map<std::string, flexible_type>(),
                                     std::map<std::string, any>(),
                                     {condition, istrue, isfalse});
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {

    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::TERNARY_OPERATOR);
    ASSERT_EQ(pnode->inputs.size(), 3);

    return std::make_shared<operator_impl>();
  }

  static std::vector<flex_type_enum> infer_type(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::TERNARY_OPERATOR);
    return infer_planner_node_type(pnode->inputs[1]);
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::TERNARY_OPERATOR);
    return infer_planner_node_length(pnode->inputs[0]);
  }

};

typedef operator_impl<planner_node_type::TERNARY_OPERATOR> op_ternary_operator;


/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_TERNARY_OPERATOR_HPP
