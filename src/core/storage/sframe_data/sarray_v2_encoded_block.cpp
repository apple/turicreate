/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/sarray_v2_encoded_block.hpp>
#include <core/storage/sframe_data/sarray_v2_type_encoding.hpp>
#include <cmath>
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

encoded_block_range::encoded_block_range(const encoded_block& block)
 : m_block(block.m_block) {
   decoder.reset(new typed_decode_stream(m_block.m_block_info,
                                         m_block.m_data->data(),
                                         m_block.m_data->size()));
 }

void encoded_block_range::release() {
  decoder.reset();
  m_block.m_data.reset();
}

encoded_block_range::~encoded_block_range() {}

size_t encoded_block_range::decode_to(flexible_type* write_target,
                                        size_t numel) {
  if (numel == 0) return 0;
  return decoder->read({write_target, numel}, 0);
}

void encoded_block_range::skip(size_t n) {
  if (n == 0) return;
  decoder->read({nullptr, 0}, n);
}

} // v2_block_impl
} // turicreate
