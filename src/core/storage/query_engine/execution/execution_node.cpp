/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/sframe_rows.hpp>
#include <core/storage/sframe_data/sframe_config.hpp>
#include <core/globals/globals.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/execution/execution_node.hpp>
#include <core/system/cppipc/cppipc.hpp>
#include <core/util/coro.hpp>

namespace turi {
namespace query_eval {

execution_node::execution_node(const std::shared_ptr<query_operator>& op,
                               const std::vector<std::shared_ptr<execution_node> >& inputs) {
  init(op, inputs);
}
void execution_node::init(const std::shared_ptr<query_operator>& op,
                          const std::vector<std::shared_ptr<execution_node> >& inputs) {
  m_operator = op;
  int num_inputs = m_operator->attributes().num_inputs;
  // num_inputs may be negative if it does not care about the number of inputs.
  if (num_inputs >= 0) {
    ASSERT_EQ(inputs.size(), (size_t)num_inputs);
  }

  // register each source
  for (auto& i : inputs) {
    input_node node;
    node.m_node = i;
    node.m_consumer_id = i->register_consumer();
    m_inputs.push_back(node);
  }
  reset();
}

void execution_node::reset() {
  if (m_coroutines_started) {
    m_consumer_pos.assign(m_consumer_pos.size(), 0);
    m_coroutines_started = false;
    for (size_t i = 0;i < m_inputs.size(); ++i) {
      m_inputs[i].m_node->reset();
    }
    m_exception_occured = false;
    m_exception = std::exception_ptr();
  }
  m_output_queue.reset();
}


void execution_node::start_coroutines() {
  // create the output queue
  m_output_queue.reset(new broadcast_queue<std::shared_ptr<sframe_rows>>(m_consumer_pos.size(), 2));

  // restart the coroutine
  m_coroutines_started = true;

  auto attributes = m_operator->attributes();
  supports_skipping =
      attributes.attribute_bitfield & query_operator_attributes::SUPPORTS_SKIPPING;

  is_linear_operator =
      attributes.attribute_bitfield & query_operator_attributes::LINEAR;

  /*
   * The consumers will request from the current coroutine, whether
   * to skip the block or not. This is stored in the state
   * m_skip_next_block.
   *
   * Requesting for a skipped block is merely an optimization. Even when a
   * block is skipped, the producer coroutines are triggered; its just that
   * skipping is possible, and ocassionally nullptrs can be passed instead of
   * full rows.
   *
   * The handling logic gets slightly interesting however, since we have
   * information going backwards up the execution graph.
   *
   *  - If the operator supports skipping: i.e. it recognizes the skipping
   *  state, we send it the skipping state by returning
   *  emit_state::SKIP_NEXT_BLOCK.
   *
   *  - If the operator does not support skipping, but is a linear operator,
   *  we can pull a trick by making it seem like the input is shorter.
   *  Thus we do not return to the coroutine, but we bypass it, requesting a skip
   *  from its previous blocks.
   *
   *  - If the operator does not support skipping AND is a non-linear operator
   *  we need to proess it normally.
   */
  m_context = std::make_shared<query_context>(this, sframe_config::SFRAME_READ_BATCH_SIZE);
}

std::shared_ptr<sframe_rows> execution_node::get_next(size_t consumer_id, bool skip) {
  if (cppipc::must_cancel()) {
    throw("Canceled by user");
  }

  m_skip_next_block = skip;

  DASSERT_LT(consumer_id, m_consumer_pos.size());
  while (m_coroutines_started == false ||
         (m_output_queue->empty(consumer_id) && m_operator->coro_running()))  {
    if (m_coroutines_started == false) {
      start_coroutines();
    }
    try {
      if (m_skip_next_block) {
        if (supports_skipping || !is_linear_operator) {
          m_operator->execute(*m_context);
        } else {
          // make it look like the input is shorter
          // just consume the inputs
          for (size_t i = 0;i < num_inputs(); ++i) {
            get_next_from_input(i, true);
          }
          add_operator_output(nullptr);
        }
      } else {
        m_operator->execute(*m_context);
      }
    } catch(...) {
      m_exception_occured = true;
      m_exception = std::current_exception();
    }
  }

  // end of data
  if (m_output_queue->empty(consumer_id) && m_operator->coro_running() == false) return nullptr;

  ASSERT_TRUE(!m_output_queue->empty(consumer_id));

  std::shared_ptr<sframe_rows> ret;
  m_output_queue->pop(consumer_id, ret);
  ++m_consumer_pos[consumer_id];

  if (skip) return nullptr;
  else return ret;
}

void execution_node::add_operator_output(const std::shared_ptr<sframe_rows>& rows) {
  m_output_queue->push(rows);
}

std::shared_ptr<sframe_rows> execution_node::get_next_from_input(size_t input_id, bool skip) {
  ASSERT_LT(input_id, m_inputs.size());
  auto& input = m_inputs[input_id];
  return input.m_node->get_next(input.m_consumer_id, skip);
}

size_t execution_node::register_consumer() {
  m_consumer_pos.push_back(0);
  return m_consumer_pos.size() - 1;
}


} // query_eval
} // turicreate
