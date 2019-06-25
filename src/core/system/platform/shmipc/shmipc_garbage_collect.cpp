/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <fstream>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <core/storage/fileio/fileio_constants.hpp>
#include <process/process_util.hpp>
#include <boost/filesystem.hpp>

#if BOOST_VERSION <= 105600 && defined(_WIN32)
#define BOOST_INTERPROCESS_WIN32_PRIMITIVES_HPP
#include "boost_alt_winapi.h"
#endif
#include <boost/interprocess/shared_memory_object.hpp>
namespace turi {
namespace shmipc {

namespace fs = boost::filesystem;

struct raii_deleter {
  raii_deleter(const std::string& shmname,
               const std::string& tagfile)
      :m_shmname(shmname), m_tagfilename(tagfile) {
  }
  ~raii_deleter() {
    // failed deletion is ok
    try {
      boost::interprocess::shared_memory_object::remove(m_shmname.c_str());
    } catch (...) { }

    // failed deletion is ok
    try {
      fileio::delete_path(m_tagfilename);
    } catch (...) { }
  }
  std::string m_shmname;
  std::string m_tagfilename;
};

/**
 * Return [TMPDIR]/glshm_[userid]
 */
static fs::path shared_memory_tagfile_path() {
  auto temp = fileio::get_system_temp_directory();
  fs::path path = temp;
  path /= "glshm_" + get_system_user_name();
  if (!fs::is_directory(path)) fs::create_directories(path);
  return path;
}

void garbage_collect() {
  auto taglist =
      fileio::get_directory_listing(shared_memory_tagfile_path().generic_string());

  // enumerate all files in [TMPDIR]/glshm_[userid]
  // Each file contains a pid in it.
  for (auto& file: taglist) {
    auto shmname = fileio::get_filename(file.first);
    std::ifstream fin(file.first.c_str());
    size_t pid = 0;
    fin >> pid;
    if (pid != 0 && is_process_running(pid) == false) {
      // use the raii deleter's destructor to delete the shared memory and the
      // tag file
      raii_deleter(shmname, file.first);
    }
  }
}


std::shared_ptr<raii_deleter>
register_shared_memory_name(std::string m_name) {
  garbage_collect();

  // tag file is located as [system temp directory]/glshm_[userid]/[shmname]
  // tag file contains a PID in it.
  auto tagfile = (shared_memory_tagfile_path() / m_name).generic_string();
  std::ofstream fout(tagfile);
  fout << get_my_pid();
  fout.close();
  return std::make_shared<raii_deleter>(m_name, tagfile);
}

} // shmipc
} // turicreate
