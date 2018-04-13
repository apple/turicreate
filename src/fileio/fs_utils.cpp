/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <regex>
#include <fileio/fs_utils.hpp>
#include <fileio/hdfs.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <fileio/fixed_size_cache_manager.hpp>
#include <fileio/file_handle_pool.hpp>
#include <fileio/s3_api.hpp>
#include <fileio/sanitize_url.hpp>
#include <export.hpp>

namespace turi {
namespace fileio {


// import boost filesystem
namespace fs = boost::filesystem;

/**
 * Return canonical absolute path, eliminating dots, and symlinks
 */
EXPORT std::string make_canonical_path(const std::string& path) {
  namespace fs = boost::filesystem;
  auto p = fs::path(path);
  try {
    if (p.is_absolute()) {
      return fs::canonical(p).string();
    } else {
      return fs::canonical(fs::absolute(p)).string();
    }
  } catch(fs::filesystem_error e) {
    log_and_throw(std::string("Invalid path: ") + path + ". " + e.what());
  }
}


/**
 * A helper function to parse the hdfs url.
 * Return a tuple of host, port, and path.
 */
std::tuple<std::string, std::string, std::string> parse_hdfs_url(std::string url) {

  const std::string default_host = "default";
  const std::string default_port = "0";
  const std::string default_path = "";

  auto warn_and_return_default = [=](const std::string& reason = ""){
    logstream(LOG_WARNING) << "Cannot parse hdfs url: " << url << ". " << reason << std::endl;
    return std::make_tuple(default_host, default_port, default_path);
  }; 

  if (!boost::starts_with(url, "hdfs://")) {
    return warn_and_return_default();
  };

  // get the string after hdfs://
  std::string base(url.begin()+7, url.end());
  std::string host = default_host;
  std::string port = default_port;
  std::string path = default_path;

  /**
   * "/" and ":" are not allowed in the the path elements, the following code
   * should parse correctly three forms of urls:
   * hdfs:///foo/bar
   * hdfs://hostname/foo/bar
   * hdfs://hostname:port/foo/bar
   */
  // find the first '/' character
  auto first_slash_pos = base.find_first_of("/");

  if (first_slash_pos  == std::string::npos) {
    // No match
    return warn_and_return_default();
  } else if (first_slash_pos == 0) {
    // Matches hdfs:///foo/bar
    path = base;
  } else {
    // Matches hdfs://PREFIX/PATH
    auto prefix = std::string(base.begin(), base.begin()+first_slash_pos);
    path = std::string(base.begin()+first_slash_pos, base.end());
    // Parse the prefix:
    // case 1. HOSTNAME:PORT
    // case 2. HOSTNAME
    auto first_colon_pos = prefix.find_first_of(":");
    if (first_colon_pos == std::string::npos) {
      // Matches hdfs://HOSTNAME/PATH
      host = prefix;
    } else {
      // Matchees hdfs://HOSTNAME:PORT/PATH
      host = std::string(prefix.begin(), prefix.begin() + first_colon_pos);
      port = std::string(prefix.begin() + first_colon_pos+ 1, prefix.end());
    }
  }

  // Sanity check: host should not contain '/' or ':'
  if (host.find_first_of('/') != std::string::npos ||
      host.find_first_of(':') != std::string::npos) {
    std::stringstream msg;
    msg << "host = " << host << " must not contain '/' or ':'";
    warn_and_return_default(msg.str());
  }

  // Sanity check:
  for (auto c: port) {
    if (!isdigit(c)) {
      std::stringstream msg;
      msg << "port = " << port << " must be all digits";
      return warn_and_return_default(msg.str());
    }
  }

  // Sanity check:
  if (path.find_first_of(':') != std::string::npos) {
    std::stringstream msg;
    msg << "path = " << path << " must not contain ':'";
    return warn_and_return_default(msg.str());
  }
  return std::make_tuple(host, port, path);
}

EXPORT file_status get_file_status(const std::string& path) {
  if(boost::starts_with(path, "hdfs://")) {
    // hdfs
    std::string host, port, hdfspath;
    std::tie(host, port, hdfspath) = parse_hdfs_url(path);
    try {
      // get the HDFS object
      auto& hdfs = turi::hdfs::get_hdfs(host, std::stoi(port));
      // fail we we are unable to construct the HDFS object
      if (!hdfs.good()) return file_status::FS_UNAVAILABLE;
      // we are good. Use the HDFS accessors to figure out what to return
      if (!hdfs.path_exists(hdfspath)) return file_status::MISSING;
      else if (hdfs.is_directory(hdfspath)) return file_status::DIRECTORY;
      else return file_status::REGULAR_FILE;
    } catch(...) {
      // failure for some reason. fail with missing
      return file_status::MISSING;
    }
  } else if (boost::starts_with(path, fileio::get_cache_prefix())) {
    // this is a cache file. it is only REGULAR or MISSING
    try {
      fixed_size_cache_manager::get_instance().get_cache(path);
      return file_status::REGULAR_FILE;
    } catch (...) {
      return file_status::MISSING;
    }
  } else if (boost::starts_with(path, "s3://")) {
    std::pair<bool, bool> ret = is_directory(path);
    if (ret.first == false) return file_status::MISSING;
    else if (ret.second == false) return file_status::REGULAR_FILE;
    else if (ret.second == true) return file_status::DIRECTORY;
  } else if (is_web_protocol(get_protocol(path))) {
    return file_status::REGULAR_FILE;
    // some other web protocol?
  } else {
    // regular file
    struct stat statout;
    int ret = stat(path.c_str(), &statout);
    if (ret != 0) return file_status::MISSING;
    if (S_ISDIR(statout.st_mode)) return file_status::DIRECTORY;
    else return file_status::REGULAR_FILE;
  }
  return file_status::MISSING;
}


std::vector<std::pair<std::string, file_status>>
get_directory_listing(const std::string& path) {
  std::vector<std::pair<std::string, file_status> > ret;
  if(boost::starts_with(path, "hdfs://")) {
    // hdfs
    std::string host, port, hdfspath;
    std::tie(host, port, hdfspath) = parse_hdfs_url(path);
    if (hdfspath.empty()) return ret;
    try {
      // get the HDFS object
      auto& hdfs = turi::hdfs::get_hdfs(host, std::stoi(port));
      auto dircontents = hdfs.list_files_and_stat(hdfspath);
      for (auto direntry: dircontents) {
        if (direntry.second) {
          ret.push_back({direntry.first, file_status::DIRECTORY});
        } else {
          ret.push_back({direntry.first, file_status::REGULAR_FILE});
        }
      }
      // fail we we are unable to construct the HDFS object
    } catch(...) {
      // failure for some reason. return with nothing
    }
  } else if (boost::starts_with(path, fileio::get_cache_prefix())) {
    // this is a cache file. There is no filesystem.
    // it is only REGULAR or MISSING
    return ret;
  } else if (boost::starts_with(path, "s3://")) {
    list_objects_response response = list_directory(path);
    for (auto dir: response.directories) {
      ret.push_back({dir, file_status::DIRECTORY});
    }
    for (auto obj: response.objects) {
      ret.push_back({obj, file_status::REGULAR_FILE});
    }
  } else {
    try {
      fs::path dir(path);
      auto diriter = fs::directory_iterator(path);
      auto enditer = fs::directory_iterator();
      while(diriter != enditer) {
        bool is_directory = fs::is_directory(diriter->path());
        if (is_directory) {
          ret.push_back({convert_to_generic(diriter->path().string()),
              file_status::DIRECTORY});
        } else {
          ret.push_back({convert_to_generic(diriter->path().string()),
              file_status::REGULAR_FILE});
        }
        ++diriter;
      }
    } catch(...) {
      // failure for some reason. return with nothing
    }
  }
  return ret;
}

EXPORT bool create_directory(const std::string& path) {
  file_status stat = get_file_status(path);
  if (stat != file_status::MISSING) {
    return false;
  }
  if(boost::starts_with(path, "hdfs://")) {
    // hdfs
    std::string host, port, hdfspath;
    std::tie(host, port, hdfspath) = parse_hdfs_url(path);
    try {
      // get the HDFS object
      auto& hdfs = turi::hdfs::get_hdfs(host, std::stoi(port));
      return hdfs.create_directories(hdfspath);
      // fail we we are unable to construct the HDFS object
    } catch(...) {
      // failure for some reason. return with nothing
      return false;
    }
  } else if (boost::starts_with(path, fileio::get_cache_prefix())) {
    // this is a cache file. There is no filesystem.
    return true;
  } else if (boost::starts_with(path, "s3://")) {
    // S3 doesn't need directories
    return true;
  } else {
    try {
      create_directories(fs::path(path));
    } catch (...) {
      return false;
    }
    return true;
  }
  return false;
}

EXPORT bool delete_path(const std::string& path,
                        file_status stat) {
  if (stat == file_status::FS_UNAVAILABLE) stat = get_file_status(path);

  if (stat == file_status::MISSING) {
    return false;
  }

  // For regular file, go through global file pool to make sure we don't
  // delete files that are in use by some SArray
  if (stat == file_status::REGULAR_FILE &&
    fileio::file_handle_pool::get_instance().mark_file_for_delete(path)) {
    logstream(LOG_INFO) << "Attempting to delete " << sanitize_url(path)
                        << " but it is still in use. It will be deleted"
                        << " when all references to the file are closed"
                        << std::endl;
    return true;
  } else {
    return delete_path_impl(path, stat);
  }
}

bool delete_path_impl(const std::string& path,
                      file_status stat) {
  if (stat == file_status::FS_UNAVAILABLE) stat = get_file_status(path);
  if (stat == file_status::MISSING) {
    return false;
  }
  logstream(LOG_INFO) << "Deleting " << sanitize_url(path) << std::endl;
  if(boost::starts_with(path, "hdfs://")) {
    // hdfs only has a recursive deleter. we need to make this safe
    // if the current path is a non-empty directory, fail
    if (stat == file_status::DIRECTORY) {
      if (get_directory_listing(path).size() != 0) {
        return false;
      }
    }
    // hdfs
    std::string host, port, hdfspath;
    std::tie(host, port, hdfspath) = parse_hdfs_url(path);
    try {
      // get the HDFS object
      auto& hdfs = turi::hdfs::get_hdfs(host, std::stoi(port));
      return hdfs.delete_file_recursive(hdfspath);
      // fail we we are unable to construct the HDFS object
    } catch(...) {
      // failure for some reason. return with nothing
      return false;
    }
  } else if (boost::starts_with(path, fileio::get_cache_prefix())) {
    try {
      // we ignore recursive here. since the cache can't hold directories
      auto cache_entry = fixed_size_cache_manager::get_instance().get_cache(path);
      fixed_size_cache_manager::get_instance().free(cache_entry);
      return true;
    } catch (...) {
      return false;
    }
  } else if (boost::starts_with(path, "s3://")) {
    return delete_object(path).empty();
  } else {
    try {
      fs::remove(fs::path(path));
      return true;
    } catch (...) {
      return false;
    }
  }
  return true;
}

EXPORT bool delete_path_recursive(const std::string& path) {
  file_status stat = get_file_status(path);
  if (stat == file_status::REGULAR_FILE) {
    delete_path(path);
  } else if (stat == file_status::MISSING) {
    return true;
  }
  if(boost::starts_with(path, "hdfs://")) {
    // hdfs
    std::string host, port, hdfspath;
    std::tie(host, port, hdfspath) = parse_hdfs_url(path);
    try {
      // get the HDFS object
      auto& hdfs = turi::hdfs::get_hdfs(host, std::stoi(port));
      return hdfs.delete_file_recursive(hdfspath);
      // fail we we are unable to construct the HDFS object
    } catch(...) {
      // failure for some reason. return with nothing
      return false;
    }
  } else if(boost::starts_with(path, "s3://")) {
    return delete_prefix(path).empty();
  } else if (boost::starts_with(path, fileio::get_cache_prefix())) {
    // recursive deletion not possible with cache
    return true;
  } else {
    try {
      fs::remove_all(fs::path(path));
      return true;
    } catch (...) {
      return false;
    }
  }
}

bool is_writable_protocol(std::string protocol) {
  return protocol == "hdfs" || protocol == "s3" ||
      protocol == "" || protocol == "file" || protocol == "cache";
}

bool is_web_protocol(std::string protocol) {
  return !is_writable_protocol(protocol);
}

EXPORT std::string get_protocol(std::string path) {
  size_t proto = path.find("://");
  if (proto != std::string::npos) {
    std::string ret = boost::algorithm::to_lower_copy(path.substr(0, proto));
    // Strip out file as a specific protocol
    if(ret == "file") {
      return "";
    } else {
      return ret;
    }

  } else {
    return "";
  }
}

std::string remove_protocol(std::string path) {
  size_t proto = path.find("://");
  if (proto != std::string::npos) {
    return path.substr(proto + 3); // 3 is the "://"
  } else {
    return path;
  }
}


std::string get_filename(std::string path) {
  return fs::path(path).filename().string();
}

std::string get_dirname(std::string path) {
  std::string ret_path;
  auto proto = get_protocol(path);
  auto proto_removed = remove_protocol(path);

  if(proto.size()) {
    ret_path += proto;
    ret_path += std::string("://");
  }

  ret_path += fs::path(proto_removed).parent_path().string();

  // S3 is sensitive to trailing slashes, double slashes, etc.
  
  if((ret_path.size() > 0) && (ret_path.at(ret_path.size()-1) == '/'))
    ret_path.pop_back();
  return ret_path;
}

std::string convert_to_generic(const std::string &path) {
  return fs::path(path).generic_string();
}

std::string make_relative_path(std::string root_directory, std::string path) {
  // If backslashes, convert them to slashes
  root_directory = convert_to_generic(root_directory);
  path = convert_to_generic(path);
  auto original_absolute_path = path;

  // if different protocols, it is not possible to relativize
  if (get_protocol(root_directory) != get_protocol(path)) return original_absolute_path;

  root_directory = remove_protocol(root_directory);
  path = remove_protocol(path);

  if (root_directory.empty()) root_directory = "/";

  // at this point the root directory should be a proper absolute file path
  // with a "/" at the end;
  std::vector<std::string> root_elements;
  std::vector<std::string> path_elements;
  boost::algorithm::split(root_elements, root_directory, boost::is_any_of("/"));
  // "/" is a special path. If we end with "/" delete the last element because
  // "/" will return two empty strings, where the correct result really,
  // is one empty string.
  if (boost::ends_with(root_directory, "/") && root_elements.size() > 0) {
    root_elements.pop_back();
  }
  if (!path.empty()) {
    boost::algorithm::split(path_elements, path, boost::is_any_of("/"));
  }

  // count the number of matching path elements
  size_t num_root_elements_match = 0;
  auto min_elements = std::min(root_elements.size(), path_elements.size());
  for (size_t i = 0;i < min_elements; ++i) {
    if (path_elements[i] == root_elements[i]) {
      num_root_elements_match = i + 1;
    } else {
      break;
    }
  }

  // if no path elements match, or if only an empty string path element
  // match, just use the absolute path
  if (num_root_elements_match == 0) return original_absolute_path;

  // for each of the unmatched root elements, we go "../"
  std::vector<std::string> new_relative_path_elements;
  for (size_t i = num_root_elements_match; i < root_elements.size(); ++i) {
    new_relative_path_elements.push_back("..");
  }
  // then all the rest of the regular path elements
  std::copy(path_elements.begin() + num_root_elements_match,
            path_elements.end(),
            std::inserter(new_relative_path_elements, new_relative_path_elements.end()));
  std::string retpath = boost::algorithm::join(new_relative_path_elements, "/");
  return retpath;
}

EXPORT std::string make_absolute_path(std::string root_directory, std::string path) {
  // If backslashes, convert them to slashes
  root_directory = convert_to_generic(root_directory);
  path = convert_to_generic(path);

  // normalize the root directory.
  // If it ends with a "/" drop it.
  // If it is "hdfs://" or "s3://" something like that, its ok even though it
  // ends with a "/"
  if (boost::algorithm::ends_with(root_directory, "://") == false &&
      boost::algorithm::ends_with(root_directory, "/")) {
    root_directory = root_directory.substr(0, root_directory.length() - 1);
  }

  root_directory = root_directory + "/";

  // at this point the root directory should be a proper absolute file path
  // with a "/" at the end;

  if (path.empty() || boost::algorithm::contains(path, "://")
      || boost::algorithm::starts_with(path, "/")) {
    // if path "looks" like an absolute path, just return it
    return path;
  } else {
    return root_directory + path;
  }
}

std::regex glob_to_regex(const std::string& glob) { 
  // this is horribly incomplete. But works sufficiently
  std::string glob_pattern(glob);
  boost::replace_all(glob_pattern, "/", "\\/");
  boost::replace_all(glob_pattern, "?", ".");
  boost::replace_all(glob_pattern, "*", ".*");
  return std::regex(glob_pattern);
} 

/**
 * Behaves like python os.path.split.
 * - if url is a directory, return (directory path, "")
 * - if url is a file, return (directory path, filename)
 * - if url is a glob pattern, split into (directory, pattern)
 */
std::pair<std::string, std::string> split_path_elements(const std::string& url, 
                                                        file_status& status) {
  std::pair<std::string, std::string> res;
  if (status == file_status::DIRECTORY) {
    res = std::make_pair(url, "");
  } else {
    res = std::make_pair(get_dirname(url), get_filename(url));
  }
  return res;
}

/**
 * Collects contents of "url" path, testing the rest against the glob
 * return matching file(s) as (url, status) pairs
 */
EXPORT std::vector<std::pair<std::string, file_status>> get_glob_files(const std::string& url) {
  auto trimmed_url = url;
  boost::algorithm::trim(trimmed_url);
  file_status status = get_file_status(trimmed_url);
  if (status == file_status::REGULAR_FILE) {
    // its a regular file. Ignore the glob and load it
    return {{url, file_status::REGULAR_FILE}};
  } else if(status == file_status::FS_UNAVAILABLE) {
    log_and_throw("Filesystem unavailable. Check server log for details.");
  }
  std::pair<std::string, std::string> path_elements = split_path_elements(trimmed_url, status);
  std::vector<std::pair<std::string, file_status>> files;

  if (path_elements.second == "") {
    for (auto entry : get_directory_listing(trimmed_url)) {
      files.push_back(make_pair(entry.first, entry.second));
    }
  } else {
    auto glob_regex = glob_to_regex(path_elements.second);
    for (auto entry : get_directory_listing(path_elements.first)) {
      if (std::regex_match(get_filename(entry.first), glob_regex)) {
        files.push_back(make_pair(entry.first, entry.second));
      }
    }
  }
  // unable to glob anything. 
  if (files.size() == 0) files.push_back({url, file_status::MISSING});

  return files;
}

size_t get_io_parallelism_id(const std::string url) {
  std::string protocol = get_protocol(url);

  if (is_web_protocol(protocol) || protocol == "s3" || protocol == "hdfs") {
    // web protocols, s3 and hdfs will be read in parallel always.
    // Those tend to be remote server bound.
    return (size_t)(-1);
  } else if (protocol == "cache") {
    try {
      auto cache_entry = fixed_size_cache_manager::get_instance().get_cache(url);
      if (cache_entry) {
        if (cache_entry->is_pointer()) {
          // if its a cached pointer, we can read in parallel always
          return (size_t)(-1);
        } else if (cache_entry->is_file()) {
          // if it is on file, a bit more work is needed
          // get the temp directories and figure out which one I am a prefix of.
          // each prefix gets its own ID.
          std::string filename = cache_entry->get_filename();
          std::vector<std::string> temp_directories = get_temp_directories();
          for (size_t i = 0;i < temp_directories.size(); ++i) {
            if (boost::starts_with(filename, temp_directories[i])) {
              return i;
            }
          }
        }
      }
    } catch (...) { }
  }
  // all cases, failure cases, missing files, missing cache entries, unknown
  // protocols, local files, etc.
  // assume there is just one local disk.
  return 0;
}

bool try_to_open_file(const std::string url) {
  // if file doesn't exist, we fail immediately
  if (get_file_status(url) != file_status::REGULAR_FILE) {
    return false;
  }
  bool success = true;
  try {
    general_ifstream fin(url);
    success = !fin.fail();
  } catch(...) {
    success = false;
  }
  return success;
}

void copy(const std::string src, const std::string dest) {
  general_ifstream fin(src.c_str());
  general_ofstream fout(dest.c_str());
  std::vector<char> buffer(1024*1024); // 1MB
  while(fin) {
    fin.read(buffer.data(), buffer.size());
    fout.write(buffer.data(), fin.gcount());
  }
}

bool change_file_mode(const std::string path, short mode) {
  file_status stat = get_file_status(path);
  if (stat == file_status::MISSING) {
    return false;
  }

  if(boost::starts_with(path, "hdfs://")) {
#ifdef HAS_HADOOP
    // hdfs
    std::string host, port, hdfspath;
    std::tie(host, port, hdfspath) = parse_hdfs_url(path);
    try {
      // get the HDFS object
      auto& hdfs = turi::hdfs::get_hdfs(host, std::stoi(port));
      return hdfs.chmod(hdfspath, mode);
      // fail we we are unable to construct the HDFS object
    } catch(...) {
      // failure for some reason. return with nothing
      return false;
    }
#else
      return false;
#endif
  } else if (boost::starts_with(path, fileio::get_cache_prefix())) {
    // this is a cache file. There is no filesystem.
    return true;
  } else if (boost::starts_with(path, "s3://")) {
    // S3 doesn't need directories
    return true;
  } else {
    try {
      //permissions(fs::path(path), mode);
      return false;
    } catch (...) {
      return false;
    }
    return true;
  }
  return false;

}

} // namespace fileio
} // namespace turi
