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

#include <fileio/fileio_constants.hpp>
#include <logger/assertions.hpp>
#include <util/basic_types.hpp>
#include <util/fs_util.hpp>
#include <util/string_util.hpp>

#include <sys/param.h>

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


std::string turi::fs_util::join(const std::vector<std::string>& components) {
  namespace fs = boost::filesystem;
  ASSERT_GT(components.size(), static_cast<size_t>(0));
  fs::path ret(components[0]);
  for (size_t i = 1; i < components.size(); i++) {
    ret /= components[i];
  }
  return ret.string();
}


// Generate a path under the system temporary directory.
// NOTE: This function (like the underlying boost::filesystem call) does
// not guard against race conditions, and therefore should not be used in
// security-critical settings.
std::string turi::fs_util::system_temp_directory_unique_path(
  const std::string& prefix, const std::string& suffix) {

  ASSERT_TRUE(prefix.find("%") == std::string::npos);
  ASSERT_TRUE(prefix.find("/") == std::string::npos);
  ASSERT_TRUE(suffix.find("%") == std::string::npos);
  ASSERT_TRUE(suffix.find("/") == std::string::npos);

  auto base_path = turi::fileio::get_system_temp_directory();
  auto sub_path = boost::filesystem::unique_path(
    prefix + "%%%%-%%%%-%%%%-%%%%-%%%%-%%%%-%%%%-%%%%" + suffix).native();

  return turi::fs_util::join({base_path, sub_path});
}


std::string turi::fs_util::relativize_path(
  const std::string& path, const std::string& base_path) {

  std::vector<std::string> path_tokens = split(path, "/");
  std::vector<std::string> base_path_tokens = split(base_path, "/");

  if (starts_with(path_tokens, base_path_tokens)) {
    return ::join(strip_seq_prefix(path_tokens, base_path_tokens), "/");
  } else {
    return path;
  }
}


void turi::fs_util::make_directories(const string& path) {
  try {
    boost::filesystem::create_directories(path);
  } catch (...) {
    cerr << "Directory path: " << path << endl;
    log_and_throw("Error creating directories");
  }
}


void turi::fs_util::make_directories_strict(const string& path) {
  ASSERT_TRUE(!boost::filesystem::exists(path));
  make_directories(path);
}


vector<string> turi::fs_util::list_directory(const string& path) {
  auto path_is_directory = boost::filesystem::is_directory(path);
  if (!path_is_directory) {
    cerr << path << endl;
    ASSERT_TRUE(path_is_directory);
  }
  vector<string> ret;
  for (auto& x : boost::make_iterator_range(
         boost::filesystem::directory_iterator(path), {})) {
    ret.push_back(x.path().filename().string());
  }
  return ret;
}


void turi::fs_util::copy_directory_recursive(const string& src_path, const string& dst_path) {
  ASSERT_TRUE(boost::filesystem::is_directory(src_path));
  make_directories_strict(dst_path);
  auto dst_path_boost = boost::filesystem::path(dst_path);
  for (const auto& entry_i : boost::filesystem::recursive_directory_iterator(src_path)) {
    auto src_path_i = entry_i.path().string();
    ASSERT_TRUE(starts_with(src_path_i, src_path));
    auto dst_path_boost_i = (dst_path_boost / src_path_i.substr(len(src_path)));
    boost::filesystem::copy(entry_i.path(), dst_path_boost_i);
  }
}
