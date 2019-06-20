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

namespace turi {
namespace query_eval {
using boost::coroutines::stack_traits;
/*
 * Default stack size is 64K or minimum_size() whichever is larger
 */
int64_t COROUTINE_STACK_SIZE =
    64*1024 < stack_traits::minimum_size() ?
        stack_traits::minimum_size() : 64*1024;

REGISTER_GLOBAL_WITH_CHECKS(int64_t, COROUTINE_STACK_SIZE, false,
            +[](int64_t i){
              return (i > 0 &&
                      stack_traits::minimum_size() <= static_cast<size_t>(i) &&
                      static_cast<size_t>(i) <= stack_traits::maximum_size());
            ;});

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
  auto coro_attributes = boost::coroutines::attributes(COROUTINE_STACK_SIZE);

  auto attributes = m_operator->attributes();
  bool supports_skipping =
      attributes.attribute_bitfield & query_operator_attributes::SUPPORTS_SKIPPING;

  bool is_linear_operator =
      attributes.attribute_bitfield & query_operator_attributes::LINEAR;

  /*
   * The mechanism here is somewhat subtle and can be hard to understand.
   * This ought to be cleaned up a bit.
   *
   * The 2nd lambda below, is called whenever the coroutine produces stuff.
   * Essentially, sink() leaves the current coroutine, sending information
   * to its consumers.
   *
   * The consumers however will request from the current coroutine, whether
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
   *  Thus we do not return to the coroutine, but we bypass it,
   *  Consuming the next inputs blocks, throwing it away, and sinking a nullptr
   *  value.
   *
   *  - If the operator does not support skipping AND is a non-linear operator
   *  we need to process it normally.
   */
  m_source = boost::coroutines::coroutine<void>::pull_type(
      [this, supports_skipping, is_linear_operator]
      (boost::coroutines::coroutine<void>::push_type & sink) {

        emit_state initial_operator_state = emit_state::NONE;
        if (supports_skipping && m_skip_next_block) {
          initial_operator_state = emit_state::SKIP_NEXT_BLOCK;
        }

        query_context context([this](size_t input_id, bool skip) {
                                auto ret = get_next_from_input(input_id, skip);
                                return ret;
                              },
                              [this, &sink, supports_skipping, is_linear_operator]
                              (const std::shared_ptr<sframe_rows>& rows)->emit_state{
                                add_operator_output(rows);
LABEL_GOTO_SINK_AGAIN:
                                sink();

                                // we are supposed to skip the next block
                                if (m_skip_next_block) {
                                  if (supports_skipping) {
                                    // operator supports skipping. tell it
                                    // we are skipping
                                    return emit_state::SKIP_NEXT_BLOCK;
                                  } else if (is_linear_operator) {
                                    // make it look like the input is shorter
                                    // just consume the inputs
                                    for (size_t i = 0;i < num_inputs(); ++i) {
                                      get_next_from_input(i, true);
                                    }
                                    // write a fake output, this is the skipped
                                    // block. And sink again.
                                    add_operator_output(nullptr);
                                    goto LABEL_GOTO_SINK_AGAIN;
                                  } else {
                                    // operator does not support skipping.
                                    // read it as usual
                                    return emit_state::NONE;
                                  }
                                }
                                return emit_state::NONE;
                              },
                              sframe_config::SFRAME_READ_BATCH_SIZE,
                              initial_operator_state);
        try {
          m_operator->execute(context);
        } catch(boost::coroutines::detail::forced_unwind& unwind) {
          throw;
        } catch(...) {
          m_exception_occured = true;
          m_exception = std::current_exception();
        }

      },
      coro_attributes);
}

std::shared_ptr<sframe_rows> execution_node::get_next(size_t consumer_id, bool skip) {
  if (cppipc::must_cancel()) {
    throw("Canceled by user");
  }

  m_skip_next_block = skip;

  if (m_coroutines_started == false) start_coroutines();
  DASSERT_LT(consumer_id, m_consumer_pos.size());

  // consume from source when queue is empty and there is more in source
  while (m_output_queue->empty(consumer_id) && m_source) {
    m_source();
  }
  // end of data
  if (m_output_queue->empty(consumer_id) && !m_source) return nullptr;

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
