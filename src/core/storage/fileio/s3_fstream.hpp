/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <fstream>
#include <memory>
#include <boost/iostreams/stream.hpp>
#include <core/storage/fileio/read_caching_device.hpp>
namespace dmlc {
namespace io {
class S3FileSystem;
}
class Stream;
class SeekStream;
}

namespace turi {

/**
 * \ingroup fileio
 * \internal
 * s3 file source is used to construct boost iostreams
 */
class s3_device {
 public: // boost iostream concepts
  typedef char                                          char_type;
  struct category :
      public boost::iostreams::device_tag,
      public boost::iostreams::multichar_tag,
      public boost::iostreams::closable_tag,
      public boost::iostreams::bidirectional_seekable { };
  // while this claims to be bidirectional_seekable, that is not true
  // it is only read seekable. Will fail when seeking on write
 private:
  std::string remote_fname;
  std::shared_ptr<dmlc::io::S3FileSystem> m_s3fs;
  std::shared_ptr<dmlc::Stream> m_write_stream;
  std::shared_ptr<dmlc::SeekStream> m_read_stream;
  size_t m_filesize = (size_t)(-1);
 public:
  s3_device() { }

  ~s3_device();

  s3_device(const std::string& filename, const bool write = false);

  // Because the device has bidirectional tag, close will be called
  // twice, one with the std::ios_base::in, followed by out.
  // Only close the file when the close tag matches the actual file type.
  void close(std::ios_base::openmode mode = std::ios_base::openmode());

  /** the optimal buffer size is 0. */
  inline std::streamsize optimal_buffer_size() const { return 0; }

  std::streamsize read(char* strm_ptr, std::streamsize n);

  std::streamsize write(const char* strm_ptr, std::streamsize n);

  bool good() const;

  /**
   * Seeks to a different location.
   */
  std::streampos seek(std::streamoff off,
                      std::ios_base::seekdir way,
                      std::ios_base::openmode);

  /**
   * Returns the file size of the opened file.
   * Returns (size_t)(-1) if there is no file opened, or if there is an
   * error obtaining the file size.
   */
  size_t file_size() const;


  std::shared_ptr<std::istream> get_underlying_stream();
  std::string m_filename;
}; // end of s3 device

typedef boost::iostreams::stream<s3_device> raw_s3_fstream;
typedef boost::iostreams::stream<read_caching_device<s3_device>> s3_fstream;
}
