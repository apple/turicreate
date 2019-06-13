/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// libhdfs shim library
#include <core/globals/global_constants.hpp>
#include <core/logging/logger.hpp>
#include <core/logging/assertions.hpp>
#include <vector>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <core/parallel/execute_task_in_native_thread.hpp>
#include <type_traits>
#include <process/process.hpp>
#ifdef HAS_HADOOP
#ifndef _WIN32
#include <dlfcn.h>
#else
#include <cross_platform/windows_wrapper.hpp>
#include <core/util/syserr_reporting.hpp>
#endif
extern  "C" {
#include <hdfs.h>
}

namespace fs = boost::filesystem;

extern  "C" {
#ifndef _WIN32
  static void* libhdfs_handle = NULL;
  static void* libjvm_handle = NULL;
#else
  static HINSTANCE libhdfs_handle = NULL;
  static HINSTANCE libjvm_handle = NULL;
#endif
  static bool dlopen_fail = false;
  /*
   * All the shim pointers
   */
  static hdfsFS (*ptr_hdfsConnectAsUser)(const char* host, tPort port, const char *user) = NULL;
  static hdfsFS (*ptr_hdfsConnect)(const char* host, tPort port) = NULL;
  static int (*ptr_hdfsDisconnect)(hdfsFS fs) = NULL;
  static hdfsFile (*ptr_hdfsOpenFile)(hdfsFS fs, const char* path, int flags,
                        int bufferSize, short replication, tSize blocksize) = NULL;
  static int (*ptr_hdfsCloseFile)(hdfsFS fs, hdfsFile file) = NULL;
  static int (*ptr_hdfsExists)(hdfsFS fs, const char *path) = NULL;
  static int (*ptr_hdfsSeek)(hdfsFS fs, hdfsFile file, tOffset desiredPos) = NULL;
  static tOffset (*ptr_hdfsTell)(hdfsFS fs, hdfsFile file) = NULL;
  static tSize (*ptr_hdfsRead)(hdfsFS fs, hdfsFile file, void* buffer, tSize length) = NULL;
  static tSize (*ptr_hdfsPread)(hdfsFS fs, hdfsFile file, tOffset position,
                  void* buffer, tSize length) = NULL;
  static tSize (*ptr_hdfsWrite)(hdfsFS fs, hdfsFile file, const void* buffer,
                  tSize length) = NULL;
  static int (*ptr_hdfsFlush)(hdfsFS fs, hdfsFile file) = NULL;
  static int (*ptr_hdfsAvailable)(hdfsFS fs, hdfsFile file) = NULL;
  static int (*ptr_hdfsCopy)(hdfsFS srcFS, const char* src, hdfsFS dstFS, const char* dst) = NULL;
  static int (*ptr_hdfsMove)(hdfsFS srcFS, const char* src, hdfsFS dstFS, const char* dst) = NULL;
  static int (*ptr_hdfsDelete)(hdfsFS fs, const char* path, int recursive) = NULL;
  static int (*ptr_hdfsRename)(hdfsFS fs, const char* oldPath, const char* newPath) = NULL;
  static char* (*ptr_hdfsGetWorkingDirectory)(hdfsFS fs, char *buffer, size_t bufferSize) = NULL;
  static int (*ptr_hdfsSetWorkingDirectory)(hdfsFS fs, const char* path) = NULL;
  static int (*ptr_hdfsCreateDirectory)(hdfsFS fs, const char* path) = NULL;
  static int (*ptr_hdfsSetReplication)(hdfsFS fs, const char* path, int16_t replication) = NULL;
  static hdfsFileInfo* (*ptr_hdfsListDirectory)(hdfsFS fs, const char* path,
                                  int *numEntries) = NULL;
  static hdfsFileInfo* (*ptr_hdfsGetPathInfo)(hdfsFS fs, const char* path) = NULL;
  static void (*ptr_hdfsFreeFileInfo)(hdfsFileInfo *hdfsFileInfo, int numEntries) = NULL;
  static char*** (*ptr_hdfsGetHosts)(hdfsFS fs, const char* path,
                       tOffset start, tOffset length) = NULL;
  static void (*ptr_hdfsFreeHosts)(char ***blockHosts) = NULL;
  static tOffset (*ptr_hdfsGetDefaultBlockSize)(hdfsFS fs) = NULL;
  static tOffset (*ptr_hdfsGetCapacity)(hdfsFS fs) = NULL;
  static tOffset (*ptr_hdfsGetUsed)(hdfsFS fs) = NULL;
  static int (*ptr_hdfsChown)(hdfsFS fs, const char* path, const char *owner, const char *group) = NULL;
  static int (*ptr_hdfsChmod)(hdfsFS fs, const char* path, short mode) = NULL;
  static int (*ptr_hdfsUtime)(hdfsFS fs, const char* path, tTime mtime, tTime atime) = NULL;

  // Helper functions for dlopens
  static std::vector<fs::path> get_potential_libjvm_paths();
  static std::vector<fs::path> get_potential_libhdfs_paths();
  static void try_dlopen(std::vector<fs::path> potential_paths, const char* name,
#ifndef _WIN32
                         void*& out_handle);
#else
                         HINSTANCE& out_handle);
#endif

  static void connect_shim() {
    static bool shim_attempted = false;
    if (shim_attempted == false) {
      shim_attempted = true;

      std::vector<fs::path> libjvm_potential_paths = get_potential_libjvm_paths();
      try_dlopen(libjvm_potential_paths, "libjvm", libjvm_handle);

      std::vector<fs::path> libhdfs_potential_paths = get_potential_libhdfs_paths();
      try_dlopen(libhdfs_potential_paths, "libhdfs", libhdfs_handle);

      if(libhdfs_handle == NULL) {
        logstream(LOG_ERROR) << "Error loading libhdfs.  Please make sure the environment variable HADOOP_HOME_DIR is set properly, and that libhdfs.so, libhdfs.dylib, or hdfs.dll is found in one of $(HADOOP_HOME_DIR)/lib/native/, $(HADOOP_HOME_DIR)/lib/,$(HADOOP_HOME_DIR)/libhdfs/, or $(HADOOP_HOME_DIR)/.  Also, please make sure that CLASS_PATH is set to the output of `hadoop classpath --glob`, and JAVA_HOME is set correctly." << std::endl;
      }

      dlopen_fail = (libhdfs_handle == NULL);
    }
  }

  static void* get_symbol(const char* symbol) {
    connect_shim();
    if (dlopen_fail || libhdfs_handle == NULL) return NULL;
#ifndef _WIN32
    return dlsym(libhdfs_handle, symbol);
#else

    void *ret = (void *)GetProcAddress(libhdfs_handle, symbol);
    if(ret == NULL) {
      logstream(LOG_INFO) << "GetProcAddress error: " << get_last_err_str(GetLastError()) << std::endl;
    }
    return ret;
#endif
  }

  hdfsFS hdfsConnectAsUser(const char* host, tPort port, const char *user) {
    if (!ptr_hdfsConnectAsUser) *(void**)(&ptr_hdfsConnectAsUser) = get_symbol("hdfsConnectAsUser");
    if (ptr_hdfsConnectAsUser) return turi::run_as_native(ptr_hdfsConnectAsUser,host, port, user);
    else return NULL;
  }


  hdfsFS hdfsConnect(const char* host, tPort port) {
    if (!ptr_hdfsConnect) {
      *(void**)(&ptr_hdfsConnect) = get_symbol("hdfsConnect");
    }
    if (ptr_hdfsConnect) {
      auto x = turi::run_as_native(ptr_hdfsConnect, host, port);
      if (x == NULL) {
        logstream(LOG_ERROR) << "hdfsConnect to " << host << ":" << port << " Failed" << std::endl;
      }
      return x;
    } else {
      logstream(LOG_ERROR) << "hdfsConnect failed because the hdfsConnect symbol cannot be found" << std::endl;
      return NULL;
    }
  }


  int hdfsDisconnect(hdfsFS fs)  {
    if (!ptr_hdfsDisconnect) *(void**)(&ptr_hdfsDisconnect) = get_symbol("hdfsDisconnect");
    if (ptr_hdfsDisconnect) return turi::run_as_native(ptr_hdfsDisconnect, fs);
    else return 0;
  }

  hdfsFile hdfsOpenFile(hdfsFS fs, const char* path, int flags,
                        int bufferSize, short replication, tSize blocksize) {
    if (!ptr_hdfsOpenFile) *(void**)(&ptr_hdfsOpenFile) = get_symbol("hdfsOpenFile");
    if (ptr_hdfsOpenFile) return turi::run_as_native(ptr_hdfsOpenFile, fs, path, flags, bufferSize, replication, blocksize);
    else return NULL;
  }


  int hdfsCloseFile(hdfsFS fs, hdfsFile file) {
    if(!ptr_hdfsCloseFile) *(void**)(&ptr_hdfsCloseFile) = get_symbol("hdfsCloseFile");
    if (ptr_hdfsCloseFile) return turi::run_as_native(ptr_hdfsCloseFile, fs, file);
    else return 0;
  }


  int hdfsExists(hdfsFS fs, const char *path) {
    if(!ptr_hdfsExists) *(void**)(&ptr_hdfsExists) = get_symbol("hdfsExists");
    if (ptr_hdfsExists) return turi::run_as_native(ptr_hdfsExists, fs, path);
    else return 0;
  }


  int hdfsSeek(hdfsFS fs, hdfsFile file, tOffset desiredPos) {
    if(!ptr_hdfsSeek) *(void**)(&ptr_hdfsSeek) = get_symbol("hdfsSeek");
    if (ptr_hdfsSeek) return turi::run_as_native(ptr_hdfsSeek, fs, file, desiredPos);
    else return 0;
  }


  tOffset hdfsTell(hdfsFS fs, hdfsFile file) {
    if(!ptr_hdfsTell) *(void**)(&ptr_hdfsTell) = get_symbol("hdfsTell");
    if (ptr_hdfsTell) return turi::run_as_native(ptr_hdfsTell, fs, file);
    else return 0;
  }


  tSize hdfsRead(hdfsFS fs, hdfsFile file, void* buffer, tSize length) {
    if(!ptr_hdfsRead) *(void**)(&ptr_hdfsRead) = get_symbol("hdfsRead");
    if (ptr_hdfsRead) return turi::run_as_native(ptr_hdfsRead, fs, file, buffer, length);
    else return 0;
  }


  tSize hdfsPread(hdfsFS fs, hdfsFile file, tOffset position, void* buffer, tSize length) {
    if(!ptr_hdfsPread) *(void**)(&ptr_hdfsPread) = get_symbol("hdfsPread");
    if (ptr_hdfsPread) return turi::run_as_native(ptr_hdfsPread, fs, file, position, buffer, length);
    else return 0;
  }


  tSize hdfsWrite(hdfsFS fs, hdfsFile file, const void* buffer, tSize length) {
    if(!ptr_hdfsWrite) *(void**)(&ptr_hdfsWrite) = get_symbol("hdfsWrite");
    if (ptr_hdfsWrite) return turi::run_as_native(ptr_hdfsWrite, fs, file, buffer, length);
    else return 0;
  }


  int hdfsFlush(hdfsFS fs, hdfsFile file) {
    if(!ptr_hdfsFlush) *(void**)(&ptr_hdfsFlush) = get_symbol("hdfsFlush");
    if (ptr_hdfsFlush) return turi::run_as_native(ptr_hdfsFlush, fs, file);
    else return 0;
  }


  int hdfsAvailable(hdfsFS fs, hdfsFile file) {
    if(!ptr_hdfsAvailable) *(void**)(&ptr_hdfsAvailable) = get_symbol("hdfsAvailable");
    if (ptr_hdfsAvailable) return turi::run_as_native(ptr_hdfsAvailable, fs, file);
    else return 0;
  }


  int hdfsCopy(hdfsFS srcFS, const char* src, hdfsFS dstFS, const char* dst) {
    if(!ptr_hdfsCopy) *(void**)(&ptr_hdfsCopy) = get_symbol("hdfsCopy");
    if (ptr_hdfsCopy) return turi::run_as_native(ptr_hdfsCopy, srcFS, src, dstFS, dst);
    else return 0;
  }


  int hdfsMove(hdfsFS srcFS, const char* src, hdfsFS dstFS, const char* dst) {
    if(!ptr_hdfsMove) *(void**)(&ptr_hdfsMove) = get_symbol("hdfsMove");
    if (ptr_hdfsMove) return turi::run_as_native(ptr_hdfsMove, srcFS, src, dstFS, dst);
    else return 0;
  }


  int hdfsDelete(hdfsFS fs, const char* path, int recursive) {
    if(!ptr_hdfsDelete) *(void**)(&ptr_hdfsDelete) = get_symbol("hdfsDelete");
    if (ptr_hdfsDelete) return turi::run_as_native(ptr_hdfsDelete, fs, path, recursive);
    else return 0;
  }


  int hdfsRename(hdfsFS fs, const char* oldPath, const char* newPath) {
    if(!ptr_hdfsRename) *(void**)(&ptr_hdfsRename) = get_symbol("hdfsRename");
    if (ptr_hdfsRename) return turi::run_as_native(ptr_hdfsRename, fs, oldPath, newPath);
    else return 0;
  }


  char* hdfsGetWorkingDirectory(hdfsFS fs, char *buffer, size_t bufferSize) {
    if(!ptr_hdfsGetWorkingDirectory) *(void**)(&ptr_hdfsGetWorkingDirectory) = get_symbol("hdfsGetWorkingDirectory");
    if (ptr_hdfsGetWorkingDirectory) return turi::run_as_native(ptr_hdfsGetWorkingDirectory, fs, buffer, bufferSize);
    else return NULL;
  }


  int hdfsSetWorkingDirectory(hdfsFS fs, const char* path) {
    if(!ptr_hdfsSetWorkingDirectory) *(void**)(&ptr_hdfsSetWorkingDirectory) = get_symbol("hdfsSetWorkingDirectory");
    if (ptr_hdfsSetWorkingDirectory) return turi::run_as_native(ptr_hdfsSetWorkingDirectory, fs, path);
    else return 0;
  }


  int hdfsCreateDirectory(hdfsFS fs, const char* path) {
    if(!ptr_hdfsCreateDirectory) *(void**)(&ptr_hdfsCreateDirectory) = get_symbol("hdfsCreateDirectory");
    if (ptr_hdfsCreateDirectory) return turi::run_as_native(ptr_hdfsCreateDirectory, fs, path);
    else return 0;
  }


  int hdfsSetReplication(hdfsFS fs, const char* path, int16_t replication) {
    if(!ptr_hdfsSetReplication) *(void**)(&ptr_hdfsSetReplication) = get_symbol("hdfsSetReplication");
    if (ptr_hdfsSetReplication) return turi::run_as_native(ptr_hdfsSetReplication, fs, path, replication);
    else return 0;
  }



  hdfsFileInfo* hdfsListDirectory(hdfsFS fs, const char* path, int *numEntries) {
    if(!ptr_hdfsListDirectory) *(void**)(&ptr_hdfsListDirectory) = get_symbol("hdfsListDirectory");
    if (ptr_hdfsListDirectory) return turi::run_as_native(ptr_hdfsListDirectory, fs, path, numEntries);
    else return NULL;
  }


  hdfsFileInfo* hdfsGetPathInfo(hdfsFS fs, const char* path) {
    if(!ptr_hdfsGetPathInfo) *(void**)(&ptr_hdfsGetPathInfo) = get_symbol("hdfsGetPathInfo");
    if (ptr_hdfsGetPathInfo) return turi::run_as_native(ptr_hdfsGetPathInfo, fs, path);
    else return 0;
  }


  void hdfsFreeFileInfo(hdfsFileInfo *hdfsFileInfo, int numEntries) {
    if(!ptr_hdfsFreeFileInfo) *(void**)(&ptr_hdfsFreeFileInfo) = get_symbol("hdfsFreeFileInfo");
    if (ptr_hdfsFreeFileInfo) turi::run_as_native(ptr_hdfsFreeFileInfo, hdfsFileInfo, numEntries);
  }


  char*** hdfsGetHosts(hdfsFS fs, const char* path, tOffset start, tOffset length) {
    if(!ptr_hdfsGetHosts) *(void**)(&ptr_hdfsGetHosts) = get_symbol("hdfsGetHosts");
    if (ptr_hdfsGetHosts) return turi::run_as_native(ptr_hdfsGetHosts, fs, path, start, length);
    else return NULL;
  }


  void hdfsFreeHosts(char*** blockHosts) {
    if(!ptr_hdfsFreeHosts) *(void**)(&ptr_hdfsFreeHosts) = get_symbol("hdfsFreeHosts");
    if (ptr_hdfsFreeHosts) turi::run_as_native(ptr_hdfsFreeHosts, blockHosts);
  }


  tOffset hdfsGetDefaultBlockSize(hdfsFS fs) {
    if(!ptr_hdfsGetDefaultBlockSize) *(void**)(&ptr_hdfsGetDefaultBlockSize) = get_symbol("hdfsGetDefaultBlockSize");
    if (ptr_hdfsGetDefaultBlockSize) return turi::run_as_native(ptr_hdfsGetDefaultBlockSize, fs);
    else return 0;
  }


  tOffset hdfsGetCapacity(hdfsFS fs) {
    if(!ptr_hdfsGetCapacity) *(void**)(&ptr_hdfsGetCapacity) = get_symbol("hdfsGetCapacity");
    if (ptr_hdfsGetCapacity) return turi::run_as_native(ptr_hdfsGetCapacity, fs);
    else return 0;
  }


  tOffset hdfsGetUsed(hdfsFS fs) {
    if(!ptr_hdfsGetUsed) *(void**)(&ptr_hdfsGetUsed) = get_symbol("hdfsGetUsed");
    if (ptr_hdfsGetUsed) return turi::run_as_native(ptr_hdfsGetUsed, fs);
    else return 0;
  }

  int hdfsChown(hdfsFS fs, const char* path, const char *owner, const char *group) {
    if(!ptr_hdfsChown) *(void**)(&ptr_hdfsChown) = get_symbol("hdfsChown");
    if (ptr_hdfsChown) return turi::run_as_native(ptr_hdfsChown, fs, path, owner, group);
    else return 0;
  }

  int hdfsChmod(hdfsFS fs, const char* path, short mode) {
    if(!ptr_hdfsChmod) *(void**)(&ptr_hdfsChmod) = get_symbol("hdfsChmod");
    if (ptr_hdfsChmod) return turi::run_as_native(ptr_hdfsChmod, fs, path, mode);
    else return 0;
  }

  int hdfsUtime(hdfsFS fs, const char* path, tTime mtime, tTime atime) {
    if(!ptr_hdfsUtime) *(void**)(&ptr_hdfsUtime) = get_symbol("hdfsUtime");
    if (ptr_hdfsUtime) return turi::run_as_native(ptr_hdfsUtime, fs, path, mtime, atime);
    else return 0;
  }

  static std::string get_hadoop_home_dir() {
    static std::string hadoop_home = std::getenv("HADOOP_HOME_DIR");
    return hadoop_home;
  }

  static std::vector<fs::path> get_potential_libhdfs_paths() {
    static std::vector<fs::path> libhdfs_potential_paths = {

    // Search order:
    // find one in the unity_server directory
    // find one in the local directory
    // Internal build path location; special handling there.
    // Hadoop home dir
    // Global search paths scoured by libhdfs.

#ifdef __WIN32
      fs::path(turi::GLOBALS_MAIN_PROCESS_PATH + "/hdfs.dll"),
      fs::path("./hdfs.dll"),
      fs::path(turi::GLOBALS_MAIN_PROCESS_PATH +
               "/../../../../deps/local/bin/hdfs.dll"),
      fs::path(get_hadoop_home_dir() + "/lib/native/hdfs.dll"),
      fs::path(get_hadoop_home_dir() + "/lib/hdfs.dll"),
      fs::path(get_hadoop_home_dir() + "/libhdfs/hdfs.dll"),
      fs::path(get_hadoop_home_dir() + "/hdfs.dll"),
      fs::path("hdfs.dll"),
#else

      fs::path(turi::GLOBALS_MAIN_PROCESS_PATH + "/libhdfs.so"),
      fs::path("./libhdfs.so"),
      fs::path(turi::GLOBALS_MAIN_PROCESS_PATH +
               "/../../../../deps/local/lib/libhdfs.so"),
      fs::path(get_hadoop_home_dir() + "/lib/native/libhdfs.so"),
      fs::path(get_hadoop_home_dir() + "/lib/libhdfs.so"),
      fs::path(get_hadoop_home_dir() + "/libhdfs/libhdfs.so"),
      fs::path(get_hadoop_home_dir() + "/libhdfs.so"),
      fs::path("libhdfs.so"),

#if __APPLE__  // For apple, also add in the dylib versions; it may be either.
      fs::path(turi::GLOBALS_MAIN_PROCESS_PATH + "/libhdfs.dylib"),
      fs::path("./libhdfs.dylib"),
      fs::path(turi::GLOBALS_MAIN_PROCESS_PATH +
               "/../../../../deps/local/lib/libhdfs.dylib"),
      fs::path(get_hadoop_home_dir() + "/lib/native/libhdfs.dylib"),
      fs::path(get_hadoop_home_dir() + "/lib/libhdfs.dylib"),
      fs::path(get_hadoop_home_dir() + "/libhdfs/libhdfs.dylib"),
      fs::path(get_hadoop_home_dir() + "/libhdfs.dylib"),
      fs::path("libhdfs.dylib")
#endif // End if apple.

#endif
    };

    return libhdfs_potential_paths;
  }

  static std::vector<fs::path> get_potential_libjvm_paths() {
    std::vector<fs::path> libjvm_potential_paths;

    std::vector<fs::path> search_prefixes;
    std::vector<fs::path> search_suffixes;
    std::string file_name;

    // From heuristics
#ifdef __WIN32
    search_prefixes = {""};
    search_suffixes = {
      "/jre/bin/server",
      "/bin/server"
    };
    file_name = "jvm.dll";
#elif __APPLE__
    search_prefixes = {""};
    search_suffixes = {""};
    file_name = "libjvm.dylib";


    // Run /usr/libexec/java_home to get the libjvm location
    std::string libjvm_location = "";
    std::string java_home_cmd = "/usr/libexec/java_home";
    try {
      turi::process p;
      p.popen(java_home_cmd,
              std::vector<std::string>(),
              STDOUT_FILENO);
      libjvm_location = p.read_from_child();
      logstream(LOG_INFO) << "Obtain JAVA_HOME from " << java_home_cmd << ": " << libjvm_location << std::endl;
      boost::algorithm::trim(libjvm_location);
    } catch (...) {
      logstream(LOG_WARNING) << "Error running " << java_home_cmd << std::endl;
      libjvm_location = "";
    }


    if (!libjvm_location.empty()) {
      // Make this location to be searched first `/usr/libexec/java_home`/jre/lib/server/libjvm.dylib
      search_prefixes.insert(search_prefixes.begin(), libjvm_location);
      search_suffixes.insert(search_suffixes.begin(), "/jre/lib/server");
    }

    // Add following environment variables at the beginning of the search path
    // to search_prefixes: "TURI_JAVA_HOME", or "JAVA_HOME
    for (const char* env_name : {"TURI_JAVA_HOME", "JAVA_HOME"}) {
      std::string env_value;
      if(! (env_value = std::getenv(env_name)).empty()) {
        logstream(LOG_INFO) << "Found environment variable " << env_name << ": " << env_value << std::endl;
        search_prefixes.insert(search_prefixes.begin(), env_value);
      }
    }

#else
    search_prefixes = {
      "/usr/lib/jvm/default-java",               // ubuntu / debian distros
      "/usr/lib/jvm/java",                       // rhel6
      "/usr/lib/jvm",                            // centos6
      "/usr/lib64/jvm",                          // opensuse 13
      "/usr/local/lib/jvm/default-java",         // alt ubuntu / debian distros
      "/usr/local/lib/jvm/java",                 // alt rhel6
      "/usr/local/lib/jvm",                      // alt centos6
      "/usr/local/lib64/jvm",                    // alt opensuse 13
      "/usr/local/lib/jvm/java-9-openjdk-amd64", // alt ubuntu / debian distros
      "/usr/lib/jvm/java-9-openjdk-amd64",       // alt ubuntu / debian distros
      "/usr/local/lib/jvm/java-8-openjdk-amd64", // alt ubuntu / debian distros
      "/usr/lib/jvm/java-8-openjdk-amd64",       // alt ubuntu / debian distros
      "/usr/local/lib/jvm/java-7-openjdk-amd64", // alt ubuntu / debian distros
      "/usr/lib/jvm/java-7-openjdk-amd64",       // alt ubuntu / debian distros
      "/usr/local/lib/jvm/java-6-openjdk-amd64", // alt ubuntu / debian distros
      "/usr/lib/jvm/java-6-openjdk-amd64",       // alt ubuntu / debian distros
      "/usr/lib/jvm/java-12-oracle",             // alt ubuntu
      "/usr/lib/jvm/java-11-oracle",             // alt ubuntu
      "/usr/lib/jvm/java-10-oracle",             // alt ubuntu
      "/usr/lib/jvm/java-9-oracle",              // alt ubuntu
      "/usr/lib/jvm/java-8-oracle",              // alt ubuntu
      "/usr/lib/jvm/java-7-oracle",              // alt ubuntu
      "/usr/lib/jvm/java-6-oracle",              // alt ubuntu
      "/usr/local/lib/jvm/java-12-oracle",       // alt ubuntu
      "/usr/local/lib/jvm/java-11-oracle",       // alt ubuntu
      "/usr/local/lib/jvm/java-10-oracle",       // alt ubuntu
      "/usr/local/lib/jvm/java-9-oracle",        // alt ubuntu
      "/usr/local/lib/jvm/java-8-oracle",        // alt ubuntu
      "/usr/local/lib/jvm/java-7-oracle",        // alt ubuntu
      "/usr/local/lib/jvm/java-6-oracle",        // alt ubuntu
      "/usr/lib/jvm/default",                    // alt centos
      "/usr/java/latest",                        // alt centos
    };
    search_suffixes = {
      "/jre/lib/amd64/server"
    };
    file_name = "libjvm.so";
#endif
    // From direct enviornment variable
    char* env_value = NULL;
    if ((env_value = getenv("TURI_LIBJVM_DIRECTORY")) != NULL) {
      logstream(LOG_INFO) << "Found environment variable TURI_LIBJVM_DIRECTORY: " << env_value << std::endl;
      libjvm_potential_paths.push_back(fs::path(env_value) / fs::path(file_name));
      libjvm_potential_paths.push_back(fs::path(env_value));
    }

    // Add following environment variables to search_prefixes: "TURI_JAVA_HOME", or "JAVA_HOME"
    for (const auto& env_name : {"TURI_JAVA_HOME", "JAVA_HOME"}) {
      if ((env_value = getenv(env_name)) != NULL) {
        logstream(LOG_INFO) << "Found environment variable " << env_name << ": " << env_value << std::endl;
        search_prefixes.insert(search_prefixes.begin(), env_value);
      }
    }

    // Generate cross product between search_prefixes, search_suffixes, and file_name
    for (auto& prefix: search_prefixes) {
      for (auto& suffix: search_suffixes) {
        auto path = (fs::path(prefix) / fs::path(suffix) / fs::path(file_name));
        libjvm_potential_paths.push_back(path);
      }
    }

    return libjvm_potential_paths;
  }

  static void try_dlopen(std::vector<fs::path> potential_paths, const char* name,
#ifndef _WIN32
                         void*& out_handle)
#else
                         HINSTANCE& out_handle)
#endif
  {
    // We don't want to freak customers out with failure messages as we try
    // to load these libraries.  There's going to be a few in the normal
    // case. Print them if we really didn't find libhdfs.so.
    std::vector<std::string> error_messages;

    for(auto &i : potential_paths) {
      i.make_preferred();
      logstream(LOG_INFO) << "Trying " << i.string().c_str() << std::endl;
#ifndef _WIN32
      out_handle = dlopen(i.native().c_str(), RTLD_NOW | RTLD_LOCAL);
#else
      out_handle = LoadLibrary(i.string().c_str());
#endif

      if(out_handle != NULL) {
        logstream(LOG_INFO) << "Success!" << std::endl;
        break;
      } else {
#ifndef _WIN32
        const char *err_msg = dlerror();
        if(err_msg != NULL) {
          error_messages.push_back(std::string(err_msg));
        } else {
          error_messages.push_back(std::string(" returned NULL"));
        }
#else
        error_messages.push_back(get_last_err_str(GetLastError()));
#endif
      }
    }

    if (out_handle == NULL) {
      logstream(LOG_INFO) << "Unable to load " << name << std::endl;
      for(size_t i = 0; i < potential_paths.size(); ++i) {
        logstream(LOG_INFO) << error_messages[i] << std::endl;
      }
    }
  }


}

#endif
