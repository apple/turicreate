/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <boost/version.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>


#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>




#include <util/fs_util.hpp>

bool is_hidden(const std::string path) {
  if (path.length() && path[0] == '.') {
    return true;
  }
  return false;
}

void turi::fs_util::
list_files_with_suffix(const std::string& pathname,
                       const std::string& suffix,
                       std::vector<std::string>& files,
                       bool ignore_hidden) {
  namespace fs = boost::filesystem;
  fs::path dir_path(pathname);
  fs::directory_iterator end_iter;
  files.clear();
  if ( fs::exists(dir_path) && fs::is_directory(dir_path)) {
    for( fs::directory_iterator dir_iter(dir_path) ;
         dir_iter != end_iter ; ++dir_iter) {
      if (fs::is_regular_file(dir_iter->status()) ) {
#if BOOST_FILESYSTEM_VERSION >= 3
        const std::string filename = dir_iter->path().filename().string();
#else
        const std::string filename = dir_iter->leaf();
#endif
        if (suffix.size() > 0 && !boost::ends_with(filename, suffix))
          continue;
        if (!ignore_hidden || !is_hidden(filename)) {
          files.push_back(filename);
        }
      }
    }
  }
  std::sort(files.begin(), files.end());
//   namespace fs = boost::filesystem;
//   fs::path path(pathname);
//   assert(fs::exists(path));
//   for(fs::directory_iterator iter( path ), end_iter;
//       iter != end_iter; ++iter) {
//     if( ! fs::is_directory(iter->status()) ) {

// #if BOOST_FILESYSTEM_VERSION >= 3
//       std::string filename(iter->path().filename().string());
// #else
//       std::string filename(iter->path().filename());
// #endif
//       size_t pos =
//         filename.size() >= suffix.size()?
//         filename.size() - suffix.size() : 0;
//       std::string ending(filename.substr(pos));
//       if(ending == suffix) {
// #if BOOST_FILESYSTEM_VERSION >= 3
//         files.push_back(iter->path().filename().string());
// #else
//         files.push_back(iter->path().filename());
// #endif
//       }
//     }
//   }
//  std::sort(files.begin(), files.end());
} // end of list files with suffix



void turi::fs_util::
list_files_with_prefix(const std::string& pathname,
                       const std::string& prefix,
                       std::vector<std::string>& files,
                       bool ignore_hidden) {
  namespace fs = boost::filesystem;
  fs::path dir_path(pathname);
  fs::directory_iterator end_iter;
  files.clear();
  if ( fs::exists(dir_path) && fs::is_directory(dir_path)) {
    for( fs::directory_iterator dir_iter(dir_path) ;
         dir_iter != end_iter ; ++dir_iter) {
      if (fs::is_regular_file(dir_iter->status()) ) {
        const std::string filename = dir_iter->path().filename().string();
        if (prefix.size() > 0 && !boost::starts_with(filename, prefix)) {
          continue;
        }
        if (!ignore_hidden || !is_hidden(dir_iter->path().leaf().string())) {
          files.push_back(dir_iter->path().string());
        }
      }
    }
  }
  std::sort(files.begin(), files.end());
} // end of list files with prefix





std::string turi::fs_util::
change_suffix(const std::string& fname,
              const std::string& new_suffix) {
  size_t pos = fname.rfind('.');
  assert(pos != std::string::npos);
  const std::string new_base(fname.substr(0, pos));
  return new_base + new_suffix;
} // end of change_suffix
