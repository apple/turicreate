/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_RANGE_HPP
#define TURI_SFRAME_QUERY_MANAGER_RANGE_HPP
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
 * A "range" operator which simply generates a range of integer values.
 */
template <>
struct operator_impl<planner_node_type::RANGE_NODE> : public query_operator {
 public:
  DECL_CORO_STATE(execute);
  flex_int cur;

  inline planner_node_type type() const { return planner_node_type::RANGE_NODE; }

  static std::string name() { return "range"; }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::SOURCE;
    ret.num_inputs = 0;
    return ret;
  }

  inline operator_impl(flex_int start, flex_int end)
  : m_start(start)
  , m_end(end)
  {
    ASSERT_LE(m_start, m_end);
  }

  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>(m_start, m_end);
  }
  inline bool coro_running() const {
    return CORO_RUNNING(execute);
  }

  inline void execute(query_context& context) {

    CORO_BEGIN(execute)
    cur = m_start;

    while(cur < m_end) {
      {
      auto ret = context.get_output_buffer();
      size_t len = std::min<size_t>(m_end - cur, context.block_size());

      ret->resize(1, len);
      for (auto& value: *(ret->get_columns()[0])) {
        value = cur;
        ++cur;
      }
      context.emit(ret);
      }
      CORO_YIELD();
    }
    CORO_END
  }

  static std::shared_ptr<planner_node> make_planner_node(
      flex_int start, flex_int end) {
    return planner_node::make_shared(planner_node_type::RANGE_NODE,
                                     {{"start", start},
                                      {"begin_index", 0},
                                      {"end_index", end - start}});
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::RANGE_NODE);
    ASSERT_TRUE(pnode->operator_parameters.count("start"));
    ASSERT_TRUE(pnode->operator_parameters.count("begin_index"));
    ASSERT_TRUE(pnode->operator_parameters.count("end_index"));

    flex_int start = (flex_int)pnode->operator_parameters["start"];
    size_t begin_index = pnode->operator_parameters["begin_index"];
    size_t end_index = pnode->operator_parameters["end_index"];
    return std::make_shared<operator_impl>(start + begin_index,
                                           start + end_index);
  }

  static std::vector<flex_type_enum> infer_type(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::RANGE_NODE);
    return {flex_type_enum::INTEGER};
  }


  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::RANGE_NODE);
    size_t count = pnode->operator_parameters["end_index"] -
                   pnode->operator_parameters["begin_index"];
    return count;
  }

  static std::string repr(std::shared_ptr<planner_node> pnode, pnode_tagger&) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::RANGE_NODE);
    ASSERT_TRUE(pnode->operator_parameters.count("start"));
    ASSERT_TRUE(pnode->operator_parameters.count("begin_index"));
    ASSERT_TRUE(pnode->operator_parameters.count("end_index"));

    flex_int start = pnode->operator_parameters["start"];
    size_t begin_index = pnode->operator_parameters["begin_index"];
    size_t end_index = pnode->operator_parameters["end_index"];

    std::ostringstream ss;

    ss << "Sequence(" << start << ")[" << begin_index << ":" << end_index << "]";

    return ss.str();
  }

 private:
  // m_start to m_end defines the range
  flex_int m_start; // inclusive
  flex_int m_end;   // exclusive
};

typedef operator_impl<planner_node_type::RANGE_NODE> op_range;

/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_RANGE_HPP
