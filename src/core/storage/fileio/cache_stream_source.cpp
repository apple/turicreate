/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/logging/logger.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <core/storage/fileio/cache_stream_source.hpp>
#include <core/storage/fileio/fixed_size_cache_manager.hpp>

namespace turi {
namespace fileio_impl {


cache_stream_source::cache_stream_source(cache_id_type cache_id) {
  auto& cache_manager = fileio::fixed_size_cache_manager::get_instance();
  in_block = cache_manager.get_cache(cache_id);
  if (in_block->is_pointer()) {
    in_array = in_block->get_pointer();
    array_size = in_block->get_pointer_size();
    array_cur_pos = 0;
  } else {
    in_array = NULL;
    array_size = 0;
    array_cur_pos = 0;
    logstream(LOG_INFO) << "Reading " << cache_id << " from "
                        << in_block->get_filename() << std::endl;
    in_file = std::make_shared<general_fstream_source>(in_block->get_filename());
  }
}

std::streamsize cache_stream_source::read(char* c, std::streamsize bufsize) {
  if (in_array) {
    size_t bytes_read = std::min<size_t>(bufsize, array_size - array_cur_pos);
    memcpy(c, in_array + array_cur_pos, bytes_read);
    array_cur_pos += bytes_read;
    return bytes_read;
  } else {
    return in_file->read(c, bufsize);
  }
}

void cache_stream_source::close() {
  if (in_file) {
    in_file->close();
  }
}

std::streampos cache_stream_source::seek(std::streamoff off, std::ios_base::seekdir way) {
  if (in_array) {
    std::streampos newpos;
    if (way == std::ios_base::beg) {
      newpos = off;
    } else if (way == std::ios_base::cur) {
      newpos = (std::streampos)(array_cur_pos) + off;
    } else if (way == std::ios_base::end) {
      newpos = array_size + off - 1;
    }

    if (newpos < 0 || newpos >= (std::streampos)array_size) {
      log_and_throw_io_failure("Bad seek. Index out of range.");
    }

    array_cur_pos = newpos;
    return newpos;
  }

  return in_file->seek(off, way);
}

bool cache_stream_source::is_open() const {
  if (in_file) {
    return in_file->is_open();
  }
  return true;
}

size_t cache_stream_source::file_size() const {
  if (in_file) {
    return in_file->file_size();
  }
  return array_size;
}

std::shared_ptr<std::istream> cache_stream_source::get_underlying_stream() {
  if (in_array) {
    return
        std::make_shared<boost::iostreams::stream<boost::iostreams::array_source>>(
            in_block->get_pointer(), in_block->get_pointer_size());
  } else {
    auto ret = in_file->get_underlying_stream();
    if (ret == nullptr) {
      return std::make_shared<general_ifstream>(in_block->get_filename());
    }
    else return ret;
  }
}

} // fileio_impl
} // turicreate
