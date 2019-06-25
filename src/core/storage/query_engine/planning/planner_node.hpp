/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_ENGINE_PLANNER_NODE_HPP
#define TURI_SFRAME_QUERY_ENGINE_PLANNER_NODE_HPP

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/util/any.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>

namespace turi {
namespace query_eval {

class query_operator;
struct qp_info;

/**
 * \ingroup sframe_query_engine
 * \addtogroup operators Logical Operators
 * \{
 */

/**
 * The logical node describing an operator in a logical operator graph.
 *
 * The planner node is a simple datastructure comprising of just a few
 * elements:
 *  \arg \c operator_type One of the operator enumerations in operator_properties.hpp
 *  \arg \c operator_parameters (map of string->flexible_type>: The parameters for
 *  the operator. This is operator
 *  dependent and is defined by the operator itself. Generally, user's of
 *  the planner node should not need this, and should just call
 *  planner_node_traits<...>::make_plan() to create an operator node.
 *  key names begining with "__" are reserved (for instance, for memoizations, etc)
 *  \arg \c any_operator_parameters (map of string->any): Non-portable parameters.
 *  Operators which use this will generally not work for going distributed.
 *  key names begining with "__" are reserved. (for instance, for memoizations, etc)
 *  \arg \c inputs (vector of shared_ptr<planner_node>) Inputs to the operators
 *  are defined here.
 */
struct planner_node {
  planner_node(planner_node_type operator_type,
               const std::map<std::string, flexible_type>& operator_parameters
               = std::map<std::string, flexible_type>(),
               const std::map<std::string, any>& any_operator_parameters
               = std::map<std::string, any>(),
               const std::vector<std::shared_ptr<planner_node> >& inputs
               = std::vector<std::shared_ptr<planner_node> >()):
      operator_type(operator_type),
      operator_parameters(operator_parameters),
    any_operator_parameters(any_operator_parameters),
    inputs(inputs) { }

  planner_node(planner_node&&) = default;
  planner_node(const planner_node&) = default;
  planner_node& operator=(const planner_node&) = default;
  planner_node& operator=(planner_node&&) = default;

  /** The name of the operator.
   */
  planner_node_type operator_type = planner_node_type::INVALID;

  /**
   * A generic field for holding the parameters of the operator.
   */
  std::map<std::string, flexible_type> operator_parameters;

  /**
   * This field holds all other non-portable parameters. For instance,
   * function pointers, etc. Operators / Planner nodes which depend on this
   * will generally, not work for going distributed.
   */
  std::map<std::string, any> any_operator_parameters;

  /**  The inputs to the operator.
   */
  std::vector<std::shared_ptr<planner_node> > inputs;

  /** A struct to hold the accompaning info for the node.
   */
  std::shared_ptr<qp_info> qpi;

  /**
   * Makes copy of the node.
   */
  std::shared_ptr<planner_node> clone() {
    return make_shared(operator_type, operator_parameters,
                       any_operator_parameters, inputs);
  }

  /**
   * Alternative constructor which creates a shared_ptr<planner_node>
   */
  static inline std::shared_ptr<planner_node> make_shared(
      planner_node_type operator_type,
      const std::map<std::string, flexible_type>& operator_parameters
      = std::map<std::string, flexible_type>(),
      const std::map<std::string, any>& any_operator_parameters
      = std::map<std::string, any>(),
      const std::vector<std::shared_ptr<planner_node>>& inputs
      = std::vector<std::shared_ptr<planner_node>>()) {
    return std::make_shared<planner_node>(operator_type, operator_parameters,
                                          any_operator_parameters, inputs);
  }

  ////////////////////////////////////////////////////////////////////////////////

};

/// A handy typedef
typedef std::shared_ptr<planner_node> pnode_ptr;

/// \}

} // namespace query_eval
} // namespace turi



#endif // TURI_SFRAME_QUERY_ENGINE_PLANNER_NODE_HPP
