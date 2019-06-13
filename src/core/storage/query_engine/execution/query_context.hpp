/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_QUERY_ENGINE_OPERATORS_QUERY_CONTEXT_HPP
#define TURI_SFRAME_QUERY_ENGINE_OPERATORS_QUERY_CONTEXT_HPP
#include <memory>
#include <vector>
#include <core/storage/sframe_data/sframe_rows.hpp>
namespace turi {
namespace query_eval {
class executor_node;


/**
 * \ingroup sframe_query_engine
 * \addtogroup execution Execution
 * \{
 */

/**
 * \ref query_context::emit returns a state, which informs the
 * operator about some execution detail.
 */
enum class emit_state {
  NONE, ///< Nothing of interest
  SKIP_NEXT_BLOCK ///< Caller should skip the next block.
};

/**
 * This is the object passed to the coroutine which allows the coroutine
 * to read and write values. The expected usage pattern of the coroutine is:
 *
 * \code
 * void fn(query_context& ctx) {
 *   while(1) {
 *      auto input = ctx.get_next(0); // from 1st input. get_next(1) for 2nd input, etc.
 *      auto output = get_output_buffer();
 *      // fill output buffer. Its just an sframe_rows
 *      ctx.emit(output); // only if output is non-empty.
 *   }
 * }
 * \endcode
 */
class query_context {
 public:
  query_context();
  query_context(const query_context&) = default;
  query_context(query_context&&) = default;
  ~query_context();
  query_context(std::function<std::shared_ptr<sframe_rows>(size_t, bool)> callback_on_get_input,
                std::function<emit_state(const std::shared_ptr<sframe_rows>&)> callback_on_emit,
                size_t m_buffer_size,
                emit_state initial_state);

  /**
   * Requests for the next block for the given input.
   */
  std::shared_ptr<const sframe_rows> get_next(size_t input_number);

  /**
   * Requests for the next block for the given input to the skipped.
   */
  void skip_next(size_t input_number);

  /**
   * Returns a pointer to an output buffer.
   */
  std::shared_ptr<sframe_rows> get_output_buffer();

  /**
   * Returns the initial state.
   */
  emit_state initial_state() const;

  /**
   * Emits a collection of rows. The number of rows emitted
   * MUST be the same as block_size(), except for the very last block
   * of rows.
   */
  emit_state emit(const std::shared_ptr<sframe_rows>& rows);

  /**
   * The commmunication block size.
   */
  inline size_t block_size() const {
    return m_max_buffer_size;
  }
 private:

  /// Maximum buffer size
  size_t m_max_buffer_size = 256; // some arbitrary default

  // we only need 1 buffer and to cycle between both since the linear
  // assumption means that at most one buffer may be used or given away at
  // any one point.
  std::shared_ptr<sframe_rows> m_buffers;

  std::function<std::shared_ptr<sframe_rows>(size_t, bool)> m_callback_on_get_input;
  std::function<emit_state(const std::shared_ptr<sframe_rows>&)> m_callback_on_emit;

  emit_state m_initial_state;
};

/// \}
} // query_eval
} // turicreate
#endif // TURI_SFRAME_QUERY_ENGINE_OPERATORS_QUERY_CONTEXT_HPP
