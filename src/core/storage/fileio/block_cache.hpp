/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FILEIO_BLOCK_CACHE_HPP
#define TURI_FILEIO_BLOCK_CACHE_HPP
#include <cstdint>
#include <core/util/lru.hpp>
#include <core/parallel/mutex.hpp>

namespace turi {
class general_ifstream;

/**
 * \ingroup fileio
 *
 * The block cache implements a simple key-value store for extremely large
 * values (~16MB at least). Every key can only be written to exactly once,
 * and allows for arbitrary range reads (i.e. read byte X to byte Y of this key)
 *
 * Essentially every value is stored as a single file inside the
 * storage_prefix parameter set at \ref init.
 *
 * The block_cache is safe for concurrent use.
 *
 * Use On a Distributed File System
 * --------------------------------
 *
 * The storage prefix be located on a distributed filesystem
 * (for instance HDFS or NFS). In which case, *every* machine sharing the same
 * storage prefix also shares keys.
 *
 * When sharing a storage prefix with other processes on a distributed
 * filesystem, the atomicity guarantees of the filesystem becomes important.
 *
 * In particular, on HDFS, you may find keys in a "indeterminate" state, where
 * it cannot be written to, but cannot be queried (because the writer has
 * created the file but has not finished writing to it yet).  On NFS multiple
 * machines may be able to write to the same key, but only one will win. Also
 * the length and contents of the key may be wrong if you read the key
 * while someone else is writing to it.
 *
 * Design Notes
 * ------------
 * We will like these "interesting" distributed file system properties to not
 * be true when the block_cache is merely used concurrently. So a bit of care
 * is needed to ensure atomicity, at least within the context of the same
 * block_cache object. Essentially we want write-once, but arbitrary parallel
 * reads semantics.
 */
class block_cache {
 public:
  /**
   * Constructs the block cache. init must be called before the block_cache
   * can be used.
   */
  block_cache() = default;

  /// Default destructor. Deletes all associated files.
  ~block_cache();

  // copying is disabled
  block_cache(const block_cache&) = delete;
  block_cache& operator=(const block_cache&) = delete;
  // but moves are ok.
  block_cache(block_cache&&) = default;
  block_cache& operator=(block_cache&&) = default;

  /**
   * init must be called exactly once on block cache construction before the
   * block cache can be used. Multiple calls to init will raise an exception.
   *
   * \param storage_prefix The location where all values are stored
   * \param max_file_handle_cache The maximum number of file handles to cache
   *
   * Essentially, every value is stored as a separate file inside the directory.
   */
  void init(const std::string& storage_prefix,
            size_t max_file_handle_cache=16);

  /**
   * Writes a string to a key. Returns true on success. The key must not
   * already exist. If the key already exists this fails and false is returned.
   * When operating on a distributed filesystem, note that every machine
   * sharing the same storage prefix have a common key space.
   *
   * \param key The key to write to
   * \param value The value to write
   *
   * \returns true on success, false on failure
   */
  bool write(const std::string& key, const std::string& value);

  /**
   * Evicts a particular key. Returns true on success, false on failure
   *
   * \param key The key name
   */
  bool evict_key(const std::string& key);

  /**
   * Returns the length of the value of a particular key.
   *
   * \param key The key to query
   *
   * \returns the length of the value on success, a value < 0 on failure.
   */
  int64_t value_length(const std::string& key);

  /**
   * Reads the value of a key into an output string, resizing the output string
   * if necessary; Returns the number of bytes read.
   *
   * \param key The key to read
   * \param output A reference to the output string
   * \param start Optional, denotes the start offset of the value to read.
   *              Defaults to 0.
   * \param end Optional, denotes the end offset of the value to read. The byte
   *            at end is not read. i.e. to read the first 5 bytes of the file,
   *            you call read(key, output, 0, 5); Defaults to the length of the
   *            file.
   *
   * Note that the number of bytes read can be 0 if:
   *  - start is past the end of the value
   *  - end is less than start
   *
   * If start and end are not passed, the entire block is read.
   *
   * \returns A value less than 0 on failure.
   */
  int64_t read(const std::string& key,
            char* output,
            size_t start = 0,
            size_t end = (size_t)(-1)) ;

  /**
   * string overload. Note the char* reader is faster.
   */
  int64_t read(const std::string& key,
            std::string& output,
            size_t start = 0,
            size_t end = (size_t)(-1)) ;
  /**
   * This returns then number of file handle cache hits.
   * This function is for profiling purposes since file handles are cached for
   * performance reasons.
   */
  size_t file_handle_cache_hits() const;

  /**
   * This returns then number of file handle cache misses.
   * This function is for profiling purposes since file handles are cached for
   * performance reasons.
   */
  size_t file_handle_cache_misses() const;

  /** Sets the maximum number of files managed.
   * If 0, there is no max capacity.
   */
  size_t get_max_capacity();

  /** Gets the maximum number of files managed.
   * If 0, there is no max capacity.
   */
  void set_max_capacity(size_t);

  /**
   * Gets a singleton instance. The singleton instance has this default behavior:
   *
   * Location of storage:
   *   - If temp files are located on HDFS, the cache just writes
   *   through and is always located on HDFS.
   *   - If temp files are located on local disk, the cache is set to the
   *   cache:// file system. This allows for a degree of in-memory caching.
   *
   * File handle LRU cache size:
   *   - 4 * ncpus
   */
  static block_cache& get_instance();
  static void release_instance();

 private:
  static constexpr size_t KEY_LOCK_SIZE = 256;

  /// whether the block_cache is initialized
  bool m_initialized = false;

  /// The storage prefix
  std::string m_storage_prefix;

  /// Lock on internal datastructures
  mutex m_lock;

  /// The set of files I created.
  std::set<std::string> m_created_files;

  /// The maximum number of files managed. If 0 there is no limit.
  size_t m_max_capacity = 0;

  /**
   * A lock on each key.
   * The key is hashed and key_lock[hash] is used to lock the key.
   */
  mutex key_lock[KEY_LOCK_SIZE];

  /// A cache of files to file handles
  lru_cache<std::string, std::shared_ptr<general_ifstream> > m_cache;

  /// An LRU cache of the set of files we maintain. The value_type (bool) is unused.
  lru_cache<std::string, bool> m_lru_files;

}; // class block_cache
} // turicreate
#endif
