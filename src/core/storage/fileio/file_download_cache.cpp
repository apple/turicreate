/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <core/logging/logger.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <core/storage/fileio/curl_downloader.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <core/storage/fileio/file_download_cache.hpp>
#include <core/storage/fileio/s3_api.hpp>
#include <core/storage/fileio/sanitize_url.hpp>
#include <core/export.hpp>

namespace turi {

file_download_cache::~file_download_cache() {
  try {
    clear();
  } catch(...) {
    logstream(LOG_WARNING) << "Error clearning file download cache"
                           << std::endl;
  }
}

std::string file_download_cache::get_file(const std::string& url) {
  // first check if the file has been downloaded.
  // if it has, return the downloaded location
  lock.lock();
  if (url_to_file.count(url)) {
    bool cache_dirty = false;
#ifndef TC_ENABLE_REMOTEFS
    if (boost::starts_with(url, "s3://")) {
      std::string last_modified = "";
      try {
        last_modified = get_s3_file_last_modified(url);
      } catch (...) {
        lock.unlock();
        throw;
      }
      if (last_modified != url_to_file[url].last_modified) {
        cache_dirty = true;
      }
    }
#endif
    if (!cache_dirty) {
      std::string ret = url_to_file[url].filename;
      lock.unlock();
      return ret;
    }
  }
  lock.unlock();

  // ok. we need to download the file
  // Ok, it is either local regular file, file:///, or remote urls http://.
  // For remote urls, download_url download it into to local file.
  // For local urls, download_url return as is.
  std::string localfile;
  int status; bool is_temp;
  std::tie(status, is_temp, localfile) = download_url(url);
  if (status) {
#ifdef TC_ENABLE_REMOTEFS
    log_and_throw_io_failure("Not implemented: compiled without support for http(s):// URLs.");
#else
    log_and_throw_io_failure("Fail to download from " + url +
                             ". " + get_curl_error_string(status));
#endif
  }
  if (is_temp) {
    // if it is a remote file, we check the download status code
    lock.lock();
    url_to_file[url].filename = localfile;
    url_to_file[url].last_modified = "";
    lock.unlock();
    return localfile;
  } else {
    // purely a local file. just return it
    return localfile;
  }
}

void file_download_cache::release_cache(const std::string& url) {
  // look for the file in the url_to_file map and delete it.
  lock.lock();
  if (url_to_file.count(url)) {
    delete_temp_file(url_to_file[url].filename);
    url_to_file.erase(url);
  }
  lock.unlock();
}

EXPORT file_download_cache& file_download_cache::get_instance() {
  static file_download_cache cache;
  return cache;
}

EXPORT void file_download_cache::clear() {
  for(auto p: url_to_file) {
    delete_temp_file(p.second.filename);
  }
  url_to_file.clear();
}

} // namespace turi
