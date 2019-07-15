/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstdlib>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <core/storage/fileio/fileio_constants.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <core/storage/fileio/block_cache.hpp>
#include <core/globals/globals.hpp>
#include <core/random/random.hpp>
#ifdef TC_ENABLE_REMOTEFS
#include <core/storage/fileio/hdfs.hpp>
#endif
#include <iostream>
#include <core/export.hpp>

#ifdef __APPLE__
#include <core/system/platform/config/apple_config.hpp>
#endif

namespace fs = boost::filesystem;

namespace turi {
namespace fileio {


/**
 * Finds the system temp directory.
 *
 * Really, we should be using $TMPDIR or /tmp. But Fedora 18 figured that
 * /tmp should be on tmpfs and thus should only hold small files. Thus we
 * should use /var/tmp when available. But that means we are not following
 * best practices and using $TMPDIR. so.... aargh.
 *
 * This will emit one of the following in order of preference. It will return
 * the first directory which exists. exit(1) on failure.:
 *  - NSTemporaryDirectory() -- APPLE only.
 *  - /var/tmp
 *  - $TMPDIR
 *  - /tmp
 */
std::string get_system_temp_directory() {
#ifdef __APPLE__
  // Proper thing to do is use the temporary directory determined by
  // the CF runtime
  return config::get_apple_system_temporary_directory();

#else

  boost::filesystem::path path;
#ifndef _WIN32
  char* tmpdir = getenv("TMPDIR");
#else
  char* tmpdir = getenv("TMP");
#endif
  // try $TURI_TMPDIR first
  if (boost::filesystem::is_directory("/var/tmp")) {
    path = "/var/tmp";
  } else if(tmpdir && boost::filesystem::is_directory(tmpdir)) {
    path = tmpdir;
  } else {
    if (boost::filesystem::is_directory("/tmp")) {
      path = "/tmp";
    }
  }
  return path.string();
#endif
}

static bool check_cache_file_location(std::string val) {
  boost::algorithm::trim(val);
  std::vector<std::string> paths;
  boost::algorithm::split(paths,
      val,
#ifndef _WIN32
      boost::algorithm::is_any_of(":"));
#else
      boost::algorithm::is_any_of(";"));
#endif
  if (paths.size() == 0)
    throw std::string("Value cannot be empty");
  for (std::string path: paths) {
    if (!boost::filesystem::is_directory(path))
      throw std::string("Directory: ") + path + " does not exist";
  }
  return true;
}

#ifdef TC_ENABLE_REMOTEFS
static bool check_cache_file_hdfs_location(std::string val) {
  if (get_protocol(val) == "hdfs") {
#ifdef TC_BUILD_IOS
    log_and_throw("hdfs:// URLs not supported.");
#else
    if (get_file_status(val) == file_status::DIRECTORY) {
      // test hdfs write permission by createing a test directory
      namespace fs = boost::filesystem;
      std::string host, port, hdfspath;
      std::tie(host, port, hdfspath) = parse_hdfs_url(val);
      auto& hdfs = turi::hdfs::get_hdfs(host, std::stoi(port));
      fs::path temp_dir (hdfspath);
      temp_dir /= std::string("test-") + std::to_string(random::rand());
      bool success = hdfs.create_directories(temp_dir.string());
      if (!success) {
        throw std::string("Cannot write to ") + val;
      }
      hdfs.delete_file_recursive(temp_dir.string());
      return true;
    } else {
      throw std::string("Directory: ") + val + " does not exist";
    }
#endif
  }
  throw std::string("Invalid hdfs path: ") + val;
}
#endif

EXPORT const size_t FILEIO_INITIAL_CAPACITY_PER_FILE = 1024;
EXPORT size_t FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE = 128 * 1024 * 1024;
EXPORT size_t FILEIO_MAXIMUM_CACHE_CAPACITY = 2LL * 1024 * 1024 * 1024;
EXPORT size_t FILEIO_READER_BUFFER_SIZE = 16 * 1024;
EXPORT size_t FILEIO_WRITER_BUFFER_SIZE = 96 * 1024;
EXPORT std::string S3_ENDPOINT;
EXPORT std::string S3_REGION;
// TODO: Where is the right place for this? Probably not here...
EXPORT int64_t NUM_GPUS = -1;

REGISTER_GLOBAL(int64_t, FILEIO_MAXIMUM_CACHE_CAPACITY, true);
REGISTER_GLOBAL(int64_t, FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE, true)
REGISTER_GLOBAL(int64_t, FILEIO_READER_BUFFER_SIZE, false);
REGISTER_GLOBAL(int64_t, FILEIO_WRITER_BUFFER_SIZE, false);
REGISTER_GLOBAL(std::string, S3_ENDPOINT, true);
REGISTER_GLOBAL(std::string, S3_REGION, true);
REGISTER_GLOBAL(int64_t, NUM_GPUS, true);


static constexpr char CACHE_PREFIX[] = "cache://";
static constexpr char TMP_CACHE_PREFIX[] = "cache://tmp/";
std::string get_cache_prefix() { return CACHE_PREFIX; }
std::string get_temp_cache_prefix() { return TMP_CACHE_PREFIX; }


EXPORT std::string CACHE_FILE_LOCATIONS = "CHANGEME";
EXPORT std::string CACHE_FILE_HDFS_LOCATION = "";

REGISTER_GLOBAL_WITH_CHECKS(std::string,
                            CACHE_FILE_LOCATIONS,
                            true,
                            check_cache_file_location);


#ifdef TC_ENABLE_REMOTEFS
REGISTER_GLOBAL_WITH_CHECKS(std::string,
                            CACHE_FILE_HDFS_LOCATION,
                            true,
                            check_cache_file_hdfs_location);
#endif
std::string get_cache_file_locations() {
  return CACHE_FILE_LOCATIONS;
}

void set_cache_file_locations(std::string value) {
  CACHE_FILE_LOCATIONS = value;
}

std::string get_cache_file_hdfs_location() {
  return CACHE_FILE_HDFS_LOCATION;
}

// Default SSL location for RHEL and FEDORA
#ifdef __linux__
EXPORT std::string FILEIO_ALTERNATIVE_SSL_CERT_DIR = "/etc/pki/tls/certs";
EXPORT std::string FILEIO_ALTERNATIVE_SSL_CERT_FILE = "/etc/pki/tls/certs/ca-bundle.crt";
#else
EXPORT std::string FILEIO_ALTERNATIVE_SSL_CERT_DIR = "";
EXPORT std::string FILEIO_ALTERNATIVE_SSL_CERT_FILE = "";
#endif
EXPORT int64_t FILEIO_INSECURE_SSL_CERTIFICATE_CHECKS = 0;
REGISTER_GLOBAL(std::string, FILEIO_ALTERNATIVE_SSL_CERT_FILE, true);
REGISTER_GLOBAL(std::string, FILEIO_ALTERNATIVE_SSL_CERT_DIR, true);
REGISTER_GLOBAL(int64_t, FILEIO_INSECURE_SSL_CERTIFICATE_CHECKS, true);

const std::string& get_alternative_ssl_cert_dir() {
  return FILEIO_ALTERNATIVE_SSL_CERT_DIR;
}

const std::string& get_alternative_ssl_cert_file() {
  return FILEIO_ALTERNATIVE_SSL_CERT_FILE;
}

const bool insecure_ssl_cert_checks() {
  return FILEIO_INSECURE_SSL_CERTIFICATE_CHECKS != 0;
}


static bool set_max_remote_fs_cache_entries(int64_t val) {
  if (val < 0) return false;
  block_cache::get_instance().set_max_capacity((size_t)(val));
  return true;
}

EXPORT size_t FILEIO_MAX_REMOTE_FS_CACHE_ENTRIES = 0;
REGISTER_GLOBAL_WITH_CHECKS(int64_t,
                            FILEIO_MAX_REMOTE_FS_CACHE_ENTRIES,
                            true,
                            set_max_remote_fs_cache_entries);
}

}
