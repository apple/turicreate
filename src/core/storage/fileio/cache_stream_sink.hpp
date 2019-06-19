/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef CACHE_STREAM_SINK_HPP
#define CACHE_STREAM_SINK_HPP

#include <iostream>
#include <memory>
#include <boost/iostreams/stream.hpp>
#include <core/storage/fileio/general_fstream_sink.hpp>
#include <core/storage/fileio/fixed_size_cache_manager.hpp>

namespace turi {
namespace fileio_impl {

/**
 * \internal
 *
 * A boost::iostreams::sink concept implemented using cache_block as the underlying
 * sink device.
 */
class cache_stream_sink {

 typedef fileio::cache_id_type cache_id_type;

 public:
  typedef char        char_type;
  struct category: public boost::iostreams::sink_tag,
  boost::iostreams::closable_tag,
  boost::iostreams::multichar_tag {};

  /**
   * Construct the sink from a cache_id.
   *
   * Intialize the underlying data sink, either the in memory array
   * or the on disk cache file.
   */
  explicit cache_stream_sink(cache_id_type cache_id);

  /// Destructor. CLoses the stream.
  ~cache_stream_sink();

  /**
   * Attempts to write bufsize bytes into the stream from the buffer.
   * Returns the actual number of bytes written. Returns -1 on failure.
   */
  std::streamsize write(const char* c, std::streamsize bufsize);

  /**
   * Returns true if the file is opened
   */
  bool is_open() const;

  /**
   * Closes all file handles
   */
  void close();

  /**
   * Seeks to a different location. Will fail on compressed files.
   */
  std::streampos seek(std::streamoff off, std::ios_base::seekdir way);

  /**
   * Returns true if the stream is good. See std::ios_base
   */
  bool good() const;

  /**
   * Returns true if the stream is bad. See std::ios_base
   */
  bool bad() const;

  /**
   * Returns true if a stream operation failed. See std::ios_base
   */
  bool fail() const;


 private:
   fileio::fixed_size_cache_manager& cache_manager;
   std::shared_ptr<fileio::cache_block> out_block;
   std::shared_ptr<general_fstream_sink> out_file;
};


} // end of fileio
} // end of turicreate

#endif
