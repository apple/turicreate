/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <boost/algorithm/string.hpp>
#include <logger/assertions.hpp>
#include <fileio/general_fstream_source.hpp>

namespace turi {
namespace fileio_impl {


general_fstream_source::general_fstream_source(std::string file) {
  open_file(file, boost::ends_with(file, ".gz"));
}

general_fstream_source::general_fstream_source(std::string file, 
                                               bool gzip_compressed) {
  open_file(file, gzip_compressed);
}

void general_fstream_source::open_file(std::string file, bool gzip_compressed) {

  in_file = std::make_shared<union_fstream>(file, std::ios_base::in | std::ios_base::binary);
  is_gzip_compressed = gzip_compressed;
  if (gzip_compressed) {
    decompressor = std::make_shared<boost::iostreams::gzip_decompressor>();
  }
  underlying_stream = in_file->get_istream();
}

bool general_fstream_source::is_open() const {
  return underlying_stream && !underlying_stream->bad();
}

std::streamsize general_fstream_source::read(char* c, std::streamsize bufsize) {
  if (is_gzip_compressed) {
    return decompressor->read(*underlying_stream, c, bufsize);
  } else {
    underlying_stream->read(c, bufsize);
    return underlying_stream->gcount();
  }
}

general_fstream_source::~general_fstream_source() {
  try {
    close();
  } catch(...) {
    // absorb any exceptions. This is a file that was for reading,
    // exceptions are fine here since I don't need it any more.
  }
}

void general_fstream_source::close() {
  if (decompressor) {
    decompressor->close(*underlying_stream, std::ios_base::in);
    decompressor.reset();
  }
  underlying_stream.reset();
  in_file.reset();
}

std::streampos general_fstream_source::seek(std::streamoff off, 
                                            std::ios_base::seekdir way) {
  if (!decompressor) {
    underlying_stream->clear();
    underlying_stream->seekg(off, way);
    return underlying_stream->tellg();
  } else {
    ASSERT_MSG(false, "Attempting to seek in a compressed file. Fail!");
  }
}

size_t general_fstream_source::file_size() const {
  if (in_file) {
    return in_file->file_size();
  } else {
    return (size_t)(-1);
  }
}


size_t general_fstream_source::get_bytes_read() const {
  if (underlying_stream) {
    return underlying_stream->tellg();
  } else {
    return (size_t)(-1);
  }
}

std::shared_ptr<std::istream> general_fstream_source::get_underlying_stream() const {
  if (decompressor) {
    return nullptr;
  } else {
    return underlying_stream;
  }
}

} // namespace fileio_impl
} // namespace turi
