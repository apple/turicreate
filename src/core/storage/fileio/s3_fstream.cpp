/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <fstream>
#include <core/logging/assertions.hpp>
#include <core/storage/fileio/s3_fstream.hpp>
#include <core/logging/logger.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <core/storage/fileio/s3_api.hpp>
#include <boost/algorithm/string.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <core/storage/fileio/dmlcio/s3_filesys.h>
#include <core/storage/fileio/dmlcio/filesys.h>
#include <core/storage/fileio/dmlcio/io.h>
namespace turi {

s3_device::s3_device(const std::string& filename, const bool write) {
  m_filename = filename;
  // split out the access key and secret key
  s3url url;
  parse_s3url(filename, url);

  m_s3fs = std::make_shared<dmlc::io::S3FileSystem>();
  m_s3fs->SetCredentials(url.access_key_id, url.secret_key);
  std::string url_without_credentials;
  if (url.endpoint.empty()) {
     url_without_credentials = "s3://" + url.bucket + "/" + url.object_name;
  } else {
    url_without_credentials = "s3://" + url.endpoint + "/" + url.bucket + "/" + url.object_name;
  }
  auto uri = dmlc::io::URI(url_without_credentials.c_str());
  if (write) {
    m_write_stream.reset(m_s3fs->Open(uri, "w"));
  } else {
    try {
      auto pathinfo = m_s3fs->GetPathInfo(uri);
      m_filesize = pathinfo.size;
      if (pathinfo.type != dmlc::io::kFile) {
        log_and_throw("Cannot open " + sanitize_url(filename));
      }
      m_read_stream.reset(m_s3fs->OpenForRead(uri));
    } catch (...) {
      log_and_throw("Cannot open " + sanitize_url(filename));
    }
  }
}

void s3_device::close(std::ios_base::openmode mode) {
  if (mode == std::ios_base::out && m_write_stream) {
    logstream(LOG_INFO) << "S3 Finalizing write to " << sanitize_url(m_filename) << std::endl;
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
  if (m_read_stream && !m_read_stream->AtEnd()) return true;
  else if (m_write_stream) return true;
  else return false;
}

std::streampos s3_device::seek(std::streamoff off,
                               std::ios_base::seekdir way,
                               std::ios_base::openmode openmode) {
  if (openmode == std::ios_base::in) {
    if (way == std::ios_base::beg) {
      m_read_stream->Seek(off);
    } else if (way == std::ios_base::cur) {
      size_t offset = m_read_stream->Tell();
      m_read_stream->Seek(offset + off);
    } else if (way == std::ios_base::end) {
      size_t offset = m_read_stream->Tell();
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

} // namespace turi
