/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_UNION_HPP
#define TURI_SFRAME_QUERY_MANAGER_UNION_HPP

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
 * A "union" operator combine two input stream by horizontally concat
 * the values. This is really a "zip" operator and not the SQL union.
 */
template <>
struct operator_impl<planner_node_type::UNION_NODE> : public query_operator {
 public:
  DECL_CORO_STATE(execute);
  std::vector<std::shared_ptr<const sframe_rows> > input_v;

  planner_node_type type() const { return planner_node_type::UNION_NODE; }

  static std::string name() { return "union"; }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::LINEAR;
    ret.num_inputs = -1;
    return ret;
  }

  inline operator_impl(size_t _num_inputs = 2) : num_inputs(_num_inputs) {}

  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>(*this);
  }

  inline bool coro_running() const {
    return CORO_RUNNING(execute);
  }
  inline void execute(query_context& context) {

    CORO_BEGIN(execute)
    input_v.resize(num_inputs);

    while(1) {
      {
      bool all_null = true, any_null = false;
      for(size_t i = 0; i < num_inputs; ++i) {
        input_v[i] = context.get_next(i);
        if(input_v[i] == nullptr)
          any_null = true;
        else
          all_null = false;
      }

      if(any_null) {
        ASSERT_TRUE(all_null);
        break;
      }

      auto out = context.get_output_buffer();
      auto& out_columns = out->get_columns();
      out_columns.clear();

      for(size_t i = 0; i < num_inputs; ++i) {
        std::copy(input_v[i]->get_columns().begin(), input_v[i]->get_columns().end(),
                  std::back_inserter(out_columns));
      }

      context.emit(out);
      }
      CORO_YIELD();
    }
    CORO_END
  }

 private:
    size_t num_inputs = 0;

 public:
  static std::shared_ptr<planner_node> make_planner_node(
      std::shared_ptr<planner_node> left,
      std::shared_ptr<planner_node> right) {
    return planner_node::make_shared(planner_node_type::UNION_NODE,
                                     std::map<std::string, flexible_type>(),
                                     std::map<std::string, any>(),
                                     {left, right});
  }

  static std::shared_ptr<planner_node> make_planner_node(
      const std::vector<std::shared_ptr<planner_node> >& nodes) {

    ASSERT_GE(nodes.size(), 1);

    return planner_node::make_shared(planner_node_type::UNION_NODE,
                                     std::map<std::string, flexible_type>(),
                                     std::map<std::string, any>(),
                                     nodes);
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::UNION_NODE);
    ASSERT_GE(pnode->inputs.size(), 1);

    return std::make_shared<operator_impl>(pnode->inputs.size());
  }

  static std::vector<flex_type_enum> infer_type(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::UNION_NODE);
    ASSERT_GE(pnode->inputs.size(), 1);

    std::vector<flex_type_enum> types = infer_planner_node_type(pnode->inputs[0]);

    for(size_t i = 1; i < pnode->inputs.size(); ++i) {
      auto input_types = infer_planner_node_type(pnode->inputs[i]);
      types.insert(types.end(), input_types.begin(), input_types.end());
    }

    return types;
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::UNION_NODE);
    return infer_planner_node_length(pnode->inputs[0]);
  }

  static std::string repr(std::shared_ptr<planner_node> pnode, pnode_tagger& get_tag) {
    std::ostringstream ss;
    ss << "Union(";
    bool is_first = true;
    for(const auto& pn : pnode->inputs) {
      if(!is_first) ss << ',';
      is_first = false;
      ss << get_tag(pn);
    }
    ss << ')';

    return ss.str();
  }
};

typedef operator_impl<planner_node_type::UNION_NODE> op_union;

/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_UNION_HPP
