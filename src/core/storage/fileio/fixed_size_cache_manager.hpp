/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FILEIO_FIXED_SIZE_CACHE_MANAGER_HPP
#define FILEIO_FIXED_SIZE_CACHE_MANAGER_HPP

#include <vector>
#include <string>
#include <core/parallel/pthread_tools.hpp>
#include <unordered_map>
#include <core/parallel/atomic.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <core/storage/fileio/general_fstream_sink.hpp>
#include <core/storage/fileio/fileio_constants.hpp>

namespace turi {
namespace fileio {

//forward declaration
class fixed_size_cache_manager;

typedef std::string cache_id_type;
/**
 * \ingroup fileio
 *
 * A struct that contains either an array buffer or a file resource.
 * This struct cannot be created by anything else other than the
 * fixed_size_cache_manager.
 *
 * This struct is not generally thread safe.
 */
struct cache_block {
 private:

  // Construct an in-memory cache block
  cache_block(cache_id_type cache_id, size_t max_capacity,
              fixed_size_cache_manager* owning_cache_manager);

 public:

  cache_block(const cache_block&) = delete;
  cache_block& operator=(const cache_block&) = delete;

  /**
   * If this points to an in-memory cache, attempts to extend the in-memory
   * cache have capacity at least new_capacity. Returns true on success,
   * false on failure. If false, this means that the cache block size has
   * reached the maximum capacity permitted.
   */
  bool extend_capacity(size_t new_capacity);

  inline cache_id_type get_cache_id() const {
    return cache_id;
  }

  /**
   * Returns true if this points to an in memory cache.
   */
  inline bool is_pointer() const {
    return filename.empty();
  }

  /**
   * Returns true if this points to a file
   */
  inline bool is_file() const {
    return !filename.empty();
  }

  /**
   * Returns the pointer to the in memory cache.
   */
  inline char* get_pointer() const {
    return data;
  }

  /**
   * Returns the total capacity of the in memory cache
   */
  inline const size_t get_pointer_capacity() const {
    return capacity;
  }

  /**
   * Returns the used capacity of the in memory cache
   */
  inline const size_t get_pointer_size() const {
    return size;
  }

  /**
   * Returns the disk backed filename.
   */
  inline const std::string& get_filename() const {
    return filename;
  }

  /**
   * If this is an in memory cache, writes bufsize bytes to it. Returns true
   * on success, false on failure.
   */
  inline bool write_bytes_to_memory_cache(const char* c,
                                          std::streamsize bufsize) {
    if (data == NULL) return false;
    // either we have enough capacity
    // or we are able to extend enough capacity to write it
    if (size + bufsize <= capacity || extend_capacity(size + bufsize)) {
      memcpy(data + size, c, bufsize);
      size += bufsize;
      return true;
    } else {
      return false;
    }
  }

  /**
   * If this cache block is an in memory cache, dumps it to a file returning
   * the output file handle. Fails if the cache block is not an in memory cache.
   * Thread safe.
   */
  std::shared_ptr<fileio_impl::general_fstream_sink> write_to_file();

  /**
   * Destructor. Clears all the memory in the cache block.
   */
  ~cache_block();

 private:
  // id of the block
  cache_id_type cache_id = 0;
  // maximum capacity we are allows to resize to
  size_t maximum_capacity = 0;
  // current capacity of the data in memory
  size_t capacity = 0;
  // actual content size
  size_t size = 0;
  // begin of the data in memory
  char* data = NULL;
  // name of the file on disk
  std::string filename;
  // the cache manager which created this block
  fixed_size_cache_manager* owning_cache_manager = NULL;

  /**
   * Clears, and reinitializes the cache block with a new maximum capacity.
   */
  void initialize_memory(size_t max_capacity);
  /**
   * If this points to an in memory cache, release the memory in it.
   */
  void release_memory();

  /**
   * If this points to an in memory cache, release the memory.
   * If this points to a file, delete it.
   */
  void clear();

  friend class fixed_size_cache_manager;
};

/**
 * \ingroup fileio
 *
 * A global singleton object managing the allocation/deallocation
 * of cache blocks. The basic mechanism of operation is such:
 *
 *  - For every new cache block requested:
 *    - If there is FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE free bytes,
 *      a new cache block of FILEIO_INITIAL_CAPACITY_PER_FILE is allocated, where
 *      the new cache block is permitted to grow up to
 *      FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE. The capacity is not charged as utilization
 *      until it is actually used. i.e. utilization is only incremented by
 *      FILEIO_INITIAL_CACHE_CAPACITY_PER_FILE. Then as more memory is allocated for the
 *      cache, then utilization is incremented again.
 *    - If there is < FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE free bytes available:
 *      The largest cache block is evicted. If there is
 *      FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE free bytes, Goto the first case.
 *      Otherwise, create a new cache block with all the remaining free bytes.
 *
 *  The relevant constants are thus:
 *   FILEIO_MAXIMUM_CACHE_CAPACITY : the maximum total size of all cache blocks
 *   FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE : the maximum size of each cache blocks
 *   FILEIO_INITIAL_CAPACITY_PER_FILE : the initial size of each cache blocks
 *
 *  Overcommit Behavior
 *  -------------------
 *  We try our best to maintain cache utilization below the maximum. However,
 *  it is possible to exceed maximum cache utilization under certain race
 *  conditions since we avoid locking on the cache utilization counter.
 */
class fixed_size_cache_manager {

 public:

  static fixed_size_cache_manager& get_instance();

  /**
   * Returns a temporary cache id that is not yet used by the
   * manager.
   */
  cache_id_type get_temp_cache_id(std::string suffix = "");

  /**
   * Allocate a new cache block of up to some maximum capcity.
   * If the cache_id already exists, the data will be lost.
   *
   * Returns a pointer to the cache block.
   * Thread safe.
   */
  std::shared_ptr<cache_block> new_cache(cache_id_type cache_id);

  /**
   * Returns the pointer to the cache_block assocaited with the cache_id,
   * Throws std::out_of_range if the cache_id does not exist.
   *
   * Thread safe.
   */
  std::shared_ptr<cache_block> get_cache(cache_id_type cache_id);

  /**
   * Free the data in the cache block. Delete the allocated memory or temp file
   * associated with the cache.
   *
   * NOT thread safe to call on the same block.
   */
  void free(std::shared_ptr<cache_block> block);

  /**
   * Clear all cache blocks in the manager. Reset to initial state.
   */
  void clear();

  /**
   * Returns the amount of memory being used by the caches.
   */
  inline size_t get_cache_utilization() {
    return current_cache_utilization.value;
  }

 private:
  fixed_size_cache_manager();

 public:
  ~fixed_size_cache_manager();

 private:
  fixed_size_cache_manager(const fixed_size_cache_manager& other) = delete;

  fixed_size_cache_manager& operator=(const fixed_size_cache_manager& other) = delete;

 private:
  size_t temp_cache_counter = 0;

  atomic<size_t> current_cache_utilization;

  turi::mutex mutex;
  std::unordered_map<std::string, std::shared_ptr<cache_block> > cache_blocks;

  /**
   * Increments cache utilization counter
   */
  void increment_utilization(ssize_t increment);
  /**
   * Decrements cache utilization counter
   */
  void decrement_utilization(ssize_t decrement);

  /**
   * Tries to evict some stuff out of cache.
   * Lock must be acquired when this function is called.
   */
  void try_cache_evict();

  friend struct cache_block;
};

} // end of fileio
} // end of turicreate
#endif
