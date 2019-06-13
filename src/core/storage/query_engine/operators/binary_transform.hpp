/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_BINARY_TRANSFORM_HPP
#define TURI_SFRAME_QUERY_MANAGER_BINARY_TRANSFORM_HPP

#include <functional>
#include <core/logging/assertions.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>

namespace turi {
namespace query_eval {

typedef std::function<flexible_type(const sframe_rows::row&,
                                    const sframe_rows::row&)> binary_transform_type;

/**
 * \ingroup sframe_query_engine
 * \addtogroup operators Logical Operators
 * \{
 */

/**
 * A "binary transform" operator applys a transform function on two
 * stream of input.
 */
template<>
class operator_impl<planner_node_type::BINARY_TRANSFORM_NODE> : public query_operator {
 public:

  planner_node_type type() const { return planner_node_type::BINARY_TRANSFORM_NODE; }

  static std::string name() { return "binary_transform"; }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::LINEAR;
    ret.num_inputs = 2;
    return ret;
  }

  inline operator_impl(const binary_transform_type& f,
                       flex_type_enum output_type)
      : m_transform_fn(f)
  { }

  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>(*this);
  }

  inline void execute(query_context& context) {
    while(1) {
      auto rows_left = context.get_next(0);
      auto rows_right = context.get_next(1);
      if (rows_left == nullptr && rows_right == nullptr) break;
      ASSERT_TRUE(rows_left != nullptr && rows_right != nullptr);
      ASSERT_EQ(rows_left->num_rows(), rows_right->num_rows());
      ASSERT_EQ(rows_left->num_columns(), 1);
      ASSERT_EQ(rows_right->num_columns(), 1);
      auto output_buffer = context.get_output_buffer();
      output_buffer->resize(1, rows_left->num_rows());

      auto left_iter = rows_left->cbegin();
      auto right_iter = rows_right->cbegin();
      auto out_iter = output_buffer->begin();
      while(left_iter != rows_left->cend()) {
        (*out_iter)[0] = m_transform_fn((*left_iter), (*right_iter));
        ++left_iter;
        ++right_iter;
        ++out_iter;
      }
      context.emit(output_buffer);
    }
  }

  static std::shared_ptr<planner_node> make_planner_node(
      std::shared_ptr<planner_node> left,
      std::shared_ptr<planner_node> right,
        binary_transform_type fn,
      flex_type_enum output_type) {

    return planner_node::make_shared(planner_node_type::BINARY_TRANSFORM_NODE,
                                     {{"output_type", (int)(output_type)}},
                                     {{"function", any(fn)}},
                                     {left, right});
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {

    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::BINARY_TRANSFORM_NODE);
    ASSERT_EQ(pnode->inputs.size(), 2);
    ASSERT_TRUE(pnode->operator_parameters.count("output_type"));
    ASSERT_TRUE(pnode->any_operator_parameters.count("function"));
    binary_transform_type fn;
    flex_type_enum output_type =
        (flex_type_enum)(flex_int)(pnode->operator_parameters["output_type"]);

    fn = pnode->any_operator_parameters["function"].as<binary_transform_type>();
    return std::make_shared<operator_impl>(fn, output_type);
  }

  static std::vector<flex_type_enum> infer_type(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::BINARY_TRANSFORM_NODE);
    ASSERT_TRUE(pnode->operator_parameters.count("output_type"));
    return {(flex_type_enum)(int)(pnode->operator_parameters["output_type"])};
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::BINARY_TRANSFORM_NODE);
    return infer_planner_node_length(pnode->inputs[0]);
  }

 private:
   binary_transform_type m_transform_fn;
};

typedef operator_impl<planner_node_type::BINARY_TRANSFORM_NODE> op_binary_transform;


/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_TRANSFORM_HPP
