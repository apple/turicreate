/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/sarray_v2_encoded_block.hpp>
#include <core/storage/sframe_data/sarray_v2_type_encoding.hpp>
namespace turi {
namespace v2_block_impl {

encoded_block::encoded_block() {
}

void encoded_block::init(block_info info, std::vector<char>&& data) {
  m_block = block{info,
    std::make_shared<std::vector<char>>(std::move(data))};
  m_size = info.num_elem;
}


void encoded_block::init(block_info info, std::shared_ptr<std::vector<char> > data) {
  m_block = block{info, data};
  m_size = info.num_elem;
}

encoded_block_range encoded_block::get_range() {
  return encoded_block_range(*this);
}

void encoded_block::release() {
  m_block.m_data.reset();
  m_block.m_block_info = block_info();
}

encoded_block_range::encoded_block_range(const encoded_block& block) {
  // initializes the range. Sets up the internal variables
  m_shared = std::make_shared<coro_shared_data>();
  m_block = block.m_block;
}

void encoded_block_range::coroutine_launch() {
  // launches the decoding coroutine
  auto coro_m_shared = m_shared;
  auto coro_m_block = m_block;
  coroutine_started = true;
  source =
      coroutine_type(
          // the coroutine function
          [coro_m_shared, coro_m_block]
          (boost::coroutines::coroutine<void>::push_type& sink){
            // coroutine function basically calls the decoder with a callback
            // which sticks stuff into the buffer.
            // and triggers the sink when the buffer full.
            typed_decode_stream_callback(coro_m_block.m_block_info,
                                         coro_m_block.m_data->data(),
                                         coro_m_block.m_data->size(),
                                         [&coro_m_shared, &sink](const flexible_type& val) {
                                           auto& shared = *coro_m_shared;
                                           if (shared.terminate) {
                                             return;
                                           } else if (shared.m_write_target_numel) {
                                             // there is an alternative write
                                             // target rather than the circular
                                             // buffer.
                                             (*shared.m_write_target) = val;
                                             shared.m_write_target++;
                                             shared.m_write_target_numel--;
                                             if (shared.m_write_target_numel == 0) sink();
                                           } else if(shared.m_skip) {
                                             shared.m_skip--;
                                             if (shared.m_skip == 0) sink();
                                           }
                                         });
            return;
      });
}

void encoded_block_range::call_source() {
  if (!coroutine_started) coroutine_launch();
  else source();
}

void encoded_block_range::skip(size_t n) {
  if (n == 0) return;
  if (coroutine_started && !source) return;
  m_shared->m_skip = n;
  call_source();
}

size_t encoded_block_range::fill_buffer(flexible_type* write_target,
                                        size_t numel) {
  if (numel == 0) return 0;
  if (coroutine_started && !source) return 0;
  m_shared->m_write_target = write_target;
  m_shared->m_write_target_numel = numel;
  call_source();
  // reset the pointers
  m_shared->m_write_target = nullptr;
  size_t ctr = numel - m_shared->m_write_target_numel;
  m_shared->m_write_target_numel = 0;
  return ctr;
}

void encoded_block_range::release() {
  if (source) {
    // try to unwind the coroutine cleanly.
    m_shared->terminate = true;
    source();
  }
  source = coroutine_type();
  m_shared.reset();
  m_block.m_data.reset();
}


size_t encoded_block_range::decode_to(flexible_type* decode_target, size_t num_elem) {
  return fill_buffer(decode_target, num_elem);
}

encoded_block_range::~encoded_block_range() {
  if (source) release();
}

} // v2_block_impl
} // turicreate
