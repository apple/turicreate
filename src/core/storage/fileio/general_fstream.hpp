/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FILEIO_GENERAL_ISTREAM_HPP
#define FILEIO_GENERAL_ISTREAM_HPP
#include <iostream>
#include <string>
#include <fstream>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <core/storage/fileio/general_fstream_source.hpp>
#include <core/storage/fileio/general_fstream_sink.hpp>
#include <core/export.hpp>

namespace turi {
typedef boost::iostreams::stream<fileio_impl::general_fstream_source>
    general_ifstream_base;
/**
 * \ingroup fileio
 * A generic input file stream interface that provides unified access to
 * local filesystem, HDFS, S3, in memory files, and can automatically
 * perform gzip decoding.
 *
 * Usage:
 * \code
 * general_ifstream fin("file");
 * // after which fin behaves like a regular std::ifstream object.
 * \endcode
 *
 * file can be:
 *  - local filesystem
 *  - S3 (in which case the filename must be of the form s3://... (see below)
 *  - HDFS (filename must be of the form hdfs://...)
 *  - In memory / disk paged (filename must be of the form cache://...)
 *
 * In all filesystems, random seek is allowed.
 *
 * If the file is gzip compressed, it will automatically be decoded on the fly,
 * but random seeks will be disabled.
 *
 * S3 access keys are mediated by having the filename be of the form
 * s3://[access_key_id]:[secret_key]:[endpoint][/bucket]/[object_name]
 *
 * Endpoint URLs however, are set globally via the global variable S3_ENDPOINT.
 */
class EXPORT general_ifstream : public general_ifstream_base {
 private:
   std::string opened_filename;
 public:
  /**
   * Constructs a general ifstream object when opens the filename specified.
   * The file may be on HDFS and may be gzip compressed. If the file
   * is gzip compressed, the file must be have the ".gz" suffix for it to be
   * properly identified.
   *
   * Throw an std::io_base::failure exception if failing to contruct the stream.
   */
  general_ifstream(std::string filename);

  /**
   * Constructs a general ifstream object when opens the filename specified.
   * The file may be on HDFS and may be gzip compressed.
   * This overloaded constructor allows you to explicitly specify if the file
   * was gzip compressed regardless of the filename.
   *
   * Throw an std::io_base::failure exception if failing to contruct the stream.
   */
  general_ifstream(std::string filename, bool gzip_compressed);

  /**
   * Returns the file size of the opened file.
   * Returns (size_t)(-1) if there is no file opened, or if there is an
   * error obtaining the file size.
   */
  size_t file_size();

  /**
   * Returns the number of bytes read from disk so far. Due to file
   * compression and buffering this can be very different from how many bytes
   * were read from the stream.
   */
  size_t get_bytes_read();

  /**
   * Returns the local file name used by the stream.
   */
  std::string filename() const;

  /**
   * Returns the underlying stream object
   */
  std::shared_ptr<std::istream> get_underlying_stream();
};



typedef boost::iostreams::stream<fileio_impl::general_fstream_sink>
    general_ofstream_base;

/**
 * \ingroup fileio
 * A generic output file stream interface that provides unified access to
 * local filesystem, HDFS, S3, in memory files, and can automatically
 * perform gzip decoding.
 *
 * Usage:
 * \code
 * general_ofstream fin("file");
 * // after which fout almost behaves like a regular std::ofstream object.
 * \endcode
 *
 * file can be:
 *  - local filesystem
 *  - S3 (in which case the filename must be of the form s3://... (see below)
 *  - HDFS (filename must be of the form hdfs://...)
 *  - In memory / disk paged (filename must be of the form cache://...)
 *
 * Unlike standard std::ofstream, random seek is \b not allowed. In other words,
 * only sequential write is permitted.
 *
 * If the filename ends with ".gz", gzip compression is automatically performed.
 *
 * S3 access keys are mediated by having the filename be of the form
 * s3://[access_key_id]:[secret_key]:[endpoint][/bucket]/[object_name]
 *
 * Endpoint URLs however, are set globally via the global variable S3_ENDPOINT.
 */
class EXPORT general_ofstream: public general_ofstream_base {
 private:
   std::string opened_filename;
 public:

  /**
   * Constructs a general ofstream object when opens the filename specified.
   * The file may be on HDFS. If the filename has the ".gz" suffix, it will be
   * gzip compressed.
   *
   * Throw an std::io_base::failure exception if failing to contruct the stream.
   */
  general_ofstream(std::string filename);


  /**
   * Constructs a general ofstream object when opens the filename specified.
   * The file may be on HDFS.
   * This overloaded constructor allows you to explicitly specify whether
   * the file is to to be gzip compressed, regardless of the filename.
   *
   * Throw an std::io_base::failure exception if failing to contruct the stream.
   */
  general_ofstream(std::string filename, bool gzip_compress);

  /**
   * Returns true if the stream is good. See std::ios_base
   */
  bool good() const;

  /**
   * Returns true if the stream is bad. See std::ios_base
   */
  bool bad() const;

  /**
   * Returns true if the last stream operation has failed. See std::ios_base
   */
  bool fail() const;

  /**
   * Returns the number of bytes written to disk so far. Due to file
   * compression and buffering this can be very different from how many bytes
   * were wrtten to the stream.
   */
  size_t get_bytes_written() const;

  /**
   * Returns the local file name used by the stream.
   */
  std::string filename() const;
};

} // namespace turi

#endif // TURI_UTIL_GENERAL_ISTREAM_HPP
