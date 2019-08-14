/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FILEIO_UNION_FSTREAM_HPP
#define FILEIO_UNION_FSTREAM_HPP
#include <iostream>
#include <fstream>
#include <memory>

namespace turi {

/**
 * \ingroup fileio
 * A simple union of std::fstream and turi::hdfs::fstream.
 * Also performs S3 downloading, uploading, and Curl downloading automatically.
 */
class union_fstream {

 public:
  enum stream_type {HDFS, STD, CACHE};
  /**
   * Constructs a union fstream from a filename. Based on the filename
   * (whether it begins with hdfs://, or cache://)
   * an appropriate stream type (HDFS, STD, or CACHE) is created.
   *
   * Throws an std::io_base_failure exception if fail to construct the stream.
   */
  union_fstream(std::string url,
                std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out,
                std::string proxy="");

  /// Destructor
  ~union_fstream();

  /// Returns the current stream type. Whether it is a HDFS, STD, or cache stream
  stream_type get_type() const;

  std::shared_ptr<std::istream> get_istream();

  std::shared_ptr<std::ostream> get_ostream();

  /**
   * Returns the filename used to construct the union_fstream
   */
  std::string get_name() const;

  /**
   * Returns the file size of the opened file.
   * Returns (size_t)(-1) if there is no file opened, or if there is an
   * error obtaining the file size.
   */
  size_t file_size();

 private:
  union_fstream(const union_fstream& other) = delete;

 private:

  stream_type type;
  std::string url;
  size_t m_file_size = (size_t)(-1);

  std::shared_ptr<std::istream> input_stream;
  std::shared_ptr<std::ostream> output_stream;

  // Hold the input stream from cache or s3 stream.
  std::shared_ptr<std::istream> original_input_stream_handle;
};

} // namespace turi
#endif
