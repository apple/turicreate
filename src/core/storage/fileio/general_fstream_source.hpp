/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FILEIO_GENERAL_FSTREAM_SOURCE_HPP
#define FILEIO_GENERAL_FSTREAM_SOURCE_HPP
#include <memory>
#include <fstream>
#include <boost/iostreams/stream.hpp>
#include <core/storage/fileio/union_fstream.hpp>
#include <core/storage/fileio/fileio_constants.hpp>
#include <boost/iostreams/filter/gzip.hpp>
namespace turi {
namespace fileio_impl {

/**
 * \ingroup fileio
 * \internal
 * Implements a general file stream source device which wraps the
 * union_fstream, and provides automatic gzip decompression capabilities.
 *
 * The general_fstream_souce is NOT thread-safe.
 */
class general_fstream_source {

  /// The source device must be copyable; thus the shared_ptr.
  std::shared_ptr<union_fstream> in_file;
  /// The source device must be copyable; thus the shared_ptr.
  std::shared_ptr<boost::iostreams::gzip_decompressor> decompressor;

  /// The underlying stream inside the in_file (std stream or hdfs stream)
  std::shared_ptr<std::istream> underlying_stream;

  /// Set by the constructor. whether it is gzip compressed.
  bool is_gzip_compressed = false;
 public:
  typedef char        char_type;
  struct category: public boost::iostreams::device_tag,
    boost::iostreams::closable_tag,
    boost::iostreams::multichar_tag,
    boost::iostreams::input_seekable,
    boost::iostreams::optimally_buffered_tag {};

  /**
   * Constructs a fstream source which opens a file. This file can be
   * of any protocol supported by the union_fstream, and may also be
   * gzip compressed. Gzip compression detection is automatic based on the file
   * extension. (Whether it ends in .gz)
   */
  explicit general_fstream_source(std::string file);

  /**
   * Constructs a fstream source which opens a file. This file can be
   * of any protocol supported by the union_fstream, and may also be
   * gzip compressed. Gzip compression detection is not performed, and the
   * gzip_compressed flag is used to enable/disable gzip decompression.
   */
  general_fstream_source(std::string file, bool gzip_compressed);

  /**
   * Default copy constructor. copied object shares handles with the original
   * object. Required because boost streams requires devices to be copyable.
   * This should really not be used otherwise.
   */
  general_fstream_source(const general_fstream_source&) = default;

  /**
   * Default move constructor
   */
  general_fstream_source(general_fstream_source&&) = default;


  /**
   * destructor. If all copies of this object is closed,
   * closes the file.
   */
  ~general_fstream_source();

  inline std::streamsize optimal_buffer_size() const {
    return turi::fileio::FILEIO_READER_BUFFER_SIZE;
  }

  /**
   * Returns true if the file is opened
   */
  bool is_open() const;

  /**
   * Attempts to read bufsize bytes into the buffer provided.
   * Returns the actual number of bytes read. Returns -1 on failure.
   */
  std::streamsize read(char* c, std::streamsize bufsize);

  /**
   * Closes all file handles
   */
  void close();


  /**
   * Returns the length of the open file.
   * Returns (size_t)(-1) if there is no file opened.
   */
  size_t file_size() const;

  /**
   * Returns the number of physical bytes read so far. This is an estimate,
   * especially if the file is gzip compressed.
   * Returns (size_t)(-1) if there is no file opened.
   */
  size_t get_bytes_read() const;


  /**
   * Seeks to a different location. Will fail on compressed files.
   */
  std::streampos seek(std::streamoff off, std::ios_base::seekdir way);


  /**
   * Returns the underlying stream object if possible. nullptr otherwise.
   */
  std::shared_ptr<std::istream> get_underlying_stream() const;

 private:
  /**
   * The constructors redirect to this function to open the stream.
   */
  void open_file(std::string file, bool gzip_compressed);
};

} // namespace fileio_impl
} // namespace turi
#endif
