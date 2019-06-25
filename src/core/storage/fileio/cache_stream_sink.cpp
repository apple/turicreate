/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/fileio/cache_stream_sink.hpp>

namespace turi {
namespace fileio_impl {

cache_stream_sink::cache_stream_sink(cache_id_type cache_id) :
  cache_manager(fileio::fixed_size_cache_manager::get_instance()),
  out_block(cache_manager.new_cache(cache_id)) {
  if (out_block->is_file()) {
    logstream(LOG_DEBUG) << "Writing " << cache_id << " from "
                        << out_block->get_filename() << std::endl;
    out_file = std::make_shared<general_fstream_sink>(out_block->get_filename());
  }
}

cache_stream_sink::~cache_stream_sink() {
  close();
}

std::streamsize cache_stream_sink::write (const char* c, std::streamsize bufsize) {
  if (out_file) {
    return out_file->write(c, bufsize);
  } else {
    bool write_success = out_block->write_bytes_to_memory_cache(c, bufsize);
    if (write_success) {
      return bufsize;
    } else {
      // In memory cache is full, write out to disk.
      // switch to a file handle
      out_file = out_block->write_to_file();
      return out_file->write(c, bufsize);
    }
  }
}

void cache_stream_sink::close() {
  if (out_file) {
    out_file->close();
  }
}

bool cache_stream_sink::is_open() const {
  if (out_file) {
    return out_file->is_open();
  } else {
    return out_block->get_pointer() != NULL;
  }
}

bool cache_stream_sink::good() const {
  if (out_file) {
    return out_file->good();
  } else {
    return out_block->get_pointer() != NULL;
  }
}

bool cache_stream_sink::bad() const {
  if (out_file) {
    return out_file->bad();
  } else {
    return out_block->get_pointer() == NULL;
  }
}

bool cache_stream_sink::fail() const {
  if (out_file) {
    return out_file->fail();
  } else {
    return out_block->get_pointer() == NULL;
  }
}


} // fileio_impl
} // turicreate
