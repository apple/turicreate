/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_LAMBDA_TRANSFORM_HPP
#define TURI_SFRAME_QUERY_MANAGER_LAMBDA_TRANSFORM_HPP
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/system/lambda/pylambda_function.hpp>
#include <core/system/exceptions/error_types.hpp>
namespace turi {
namespace query_eval {

/**
 * \ingroup sframe_query_engine
 * \addtogroup operators Logical Operators
 * \{
 */

/**
 * A "transform" operator that applies a python lambda function to a
 * single stream of input.
 */
template<>
class operator_impl<planner_node_type::LAMBDA_TRANSFORM_NODE> : public query_operator {

 public:
  planner_node_type type() const { return planner_node_type::LAMBDA_TRANSFORM_NODE; }

  static std::string name() { return "lambda_transform";  }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::LINEAR;
    ret.num_inputs = 1;
    return ret;
  }

  ////////////////////////////////////////////////////////////////////////////////
  inline operator_impl(std::shared_ptr<lambda::pylambda_function> lambda,
                       flex_type_enum output_type,
                       const std::vector<std::string>& column_names = {})
      : m_lambda(lambda), m_output_type(output_type),
        m_column_names(column_names) { }

  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>(*this);
  }

  inline void execute(query_context& context) {
    while(1) {
      auto rows = context.get_next(0);
      if (rows == nullptr)
        break;

      auto output = context.get_output_buffer();
      output->resize(1, rows->num_rows());
      std::vector<flexible_type> out;

      // TODO exception handling
      if (m_column_names.empty()) {
        // evalute on sarray
        m_lambda->eval(*rows, out);
      } else {
        // need column names to evalute on sframe
        m_lambda->eval(m_column_names, *rows, out);
      }

      for (size_t i = 0;i < out.size(); ++i) {
        (*output)[i][0] = convert_value_to_output_type(out[i], m_output_type);
      }
      context.emit(output);
    }
  }

  static std::shared_ptr<planner_node> make_planner_node(
      std::shared_ptr<planner_node> source,
      const std::string& lambda_str,
      flex_type_enum output_type,
      const std::vector<std::string> column_names = {},
      bool skip_undefined = false,
      int random_seed = -1) {

    flex_list column_names_list(column_names.begin(), column_names.end());
    auto lambda_function = std::make_shared<lambda::pylambda_function>(lambda_str);
    lambda_function->set_skip_undefined(skip_undefined);
    lambda_function->set_random_seed(random_seed);
    return planner_node::make_shared(planner_node_type::LAMBDA_TRANSFORM_NODE,
                                     {{"output_type", (int)(output_type)},
                                      {"lambda_str", lambda_str},
                                      {"skip_undefined", (int)(skip_undefined)},
                                      {"random_seed", (int)(random_seed)},
                                      {"column_names", column_names_list}},
                                      {{"lambda_fn", any(lambda_function)}},
                                     {source});
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::LAMBDA_TRANSFORM_NODE);
    ASSERT_EQ(pnode->inputs.size(), 1);
    ASSERT_TRUE(pnode->operator_parameters.count("output_type"));
    ASSERT_TRUE(pnode->operator_parameters.count("lambda_str"));
    ASSERT_TRUE(pnode->operator_parameters.count("column_names"));
    ASSERT_TRUE(pnode->operator_parameters.count("skip_undefined"));
    ASSERT_TRUE(pnode->operator_parameters.count("random_seed"));
    ASSERT_TRUE(pnode->any_operator_parameters.count("lambda_fn"));

    flex_type_enum output_type =
        (flex_type_enum)(flex_int)(pnode->operator_parameters["output_type"]);
    flex_list column_names_list =
        (pnode->operator_parameters["column_names"]).get<flex_list>();
    std::vector<std::string> column_names(column_names_list.begin(), column_names_list.end());

    auto fn = pnode->any_operator_parameters["lambda_fn"]
                            .as<std::shared_ptr<lambda::pylambda_function>>();
    return std::make_shared<operator_impl>(fn, output_type, column_names);
  }

  static std::vector<flex_type_enum> infer_type(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::LAMBDA_TRANSFORM_NODE);
    ASSERT_TRUE(pnode->operator_parameters.count("output_type"));
    return {(flex_type_enum)(int)(pnode->operator_parameters["output_type"])};
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::LAMBDA_TRANSFORM_NODE);
    return infer_planner_node_length(pnode->inputs[0]);
  }

  static std::string repr(std::shared_ptr<planner_node> pnode, pnode_tagger&) {
    std::ostringstream out;
    out << "PyLambda";

    flex_list column_names_list =
        (pnode->operator_parameters["column_names"]).get<flex_list>();

    if(!column_names_list.empty()) {
      out << "(";

      for(size_t i = 0; i < column_names_list.size() - 1; ++i) {
        out << column_names_list[i] << ",";
      }

      out << column_names_list.back() << ")";
    }

    return out.str();
  }

 private:
  std::shared_ptr<lambda::pylambda_function> m_lambda;
  flex_type_enum m_output_type;
  std::vector<std::string> m_column_names;

 private:
  /**
   * Helper function to convert flexible_type value to expected type.
   */
  static flexible_type convert_value_to_output_type(const flexible_type& val,
                                                    flex_type_enum type) {
    if (val.get_type() == type ||
        val.get_type() == flex_type_enum::UNDEFINED ||
        type == flex_type_enum::UNDEFINED) {
      return val;
    } else if (flex_type_is_convertible(val.get_type(), type)) {
      flexible_type res(type);
      res.soft_assign(val);
      return res;
    } else if ( (val.get_type() == flex_type_enum::VECTOR &&
                 type == flex_type_enum::LIST)
               || (val.get_type() == flex_type_enum::LIST &&
                   type == flex_type_enum::VECTOR)) {
      // empty lists / vectors cast between each other.
      flexible_type res(type);
      res.soft_assign(val);
      return res;
    } else {
      std::string message = "Cannot convert " + std::string(val) +
          " to " + flex_type_enum_to_name(type);
      logstream(LOG_ERROR) <<  message << std::endl;
      throw(bad_cast(message));
    }
  }
};

typedef operator_impl<planner_node_type::LAMBDA_TRANSFORM_NODE> op_lambda_transform;

/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_TRANSFORM_HPP
