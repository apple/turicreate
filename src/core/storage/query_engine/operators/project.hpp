/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_PROJECT_HPP
#define TURI_SFRAME_QUERY_MANAGER_PROJECT_HPP

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
 * A "project" operator will project the input source to a subset
 * of indices.
 */
template <>
struct operator_impl<planner_node_type::PROJECT_NODE> : public query_operator {
 public:

  inline planner_node_type type() const { return planner_node_type::PROJECT_NODE; }

  static std::string name() { return "project"; }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::LINEAR;
    ret.num_inputs = 1;
    return ret;
  }

  inline operator_impl(const std::vector<size_t>& indices): m_indices(indices) {
    ASSERT_GT(m_indices.size(), 0);
  };

  inline std::string print() const {
    std::stringstream str_indices;
    for (size_t i : m_indices) str_indices << i << " ";
    return name() + "(" + str_indices.str() + ")";
  }

  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>(*this);
  }

  inline void execute(query_context& context) {
    while (1) {
      auto rows = context.get_next(0);
      if (rows == nullptr)
        break;

      auto out = context.get_output_buffer();
      auto& rows_columns = rows->cget_columns();
      auto& out_columns = out->get_columns();
      out_columns.clear();
      for (size_t i = 0;i < m_indices.size(); ++i) {
        DASSERT_LT(i, m_indices.size());
        out_columns.push_back(rows_columns[m_indices[i]]);
      }
      context.emit(out);
    }
  };

  static std::shared_ptr<planner_node> make_planner_node(
      std::shared_ptr<planner_node> input,
      const std::vector<size_t>& indices) {

    DASSERT_FALSE(indices.empty());

    std::vector<flexible_type> flex_indices(indices.begin(), indices.end());

#ifndef NDEBUG
    size_t num_columns = infer_planner_node_num_output_columns(input);
    for(size_t col_idx : indices)
      DASSERT_LT(col_idx, num_columns);
#endif

    return planner_node::make_shared(planner_node_type::PROJECT_NODE,
                                     {{"indices", flex_indices}},
                                     std::map<std::string, any>(),
                                     {input});
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::PROJECT_NODE);
    ASSERT_TRUE(pnode->operator_parameters.count("indices"));
    auto flex_indices = pnode->operator_parameters.at("indices").get<flex_list>();
    std::vector<size_t> indices(flex_indices.begin(), flex_indices.end());
    return std::make_shared<operator_impl>(indices);
  }

  static std::vector<flex_type_enum> infer_type(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type,
              (int)planner_node_type::PROJECT_NODE);
    auto input_type = infer_planner_node_type(pnode->inputs[0]);
    auto flex_indices = pnode->operator_parameters.at("indices").get<flex_list>();
    std::vector<flex_type_enum> ret;
    for (auto i: flex_indices) {
      ret.push_back(input_type[i.get<flex_int>()]);
    }
    return ret;
  }
  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::PROJECT_NODE);
    return infer_planner_node_length(pnode->inputs[0]);
  }

  static std::string repr(std::shared_ptr<planner_node> pnode, pnode_tagger& ) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::PROJECT_NODE);
    ASSERT_TRUE(pnode->operator_parameters.count("indices"));
    auto flex_indices = pnode->operator_parameters["indices"].get<flex_list>();

    std::ostringstream out;
    out << "PR(";

    if(flex_indices.size() > 0) {

      // Now, make subsequent numbers the same
      std::vector<std::vector<size_t> > groups{ {flex_indices[0]} };

      for(size_t i = 1; i < flex_indices.size();++i) {
        if(flex_indices[i] == groups.back().back() + 1)
          groups.back().push_back(size_t(flex_indices[i]));
        else
          groups.push_back({flex_indices[i]});
      }

      bool is_first = true;
      for(const std::vector<size_t>& grp : groups) {
        if(!is_first)
          out << ",";

        is_first = false;

        switch(grp.size()) {
          case 1:
            out << grp[0];
            break;
          case 2:
            out << grp[0] << ',' << grp[1];
            break;
          case 3:
            out << grp[0] << ',' << grp[1] << ',' << grp[2];
            break;
          default:
            out << grp.front() << ",...," << grp.back();
            break;
        }
      }
    }

    out << ')';

    return out.str();
  }

 private:
  std::vector<size_t> m_indices;
};

typedef operator_impl<planner_node_type::PROJECT_NODE> op_project;

/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_PROJECT_HPP
