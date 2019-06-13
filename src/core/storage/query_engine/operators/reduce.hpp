/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_REDUCE_HPP
#define TURI_SFRAME_QUERY_MANAGER_REDUCE_HPP

#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/group_aggregate_value.hpp>
#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <iostream>

namespace turi {
namespace query_eval {

/**
 * \ingroup sframe_query_engine
 * \addtogroup operators Logical Operators
 * \{
 */

/**
 * Takes a stream of input reducing it to a single value.
 * Uses the \ref group_aggregate_value class to define the reduction operations.
 */
template <>
struct operator_impl<planner_node_type::REDUCE_NODE> : public query_operator {
 public:

  planner_node_type type() const { return planner_node_type::REDUCE_NODE; }

  static std::string name() { return "reduce"; }

  inline operator_impl(std::shared_ptr<group_aggregate_value> aggregator,
                       flex_type_enum output_type)
      : m_aggregator(aggregator)
      , m_output_type(output_type) { }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::SUB_LINEAR;
    ret.num_inputs = 1;
    return ret;
  }

  inline std::shared_ptr<query_operator> clone() const {
    std::shared_ptr<group_aggregate_value> agg(m_aggregator->new_instance());
    return std::make_shared<operator_impl>(agg, m_output_type);
  }

  inline void execute(query_context& context) {
    while(1) {
      auto rows = context.get_next(0);
      if (rows == nullptr)
        break;
      for (const auto& row : *rows) {
        // TODO make add_element take a sframe_row::row_reference instead
        if (row.size() == 1) m_aggregator->add_element_simple(row[0]);
        else m_aggregator->add_element(std::vector<flexible_type>(row));
      }
    }
    auto out = context.get_output_buffer();
    out->resize(1, 1);
    (*out)[0][0] = m_aggregator->emit();
    context.emit(out);
  }

  static std::shared_ptr<planner_node> make_planner_node(
      std::shared_ptr<planner_node> source,
      group_aggregate_value& aggregator,
      flex_type_enum output_type) {
    std::shared_ptr<group_aggregate_value> agg(aggregator.new_instance());
    return planner_node::make_shared(planner_node_type::REDUCE_NODE,
                                     {{"output_type", (int)(output_type)}},
                                     {{"aggregator", any(agg)}},
                                     {source});
  }

  static std::shared_ptr<query_operator> from_planner_node(

      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::REDUCE_NODE);
    ASSERT_EQ(pnode->inputs.size(), 1);
    ASSERT_TRUE(pnode->operator_parameters.count("output_type"));
    ASSERT_TRUE(pnode->any_operator_parameters.count("aggregator"));

    std::shared_ptr<group_aggregate_value> aggregator =
        pnode->any_operator_parameters["aggregator"].as<std::shared_ptr<group_aggregate_value>>();
    flex_type_enum output_type =
        (flex_type_enum)(flex_int)(pnode->operator_parameters["output_type"]);

    std::shared_ptr<group_aggregate_value> agg(aggregator->new_instance());
    return std::make_shared<operator_impl>(agg, output_type);
  }

  static std::vector<flex_type_enum> infer_type(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::REDUCE_NODE);
    ASSERT_TRUE(pnode->operator_parameters.count("output_type"));
    return {(flex_type_enum)(int)(pnode->operator_parameters["output_type"])};
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    return -1;
  }

  static std::string print(std::shared_ptr<planner_node> pnode) {
    ASSERT_TRUE(pnode->any_operator_parameters.count("aggregator"));

    std::shared_ptr<group_aggregate_value> aggregator =
        pnode->any_operator_parameters["aggregator"].as<std::shared_ptr<group_aggregate_value> >();

    return std::string("Agg.") + aggregator->name();
  }

 private:
  std::shared_ptr<group_aggregate_value> m_aggregator;
  flex_type_enum m_output_type;
};

typedef operator_impl<planner_node_type::REDUCE_NODE> op_reduce;

/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_TRANSFORM_HPP
