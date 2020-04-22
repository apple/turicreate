/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#include <boost/algorithm/string.hpp>
#include <core/logging/assertions.hpp>
#include <core/logging/logger.hpp>
#include <core/storage/fileio/s3_api.hpp>
#include <core/storage/fileio/s3_filesys.hpp>
#include <core/storage/fileio/s3_fstream.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <fstream>

#ifndef TC_DISABLE_REMOTEF

namespace turi {

s3_device::s3_device(const std::string& filename, const bool write) {
  m_filename = filename;
  // split out the access key and secret key
  s3url url;
  std::string err_msg;
  if (!parse_s3url(filename, url, err_msg)) log_and_throw(err_msg);
  m_s3fs = std::make_shared<fileio::s3::S3FileSystem>(url);

  logstream(LOG_INFO) << "s3_device constructor is invoked" << std::endl;

  if (write) {
    m_write_stream = m_s3fs->Open(url, "w");
  } else {
    try {
      logstream(LOG_DEBUG) << "s3_device reset read_stream" << std::endl;
      m_read_stream = m_s3fs->OpenForRead(url, false /* no throw */);
      ASSERT_TRUE(m_read_stream != nullptr);
      // this is a bad design to cache the filesize
      m_filesize = m_read_stream->FileSize();
    } catch (const error::io_error& e) {
      std::stringstream ss;
      ss << "Cannot open " + sanitize_url(filename)
         << "Error code:" << e.what();
      log_and_throw_io_failure(ss.str());
    } catch (...) {
      log_and_throw_io_failure("Cannot open " + sanitize_url(filename));
    }
  }
}

void s3_device::close(std::ios_base::openmode mode) {
  if (mode == std::ios_base::out && m_write_stream) {
    logstream(LOG_INFO) << "S3 Finalizing write to " << sanitize_url(m_filename)
                        << std::endl;
    m_write_stream->Close();
    m_write_stream.reset();
  } else if (mode == std::ios_base::in && m_read_stream) {
    m_read_stream->Close();
    m_read_stream.reset();
  }
}

std::streamsize s3_device::read(char* strm_ptr, std::streamsize n) {
  return m_read_stream->Read((void*)strm_ptr, n);
}

std::streamsize s3_device::write(const char* strm_ptr, std::streamsize n) {
  m_write_stream->Write((void*)(strm_ptr), n);
  return n;
}

bool s3_device::good() const {
  if (m_read_stream && !m_read_stream->AtEnd())
    return true;
  else if (m_write_stream)
    return true;
  else
    return false;
}

std::streampos s3_device::seek(std::streamoff off, std::ios_base::seekdir way,
                               std::ios_base::openmode openmode) {
  if (openmode == std::ios_base::in) {
    if (way == std::ios_base::beg) {
      m_read_stream->Seek(off);
    } else if (way == std::ios_base::cur) {
      size_t offset = m_read_stream->Tell();
      m_read_stream->Seek(offset + off);
    } else if (way == std::ios_base::end) {
      size_t offset = m_filesize;
      DASSERT_TRUE(m_filesize == m_read_stream->FileSize());
      m_read_stream->Seek(offset + off - 1);
    }
    return m_read_stream->Tell();
  } else {
    ASSERT_MSG(false, "Unable to seek!");
    ASSERT_UNREACHABLE();
  }
}

size_t s3_device::file_size() const {
  if (m_read_stream) {
    return m_filesize;
  } else {
    return (size_t)(-1);
  }
}

std::shared_ptr<std::istream> s3_device::get_underlying_stream() {
  return nullptr;
}

s3_device::~s3_device() {
  m_write_stream.reset();
  m_read_stream.reset();
  m_s3fs.reset();
}

}  // namespace turi

#endif  // End ifndef TC_DISABLE_REMOTEF
