/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_IDENTITY_NODE_HPP_
#define TURI_SFRAME_QUERY_MANAGER_IDENTITY_NODE_HPP_

#include <core/data/flexible_type/flexible_type.hpp>
#include <core/random/random.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>

namespace turi {
namespace query_eval {

/**
 * A no-op operator. Does not have a physical equivalent but only has
 * a logical form. Used as a sentinel for the query optimizer.
 */
template<>
class operator_impl<planner_node_type::IDENTITY_NODE> : public query_operator {
 public:
  planner_node_type type() const { return planner_node_type::IDENTITY_NODE; }

  static std::string name() { return "identity_node";  }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::LOGICAL_NODE_ONLY;
    ret.num_inputs = 1;
    return ret;
  }

  ////////////////////////////////////////////////////////////////////////////////

  inline operator_impl() {}

  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>(*this);
  }

  static std::shared_ptr<planner_node> make_planner_node(std::shared_ptr<planner_node> pnode) {
    auto pn = planner_node::make_shared(planner_node_type::IDENTITY_NODE);
    pn->inputs = {pnode};
    return pn;
  }

  static std::vector<flex_type_enum> infer_type(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ(pnode->inputs.size(), 1);
    return infer_planner_node_type(pnode->inputs[0]);
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ(pnode->inputs.size(), 1);
    return infer_planner_node_length(pnode->inputs[0]);
  }

};

typedef operator_impl<planner_node_type::IDENTITY_NODE> optonly_identity_operator;

} // query_eval
} // turicreate

#endif
