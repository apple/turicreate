/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef _WIN32
#include <pwd.h>
#else
#include <cross_platform/windows_wrapper.hpp>
#include <Lmcons.h>
#endif

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <set>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <parallel/mutex.hpp>
#include <logger/logger.hpp>
#include <logger/assertions.hpp>
#include <fileio/fileio_constants.hpp>
#include <fileio/fs_utils.hpp>
#include <util/syserr_reporting.hpp>
#include <process/process_util.hpp>
#include <network/net_util.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <export.hpp>

namespace turi {

// import boost filesystem
namespace fs = boost::filesystem;

namespace fs_impl {

struct tempfile_information {
  /// Lock on all the structures below
  mutex lock;

  /// Lists all the tempfile creates
  std::set<std::string> tempfile_history;

  /// lists all the temporary directories created by this process
  std::set<boost::filesystem::path> process_temp_directories;

  /// Counts the number of temp files created
  size_t temp_file_counter;

  /// Used to generate unique file IDs
  boost::uuids::random_generator uuid_generator;
}; 


/**
 * Returns the process-wide singleton managing all the temp file information.
 * The object inside is created as a pointer; all the data above is meant to 
 * leak! Basically we must carefully control the ordering at which this
 * object is destroyed. (See \ref turi::global_teardown).
 */
EXPORT tempfile_information& get_temp_info() {
  // this is a pointer 
  static tempfile_information* temp_info = new tempfile_information();
  return *temp_info;
}

} // fs_impl

// forward declarations for some stuff the temp_file_deleter needs
static fs::path get_current_process_temp_directory(size_t idx);


using namespace fs_impl;


/**
 * Returns all the temp directories available.
 */
std::vector<std::string> get_temp_directories() {
  std::vector<std::string> paths;
  if(fileio::get_cache_file_locations() == std::string("CHANGEME")) {
    fileio::set_cache_file_locations(turi::fileio::get_system_temp_directory());
  }
  auto cache_file_locations = fileio::get_cache_file_locations();
  boost::algorithm::split(paths,
                          cache_file_locations,
#ifndef _WIN32
                          boost::algorithm::is_any_of(":"));
#else
                          boost::algorithm::is_any_of(";"));
#endif
  return paths;
}

/**
 * Get the current system user name
 */
std::string get_system_user_name() {
#ifndef _WIN32
  struct passwd* p = getpwuid(getuid());
  if (p != NULL) return p->pw_name;
  else return "";
#else
  DWORD username_len = UNLEN+1;
  char username[UNLEN+1];
  if(GetUserName(username, &username_len)) {
    return username;
  } else {
    auto err_code = GetLastError();
    logstream(LOG_INFO) << "Could not get username: " <<
      get_last_err_str(err_code) << std::endl;
    return "";
  }
#endif
}


/**
 * Return turicreate-[user_name]
 */
static std::string get_turicreate_temp_directory_prefix() {
  std::string turicreate_name = "turicreate";
  std::string user_name = get_system_user_name();
  if (!user_name.empty()) {
    turicreate_name = turicreate_name + "-" + user_name;
  }
  return turicreate_name;
}

static fs::path get_current_process_hdfs_temp_directory() {
  fs::path path;
  if (fileio::get_cache_file_hdfs_location() != "") {
    path = fs::path(fileio::get_cache_file_hdfs_location());
    std::string turicreate_name = get_turicreate_temp_directory_prefix();
    path /= turicreate_name;
  }
  return path;
}

/**
 * Returns the number of temp directories available.
 */
size_t num_temp_directories() {
  return get_temp_directories().size();
}


/**
 * Gets the 'idx''th Turi temp directory.
 * 
 * The temp directories are turicreate-[username] appended to the temp directory.
 *
 * idx can be any value in which case the indices will loop around.
 *
 * Ex: Say there are 2 temp directories. /tmp and /var/tmp.
 *
 * get_turicreate_temp_directory(0) will return "/tmp/turicreate-[username]"
 * get_turicreate_temp_directory(1) will return "/var/tmp/turicreate-[username]"
 * and get_turicreate_temp_directory(2) will return "/tmp/turicreate-[username]"
 */
static fs::path get_turicreate_temp_directory(size_t idx) {
  auto temp_dirs = get_temp_directories();
  ASSERT_GT(temp_dirs.size(), 0);
  fs::path temp_path = temp_dirs[idx % temp_dirs.size()];
  std::string turicreate_name = get_turicreate_temp_directory_prefix();
  fs::path path(temp_path / turicreate_name);
  return path;
}

/**
 * Finds the temp directory for the current process.
 *
 * The temp directories are turicreate-[username]/procname appended to the
 * temp directory.
 *
 * idx can be any value in which case the indices will loop around.
 *
 * Ex: Say there are 2 temp directories. /tmp and /var/tmp.
 *
 * get_current_process_temp_directory(0) will return "/tmp/turicreate-[username]/[pid]"
 * get_current_process_temp_directory(1) will return "/var/tmp/turicreate-[username]/[pid]"
 * and get_current_process_temp_directory(2) will return "/tmp/turicreate-[username]/[pid]"
 */
static fs::path get_current_process_temp_directory(size_t idx) {
  return get_turicreate_temp_directory(idx) / std::to_string(getpid());
}



/**
 * Creates the current process's temp directory if it does not already exist.
 *
 * idx can be any value in which case the indices will loop around.
 */
static void create_current_process_temp_directory(std::string path) {
  bool success = true;

  try {
    if (fileio::get_file_status(path) != fileio::file_status::DIRECTORY) {
      success = fileio::create_directory(path);
      if(success)
        get_temp_info().process_temp_directories.insert(path);
    }
  } catch (...) {
    success = false;
  }

  if(! success) {
    std::stringstream error_message;
    error_message << "Unable to a create temporary directory at \""
                  << path << "\". This location can be changed by calling:\n"
                  << "turicreate.config.set_runtime_config('TURI_CACHE_FILE_LOCATIONS', <writable path>)\n"
                  << std::endl;
    log_and_throw(error_message.str());
  }
}



static void delete_proc_directory(fs::path path) {
  // we could use remove_all but that causes problems, I suspect
  // when multiple processes are trying to reap simultaneously.
  std::vector<fs::path> files_to_delete;
  auto diriter = fs::recursive_directory_iterator(path, 
                                                  fs::symlink_option::no_recurse);
  auto enditer = fs::recursive_directory_iterator();
  while(diriter != enditer) {
    if (fs::is_regular(diriter->path()) ||
        fs::is_directory(diriter->path()) ||
        fs::status(diriter->path()).type() == fs::socket_file) {
      files_to_delete.push_back(diriter->path());
    }
    ++diriter;
  }
  // now delete all the files
  for (auto& p: files_to_delete) {
    // remove if possible. Ignore exceptions
    try {
      fs::remove(p);
      logstream(LOG_DEBUG) << "Deleting " << p << std::endl;
    } catch (...) {
      logstream(LOG_WARNING) << "Unable to delete " << p << std::endl;
    }
  }
  // delete the root
  // remove if possible. Ignore exceptions
  try {
    fs::remove(path);
    logstream(LOG_DEBUG) << "Deleting " << path << std::endl;
  } catch (...) { }
}

/**
 * we will store the temporary files in [tmp_directory]/turicreate/[procid]
 * This searches in turicreate's temp directory for unused temporary files
 * (what procids no longer exist) and deletes them.
 */
EXPORT void reap_unused_temp_files() {
  // loop through all the subdirectories in get_turicreate_temp_directory()
  // and unlink if the pid does not exist
  size_t temp_dir_size = num_temp_directories();
  for (size_t idx = 0; idx < temp_dir_size; ++idx) {
    try {
      fs::path temp_dir(get_turicreate_temp_directory(idx));
      auto diriter = fs::directory_iterator(temp_dir);
      auto enditer = fs::directory_iterator();

      while(diriter != enditer) {
        auto path = diriter->path();
        if (fs::is_directory(path)) {
          try {
            long pid = std::stol(path.filename().string());
            if(!is_process_running(pid)) {
              // PID no longer exists.
              // delete it
              logstream(LOG_EMPH) << "Deleting orphaned temp directory found in "
                << path.string() << std::endl;

              delete_proc_directory(path);
            }
          } catch (...) {
            // empty catch. if the path does not parse as an
            // integer, ignore it.
            logstream(LOG_WARNING)
                << "Unexpcted file in Turi's temp directory: " << path
                << std::endl;
          }
        }
        ++diriter;
      }
    } catch (...) {
      // Failures are ok. we just stop.
    }
  }
}


EXPORT std::string get_temp_name(const std::string& prefix, bool _prefer_hdfs) {
  std::lock_guard<mutex> lg(get_temp_info().lock);

  // Local system temp dir
  fs::path path(get_current_process_temp_directory(get_temp_info().temp_file_counter++));
  // hdfs temp dir
  fs::path hdfs_path(get_current_process_hdfs_temp_directory());
  if (_prefer_hdfs && !hdfs_path.empty()) {
    path = hdfs_path;
  }
  // create the directories if they do not exist
  create_current_process_temp_directory(path.string());
  
  if (prefix.empty()) {
    std::stringstream strm;
    strm << boost::lexical_cast<std::string>(get_temp_info().uuid_generator());
    path /= strm.str();
  } else {
    path /= prefix;
  }

  std::string ret = path.generic_string();
  // write the tempfile into the history
  get_temp_info().tempfile_history.insert(ret);

  return ret;
};

std::string get_temp_name_prefer_hdfs(const std::string& prefix) {
  bool prefer_hdfs = true;
  return get_temp_name(prefix, prefer_hdfs);
}

/**
 * Deletes the file with the name s
 */
bool delete_temp_file(std::string s) {
  std::lock_guard<mutex> lg(get_temp_info().lock);
  // search in the history of all tempfiles generated
  bool found = false;
  // I need to check lower_bound and lower_bound - 1
  auto iter = get_temp_info().tempfile_history.lower_bound(s);
  auto begin = get_temp_info().tempfile_history.begin();
  auto end = get_temp_info().tempfile_history.end();
  // check lower_bound see if it is a prefix
  if (iter != end &&
      boost::starts_with(s, *iter)) {
    found = true;
    get_temp_info().tempfile_history.erase(iter);
  }
  // check lower_bound - 1 see if it is a prefix
  else if (iter != begin) {
    iter--;
    if (boost::starts_with(s, *iter)) {
      found = true;
      get_temp_info().tempfile_history.erase(iter);
    }
  }

  if (found) {
    logstream(LOG_DEBUG) << "Deleting " << s << "\n";
    return fileio::delete_path(s);
  } else {
    return false;
  }
}



void delete_temp_files(std::vector<std::string> files) {
  std::lock_guard<mutex> lg(get_temp_info().lock);
  // search in the history of all tempfiles generated
  // and unlink all matching files
  std::set<std::string> found_prefixes;
  for(std::string file: files) {
    bool found = false;
    // I need to check lower_bound and lower_bound - 1
    auto iter = get_temp_info().tempfile_history.lower_bound(file);
    auto begin = get_temp_info().tempfile_history.begin();
    auto end = get_temp_info().tempfile_history.end();
    // check lower_bound see if it is a prefix
    if (iter != end && boost::starts_with(file, *iter)) {
      found = true;
      found_prefixes.insert(*iter);
    }
    // check lower_bound - 1 see if it is a prefix
    if (iter != begin) {
      iter--;
      if (boost::starts_with(file, *iter)) {
        found = true;
        found_prefixes.insert(*iter);
      }
    }
    if (found) {
      logstream(LOG_DEBUG) << "Deleting " << file << "\n";
      fileio::delete_path(file);
    }
  }
  // now to clear the found prefixes
  for(std::string prefix: found_prefixes) {
    get_temp_info().tempfile_history.erase(prefix);
  }
}

void reap_current_process_temp_files() {
  // remove all if possible. Ignore exceptions
  // We go straight to delete_path_impl here to avoid the reference counting
  // mechanism on temp filesr At this point we are just doing cleanup.
  for (auto fname: get_temp_info().tempfile_history) {
    turi::fileio::delete_path_impl(fname);
  }
  for (auto current_temp_dir: get_temp_info().process_temp_directories) {
    auto string_dir = current_temp_dir.string();
    try {
      if (fileio::get_protocol(string_dir) == "hdfs") {
        // we only reap hdfs if the directory is empty
        // since other people could be sharing this
        logstream(LOG_DEBUG) << "Non-recursive deletion of "
                             << current_temp_dir << std::endl;
        turi::fileio::delete_path_impl(string_dir);
      } else {
        logstream(LOG_DEBUG) << "Recursive deletion of "
                             << current_temp_dir << std::endl;
        turi::fileio::delete_path_recursive(string_dir);
      }
    } catch (...) { }
  }
}

} // namespace turi
