/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/fileio/block_cache.hpp>
#include <core/storage/fileio/fs_utils.hpp>
#include <core/storage/fileio/general_fstream.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <core/logging/logger.hpp>
#include <core/logging/assertions.hpp>
#include <core/util/md5.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/parallel/atomic.hpp>
#include <mutex>
namespace turi {
constexpr size_t block_cache::KEY_LOCK_SIZE;


void block_cache::init(const std::string& storage_prefix,
                       size_t max_file_handle_cache) {
  if (m_initialized) log_and_throw("Multiple initialization of block_cache");
  m_storage_prefix = storage_prefix;
  m_cache.set_size_limit(max_file_handle_cache);
  m_initialized = true;
}

bool block_cache::write(const std::string& key, const std::string& value) {
  ASSERT_TRUE(m_initialized);
  auto strhash = md5(key);
  auto locknum = hash64(key) % KEY_LOCK_SIZE;
  auto filename = m_storage_prefix + strhash;

  std::unique_lock<mutex> lock(key_lock[locknum]);
  try {

    // if file already exists fail
    if (fileio::get_file_status(filename).first != fileio::file_status::MISSING) {
      return false;
    }

    general_ofstream fout(filename);
    // cannot open file. fail.
    if (!fout.good()) return false;
    fout.write(value.c_str(), value.length());
    // cannot write. fail
    if (!fout.good()) return false;
    fout.close();

    std::string to_evict;
    {
      std::unique_lock<mutex> global_lock(m_lock);
      m_created_files.insert(filename);
      m_lru_files.insert(filename, true);
      if (m_max_capacity > 0 && m_created_files.size() > m_max_capacity) {
        auto iter = m_lru_files.rbegin();
        if (iter != m_lru_files.rend()) {
          to_evict = iter->first;
        }
      }
    }
    if (to_evict.length() > 0) {
      logstream(LOG_INFO) << "Evicting " << to_evict << std::endl;
      evict_key(to_evict);
    }
    return true;
  } catch (...) {
    // all exceptions result in failure.
    return false;
  }
}

int64_t block_cache::value_length(const std::string& key) {
  ASSERT_TRUE(m_initialized);
  auto strhash = md5(key);
  auto locknum = hash64(key) % KEY_LOCK_SIZE;
  auto filename = m_storage_prefix + strhash;

  std::unique_lock<mutex> lock(key_lock[locknum]);
  try {
    general_ifstream fin(filename);
    if (!fin.good()) return -1;
    return fin.file_size();

  } catch (...) {
    return -1;
  }
}

int64_t block_cache::read(const std::string& key,
                          std::string& output,
                          size_t start,
                          size_t end) {
  // we need to get the end size if we do not know it so that
  // we have resize the output string.
  if (end == (size_t)(-1)) end = value_length(key);
  if (end == (size_t)(-1)) return -1;
  size_t length = end > start ? end - start : 0 ;
  output.resize(length);
  return read(key, &(output[0]), start, end);
}


int64_t block_cache::read(const std::string& key,
                          char* output,
                          size_t start,
                          size_t end) {
  static atomic<size_t> ctr;
  if (ctr.inc() % 4096 == 0) {
    logstream(LOG_INFO) << "Block Cache Hits: " << file_handle_cache_hits()
                        << " Misses: " << file_handle_cache_misses() << std::endl;
  }
  ASSERT_TRUE(m_initialized);

  auto strhash = md5(key);
  auto locknum = hash64(key) % KEY_LOCK_SIZE;
  auto filename = m_storage_prefix + strhash;

  // no one should be hitting the same filename
  std::unique_lock<mutex> lock(key_lock[locknum]);

  std::shared_ptr<general_ifstream> read_stream;

  // see if we already have a file handle in the cache
  bool from_cache = false;
  {
    std::unique_lock<mutex> global_lock(m_lock);
    m_lru_files.query(filename);
    auto cache_entry = m_cache.query(filename);
    if (cache_entry.first) {
      read_stream = cache_entry.second;
      from_cache = true;
    }
  }
  // not in cache. open it
  if (read_stream == nullptr) {
    try {
      if (fileio::get_file_status(filename).first == fileio::file_status::REGULAR_FILE) {
        read_stream = std::make_shared<general_ifstream>(filename);
      }
    } catch (...) {
      return -1;
    }
  }
  if (read_stream == nullptr || read_stream->good() == false) return -1;

  // fix up the start and end positions
  if (end == (size_t)(-1)) end = read_stream->file_size();
  int64_t retval = 0;
  size_t length = end > start ? end - start : 0 ;

  read_stream->clear();
  read_stream->seekg(start);
  if (read_stream->good() && length > 0) {
    read_stream->read(&(output[0]), length);
    retval = read_stream->gcount();
  }

  if (!read_stream->good()) {
    retval = -1;
    // ok. stream is bad and it came from the cache. We need to delete
    // it from the cache so no one uses it again.
    if (from_cache) {
      std::unique_lock<mutex> global_lock(m_lock);
      m_cache.erase(filename);
      read_stream.reset();
    }
  }

  // If we created a new stream and did not come from cache,
  // put it into the cache.
  if (read_stream && !from_cache) {
    std::unique_lock<mutex> global_lock(m_lock);
    m_cache.insert(filename, read_stream);
  }
  return retval;
}

bool block_cache::evict_key(const std::string& key) {
  auto strhash = md5(key);
  auto locknum = hash64(key) % KEY_LOCK_SIZE;
  auto filename = m_storage_prefix + strhash;
  // no one should be hitting the same filename
  std::unique_lock<mutex> lock(key_lock[locknum]);
  // acquire global lock since we need to touch the lru cache and created files
  std::unique_lock<mutex> global_lock(m_lock);
  m_cache.erase(filename);             // file handle
  m_created_files.erase(filename);     // created file list
  m_lru_files.erase(filename);
  return fileio::delete_path(filename); // actual file
}

size_t block_cache::file_handle_cache_hits() const {
  return m_cache.hits();
}

size_t block_cache::file_handle_cache_misses() const {
  return m_cache.misses();
}

size_t block_cache::get_max_capacity() {
  return m_max_capacity;
}
void block_cache::set_max_capacity(size_t mc) {
  m_max_capacity = mc;
}

block_cache::~block_cache() {
  if (m_initialized) {
    for (const auto& f : m_created_files) {
      // delete every file we created. ignore failures
      try {
        fileio::delete_path(f);
      } catch (...) { }
    }
  }
}

static std::once_flag block_cache_is_initialized;
static std::shared_ptr<block_cache> bc;

block_cache& block_cache::get_instance() {
  std::call_once(block_cache_is_initialized,
                 []() {
                   bc = std::make_shared<block_cache>();
                   auto temp_name = get_temp_name_prefer_hdfs("block_caches-");
                   // if temporary storage is on hdfs, we use it to share
                   // data across processes. Otherwise, stick it into cache://
                   fileio::delete_path(temp_name);
                   if (fileio::get_protocol(temp_name) == "hdfs") {
                     logstream(LOG_INFO) << "Storing S3 Block Caches on HDFS" << std::endl;
                     bc->init(temp_name, 4*thread::cpu_count());
                   } else {
                     logstream(LOG_INFO) << "Storing S3 Block Caches in memory cache" << std::endl;
                     bc->init("cache://block_caches-", 4*thread::cpu_count());
                   }
                 });
  return *bc;
}

void block_cache::release_instance() {
  bc.reset();
}

} // namespace turi
