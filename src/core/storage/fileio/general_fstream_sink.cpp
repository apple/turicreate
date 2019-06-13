/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <boost/algorithm/string.hpp>
#include <core/storage/fileio/general_fstream_sink.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <core/logging/logger.hpp>
namespace turi {
namespace fileio_impl {


general_fstream_sink::general_fstream_sink(std::string file) {
  open_file(file, boost::ends_with(file, ".gz"));
}

general_fstream_sink::general_fstream_sink(std::string file,
                                               bool gzip_compressed) {
  open_file(file, gzip_compressed);
}

void general_fstream_sink::open_file(std::string file, bool gzip_compressed) {
  sanitized_filename = sanitize_url(file);
  out_file = std::make_shared<union_fstream>(file, std::ios_base::out | std::ios_base::binary);
  is_gzip_compressed = gzip_compressed;
  if (gzip_compressed) {
    compressor = std::make_shared<boost::iostreams::gzip_compressor>();
  }
  // get the underlying stream inside the union stream
  underlying_stream = out_file->get_ostream();
}

bool general_fstream_sink::is_open() const {
  return underlying_stream && !underlying_stream->bad();
}

std::streamsize general_fstream_sink::write(const char* c,
                                            std::streamsize bufsize) {
#ifdef _WIN32
// windows has interesting issues if bufsize >= 2GB
// we cut up the buffer and read it in 1GB increments
  const std::streamsize WIN_WRITE_LIMIT = 1LL*1024*1024*1024; // 1GB
  // cut into smaller buffers
  std::streamsize remaining_size = bufsize;
  while(remaining_size > 0) {
    std::streamsize limit = std::min(remaining_size, WIN_WRITE_LIMIT);
    if (is_gzip_compressed) {
      compressor->write(*underlying_stream, c, limit);
    } else {
      underlying_stream->write(c, limit);
      if (underlying_stream->fail()) return 0;
    }
    remaining_size -= limit;
    c += limit;
  }
  return bufsize;
#else
  if (is_gzip_compressed) {
    return compressor->write(*underlying_stream, c, bufsize);
  } else {
    underlying_stream->write(c, bufsize);
    if (underlying_stream->fail()) return 0;
    else return bufsize;
  }
#endif
}

general_fstream_sink::~general_fstream_sink() {
  // if I am the only reference to the object, close it.
  if (out_file && out_file.unique()) {
    try {
      close();
    } catch (...) {
      logstream(LOG_ERROR) << "Exception occured on closing "
                           << sanitized_filename
                           << ". The file may not be properly written" << std::endl;
    }
  }
}

void general_fstream_sink::close() {
  if (compressor) {
    compressor->close(*underlying_stream, std::ios_base::out);
    compressor.reset();
  }
  underlying_stream.reset();
  out_file.reset();
}


bool general_fstream_sink::good() const {
  return underlying_stream && underlying_stream->good();
}

bool general_fstream_sink::bad() const {
  // if stream is NULL. the stream is bad
  if (underlying_stream == nullptr) return true;
  return underlying_stream->bad();
}

bool general_fstream_sink::fail() const {
  // if stream is NULL. the stream is bad
  if (underlying_stream == nullptr) return true;
  return underlying_stream->fail();
}

size_t general_fstream_sink::get_bytes_written() const {
  if (underlying_stream) {
    return underlying_stream->tellp();
  } else {
    return (size_t)(-1);
  }
}

} // namespace fileio_impl
} // namespace turi
