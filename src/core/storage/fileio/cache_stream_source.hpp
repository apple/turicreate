/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CACHE_STREAM_SOURCE_HPP
#define CACHE_STREAM_SOURCE_HPP

#include <iostream>
#include <boost/iostreams/stream.hpp>
#include <core/storage/fileio/general_fstream_source.hpp>
#include <core/storage/fileio/fixed_size_cache_manager.hpp>

namespace turi {
namespace fileio_impl {

/**
 * \internal
 *
 * A boost::iostreams::input_seekable concept implemented using cache_block as the underlying
 * source device.
 */
class cache_stream_source {

 typedef fileio::cache_id_type cache_id_type;

 public:
  typedef char        char_type;
  struct category: public boost::iostreams::device_tag,
  boost::iostreams::closable_tag,
  boost::iostreams::multichar_tag,
  boost::iostreams::input_seekable {};

  /**
   * Construct the source from a cache_id.
   *
   * Intialize the underlying datasources, either the in memory array
   * or the on disk cache file.
   */
  explicit cache_stream_source(cache_id_type cache_id);

  /**
   * Attempts to read bufsize bytes into the buffer provided.
   * Returns the actual number of bytes read.
   */
  std::streamsize read(char* c, std::streamsize bufsize);

  /**
   * Closes all file handles.
   */
  void close();

  /**
   * Returns true if the stream is opened.
   */
  bool is_open() const;

  /**
   * Seeks to a different location. Will fail on compressed files.
   */
  std::streampos seek(std::streamoff off, std::ios_base::seekdir way);

  /**
   * Returns the file size of the opened file.
   * Returns (size_t)(-1) if there is no file opened, or if there is an
   * error obtaining the file size.
   */
  size_t file_size() const;

  /**
   * Returns the underlying stream object. The underlying stream object
   * is an boost array_source if the cache is in memory, and is a
   * some other stream object otherwise.
   */
  std::shared_ptr<std::istream> get_underlying_stream();

 private:
  char* in_array;
  size_t array_size;
  size_t array_cur_pos;
  std::shared_ptr<fileio::cache_block> in_block;
  std::shared_ptr<general_fstream_source> in_file;
};


} // end of fileio
} // end of turicreate

#endif
