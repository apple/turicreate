/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_GENERALIZED_UNION_PROJECT_NODE_HPP
#define TURI_SFRAME_QUERY_MANAGER_GENERALIZED_UNION_PROJECT_NODE_HPP

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
struct operator_impl<planner_node_type::GENERALIZED_UNION_PROJECT_NODE> : public query_operator {
 public:
  DECL_CORO_STATE(execute);

  std::vector<std::shared_ptr<const sframe_rows> > input_v;

  std::vector<
    std::vector<
      std::shared_ptr<
        std::vector<turi::flexible_type> > > > input_columns;

  planner_node_type type() const { return planner_node_type::GENERALIZED_UNION_PROJECT_NODE; }

  static std::string name() { return "union-project"; }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::LINEAR;
    ret.num_inputs = -1;
    return ret;
  }

  inline operator_impl(size_t _num_inputs) : num_inputs(_num_inputs) {}

  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>(*this);
  }

  inline bool coro_running() const {
    return CORO_RUNNING(execute);
  }
  inline void execute(query_context& context) {
    CORO_BEGIN(execute)
    input_v.resize(num_inputs);
    input_columns.resize(num_inputs);

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

      for(size_t i = 0; i < num_inputs; ++i) {
        input_columns[i] = std::move(input_v[i]->get_columns());
      }

      auto out = context.get_output_buffer();
      auto& out_columns = out->get_columns();
      out_columns.clear();

      for(const std::pair<size_t, size_t>& p : index_map) {
        out_columns.push_back(input_columns[p.first][p.second]);
      }

      context.emit(out);
      }
      CORO_YIELD();
    }
    CORO_END
  }

 private:
  size_t num_inputs = 0;

  // List of (input, columns) making up the output column
  std::vector<std::pair<size_t, size_t> > index_map;

 public:

  static std::shared_ptr<planner_node> make_planner_node(
      const std::vector<std::shared_ptr<planner_node> >& inputs,
      const std::vector<std::pair<size_t, size_t> >& index_mappings) {

    ASSERT_GE(inputs.size(), 1);

    flex_dict _index_map(index_mappings.begin(), index_mappings.end());

    return planner_node::make_shared(
        planner_node_type::GENERALIZED_UNION_PROJECT_NODE,
      { {"index_map", _index_map} },
        std::map<std::string, any>(),
        inputs);
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::GENERALIZED_UNION_PROJECT_NODE);
    ASSERT_GE(pnode->inputs.size(), 1);

    auto ret = std::make_shared<operator_impl>(pnode->inputs.size());

    const flex_dict& _index_map = pnode->operator_parameters.at("index_map").get<flex_dict>();
    ret->index_map.assign(_index_map.begin(), _index_map.end());

    return ret;
  }

  static std::vector<flex_type_enum> infer_type(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::GENERALIZED_UNION_PROJECT_NODE);
    ASSERT_GE(pnode->inputs.size(), 1);

    std::vector<std::vector<flex_type_enum> > raw_types(pnode->inputs.size());

    for(size_t i = 0; i < pnode->inputs.size(); ++i) {
      raw_types[i] = infer_planner_node_type(pnode->inputs[i]);
    }

    const flex_dict& _index_map = pnode->operator_parameters.at("index_map").get<flex_dict>();
    std::vector<flex_type_enum> out_types(_index_map.size());

    for(size_t i = 0; i < _index_map.size(); ++i) {
      out_types[i] = raw_types[_index_map[i].first.get<flex_int>()][_index_map[i].second.get<flex_int>()];
    }

    return out_types;
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::GENERALIZED_UNION_PROJECT_NODE);
    return infer_planner_node_length(pnode->inputs[0]);
  }

  static std::string repr(std::shared_ptr<planner_node> pnode, pnode_tagger& get_tag) {
    std::ostringstream out;
    out << "UP(";

    const flex_dict& _index_map = pnode->operator_parameters.at("index_map").get<flex_dict>();

    if(!_index_map.empty()) {

      std::vector<std::pair<size_t, std::vector<size_t> > >
          groups{ {size_t(_index_map[0].first), { size_t(_index_map[0].second)} } };

      for(size_t i = 1; i < _index_map.size(); ++i) {
        if(_index_map[i].first == groups.back().first
           && _index_map[i].second == groups.back().second.back() + 1) {
          groups.back().second.push_back(size_t(_index_map[i].second));
        } else {
          groups.push_back({_index_map[i].first, {_index_map[i].second} });
        }
      }

      bool is_first = true;
      for(size_t i = 0; i < groups.size(); ++i) {
        const auto& grp = groups[i];

        if(!is_first) {
          if(groups[i - 1].first != groups[i].first)
            out << ";";
          else
            out << ",";
        }

        is_first = false;

        if(i == 0 || grp.first != groups[i-1].first)
          out << get_tag(pnode->inputs[grp.first]) << ':';

        switch(grp.second.size()) {
          case 1:
            out << grp.second[0];
            break;
          case 2:
            out << grp.second[0] << ',' << grp.second[1];
            break;
          case 3:
            out << grp.second[0] << ',' << grp.second[1] << ',' << grp.second[2];
            break;
          default:
            out << grp.second.front() << ",...," << grp.second.back();
            break;
        }
      }
    }

    out << ')';

    return out.str();
  }
};

typedef operator_impl<planner_node_type::GENERALIZED_UNION_PROJECT_NODE> op_union_project;

/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_UNION_HPP
