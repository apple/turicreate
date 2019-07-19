/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/sframe_rows.hpp>
#include <core/storage/query_engine/execution/query_context.hpp>
#include <core/storage/query_engine/execution/execution_node.hpp>

namespace turi {
namespace query_eval {

query_context::query_context() {
  m_buffers = std::make_shared<sframe_rows>();
}
query_context::query_context(execution_node* exec_node,
                            size_t max_buffer_size)
    : m_max_buffer_size(max_buffer_size),
    m_exec_node(exec_node) {
  m_buffers = std::make_shared<sframe_rows>();
}

std::shared_ptr<sframe_rows> query_context::get_output_buffer() {
  return m_buffers;
}

void query_context::emit(const std::shared_ptr<sframe_rows>& rows) {
  m_exec_node->add_operator_output(rows);
}

std::shared_ptr<const sframe_rows> query_context::get_next(size_t input_number) {
  return m_exec_node->get_next_from_input(input_number, false);
}

void query_context::skip_next(size_t input_number) {
  m_exec_node->get_next_from_input(input_number, true);
}

bool query_context::should_skip() {
  return m_exec_node->m_skip_next_block;
}

query_context::~query_context() { }

} // query_eval
} // turicreate
