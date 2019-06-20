/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/fileio/hdfs.hpp>
#include <map>
#include <memory>
#include <system_error>
#include <core/parallel/mutex.hpp>
#include <pthread.h>
#include <core/export.hpp>

namespace turi {

/**************************************************************************/
/*                                                                        */
/*              Static method to get a connection to hdfs                 */
/*                                                                        */
/**************************************************************************/

// This is an intentional leak of raw pointers,
// including the mutex, the hdfs object, and the map of hdfs objects
// because termination depends on the hdfs object to be around
// to properly cleanup temp files in the hdfs.
static mutex* mtx;
static pthread_once_t mtx_is_initialized = PTHREAD_ONCE_INIT;

static void init_mtx() { mtx = new mutex(); }

hdfs& EXPORT hdfs::get_hdfs() {
  pthread_once(&mtx_is_initialized, init_mtx);
  ASSERT_MSG(mtx != NULL, "mutex not initialized");
  std::lock_guard<mutex> guard(*mtx);
  static hdfs* fs = nullptr;
  if (fs == nullptr) fs = new hdfs;
  return *fs;
}

hdfs& EXPORT hdfs::get_hdfs(std::string host, size_t port) {
  pthread_once(&mtx_is_initialized, init_mtx);
  ASSERT_MSG(mtx != NULL, "mutex not initialized");
  std::lock_guard<mutex> guard(*mtx);
  // Same as above. The use of raw pointer for hdfs object is intentional.
  typedef std::map< std::pair<std::string, size_t>, hdfs* > fs_pool_type;
  static  fs_pool_type* fs_pool = new fs_pool_type();
  if (fs_pool->count({host, port}) == 0) {
    hdfs* ptr = new hdfs(host, port);
    (*fs_pool)[{host, port}] = ptr;
  }
  return *((*fs_pool)[{host, port}]);
}

#ifdef HAS_HADOOP
/**************************************************************************/
/*                                                                        */
/*                    Implentation of turi::hdfs                     */
/*                                                                        */
/**************************************************************************/
hdfs::hdfs(const std::string& host, tPort port) {
  logstream(LOG_INFO) << "Connecting to HDFS. Host: " << host << " Port: " << port << std::endl;
  filesystem =  hdfsConnect(host.c_str(), port);
  if (filesystem == NULL) {
    logstream(LOG_ERROR) << "Fail connecting to hdfs" << std::endl;
  }
}

std::vector<std::string> hdfs::list_files(const std::string& path) const {
  std::vector<std::pair<std::string, bool> > files = list_files_and_stat(path);
  std::vector<std::string> ret;
  for(auto& file : files) {
    ret.push_back(file.first);
  }
  return ret;
}


std::vector<std::pair<std::string, bool> > hdfs::list_files_and_stat(const std::string& path) const {
  ASSERT_TRUE(good());

  std::vector<std::pair<std::string, bool> > files;

  if (!is_directory(path)) {
    return files;
  }

  int num_files = 0;
  hdfsFileInfo* hdfs_file_list_ptr =
    hdfsListDirectory(filesystem, path.c_str(), &num_files);
  // copy the file list to the string array
  for(int i = 0; i < num_files; ++i)  {
    files.push_back(std::make_pair(std::string(hdfs_file_list_ptr[i].mName) ,
                                   hdfs_file_list_ptr[i].mKind == kObjectKindDirectory));
  }
  // free the file list pointer
  hdfsFreeFileInfo(hdfs_file_list_ptr, num_files);
  return files;
}

size_t hdfs::file_size(const std::string& path) const {
  ASSERT_TRUE(good());
  hdfsFileInfo *file_info = hdfsGetPathInfo(filesystem, path.c_str());
  if (file_info == NULL) return (size_t)(-1);
  size_t ret_file_size = file_info->mSize;
  hdfsFreeFileInfo(file_info, 1);
  return ret_file_size;
}

bool hdfs::path_exists(const std::string& path) const {
  ASSERT_TRUE(good());
  return hdfsExists(filesystem, path.c_str()) == 0;
}

bool hdfs::is_directory(const std::string& path) const {
  ASSERT_TRUE(good());
  hdfsFileInfo *file_info = hdfsGetPathInfo(filesystem, path.c_str());
  if (file_info == NULL) return false;
  bool ret = (file_info->mKind == kObjectKindDirectory);
  hdfsFreeFileInfo(file_info, 1);
  return ret;
}



bool hdfs::create_directories(const std::string& path) const {
  return hdfsCreateDirectory(filesystem, path.c_str()) == 0;
}

bool hdfs::chmod(const std::string& path, short mode) const {
  return hdfsChmod(filesystem, path.c_str(), mode) == 0;
}

bool hdfs::delete_file_recursive(const std::string& path) const {
  return hdfsDelete(filesystem, path.c_str(), 1) == 0;
}
/**************************************************************************/
/*                                                                        */
/*             Implementation of turi::hdfs::hdfs_device              */
/*                                                                        */
/**************************************************************************/
hdfs::hdfs_device::hdfs_device(const hdfs& hdfs_fs, const std::string& filename, const bool write) :
  filesystem(hdfs_fs.filesystem)  {
  if (!hdfs_fs.good()) {
    return;
  }
  // open the file
  const int flags = write? O_WRONLY : O_RDONLY;
  const int buffer_size = 0; // use default
  const short replication = 0; // use default
  const tSize block_size = 0; // use default;
  m_file_size = hdfs_fs.file_size(filename);
  file = hdfsOpenFile(filesystem, filename.c_str(), flags, buffer_size,
      replication, block_size);
  logstream(LOG_INFO) << "HDFS open " << filename << " write = " << write << std::endl;
  if (file == NULL) {
    logstream(LOG_ERROR) << "Fail opening file." << std::endl;
    log_and_throw_io_failure("Error opening file.");
  }
}

void hdfs::hdfs_device::close(std::ios_base::openmode mode) {
  if(file == NULL) return;
  if(file->type == HDFS_STREAMTYPE_OUTPUT && mode == std::ios_base::out) {
    const int flush_error = hdfsFlush(filesystem, file);
    if (flush_error != 0) {
      log_and_throw_io_failure("Error on flush.");
    };
    const int close_error = hdfsCloseFile(filesystem, file);
    file = NULL;
    if (close_error != 0) {
      log_and_throw_io_failure("Error on close.");
    };
  } else if (file->type == HDFS_STREAMTYPE_INPUT && mode == std::ios_base::in) {
    const int close_error = hdfsCloseFile(filesystem, file);
    if (close_error != 0) {
      log_and_throw_io_failure("Error on close.");
    };
    file = NULL;
  } else {
    return;
  }
}

std::streamsize hdfs::hdfs_device::read(char* strm_ptr, std::streamsize n) {
  std::streamsize ret = hdfsRead(filesystem, file, strm_ptr, n);
  if (ret == -1) {
    log_and_throw_io_failure("Read Error.");
  }
  return ret;
}

std::streamsize hdfs::hdfs_device::write(const char* strm_ptr, std::streamsize n) {
  std::streamsize ret = hdfsWrite(filesystem, file, strm_ptr, n);
  if (ret == -1) {
    log_and_throw_io_failure("Write Error.");
  }
  return ret;
}


std::streampos hdfs::hdfs_device::seek(std::streamoff off,
                                       std::ios_base::seekdir way,
                                       std::ios_base::openmode) {
  if (way == std::ios_base::beg) {
    hdfsSeek(filesystem, file, off);
  } else if (way == std::ios_base::cur) {
    tOffset offset = hdfsTell(filesystem, file);
    hdfsSeek(filesystem, file, offset + off);
  } else if (way == std::ios_base::end) {
    hdfsSeek(filesystem, file, m_file_size + off - 1);
  }
  return hdfsTell(filesystem, file);
}
#endif

}
