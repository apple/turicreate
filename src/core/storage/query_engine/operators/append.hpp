/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_APPEND_HPP
#define TURI_SFRAME_QUERY_MANAGER_APPEND_HPP

#include <core/logging/assertions.hpp>
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
 * A append operator append two input streams; typedefed \ref op_append.
 */
template<>
class operator_impl<planner_node_type::APPEND_NODE> : public query_operator {
 public:

  planner_node_type type() const { return planner_node_type::APPEND_NODE; }

  static std::string name() { return "append"; }

  inline operator_impl() { };

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::NONE;
    ret.num_inputs = 2;
    return ret;
  }


  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>();
  }

  inline void execute(query_context& context) {
    std::shared_ptr<sframe_rows> out;
    size_t outidx = 0;
    for (size_t input = 0; input < 2; ++input) {
      // read some input
      auto input_rows = context.get_next(input);
      if (input_rows != nullptr && out == nullptr) {
        // we have input, but we don't have an output buffer. Make one
        out = context.get_output_buffer();
        out->resize(input_rows->num_columns(), context.block_size());
        outidx = 0;
      }
      // keep looping over this input
      while(input_rows != nullptr) {
        for (const auto& row: *input_rows) {
          (*out)[outidx] = row;
          ++outidx;
          // output buffer is full. give it away and acquire a new one
          if (outidx == context.block_size()) {
            context.emit(out);
            out = context.get_output_buffer();
            out->resize(input_rows->num_columns(), context.block_size());
            outidx = 0;
          }
        }
        input_rows = context.get_next(input);
      }
    }
    if (outidx && out) {
      out->resize(out->num_columns(), outidx);
      context.emit(out);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////

  /**
   * Creates a logical append node that appends the left and right nodes.
   */
  static std::shared_ptr<planner_node> make_planner_node(std::shared_ptr<planner_node> left,
                                                         std::shared_ptr<planner_node> right) {
    return planner_node::make_shared(planner_node_type::APPEND_NODE,
                                     std::map<std::string, flexible_type>(),
                                     std::map<std::string, any>(),
                                     {left, right});
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ(pnode->inputs.size(), 2);
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::APPEND_NODE);
    return std::make_shared<operator_impl>();
  }

  static std::vector<flex_type_enum> infer_type(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::APPEND_NODE);
    std::vector<std::vector<flex_type_enum>> types;
    for (auto input: pnode->inputs) {
      types.push_back(infer_planner_node_type(input));
    }
    ASSERT_MSG(types.size() != 0, "Append with no input");
    // check that all types are equal
    // check that they all line up with types[0]
    for (size_t i = 1; i < types.size(); ++i) {
      ASSERT_EQ(types[i].size(), types[0].size());
      for (size_t j = 0; j < types[i].size(); ++j) {
        ASSERT_EQ((int)types[i][j], (int)types[0][j]);
      }
    }
    return types[0];
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::APPEND_NODE);
    int64_t ret_length = 0;
    for (auto input: pnode->inputs) {
      int64_t input_length = infer_planner_node_length(input);
      if (input_length == -1) return -1;
      ret_length += input_length;
    }
    return ret_length;
  }

  static std::string repr(std::shared_ptr<planner_node> pnode, pnode_tagger& get_tag) {
    ASSERT_EQ(pnode->inputs.size(), 2);
    return std::string("Append(") + get_tag(pnode->inputs[0]) + "," + get_tag(pnode->inputs[1]) + ")";
  }

};

typedef operator_impl<planner_node_type::APPEND_NODE> op_append;

/// \}

} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_APPEND_HPP
