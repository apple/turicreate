/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_LOGICAL_FILTER_HPP
#define TURI_SFRAME_QUERY_MANAGER_LOGICAL_FILTER_HPP
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>

namespace turi {
namespace query_eval {

/**
 * \ingroup sframe_query_engine
 * \addtogroup operators Logical Operators
 * \{
 */

/**
 * A "logical_filter" operator which takes two inputs of the same size:
 * "values", and "logical indices", and output the value in "values" for which
 * the logical index is 1.
 */
template<>
class operator_impl<planner_node_type::LOGICAL_FILTER_NODE> : public query_operator {
 public:

  planner_node_type type() const { return planner_node_type::LOGICAL_FILTER_NODE; }

  static std::string name() { return "logical_filter"; }

  inline operator_impl() { };

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::SUB_LINEAR;
    ret.num_inputs = 2;
    return ret;
  }

  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>(*this);
  }

  // tests if the first column of col is all zeros
  bool is_all_zero(const std::shared_ptr<const sframe_rows>& col) {
    // if it is all zero, we can skip the left data
    for (auto& row: *col) {
      if (!(row[0].is_zero())) return false;
    }
    return true;
  }

  inline void execute(query_context& context) {
    // read one block
    auto rows_left = context.get_next(0);
    auto rows_right = context.get_next(1);
    if (rows_left == nullptr && rows_right == nullptr) return;
    ASSERT_TRUE(rows_left != nullptr && rows_right != nullptr);

    // set up the output shape
    auto output_buffer = context.get_output_buffer();
    size_t cur_output_index = 0;
    size_t ncols = rows_left->num_columns();
    size_t nrows = context.block_size();
    output_buffer->resize(ncols, nrows);


    while(1) {
      ASSERT_TRUE(rows_left != nullptr && rows_right != nullptr);
      ASSERT_EQ(rows_left->num_rows(), rows_right->num_rows());

      auto left_iter = rows_left->cbegin();
      auto right_iter = rows_right->cbegin();
      while(left_iter != rows_left->cend()) {
        if (!(*right_iter)[0].is_zero()) {
          (*output_buffer)[cur_output_index] = (*left_iter);
          ++cur_output_index;
          if (cur_output_index == nrows) {
            context.emit(output_buffer);
            output_buffer = context.get_output_buffer();
            output_buffer->resize(ncols, nrows);
            cur_output_index = 0;
          }
        }
        ++left_iter;
        ++right_iter;
      }
      bool has_data = false;
      do {
        // get the binary column first
        rows_right = context.get_next(1);
        // skip left if it is all zeros
        if (rows_right != nullptr && is_all_zero(rows_right)) {
          context.skip_next(0);
        } else {
          has_data = true;
          rows_left = context.get_next(0);
        }
        // loop until there is really no data.
      } while(has_data == false);
      if(rows_left == nullptr && rows_right == nullptr) break;
    }

    if (cur_output_index > 0) {
      output_buffer->resize(ncols, cur_output_index);
      context.emit(output_buffer);
    }
  }

  static std::shared_ptr<planner_node> make_planner_node(
      std::shared_ptr<planner_node> left,
      std::shared_ptr<planner_node> right) {
    return planner_node::make_shared(planner_node_type::LOGICAL_FILTER_NODE,
                                     std::map<std::string, flexible_type>(),
                                     std::map<std::string, any>(),
                                     {left, right});
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type,
              (int)planner_node_type::LOGICAL_FILTER_NODE);
    ASSERT_EQ(pnode->inputs.size(), 2);
    return std::make_shared<operator_impl>();
  }

  static std::vector<flex_type_enum> infer_type(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type,
              (int)planner_node_type::LOGICAL_FILTER_NODE);
    ASSERT_EQ(pnode->inputs.size(), 2);
    return infer_planner_node_type(pnode->inputs[0]);
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    return -1;
  }

  static std::string repr(std::shared_ptr<planner_node> pnode, pnode_tagger& get_tag) {
    ASSERT_EQ(pnode->inputs.size(), 2);
    return std::string("Filter(") + get_tag(pnode->inputs[0]) + "[" + get_tag(pnode->inputs[1]) + "])";
  }

};

typedef operator_impl<planner_node_type::LOGICAL_FILTER_NODE> op_logical_filter;

/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_LOGICAL_FILTER_HPP
