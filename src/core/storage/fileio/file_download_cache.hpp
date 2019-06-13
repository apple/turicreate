/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FILEIO_FILE_DOWNLOAD_CACHE_HPP
#define TURI_FILEIO_FILE_DOWNLOAD_CACHE_HPP
#include <string>
#include <unordered_map>
// this is a circular dependency that needs to be eliminated
#include <core/parallel/mutex.hpp>
namespace turi {

/**
 * \ingroup fileio
 * Provides URL download and caching capabilities.
 * \ref file_download_cache::get_instance() provides a singleton instance of
 * the file_download_cache. Using the file_download_cache simply involves
 * calling the \ref file_download_cache::get_file function which take as its
 * first argument, a general URL (s3, https, http, file, etc) and returns
 * a local file name which can be used to access the file downloaded from the
 * URL.
 *
 * The file_download_cache will cache all the temporary files and avoid
 * re-downloading identical URLs. \ref file_download_cache::release_cache
 * can be used to force a file to be uncached.
 *
 * \ref get_file() is safe to use concurrently. \ref release_cache() has to be
 * used carefully since there are race condition concerns if the downloaded
 * file is still being used by another thread.
 *
 * For s3 files, cache will be updated based on last modification time.
 */
class file_download_cache {
 public:
  /// deletes all downloaded temporary files
  ~file_download_cache();

  /**
   * downloads the URL (it can be s3, https, http, file, or even local file)
   * and returns a local file name at which the contents at the URL can be
   * read from.
   *
   * This function can be safely run in parallel. Though if the same file is
   * requested in two threads simultaneously, the file may be downloaded
   * twice.
   *
   * May throw exceptions if the URL cannot be downloaded.
   */
  std::string get_file(const std::string& url);

  /**
   * Releases the cached copy of the contents of a given URL.
   *
   * This function can be safely run in parallel, but there is the risk that
   * the file may will be used / referenced by another thread. The caller must
   * be careful to guarantee that the local file can be deleted.
   */
  void release_cache(const std::string& url);

  /**
   * Release all cached files
   */
  void clear();

  /**
   * Obtains the global singleton instance of the file download cache
   */
  static file_download_cache& get_instance();

 private:
  struct file_metadata {
    std::string filename;
    std::string last_modified;
  };

 private:
  std::unordered_map<std::string, file_metadata> url_to_file;
  mutex lock;
};


} // namespace turi
#endif
