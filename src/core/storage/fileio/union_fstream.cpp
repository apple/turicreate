/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */
#include <boost/algorithm/string.hpp>
#include <cerrno>
#include <core/logging/logger.hpp>
#include <core/storage/fileio/cache_stream.hpp>
#include <core/storage/fileio/curl_downloader.hpp>
#include <core/storage/fileio/file_download_cache.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/storage/fileio/hdfs.hpp>
#include <core/storage/fileio/s3_fstream.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <core/storage/fileio/union_fstream.hpp>
#include <cstring>

namespace turi {

/**
 * A simple union of
 *
 * 1. cache file (local, hopefully in-memory)
 * 2. turi::hdfs::fstream (remote)
 * 3. s3_fstream (remote)
 * 4. turi::fileio::cache_stream (local file, and remote http, https)
 *
 */
union_fstream::union_fstream(std::string url, std::ios_base::openmode mode,
                             std::string proxy)
    : url(url) {
  input_stream = NULL;
  output_stream = NULL;

  if ((mode & std::ios_base::in) && (mode & std::ios_base::out)) {
    // If the mode is both in and out, raise exception.
    log_and_throw_io_failure(
        "Invalid union_fstream open mode: cannot be both in and out");
  } else if (!(mode & std::ios_base::in) && !(mode & std::ios_base::out)) {
    // If the mode is neither in nor out, raise exception.
    log_and_throw_io_failure(
        "Invalid union_fstream open mode: cannot be neither in nor out");
  }

  bool is_output_stream = (mode & std::ios_base::out);
  auto protocol = fileio::get_protocol(url);
  // since there's temp cache file starts with protocol cache,
  // we need special scrutiny to use the full prefix
  if (protocol == "cache") {
    // Cache file type
    ASSERT_MSG(boost::algorithm::istarts_with(url, fileio::get_cache_prefix()),
               "invalid url cache predix");
    type = CACHE;
    if (is_output_stream) {
      output_stream.reset(new fileio::ocache_stream(url));
    } else {
      auto cachestream = std::make_shared<fileio::icache_stream>(url);
      input_stream = (*cachestream)->get_underlying_stream();
      if (input_stream == nullptr) input_stream = cachestream;
      m_file_size = (*cachestream)->file_size();
      original_input_stream_handle =
          std::static_pointer_cast<std::istream>(cachestream);
    }
  } else if (protocol == "hdfs") {
#ifndef TC_DISABLE_REMOTEFS
    // HDFS file type
    type = HDFS;
    std::string host, port, path;
    std::tie(host, port, path) = fileio::parse_hdfs_url(url);
    logstream(LOG_INFO) << "HDFS URL parsed: Host: " << host
                        << " Port: " << port << " Path: " << path << std::endl;
    if (host.empty() && port.empty() && path.empty()) {
      log_and_throw_io_failure("Invalid hdfs url: " + url);
    }
    try {
      auto& hdfs = turi::hdfs::get_hdfs(host, std::stoi(port));
      ASSERT_TRUE(hdfs.good());
      if (is_output_stream) {
        output_stream.reset(new turi::hdfs::fstream(hdfs, path, true));
      } else {
        input_stream.reset(new turi::hdfs::fstream(hdfs, path, false));
        m_file_size = hdfs.file_size(path);
      }
    } catch (...) {
      log_and_throw_io_failure("Unable to open " + url);
    }
#else
    log_and_throw_io_failure("Cannot open " + url +
                             " for reading; Remote FS support disabled.");
#endif
  } else if (protocol == "s3") {
#ifndef TC_DISABLE_REMOTEFS
    // the S3 file type currently works by download/uploading a local file
    // i.e. the s3_stream simply remaps a local file stream
    type = STD;
    if (is_output_stream) {
      output_stream = std::make_shared<s3_fstream>(url, true);
    } else {
      auto s3stream = std::make_shared<s3_fstream>(url, false);
      input_stream = (*s3stream)->get_underlying_stream();
      if (input_stream == nullptr) input_stream = s3stream;
      m_file_size = (*s3stream)->file_size();
      original_input_stream_handle =
          std::static_pointer_cast<std::istream>(s3stream);
    }
#else
    log_and_throw_io_failure("Cannot open " + url +
                             " for reading; Remote FS support disabled.");
#endif
  } else {
    // Remove the preceeding file:// if it's a local URL starting with file://
    if (protocol == "file") {
      url = url.substr(7);
    }

    /*
     * now, it can be
     * 1. local file
     * 2. http or https
     */
    if (is_output_stream) {
      // Output stream must be a local openable file.
      output_stream.reset(new std::ofstream(url, std::ofstream::binary));
      if (!output_stream->good()) {
        log_and_throw_current_io_failure();
      }
    } else {
      // handles http and https cases
      url = file_download_cache::get_instance().get_file(url);
      input_stream.reset(new std::ifstream(url, std::ifstream::binary));
      if (!input_stream->good()) {
        input_stream.reset();
        log_and_throw_io_failure("Cannot open " + url + " for reading");
      }
      // get the file size
      {
        std::ifstream fin;
        fin.open(url.c_str(), std::ifstream::binary);
        if (fin.good()) {
          fin.seekg(0, std::ios::end);
          m_file_size = fin.tellg();
        }
      }
    }
  }

  if (is_output_stream) {
    ASSERT_TRUE(output_stream->good());
  } else {
    ASSERT_TRUE(input_stream->good());
  }
}

size_t union_fstream::file_size() { return m_file_size; }

/// Destructor
union_fstream::~union_fstream() {}

/// Returns the current stream type. Whether it is a HDFS stream or a STD stream
union_fstream::stream_type union_fstream::get_type() const { return type; }

std::shared_ptr<std::istream> union_fstream::get_istream() {
  ASSERT_TRUE(input_stream != NULL);
  input_stream->exceptions(std::ios_base::badbit);
  return input_stream;
}

std::shared_ptr<std::ostream> union_fstream::get_ostream() {
  ASSERT_TRUE(output_stream != NULL);
  output_stream->exceptions(std::ios_base::badbit);
  return output_stream;
}

/**
 * Returns the filename used to construct the union_fstream
 */
std::string union_fstream::get_name() const { return url; }

}  // namespace turi
