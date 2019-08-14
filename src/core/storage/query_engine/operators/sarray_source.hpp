/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_SARRAY_SOURCE_HPP
#define TURI_SFRAME_QUERY_MANAGER_SARRAY_SOURCE_HPP
#include <sstream>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/planning/planner_node.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/util/coro.hpp>

namespace turi {
namespace query_eval {


template <>
struct operator_impl<planner_node_type::SARRAY_SOURCE_NODE> : public query_operator{

/**
 * \ingroup sframe_query_engine
 * \addtogroup operators Logical Operators
 * \{
 */

  /**
 * A "sarray_source" operator generates values from a physical sarray.
 */
 public:
  DECL_CORO_STATE(execute);
  size_t start = 0;
  size_t block_size = 0;
  bool skip_next_block = false;
  size_t end;
  std::shared_ptr<sframe_rows>  rows;

  planner_node_type type() const { return planner_node_type::SARRAY_SOURCE_NODE; }

  static std::string name() { return "sarray_source"; }

  inline operator_impl(std::shared_ptr<sarray<flexible_type> > source,
                       size_t begin_index = 0, size_t end_index = size_t(-1) )
      : m_source(source)
      , m_begin_index(begin_index)
      , m_end_index(end_index == size_t(-1) ? m_source->size() : end_index)
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

  inline bool coro_running() const {
    return CORO_RUNNING(execute);
  }
  inline void execute(query_context& context) {
    CORO_BEGIN(execute)
    if (!m_reader) m_reader = m_source->get_reader();
    start = m_begin_index;
    block_size = context.block_size();
    skip_next_block = context.should_skip();

    while (start != m_end_index) {
      rows = context.get_output_buffer();
      end = std::min(start + block_size, m_end_index);
      if (skip_next_block == false) {
        m_reader->read_rows(start, end, *rows);
        context.emit(rows);
        CORO_YIELD();
      } else {
        context.emit(nullptr);
        CORO_YIELD();
      }
      skip_next_block = context.should_skip();
      start = end;
    }
    CORO_END
  }

  static std::shared_ptr<planner_node> make_planner_node(
      std::shared_ptr<sarray<flexible_type> > source, size_t begin_index = 0, size_t _end_index = -1) {
    std::stringstream strm;
    oarchive oarc(strm);
    oarc << source->get_index_info();
    auto type = source->get_type();

    size_t end_index = (_end_index == size_t(-1)) ? source->size() : _end_index;

    DASSERT_LE(begin_index, end_index);
    DASSERT_LE(end_index, source->size());

    // we need to keep a copy of the source in the node for reference counting
    // reasons.
    return planner_node::make_shared(planner_node_type::SARRAY_SOURCE_NODE,
                                     {{"index", strm.str()},
                                      {"type", (flex_int)type},
                                      {"begin_index", begin_index},
                                      {"end_index", end_index}},
                                     {{"sarray", any(source)}});
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type,
              (int)planner_node_type::SARRAY_SOURCE_NODE);
    ASSERT_TRUE(pnode->any_operator_parameters.count("sarray"));
    auto source = pnode->any_operator_parameters["sarray"]
        .as<std::shared_ptr<sarray<flexible_type>>>();

    size_t begin_index = pnode->operator_parameters.at("begin_index");
    size_t end_index = pnode->operator_parameters.at("end_index");

    return std::make_shared<operator_impl>(source, begin_index, end_index);
  }

  static std::vector<flex_type_enum> infer_type(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::SARRAY_SOURCE_NODE);
    flex_type_enum type =
        (flex_type_enum)(flex_int)(pnode->operator_parameters["type"]);
    return {type};
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::SARRAY_SOURCE_NODE);
    flex_int length = (pnode->operator_parameters.at("end_index")
                       - pnode->operator_parameters.at("begin_index"));
    return length;
  }

  /** Given an sarray, returns a small number uniquely associated with
   * that sarray.  This number is unique over the course of the
   * program run.
   */
  static size_t unique_sarray_tag(const std::shared_ptr<sarray<flexible_type> >& sa) {
    static mutex access_lock;
    std::lock_guard<mutex> _lg(access_lock);

    static size_t current_number = 0;
    static std::map<ptrdiff_t, std::pair<std::weak_ptr<sarray<flexible_type> >, size_t> > tracked_numbers;

    ptrdiff_t key = ptrdiff_t(sa.get());

    auto it = tracked_numbers.find(key);

    if(it != tracked_numbers.end()) {
      if(!it->second.first.expired())
        return it->second.second;
      else
        tracked_numbers.erase(it);
    }

    ++current_number;

    // Purge out expired weak pointers if they are present
    if(current_number % 256 == 0) {
      for(auto it = tracked_numbers.begin(); it != tracked_numbers.end();) {
        if(it->second.first.expired())
          it = tracked_numbers.erase(it);
        else
          ++it;
      }
    }

    tracked_numbers[key] = {sa, current_number};
    return current_number;
  }

  static std::string repr(std::shared_ptr<planner_node> pnode, pnode_tagger&) {
    std::ostringstream out;

    auto source = pnode->any_operator_parameters["sarray"]
        .as<std::shared_ptr<sarray<flexible_type> > >();

    out << "S" << unique_sarray_tag(source);

    size_t begin_index = pnode->operator_parameters.at("begin_index");
    size_t end_index = pnode->operator_parameters.at("end_index");

    if(begin_index != 0 || end_index != source->size()) {
      out << "[" << begin_index << "," << end_index << "]";
    }
    return out.str();
  }

 private:
  std::shared_ptr<sarray<flexible_type>> m_source;
  size_t m_begin_index, m_end_index;
  std::shared_ptr<sarray_reader<flexible_type>> m_reader;
};

typedef operator_impl<planner_node_type::SARRAY_SOURCE_NODE> op_sarray_source;

/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_SARRAY_SOURCE_HPP
