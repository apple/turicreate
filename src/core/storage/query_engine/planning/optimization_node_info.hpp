/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_ENGINE_QUERY_NODE_INFO_H_
#define TURI_SFRAME_QUERY_ENGINE_QUERY_NODE_INFO_H_

#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/storage/query_engine/planning/planner_node.hpp>
#include <memory>
#include <set>

namespace turi { namespace query_eval {

struct node_info;
typedef std::shared_ptr<node_info> node_info_ptr;
typedef std::shared_ptr<const node_info> cnode_info_ptr;

/**  The node_info is the struct over which the optimizations are
 *   performed.  It is here to make the query optimization and
 *   execution easy to write and work with.
 *
 */
struct node_info {

  node_info(std::shared_ptr<planner_node> _pnode)
      : pnode(_pnode)
      , type(pnode->operator_type)
      , attributes(planner_node_type_to_attributes(type))
  {

  }

  /** The planner node we are working with.
   */
  std::shared_ptr<planner_node> pnode;

  /** The type of the planner node.
   */
  planner_node_type type;

  /**  The attributes from the operator class.
   */
  query_operator_attributes attributes;

  /** Used internally to track and build the graph.  Used for the
   *  query optimizer.
   */
  std::vector<node_info_ptr> inputs, outputs;

  ////////////////////////////////////////////////////////////////////////////////
  // Buffer stuff used in the optimization

  bool node_visited = false;

  // Marked as discarded; typically because another node replaced it.
  bool node_discarded = false;

  ////////////////////////////////////////////////////////////////////////////////
  // Handy methods for the optimization

 private: mutable size_t _num_columns = size_t(-1);
 public:

  /** The number of output columns.  Cached.
   */
  size_t num_columns() const {
    if(_num_columns == size_t(-1))
      _num_columns = infer_planner_node_num_output_columns(pnode);

    return _num_columns;
  }

  bool is_source_node() const { return query_eval::is_source_node(attributes); }

  bool is_linear_transform() const { return query_eval::is_linear_transform(attributes); }

  bool is_sublinear_transform() const { return query_eval::is_sublinear_transform(attributes); }

  size_t length() const { return size_t(infer_planner_node_length(pnode)); }

  ////////////////////////////////////////////////////////////////////////////////
  // Shortcut functions for accessing the parameters.

  const flexible_type& p(const std::string& s) const {
    auto it = this->pnode->operator_parameters.find(s);
    ASSERT_MSG(it != this->pnode->operator_parameters.end(),
               (std::string("Parameter ") + s + " not valid in node of type "
                + planner_node_type_to_name(this->type)).c_str());


    return it->second;
  }

  bool has_p(const std::string& s) const {
    auto it = this->pnode->operator_parameters.find(s);
    return (it != this->pnode->operator_parameters.end());
  }

  template <typename T>
  const T& any_p(const std::string& s) const {
    auto it = this->pnode->any_operator_parameters.find(s);
    ASSERT_MSG(it != this->pnode->any_operator_parameters.end(),
               (std::string("Any-parameter ") + s + " not valid in node of type "
                + planner_node_type_to_name(this->type)).c_str());

    return it->second.as<T>();
  }

  bool has_any_p(const std::string& s) const {
    auto it = this->pnode->any_operator_parameters.find(s);
    return (it != this->pnode->any_operator_parameters.end());
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Convenience functions

  inline bool input_type_present(planner_node_type t, size_t threshhold = 1) const {
    size_t count = 0;

    for(const auto& n : inputs) {
      if(n->type == t) {
        ++count;
        if(threshhold == 1 || count >= threshhold)
          return true;
      }
    }

    return false;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Debugging tool

 private:
#ifndef NDEBUG
  inline void _debug_check_consistency(std::set<const node_info*>& seen) const {
    if(seen.count(this))
      return;
    seen.insert(this);

    DASSERT_TRUE(pnode->operator_type == type);
    DASSERT_EQ(pnode->inputs.size(), inputs.size());
    DASSERT_TRUE(is_source_node() || !inputs.empty());

    if(attributes.num_inputs != -1)
      DASSERT_EQ(inputs.size(), attributes.num_inputs);

    DASSERT_EQ(num_columns(), infer_planner_node_num_output_columns(pnode));

    {
      // Make sure that all inputs and outputs are consistent.
      std::map<const node_info*, size_t> input_counts;
      for(size_t i = 0; i < inputs.size(); ++i)
        input_counts[inputs[i].get()] += 1;

      for(size_t i = 0; i < inputs.size(); ++i) {

        DASSERT_TRUE(pnode->inputs[i] == inputs[i]->pnode);
        size_t n_present = 0;
        for(const cnode_info_ptr& out : inputs[i]->outputs) {

          if(out.get() == this)
            ++n_present;
        }

        DASSERT_EQ(n_present, input_counts.at(inputs[i].get()));
      }
    }

    {
      // Make sure that all inputs and outputs are consistent.
      std::map<const node_info*, size_t> output_counts;
      for(size_t i = 0; i < outputs.size(); ++i)
        output_counts[outputs[i].get()] += 1;

      for(size_t i = 0; i < outputs.size(); ++i) {
        size_t n_present = 0;
        for(const cnode_info_ptr& out : outputs[i]->inputs) {
          if(out.get() == this)
            ++n_present;
        }
        DASSERT_EQ(n_present, output_counts.at(outputs[i].get()));
      }

    }

    for(size_t i = 0; i < outputs.size(); ++i) {
      outputs[i]->_debug_check_consistency(seen);
    }

    for(size_t i = 0; i < inputs.size(); ++i) {
      inputs[i]->_debug_check_consistency(seen);
    }
  }
#endif
 public:
  inline void _debug_check_consistency() const {
#ifndef NDEBUG
    std::set<const node_info*> seen;
    _debug_check_consistency(seen);
#endif
  }
};

}}

#endif
