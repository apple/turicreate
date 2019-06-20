/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
extern "C" {
#include <lz4/lz4.h>
}
#include <core/storage/sframe_data/sarray_v2_block_writer.hpp>
#include <core/storage/sframe_data/sarray_index_file.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>
#include <core/storage/sframe_data/sarray_v2_type_encoding.hpp>

namespace turi {
namespace v2_block_impl {

void block_writer::init(std::string group_index_file,
                        size_t num_segments,
                        size_t num_columns) {
  m_output_files.resize(num_segments);
  m_output_file_locks.resize(num_segments);
  m_output_bytes_written.resize(num_segments);

  // 1x for the compression buffer,
  // 1x for the flexible_type serialization buffer
  m_buffer_pool.init(2 * num_segments);

  m_blocks.resize(num_segments);
  for (auto& m_blockseg: m_blocks) m_blockseg.resize(num_columns);
  m_index_info.group_index_file = group_index_file;
  m_index_info.version = 2;
  m_index_info.nsegments = num_segments;
  m_index_info.segment_files.resize(num_segments);
  m_index_info.columns.resize(num_columns);

  // fill in the per column information of m_index_info.
  for (size_t col = 0;col < m_index_info.columns.size(); ++col) {
    m_index_info.columns[col].index_file =
        m_index_info.group_index_file + ":" + std::to_string(col);
    m_index_info.columns[col].version = 2;
    m_index_info.columns[col].nsegments = m_index_info.nsegments;
    m_index_info.columns[col].segment_files = m_index_info.segment_files;

    for (std::string& segf: m_index_info.columns[col].segment_files) {
      segf = segf + ":" + std::to_string(col);
    }
    m_index_info.columns[col].segment_sizes.resize(m_index_info.nsegments, 0);
  }
}

void block_writer::open_segment(size_t segmentid, std::string filename) {
  ASSERT_LT(segmentid, m_index_info.nsegments);
  ASSERT_TRUE(m_output_files[segmentid] == NULL);
  m_output_files[segmentid].reset(new general_ofstream(filename,
                                                    /* must not compress!
                                                     * We need the blocks!*/
                                                     false));
  m_index_info.segment_files[segmentid] = filename;
  // update the per column segment file
  for (size_t col = 0;col < m_index_info.columns.size(); ++col) {
    m_index_info.columns[col].segment_files[segmentid] =
        m_index_info.segment_files[segmentid] + ":" + std::to_string(col);
  }

  if (m_output_files[segmentid]->fail()) {
    log_and_throw("Unable to open segment data file " + filename);
  }

}

void block_writer::set_options(const std::string& option, int64_t value) {
  if (option == "disable_padding") {
    m_disable_padding = value;
  }
}

static char padding_bytes[4096] = {0};

size_t block_writer::write_block(size_t segment_id,
                                 size_t column_id,
                                 char* data,
                                 block_info block) {
  DASSERT_LT(segment_id, m_index_info.nsegments);
  DASSERT_LT(column_id, m_index_info.columns.size());
  DASSERT_TRUE(m_output_files[segment_id] != NULL);
  // try to compress the data
  size_t compress_bound = LZ4_compressBound(block.block_size);
  auto compression_buffer = m_buffer_pool.get_new_buffer();
  compression_buffer->resize(compress_bound);
  char* cbuffer = compression_buffer->data();
  size_t clen = compress_bound;
  clen = LZ4_compress(data, cbuffer, block.block_size);

  char* buffer_to_write = NULL;
  size_t buffer_to_write_len = 0;
  if (clen < COMPRESSION_DISABLE_THRESHOLD * block.block_size) {
    // compression has a benefit!
    block.flags |= LZ4_COMPRESSION;
    block.length = clen;
    buffer_to_write = cbuffer;
    buffer_to_write_len = clen;
  } else {
    // compression has no benefit! do not compress!
    // unset LZ4
    block.flags &= (~(size_t)LZ4_COMPRESSION);
    block.length = block.block_size;
    buffer_to_write = data;
    buffer_to_write_len = block.block_size;
  }

  size_t padding = ((buffer_to_write_len + 4095) / 4096) * 4096 - buffer_to_write_len;
  if (m_disable_padding) padding = 0;
  ASSERT_LT(padding, 4096);
  // write!
  m_output_file_locks[segment_id].lock();
  block.offset = m_output_bytes_written[segment_id];
  m_output_bytes_written[segment_id] += buffer_to_write_len + padding;
  m_index_info.columns[column_id].segment_sizes[segment_id] += block.num_elem;
  m_output_files[segment_id]->write(buffer_to_write, buffer_to_write_len);
  m_output_files[segment_id]->write(padding_bytes, padding);
  m_blocks[segment_id][column_id].push_back(block);
  m_output_file_locks[segment_id].unlock();

  m_buffer_pool.release_buffer(std::move(compression_buffer));

  if (!m_output_files[segment_id]->good()) {
    log_and_throw_io_failure("Fail to write. Disk may be full.");
  }
  return buffer_to_write_len;
}

size_t block_writer::write_typed_block(size_t segment_id,
                                       size_t column_id,
                                       const std::vector<flexible_type>& data,
                                       block_info block) {
  auto serialization_buffer = m_buffer_pool.get_new_buffer();
  oarchive oarc(*serialization_buffer);
  typed_encode(data, block, oarc);
  size_t ret = write_block(segment_id, column_id, serialization_buffer->data(), block);
  m_buffer_pool.release_buffer(std::move(serialization_buffer));
  return ret;
}


void block_writer::close_segment(size_t segment_id) {
  emit_footer(segment_id);
  m_output_files[segment_id].reset();
}

group_index_file_information& block_writer::get_index_info() {
  return m_index_info;
}

void block_writer::write_index_file() {
  write_array_group_index_file(m_index_info.group_index_file,
                               m_index_info);
}

void block_writer::emit_footer(size_t segment_id) {
  // prepare the footer
  // write out all the block headers
  oarchive oarc;
  oarc << m_blocks[segment_id];
  m_output_files[segment_id]->write(oarc.buf, oarc.off);
  uint64_t footer_size = oarc.off;

  m_output_files[segment_id]->write(reinterpret_cast<char*>(&footer_size),
                                 sizeof(footer_size));
  free(oarc.buf);

  if (!m_output_files[segment_id]->good()) {
    log_and_throw_io_failure("Fail to write. Disk may be full.");
  }
}


} // namespace v2_block_impl
} // namespace turi
