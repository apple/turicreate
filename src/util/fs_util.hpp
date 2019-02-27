/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FS_UTIL
#define TURI_FS_UTIL

#include <string>
#include <vector>


namespace turi {

  namespace fs_util {

    /**
     * List all the files with the given suffix at the pathname
     * location
     */
    void list_files_with_suffix(const std::string& pathname,
                                const std::string& suffix,
                                std::vector<std::string>& files,
                                bool ignore_hidden=true);


    /**
     * List all the files with the given prefix at the pathname
     * location
     */
    void list_files_with_prefix(const std::string& pathname,
                                const std::string& prefix,
                                std::vector<std::string>& files,
                                bool ignore_hidden=true);


    /// \ingroup util_internal
    std::string change_suffix(const std::string& fname,
                                     const std::string& new_suffix);


    std::string join(const std::vector<std::string>& components);

    // Generate a path under the system temporary directory.
    // NOTE: This function (like the underlying boost::filesystem call) does
    // not guard against race conditions, and therefore should not be used in
    // security-critical settings.
    std::string system_temp_directory_unique_path(
      const std::string& prefix, const std::string& suffix);

    std::string relativize_path(const std::string& path, const std::string& base_path);

    std::vector<std::string> list_directory(const std::string& path);

    void make_directories(const std::string& path);

    void make_directories_strict(const std::string& path);

    void copy_directory_recursive(
      const std::string& src_path, const std::string& dst_path);
  }; // end of fs_utils


}; // end of turicreate
#endif
