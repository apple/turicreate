/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_TRANSFORM_HPP
#define TURI_SFRAME_QUERY_MANAGER_TRANSFORM_HPP
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/random/random.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/util/coro.hpp>
namespace turi {
namespace query_eval {

typedef std::function<flexible_type(const sframe_rows::row&)> transform_type;

/**
 * \ingroup sframe_query_engine
 * \addtogroup operators Logical Operators
 * \{
 */

/**
 * A "transform" operator applys a transform function on a
 * stream of input.
 */
template<>
class operator_impl<planner_node_type::TRANSFORM_NODE> : public query_operator {
 public:
  DECL_CORO_STATE(execute);
  planner_node_type type() const { return planner_node_type::TRANSFORM_NODE; }

  static std::string name() { return "transform";  }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::LINEAR;
    ret.num_inputs = 1;
    return ret;
  }

  ////////////////////////////////////////////////////////////////////////////////

  inline operator_impl(const transform_type& f,
                       flex_type_enum output_type,
                       int random_seed=-1)
      : m_transform_fn(f), m_output_type(output_type), m_random_seed(random_seed)
  { }

  inline std::shared_ptr<query_operator> clone() const {
    return std::make_shared<operator_impl>(*this);
  }

  inline bool coro_running() const {
    return CORO_RUNNING(execute);
  }
  inline void execute(query_context& context) {
    CORO_BEGIN(execute)
    if (m_random_seed != -1){
      random::get_source().seed(m_random_seed + thread::thread_id());
    }
    while(1) {
      {
      auto rows = context.get_next(0);
      if (rows == nullptr)
        break;
      auto output = context.get_output_buffer();
      output->resize(1, rows->num_rows());

      auto iter = rows->cbegin();
      auto output_iter = output->begin();
      while(iter != rows->cend()) {
        auto outval = m_transform_fn((*iter));
        if (m_output_type == flex_type_enum::UNDEFINED ||
            outval.get_type() == m_output_type ||
            outval.get_type() == flex_type_enum::UNDEFINED) {
          (*output_iter)[0] = outval;
        } else {
          flexible_type f(m_output_type);
          f.soft_assign(outval);
          (*output_iter)[0] = f;
        }
        ++output_iter;
        ++iter;
      }
      context.emit(output);
      }
      CORO_YIELD();
    }
    CORO_END
  }

  static std::shared_ptr<planner_node> make_planner_node(
      std::shared_ptr<planner_node> source,
      transform_type fn,
      flex_type_enum output_type,
      int random_seed=-1) {
    return planner_node::make_shared(planner_node_type::TRANSFORM_NODE,
                                     {{"output_type", (int)(output_type)},
                                      {"random_seed", random_seed}},
                                     {{"function", any(fn)}},
                                     {source});
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::TRANSFORM_NODE);
    ASSERT_EQ(pnode->inputs.size(), 1);
    ASSERT_TRUE(pnode->operator_parameters.count("output_type"));
    ASSERT_TRUE(pnode->any_operator_parameters.count("function"));
    transform_type fn;
    flex_type_enum output_type =
        (flex_type_enum)(flex_int)(pnode->operator_parameters["output_type"]);
    fn = pnode->any_operator_parameters["function"].as<transform_type>();
    int random_seed = (int)(flex_int)(pnode->operator_parameters["random_seed"]);
    return std::make_shared<operator_impl>(fn, output_type, random_seed);
  }

  static std::vector<flex_type_enum> infer_type(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::TRANSFORM_NODE);
    ASSERT_TRUE(pnode->operator_parameters.count("output_type"));
    return {(flex_type_enum)(int)(pnode->operator_parameters["output_type"])};
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::TRANSFORM_NODE);
    return infer_planner_node_length(pnode->inputs[0]);
  }

 private:
  transform_type m_transform_fn;
  flex_type_enum m_output_type;
  int m_random_seed;
};

typedef operator_impl<planner_node_type::TRANSFORM_NODE> op_transform;

/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_TRANSFORM_HPP
