/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_SFRAME_SOURCE_HPP
#define TURI_SFRAME_QUERY_MANAGER_SFRAME_SOURCE_HPP

#include <sstream>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/operators/sarray_source.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/storage/sframe_data/sframe.hpp>

namespace turi {
namespace query_eval {

/**
 * \ingroup sframe_query_engine
 * \addtogroup operators Logical Operators
 * \{
 */

/**
 * A "sframe_source" operator generates values from a physical sarray.
 */
template <>
struct operator_impl<planner_node_type::SFRAME_SOURCE_NODE> : public query_operator {
 public:

  planner_node_type type() const { return planner_node_type::SFRAME_SOURCE_NODE; }

  static std::string name() { return "sframe_source"; }

  inline operator_impl(sframe source, size_t begin_index = 0, size_t end_index = size_t(-1) )
      : m_source(source)
      , m_begin_index(begin_index)
      , m_end_index(end_index == size_t(-1) ? m_source.size() : end_index)
  { }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::SOURCE |
        query_operator_attributes::SUPPORTS_SKIPPING;
    ret.num_inputs = 0;
    return ret;
  }


  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>(m_source);
  }

  inline void execute(query_context& context) {
    if (!m_reader) m_reader = m_source.get_reader();
    auto start = m_begin_index;
    std::shared_ptr<sframe_rows> rows;
    auto block_size = context.block_size();
    bool skip_next_block = false;
    emit_state state = context.initial_state();

    while (start != m_end_index) {
      auto rows = context.get_output_buffer();
      auto end = std::min(start + block_size, m_end_index);
      if (skip_next_block == false) {
        m_reader->read_rows(start, end, *rows);
        state = context.emit(rows);
      } else {
        state = context.emit(nullptr);
      }
      skip_next_block = state == emit_state::SKIP_NEXT_BLOCK;
      start = end;
    }
  }

  static std::shared_ptr<planner_node> make_planner_node(
      sframe source, size_t begin_index = 0, size_t _end_index = -1) {
    std::stringstream strm;
    oarchive oarc(strm);
    oarc << source.get_index_info();
    auto types = source.column_types();

    size_t end_index = (_end_index == size_t(-1)) ? source.size() : _end_index;
    DASSERT_LE(begin_index, end_index);
    DASSERT_LE(end_index, source.size());

    flex_list type_list(types.size());
    for (size_t i = 0; i < types.size(); ++i) {
      type_list[i] = flex_int(types[i]);
    }


    // we need to keep a copy of the source in the node for reference counting
    // reasons.
    return planner_node::make_shared(planner_node_type::SFRAME_SOURCE_NODE,
                                     {{"index", strm.str()},
                                      {"types", type_list},
                                      {"begin_index", begin_index},
                                      {"end_index", end_index}},
                                     {{"sframe", any(source)}});
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type,
              (int)planner_node_type::SFRAME_SOURCE_NODE);

    ASSERT_TRUE(pnode->any_operator_parameters.count("sframe"));
    auto source = pnode->any_operator_parameters.at("sframe").as<sframe>();

    size_t begin_index = pnode->operator_parameters.at("begin_index");
    size_t end_index = pnode->operator_parameters.at("end_index");

    return std::make_shared<operator_impl>(source, begin_index, end_index);
  }

  static std::vector<flex_type_enum> infer_type(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::SFRAME_SOURCE_NODE);
    flex_list type = pnode->operator_parameters.at("types");
    std::vector<flex_type_enum> ret;
    for (auto t: type) ret.push_back((flex_type_enum)(flex_int)(t));
    return ret;
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::SFRAME_SOURCE_NODE);
    flex_int length = (pnode->operator_parameters.at("end_index")
                       - pnode->operator_parameters.at("begin_index"));
    return length;
  }

  static std::string repr(std::shared_ptr<planner_node> pnode, pnode_tagger&) {
    std::ostringstream out;

    auto source = pnode->any_operator_parameters.at("sframe").as<sframe>();

    out << "SF(";

    if(source.num_columns() > 0) {
      std::vector<size_t> columns(source.num_columns());

      for(size_t i = 0; i < source.num_columns(); ++i) {
        columns[i] = op_sarray_source::unique_sarray_tag(source.select_column(i));
      }

      // Now, make subsequent numbers the same
      std::vector<std::vector<size_t> > groups{ {columns[0]} };

      for(size_t i = 1; i < columns.size();++i) {
        if(columns[i] == groups.back().back() + 1)
          groups.back().push_back(columns[i]);
        else
          groups.push_back({columns[i]});
      }

      bool is_first = true;
      for(const std::vector<size_t>& grp : groups) {
        if(!is_first)
          out << ",";

        is_first = false;

        switch(grp.size()) {
          case 1:
            out << "S" << grp[0];
            break;
          case 2:
            out << "S" << grp[0] << ",S" << grp[1];
            break;
          default:
            out << "S" << grp.front() << ",...,S" << grp.back();
            break;
        }
      }

    }
    out << ")";

    size_t begin_index = pnode->operator_parameters.at("begin_index");
    size_t end_index = pnode->operator_parameters.at("end_index");

    if(begin_index != 0 || end_index != source.num_rows()) {
      out << "[" << begin_index << "," << end_index << "]";
    }

    return out.str();
  }

 private:
  sframe m_source;
  size_t m_begin_index, m_end_index;
  std::shared_ptr<sframe_reader> m_reader;
};

typedef operator_impl<planner_node_type::SFRAME_SOURCE_NODE> op_sframe_source;

/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_SFRAME_SOURCE_HPP
