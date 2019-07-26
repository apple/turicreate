/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_MANAGER_GENERALIZED_TRANSFORM_HPP
#define TURI_SFRAME_QUERY_MANAGER_GENERALIZED_TRANSFORM_HPP
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/random/random.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/storage/query_engine/operators/operator.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/operators/operator_properties.hpp>
#include <core/util/coro.hpp>
namespace turi {
namespace query_eval {

typedef std::function<void (const sframe_rows::row&,
                            sframe_rows::row&)> generalized_transform_type;

/**
 * \ingroup sframe_query_engine
 * \addtogroup operators Logical Operators
 * \{
 */

/**
 * The generalized transform operator is like the transform operator
 * but produces a vector output.
 */
template<>
class operator_impl<planner_node_type::GENERALIZED_TRANSFORM_NODE> : public query_operator {
 public:
  DECL_CORO_STATE(execute);

  planner_node_type type() const { return planner_node_type::GENERALIZED_TRANSFORM_NODE; }

  static std::string name() { return "generalized_transform";  }

  static query_operator_attributes attributes() {
    query_operator_attributes ret;
    ret.attribute_bitfield = query_operator_attributes::LINEAR;
    ret.num_inputs = 1;
    return ret;
  }

  ////////////////////////////////////////////////////////////////////////////////

  inline operator_impl(const generalized_transform_type& f,
                       const std::vector<flex_type_enum>& output_types,
                       int random_seed=-1)
      : m_transform_fn(f), m_output_types(output_types), m_random_seed(random_seed)
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
        output->resize(m_output_types.size(), rows->num_rows());

        auto iter = rows->cbegin();
        auto output_iter = output->begin();
        while(iter != rows->cend()) {
          m_transform_fn((*iter), (*output_iter));
          ++output_iter;
          ++iter;
        }
        output->type_check_inplace(m_output_types);
        context.emit(output);
      }
      CORO_YIELD();
    }
    CORO_END
  }

  static std::shared_ptr<planner_node> make_planner_node(
      std::shared_ptr<planner_node> source,
      generalized_transform_type fn,
      const std::vector<flex_type_enum>& output_types,
      int random_seed=-1) {

    flex_list type_list(output_types.size());
    for (size_t i = 0; i < output_types.size(); ++i) {
      type_list[i] = flex_int(output_types[i]);
    }

    return planner_node::make_shared(planner_node_type::GENERALIZED_TRANSFORM_NODE,
                                     {{"output_types", type_list},
                                      {"random_seed", random_seed}},
                                     {{"function", any(fn)}},
                                     {source});
  }

  static std::shared_ptr<query_operator> from_planner_node(
      std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::GENERALIZED_TRANSFORM_NODE);
    ASSERT_EQ(pnode->inputs.size(), 1);
    ASSERT_TRUE(pnode->operator_parameters.count("output_types"));
    ASSERT_TRUE(pnode->any_operator_parameters.count("function"));
    generalized_transform_type fn;

    flex_list list_output_types = pnode->operator_parameters["output_types"];
    std::vector<flex_type_enum> output_types;
    for (auto t: list_output_types) output_types.push_back((flex_type_enum)(flex_int)t);

    fn = pnode->any_operator_parameters["function"].as<generalized_transform_type>();

    int random_seed = (int)(flex_int)(pnode->operator_parameters["random_seed"]);
    return std::make_shared<operator_impl>(fn, output_types, random_seed);
  }

  static std::vector<flex_type_enum> infer_type(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::GENERALIZED_TRANSFORM_NODE);
    ASSERT_TRUE(pnode->operator_parameters.count("output_types"));

    flex_list outtypes = pnode->operator_parameters["output_types"];
    std::vector<flex_type_enum> ret;
    for (auto t: outtypes) ret.push_back((flex_type_enum)(flex_int)t);
    return ret;
  }

  static int64_t infer_length(std::shared_ptr<planner_node> pnode) {
    ASSERT_EQ((int)pnode->operator_type, (int)planner_node_type::GENERALIZED_TRANSFORM_NODE);
    return infer_planner_node_length(pnode->inputs[0]);
  }

  static std::string repr(std::shared_ptr<planner_node> pnode, pnode_tagger&) {
    size_t n_outs = infer_length(pnode);

    switch(n_outs) {
      case 1:  return "Tr->[C0]";
      case 2:  return "Tr->[C0,C1]";
      case 3:  return "Tr->[C0,C1,C2]";
      default: {
        std::ostringstream out;
        out << "Tr->[C0,...,C" << (n_outs - 1) << "]";
        return out.str();
      }
    }
  }

 private:
  generalized_transform_type m_transform_fn;
  std::vector<flex_type_enum> m_output_types;
  int m_random_seed;
};

typedef operator_impl<planner_node_type::GENERALIZED_TRANSFORM_NODE> op_generalized_transform;

/// \}
} // query_eval
} // turicreate

#endif // TURI_SFRAME_QUERY_MANAGER_TRANSFORM_HPP
