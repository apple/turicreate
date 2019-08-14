/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_ENGINE_OPERATORS_QUERY_OPERATOR_HPP
#define TURI_SFRAME_QUERY_ENGINE_OPERATORS_QUERY_OPERATOR_HPP
#include <cstddef>
#include <string>
#include <map>
#include <memory>
#include <core/util/any.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/query_engine/planning/planner_node.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
namespace turi {
namespace query_eval {

class query_context;

/**
 * \ingroup sframe_query_engine
 * \addtogroup operators Logical Operators
 * \{
 */


/**
 * Basic attributes about the operator.
 */
struct query_operator_attributes {
  enum attribute {
    NONE = 0, ///< No attribute

    LINEAR = 1, /** A linear input operator consumes input sources at the
                         same rate and emit outputs at the same rate*/
    SUB_LINEAR = 2, /** A sublinear operator consumes input sources at the
                       same rate, but may generate output at a different
                       lower or higher rate */
    SOURCE = 4, /** A source operator is a direct source from an sframe
                 * or sarray and has no inputs. */

    LOGICAL_NODE_ONLY = 8, /** A node that never turns into an
                            * executor; it simply is a logical node
                            * only, possibly used in the query
                            * optimizer. */

    SUPPORTS_SKIPPING = 256, /** If the operator can correctly handle the
                                skip_next_block emit state */



  };

  size_t attribute_bitfield;  ///< A bitfield of the attribute enum
  int num_inputs;  ///< Number of inputs expected to the operator
};

/**
 *  The query operator base class.
 *
 *  All operators must inherit from this class, implementing the
 *  virtual functions described below. The member functions describe
 *  how the class behaves, which in turn describe the capabilities of
 *  the operator and how execution is performed.
 *
 *  In addition, all of the operators must implement a set of static
 *  functions that describe how they behave.   These are:
 *
 *  name() returns the name of the operator.  Used for logging.
 *      static std::string name()
 *
 *  make_planner_node() A factory function for creating a planner node.  Takes
 *  any user defined arguments related to the operator.
 *
 *      static std::shared_ptr<planner_node> make_planner_node(...).
 *
 *  from_planner_node() converts the planner node to its operator form.
 *
 *      static std::shared_ptr<query_operator> from_planner_node(std::shared_ptr<planner_node> pnode)
 *
 *  infer_type() returns a vector of the output types for each column.
 *
 *      static std::vector<flex_type_enum> infer_type(std::shared_ptr<planner_node> pnode)
 *
 *  infer_length() Returns the length if known, and -1 otherwise.
 *
 *      static int64_t infer_length(std::shared_ptr<planner_node> pnode)
 *
 *
 */
class query_operator {
 public:

  virtual ~query_operator() = default;

  virtual planner_node_type type() const = 0;

  virtual bool coro_running() const = 0;

  /**
   * Basic execution attributes about the query.
   */
  inline query_operator_attributes attributes() {
    return planner_node_type_to_attributes(this->type());
  }

  /**
   * Pretty prints the operator including all additional parameters.
   */
  inline std::string name() const {
    return planner_node_type_to_name(this->type());
  }

  /**
   * Pretty prints the operator including all additional parameters.
   */
  virtual std::string print() const {
    return this->name();
  }

  /**
   * Makes a copy of the object.
   */
  virtual std::shared_ptr<query_operator> clone() const = 0;

  /**
   * Executes a query.
   */
  virtual void execute(query_context& context) { ASSERT_TRUE(false); }

  /** The base case -- the logical-only nodes don't use this.
   *
   */
  static std::shared_ptr<query_operator> from_planner_node(std::shared_ptr<planner_node>) {
    ASSERT_TRUE(false);
    return std::shared_ptr<query_operator>();
  }

  static std::string repr(std::shared_ptr<planner_node> pnode, pnode_tagger& get_tag) {
    return planner_node_type_to_name(pnode->operator_type);
  }

};

////////////////////////////////////////////////////////////////////////////////
// Generic fall through operator_impl implementation

template <planner_node_type PType>
class operator_impl : query_operator_attributes {

  planner_node_type type() const { return planner_node_type::INVALID; }

  std::shared_ptr<query_operator> clone() const {
    ASSERT_TRUE(false);
    return nullptr;
  }

  void execute(query_context& context) {
    ASSERT_TRUE(false);
  }
};


/// \}

} // namespace query_eval
} // namespace turi

#endif // TURI_SFRAME_QUERY_ENGINE_OPERATORS_QUERY_OPERATOR_HPP
