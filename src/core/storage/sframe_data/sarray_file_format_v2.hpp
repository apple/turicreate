/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_SARRAY_FILE_FORMAT_V2_HPP
#define TURI_SFRAME_SARRAY_FILE_FORMAT_V2_HPP
#include <string>
#include <memory>
#include <typeinfo>
#include <map>
#include <core/parallel/mutex.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <core/logging/logger.hpp>
#include <core/random/random.hpp>
#include <core/util/dense_bitset.hpp>
#include <core/storage/sframe_data/sarray_file_format_interface.hpp>
#include <core/storage/sframe_data/sarray_index_file.hpp>
#include <core/storage/fileio/general_fstream.hpp>
#include <core/storage/fileio/temp_files.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>
#include <core/storage/sframe_data/sarray_v2_block_manager.hpp>
#include <core/storage/sframe_data/sarray_v2_block_writer.hpp>
#include <core/storage/sframe_data/sarray_v2_encoded_block.hpp>
#include <core/system/cppipc/server/cancel_ops.hpp>
namespace turi {



/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_internal SFrame Internal
 * \{
 */

/**
 * This class implements the version 2 file format.
 * See the sarray_v2_block_manager for format details.
 */
template <typename T>
class sarray_format_reader_v2: public sarray_format_reader<T> {
 public:
  /// Default Constructor
  inline sarray_format_reader_v2():
      m_manager(v2_block_impl::block_manager::get_instance()) {
  }

  /**
   * Destructor. Also closes the sarray if open.
   */
  inline ~sarray_format_reader_v2() {
    close();
  }

  /// deleted copy constructor
  sarray_format_reader_v2(const sarray_format_reader_v2& other) = delete;

  /// deleted assignment
  sarray_format_reader_v2& operator=(const sarray_format_reader_v2& other) = delete;


  /**
   * Open has to be called before any of the other functions are called.
   * Throws a string exception if it is unable to open the index file, or if
   * there is a format error in the sarray index file.
   *
   * Will throw an exception if a file set is already open.
   */
  void open(index_file_information index) {
    close();
    m_index_info = index;
    m_block_list.clear();
    m_start_row.clear();
    m_segment_list.clear();
    m_num_rows = 0;
    size_t row_count = 0;

    for (size_t i = 0;i < index.segment_files.size(); ++i) {
      auto columnaddr =  m_manager.open_column(index.segment_files[i]);
      m_segment_list.push_back(columnaddr);
      size_t nblocks = m_manager.num_blocks_in_column(columnaddr);

      size_t segment_id, column_id;
      std::tie(segment_id, column_id) = columnaddr;

      const std::vector<std::vector<v2_block_impl::block_info>>& segment_blocks = m_manager.get_all_block_info(segment_id);

      for (size_t j = 0; j < nblocks; ++j) {
        block_address blockaddr{std::get<0>(columnaddr), std::get<1>(columnaddr), j};
        m_start_row.push_back(row_count);
        row_count += segment_blocks[column_id][j].num_elem;
        m_block_list.push_back(blockaddr);
      }
    }
    for (auto& ssize: m_index_info.segment_sizes) m_num_rows += ssize;
    m_cache.clear();
    m_cache.resize(m_block_list.size());
    m_used_cache_entries.resize(m_block_list.size());
    m_used_cache_entries.clear();
    // it is convenient for m_start_row to have one more entry which is
    // the total # elements in the file
    m_start_row.push_back(m_num_rows);
    ASSERT_EQ(m_num_rows, row_count);
  }

  /**
   * Opens a single sidx file
   */
  void open(std::string sidx_file) {
    open(read_index_file(sidx_file));
  }
  /**
   * Closes an sarray file set. No-op if the array is already closed.
   */
  void close() {
    // close all columns
    for (auto column: m_segment_list) {
      m_manager.close_column(column);
    }
    m_segment_list.clear();
    m_cache.clear();
  }

  /**
   * Return the number of segments in the sarray
   */
  size_t num_segments() const {
    return m_index_info.nsegments;
  }

  /**
   * Returns the number of elements in a given segment.
   */
  size_t segment_size(size_t segmentid) const {
    DASSERT_LT(segmentid, m_index_info.nsegments);
    return m_index_info.segment_sizes[segmentid];
  }

  /**
   * Gets the contents of the index file information read from the index file
   */
  const index_file_information& get_index_info() const {
    return m_index_info;
  }

  /**
   * Returns the index_file of the array (the argument in \ref open)
   */
  std::string get_index_file() const {
    return m_index_info.index_file;
  }

  size_t read_rows(size_t row_start,
                   size_t row_end,
                   sframe_rows& out_obj);

  /**
   * Reads a collection of rows, storing the result in out_obj.
   * This function is independent of the open_segment/read_segment/close_segment
   * functions, and can be called anytime. This function is also fully
   * concurrent.
   * \param row_start First row to read
   * \param row_end one past the last row to read (i.e. EXCLUSIVE). row_end can
   *                be beyond the end of the array, in which case,
   *                fewer rows will be read.
   * \param out_obj The output array
   * \returns Actual number of rows read. Return (size_t)(-1) on failure.
   *
   * \note This function is currently only optimized for "mostly" sequential
   * reads. i.e. we are expecting read_rows(a, b), to be soon followed by
   * read_rows(b,c), etc.
   */
  size_t read_rows(size_t row_start,
                   size_t row_end,
                   std::vector<T>& out_obj) {
    if (row_end > m_num_rows) row_end = m_num_rows;
    if (row_start >= row_end) {
      out_obj.clear();
      return 0;
    }
    out_obj.resize(row_end - row_start);
    fetch_rows_from_cache(row_start, row_end, out_obj);

    if(cppipc::must_cancel()) {
      throw(std::string("Cancelled by user."));
    }
    return out_obj.size();
  }

 private:
  typedef v2_block_impl::block_address block_address;
  typedef v2_block_impl::column_address column_address;
  typedef v2_block_impl::block_info block_info;

  /// A reference to the manager
  v2_block_impl::block_manager& m_manager;

  /// The index information of this array
  index_file_information m_index_info;
  /// NUmber of rows of this array
  size_t m_num_rows;
  std::vector<block_address> m_block_list;
  std::vector<size_t> m_start_row;
  std::vector<column_address> m_segment_list;

  /**
   * this describes one cache block.
   *
   * Each cache_entry is a decoded block. m_start_row provides the information
   * as to which row this block contains. buffer_start_row is the first
   * row in the buffer which is usable.
   *
   * The caching algorithm works as such:
   *  - fetch the cache entry from file for a given block if it doesn't exist.
   *    If we exceed the maximum cache limit, we evict something random
   *  - If buffer_start_row matches the first requested row, this is a
   *    sequential access and we use "moves" to move the read data into the
   *    user's buffer. We will then have to update the buffer_start_row since
   *    some data is now missing from the buffer. If the cache becomes empty
   *    we evict the entry.)
   *  - If buffer_start_row does not match the first requested row, it is
   *    a random access and we use copies.
   *
   * The random eviction process works as such:
   *  - m_used_cache_entries is a bitfield which lists the buffers in use
   *  - m_cache_size is an atomic counter which counts the number of buffers
   *  - When an eviction happens, we pick a random block number and search
   *  for the next block number which contains a cache entry, and try to evict
   *  that.
   */
  struct cache_entry {
    cache_entry() = default;
    cache_entry(const cache_entry& other) = default;

    cache_entry(cache_entry&& other) {
      buffer_start_row = std::move(other.buffer_start_row);
      is_encoded = std::move(other.is_encoded);
      buffer = std::move(other.buffer);
      encoded_buffer = std::move(other.encoded_buffer);
      encoded_buffer_reader = std::move(other.encoded_buffer_reader);
    }

    cache_entry& operator=(const cache_entry& other) = default;

    cache_entry& operator=(cache_entry&& other) {
      buffer_start_row = std::move(other.buffer_start_row);
      is_encoded = std::move(other.is_encoded);
      buffer = std::move(other.buffer);
      encoded_buffer = std::move(other.encoded_buffer);
      encoded_buffer_reader = std::move(other.encoded_buffer_reader);
    }
    turi::simple_spinlock lock;
    /// First accessible row in buffer. Either encoded or decoded.
    size_t buffer_start_row = 0;
    // whether this cache entry is held encoded or decoded
    bool is_encoded = false;
    bool has_data = false;
    // if it is held decoded
    std::shared_ptr<std::vector<T> > buffer;
    // if it is held encoded
    v2_block_impl::encoded_block encoded_buffer;
    v2_block_impl::encoded_block_range encoded_buffer_reader;
  };

  mutex m_lock;
  /**
   * This lists the cache entriese that have values in them
   */
  dense_bitset m_used_cache_entries;
  /// The number of cached blocks. If this gets big we need to evict something
  atomic<size_t> m_cache_size;
  /**
   * There is one cache object for each block
   */
  std::vector<cache_entry> m_cache;
  static buffer_pool<std::vector<T> > m_buffer_pool;

  /**
   * Extracts as many from fetch_start to fetch_end from the cache
   * inserting into out_obj. out_obj must be resized to fetch_end - fetch_start.
   *
   * See \ref cache_entry for details on the caching process.
   */
  void fetch_rows_from_cache(size_t fetch_start,
                             size_t fetch_end,
                             std::vector<T>& out_obj);

  void ensure_cache_decoded(cache_entry& cache, size_t block_number);

  /**
   * Releases a cache entry.
   * Releases the buffer back to the pool and update the bitfield and
   * cache_size counters.
   */
  void release_cache(size_t block_number) {
    // if there is something to release
    if (m_cache[block_number].has_data) {
//       std::cerr << "Releasing cache : " << block_number << std::endl;
      m_buffer_pool.release_buffer(std::move(m_cache[block_number].buffer));
      m_cache[block_number].buffer.reset();
      m_cache[block_number].encoded_buffer.release();
      m_cache[block_number].encoded_buffer_reader.release();
      m_cache[block_number].has_data = false;
      m_used_cache_entries.clear_bit(block_number);
      m_cache_size.dec();
    }
  }

  /**
   * Picks a random number and evicts the next block after the number
   * (looping around).
   */
  void try_evict_something_from_cache() {
    size_t b = turi::random::fast_uniform<size_t>(0, m_cache.size() - 1);
    /*
     * if the current bit is not 1, try to find the next one bit
     * if there is no bit after that, loop around, reset and 0 and try the bit
     * after that
     */
    if (!m_used_cache_entries.get(b) &&
        !m_used_cache_entries.next_bit(b)) {
      // loop around to 0
      b = 0;
      if (!m_used_cache_entries.get(b)) m_used_cache_entries.next_bit(b);
    }
    std::unique_lock<turi::simple_spinlock> cache_lock_guard(m_cache[b].lock, std::defer_lock);
    if (cache_lock_guard.try_lock()) {
      release_cache(b);
    }
  }

  void fetch_cache_from_file(size_t block_number, cache_entry& ret);

  size_t block_offset_containing_row(size_t row) {
    auto pos = std::lower_bound(m_start_row.begin(), m_start_row.end(), row);
    size_t blocknum = std::distance(m_start_row.begin(), pos);
    // common case
    if (blocknum < m_block_list.size()) {
      // the block containing this could be either at blocknum, or blocknum - 1
      if (m_start_row[blocknum] == row) {
        return blocknum;
      } else {
        return blocknum - 1;
      }
    } else {
      // the last block
      return m_block_list.size() - 1;
    }
  }
};

template <typename T>
buffer_pool<std::vector<T> > sarray_format_reader_v2<T>::m_buffer_pool;

// specialization for fetch_cache_from_file when T is a flexible_type
// since this permits an encoded representation
template <>
inline void
sarray_format_reader_v2<flexible_type>::
fetch_cache_from_file(size_t block_number, cache_entry& ret) {
//   std::cerr << "Fetching from file: " << block_number << std::endl;
  // don't use the buffer. hold as encoded always when reading from a
  // flexible_type file
  if (ret.buffer) {
    m_buffer_pool.release_buffer(std::move(ret.buffer));
    ret.buffer.reset();
  }
  block_address block_addr = m_block_list[block_number];
  v2_block_impl::block_info* info;
  auto buffer = m_manager.read_block(block_addr, &info);
  if (buffer == nullptr) {
    log_and_throw("Unexpected block read failure. Bad file?");
  }
  ret.buffer_start_row = m_start_row[block_number];
  ret.encoded_buffer.init(*info, buffer);
  ret.encoded_buffer_reader = ret.encoded_buffer.get_range();
  ret.is_encoded = true;
  ret.has_data = true;
  if (m_used_cache_entries.get(block_number) == false) m_cache_size.inc();
  m_used_cache_entries.set_bit(block_number);
  // evict something random
  // we will only loop at most this number of times
  int num_to_evict = (int)(m_cache_size.value) -
      SFRAME_MAX_BLOCKS_IN_CACHE;
  while(num_to_evict > 0 &&
        m_cache_size.value > SFRAME_MAX_BLOCKS_IN_CACHE) {
    try_evict_something_from_cache();
    --num_to_evict;
  }
}

template <typename T>
inline void
sarray_format_reader_v2<T>::
fetch_cache_from_file(size_t block_number, cache_entry& ret) {
//   std::cerr << "Fetching from file: " << block_number << std::endl;
  if (!ret.buffer) ret.buffer = m_buffer_pool.get_new_buffer();
  block_address block_addr = m_block_list[block_number];
  if (!m_manager.read_block(block_addr, *ret.buffer, NULL)) {
    log_and_throw("Unexpected block read failure. Bad file?");
  }
  ret.buffer_start_row = m_start_row[block_number];
  ret.is_encoded = false;
  ret.has_data = true;
  if (m_used_cache_entries.get(block_number) == false) m_cache_size.inc();
  m_used_cache_entries.set_bit(block_number);
  // evict something random
  // we will only loop at most this number of times
  int num_to_evict = (int)(m_cache_size.value) -
      SFRAME_MAX_BLOCKS_IN_CACHE;
  while(num_to_evict > 0 &&
        m_cache_size.value > SFRAME_MAX_BLOCKS_IN_CACHE) {
    try_evict_something_from_cache();
    --num_to_evict;
  }
}


template <>
inline void sarray_format_reader_v2<flexible_type>::
ensure_cache_decoded(cache_entry& cache, size_t block_number) {
  if (cache.is_encoded) {
    cache.buffer = m_buffer_pool.get_new_buffer();
    auto data = cache.encoded_buffer.get_block_data();
    v2_block_impl::typed_decode(cache.encoded_buffer.get_block_info(),
                                data->data(),
                                data->size(),
                                *cache.buffer);
    // clear the encoded buffer information
    cache.encoded_buffer.release();
    cache.encoded_buffer_reader.release();
    // reset the start row
    cache.is_encoded = false;
    cache.buffer_start_row = m_start_row[block_number];
  }
}


template <typename T>
inline void sarray_format_reader_v2<T>::
ensure_cache_decoded(cache_entry& cache, size_t block_number) {
  ASSERT_MSG(false, "Attempting to type decode a non-flexible_type column");
}


// specialization for fetch_rows_from_cache when T is a flexible_type
// to permit pulling from the encoded representation
template <>
inline void sarray_format_reader_v2<flexible_type>::
fetch_rows_from_cache(size_t fetch_start,
                           size_t fetch_end,
                           std::vector<flexible_type>& out_obj) {
  // find block address containing fetch_start and block containing fetch_end
  size_t start_offset = block_offset_containing_row(fetch_start);
  size_t end_offset = block_offset_containing_row(fetch_end - 1) + 1;
  size_t output_idx = 0;
  for (size_t i = start_offset; i < end_offset; ++i) {
    size_t first_row_to_fetch_in_this_block = std::max(fetch_start, m_start_row[i]);
    size_t last_row_to_fetch_in_this_block = std::min(fetch_end, m_start_row[i+1]);
    auto& cache = m_cache[i];
    std::unique_lock<turi::simple_spinlock> cache_lock_guard(cache.lock);
    if (!cache.has_data) {
      fetch_cache_from_file(i, cache);
    }
    if (cache.buffer_start_row < first_row_to_fetch_in_this_block && cache.is_encoded) {
      // fast forward
      size_t diff = first_row_to_fetch_in_this_block - cache.buffer_start_row;
      cache.encoded_buffer_reader.skip(diff);
      cache.buffer_start_row  = first_row_to_fetch_in_this_block;
    }

    if (cache.buffer_start_row == first_row_to_fetch_in_this_block) {
      // this is a sequential read
      // we do moves, and we can handle encoded reads
      if (cache.is_encoded) {
        size_t num_elem = last_row_to_fetch_in_this_block - first_row_to_fetch_in_this_block;
        cache.encoded_buffer_reader.decode_to(&(out_obj[output_idx]), num_elem);
        output_idx += num_elem;
        cache.buffer_start_row = last_row_to_fetch_in_this_block;
      } else {
        size_t input_offset = m_start_row[i];
        for (size_t j = first_row_to_fetch_in_this_block;
             j < last_row_to_fetch_in_this_block;
             ++j) {
          out_obj[output_idx++] = (*cache.buffer)[j - input_offset];
        }
      }
      if (last_row_to_fetch_in_this_block == m_start_row[i + 1]) {
        // we have exhausted this cache
        release_cache(i);
      }
    } else {
      // non sequential read
      // we copy without updating the start_row
      ensure_cache_decoded(cache, i);
      size_t input_offset = m_start_row[i];
      for (size_t j = first_row_to_fetch_in_this_block;
           j < last_row_to_fetch_in_this_block;
           ++j) {
        out_obj[output_idx++] = (*cache.buffer)[j - input_offset];
      }
    }
  }
}

template <typename T>
inline void sarray_format_reader_v2<T>::
fetch_rows_from_cache(size_t fetch_start,
                           size_t fetch_end,
                           std::vector<T>& out_obj) {
  // find block address containing fetch_start and block containing fetch_end
//   std::cerr << "Fetching from cache: " << fetch_start << " " << fetch_end << std::endl;
  size_t start_offset = block_offset_containing_row(fetch_start);
  size_t end_offset = block_offset_containing_row(fetch_end - 1) + 1;
  size_t output_idx = 0;
  for (size_t i = start_offset; i < end_offset; ++i) {
    size_t first_row_to_fetch_in_this_block = std::max(fetch_start, m_start_row[i]);
    size_t last_row_to_fetch_in_this_block = std::min(fetch_end, m_start_row[i+1]);
    auto& cache = m_cache[i];
    std::unique_lock<turi::simple_spinlock> cache_lock_guard(cache.lock);
    if (!cache.buffer ||
        cache.buffer_start_row > first_row_to_fetch_in_this_block) {
      // we need to reload the cache
      fetch_cache_from_file(i, cache);
    }
    if (cache.buffer_start_row == first_row_to_fetch_in_this_block) {
      // this is a sequential read
      // encoded reads are impossible
      size_t input_offset = m_start_row[i];
      for (size_t j = first_row_to_fetch_in_this_block;
           j < last_row_to_fetch_in_this_block;
           ++j) {
        out_obj[output_idx++] = std::move((*cache.buffer)[j - input_offset]);
      }
      cache.buffer_start_row = last_row_to_fetch_in_this_block;
      if (last_row_to_fetch_in_this_block == m_start_row[i + 1]) {
        // we have exhausted this cache
        release_cache(i);
      }
    } else {
      // non sequential read
      // we copy without updating the start_row
      size_t input_offset = m_start_row[i];
      for (size_t j = first_row_to_fetch_in_this_block;
           j < last_row_to_fetch_in_this_block;
           ++j) {
        out_obj[output_idx++] = (*cache.buffer)[j - input_offset];
      }
    }
  }
}

template <>
inline size_t sarray_format_reader_v2<flexible_type>::
read_rows(size_t row_start,
          size_t row_end,
          sframe_rows& out_obj) {
  return sarray_format_reader<flexible_type>::read_rows(row_start, row_end, out_obj);
}


template <typename T>
inline size_t sarray_format_reader_v2<T>::
read_rows(size_t row_start,
          size_t row_end,
          sframe_rows& out_obj) {
  ASSERT_MSG(false, "Attempting to type decode a non-flexible_type column");
  return 0;
}


/**
 * The array group writer which emits array v2 file formats.
 */
template <typename T>
class sarray_group_format_writer_v2: public sarray_group_format_writer<T> {
 public:
  /**
   * Open has to be called before any of the other functions are called.
   * No files are actually opened at this point.
   */
  void open(std::string index_file,
            size_t segments_to_create,
            size_t columns_to_create) {
    if (columns_to_create == 0) {
      segments_to_create = 0;
    }
    m_array_open = true;
    m_writer.init(index_file, segments_to_create, columns_to_create);
    m_nsegments = segments_to_create;
    m_column_buffers.resize(columns_to_create);
    for (size_t i = 0; i < columns_to_create; ++i) {
      m_column_buffers[i].segment_data.resize(segments_to_create);
    }
    for (size_t i = 0; i < m_nsegments; ++i) {
      open_segment(i);
    }
  }

  /**
   * Sets write options. See \ref v2_block_impl::block_writer::set_options
   */
  void set_options(const std::string& option, int64_t value) {
    m_writer.set_options(option, value);
  }

  /**
   * Gets a modifiable reference to the index file information which will
   * be written to the index file.
   */
  group_index_file_information& get_index_info() {
    return m_writer.get_index_info();
  }

  /**
   * Writes a row to the array group
   */
  void write_segment(size_t segmentid,
                     const std::vector<T>& v) {
    DASSERT_LT(segmentid, m_nsegments);
    DASSERT_LE(v.size(), m_column_buffers.size());
    DASSERT_EQ(m_array_open, true);
    for (size_t i = 0;i < v.size(); ++i) {
      write_segment(i, segmentid, v[i]);
    }
  }

  /**
   * Writes a row to the array group
   */
  void write_segment(size_t segmentid,
                     std::vector<T>&& v) {
    DASSERT_LT(segmentid, m_nsegments);
    DASSERT_LE(v.size(), m_column_buffers.size());
    DASSERT_EQ(m_array_open, true);
    for (size_t i = 0;i < v.size(); ++i) {
      write_segment(i, segmentid, std::forward<T>(v[i]));
    }
  }

  /**
   * Writes a row to the array group
   */
  void write_segment(size_t columnid,
                     size_t segmentid,
                     const T& t) {
    DASSERT_LT(segmentid, m_nsegments);
    DASSERT_LT(columnid, m_column_buffers.size());
    DASSERT_EQ(m_array_open, true);
    m_column_buffers[columnid].segment_data[segmentid].push_back(t);
    if (m_column_buffers[columnid].segment_data[segmentid].size() >=
        m_column_buffers[columnid].elements_before_flush) {
      flush_block(columnid, segmentid);
    }
  }

  /**
   * Writes a collection of rows to a column
   */
  void write_column(size_t columnid,
                    size_t segmentid,
                    const std::vector<T>& t) {
    DASSERT_LT(segmentid, m_nsegments);
    DASSERT_LT(columnid, m_column_buffers.size());
    DASSERT_EQ(m_array_open, true);

    auto& el_before_flush = m_column_buffers[columnid].elements_before_flush;
    auto& buffer = m_column_buffers[columnid].segment_data[segmentid];
    for(const auto& elem: t) {
      buffer.push_back(elem);
      if (buffer.size() >= el_before_flush) {
        flush_block(columnid, segmentid);
      }
    }
  }

  /**
   * Writes a collection of rows to a column
   */
  void write_column(size_t columnid,
                    size_t segmentid,
                    std::vector<T>&& t) {
    DASSERT_LT(segmentid, m_nsegments);
    DASSERT_LT(columnid, m_column_buffers.size());
    DASSERT_EQ(m_array_open, true);

    auto el_before_flush = m_column_buffers[columnid].elements_before_flush;
    auto& buffer = m_column_buffers[columnid].segment_data[segmentid];
    for(const auto& elem: t) {
      buffer.push_back(std::move(elem));
      if (buffer.size() >= el_before_flush) {
        flush_block(columnid, segmentid);
        el_before_flush = m_column_buffers[columnid].elements_before_flush;
      }
    }
  }

  /**
   * Writes a row to the array group
   */
  void write_segment(size_t columnid,
                     size_t segmentid,
                     T&& t) {
    DASSERT_LT(segmentid, m_nsegments);
    DASSERT_LT(columnid, m_column_buffers.size());
    DASSERT_EQ(m_array_open, true);
    m_column_buffers[columnid].segment_data[segmentid].push_back(std::forward<T>(t));
    if (m_column_buffers[columnid].segment_data[segmentid].size() >=
        m_column_buffers[columnid].elements_before_flush) {
      flush_block(columnid, segmentid);
    }
  }

  void write_segment(size_t segmentid, const sframe_rows& rows);

  /** Closes all writes
   */
  void close() {
    ASSERT_EQ(m_array_open, true);
    // flush all data
    m_array_open = false;
    for (size_t i = 0;i < m_nsegments; ++i) {
      for (size_t j = 0;j < m_column_buffers.size(); ++j) {
        flush_block(j, i);
      }
      m_writer.close_segment(i);
    }
    /*
     * for (size_t i = 0;i < m_column_buffers.size(); ++i) {
     *   logstream(LOG_INFO) << "Writing column " << i
     *                       << " with total utilization of "
     *                       << m_column_buffers[i].total_bytes_written << std::endl;
     * }
     */
  }

  /**
   * Flushes all writes for a particular segment
   */
  void flush_segment(size_t segmentid) {
    for (size_t j = 0;j < m_column_buffers.size(); ++j) {
      flush_block(j, segmentid);
      m_column_buffers[j].segment_data[segmentid].shrink_to_fit();
    }
  }

  /**
   * Flushes the index_file_information to disk
   */
  void write_index_file() {
    m_writer.write_index_file();
  }

  /**
   * Returns the number of segments
   */
  size_t num_segments() const {
    ASSERT_EQ(m_array_open, true);
    return m_nsegments;
  }

  /**
   * Returns the number of columns
   */
  size_t num_columns() const {
    ASSERT_EQ(m_array_open, true);
    return m_column_buffers.size();
  }

 private:
  /// whether the array is open
  bool m_array_open = false;
  /// The number of segments
  size_t m_nsegments;
  /// The writer
  v2_block_impl::block_writer m_writer;

  struct column_buffer {
    // Stores in memory, the last block that has not been written for each
    // segment.  When the block has been written, the archive is cleared.
    simple_spinlock lock;
    std::vector<std::vector<T> > segment_data;
    size_t elements_before_flush = SARRAY_WRITER_INITAL_ELEMENTS_PER_BLOCK;
    size_t total_bytes_written = 0;
    size_t total_elements_written = 0;
  };

  std::vector<column_buffer> m_column_buffers;
  /**
   * Makes a particular segment writable with \ref write_segment
   * Should throw an exception if the segment is already open, or if
   * the segment ID does not exist. Each segment should only be open once.
   */
  void open_segment(size_t segmentid) {
    Dlog_func_entry();
    ASSERT_MSG(m_array_open, "sarray not open");

    std::string index_file = m_writer.get_index_info().group_index_file;
    std::string filename;
    // put it in the same location as the index file
    // generate a prefix for the file. if segmentid is 1, this generates 0001
    // if segmentid is 2 this generates 0002, etc.
    std::stringstream strm;
    strm << index_file.substr(0, index_file.length() - 5) << ".";
    strm.fill('0'); strm.width(4);
    strm << segmentid;
    filename = strm.str();
    logstream(LOG_DEBUG) << "Open segment " << segmentid
                         << " for write on " << filename << std::endl;
    m_writer.open_segment(segmentid, filename);
  }

  /**
   * Flushes the current contents of a segment of a column
   */
  void flush_block(size_t columnid, size_t segmentid);
};

/// \}
template <>
inline void sarray_group_format_writer_v2<flexible_type>::flush_block(size_t columnid,
                                                               size_t segmentid) {
  // flexible_type specialization. writes typed blocks.
  // if there is no data to write, skip
  auto& colbuf = m_column_buffers[columnid];
  if (colbuf.segment_data[segmentid].empty()) return;
  size_t write_size = colbuf.segment_data[segmentid].size();
  size_t ret = m_writer.write_typed_block(segmentid,
                                          columnid,
                                          colbuf.segment_data[segmentid],
                                          v2_block_impl::block_info());
  colbuf.segment_data[segmentid].clear();
  // update the column buffer counters and estimates the number of elements
  // before the next flush.
  std::lock_guard<simple_spinlock> guard(colbuf.lock);
  colbuf.total_bytes_written += ret;
  colbuf.total_elements_written += write_size;
  colbuf.elements_before_flush = (float)(SFRAME_DEFAULT_BLOCK_SIZE) / (
      (float)(colbuf.total_bytes_written+1) / (float)(colbuf.total_elements_written+1));
  colbuf.elements_before_flush = std::max(colbuf.elements_before_flush,
                                          SARRAY_WRITER_MIN_ELEMENTS_PER_BLOCK);
  colbuf.elements_before_flush = std::min(colbuf.elements_before_flush,
                                          SFRAME_WRITER_MAX_BUFFERED_CELLS / (m_nsegments * m_column_buffers.size()));
  colbuf.elements_before_flush = std::min(colbuf.elements_before_flush,
                                          SFRAME_WRITER_MAX_BUFFERED_CELLS_PER_BLOCK);
}

template <typename T>
inline void sarray_group_format_writer_v2<T>::flush_block(size_t columnid,
                                                          size_t segmentid){
  // regular type specialization. writes bytes
  // if there is no data to write, skip
  auto& colbuf = m_column_buffers[columnid];
  if (colbuf.segment_data[segmentid].empty()) return;
  size_t write_size = colbuf.segment_data[segmentid].size();
  size_t ret = m_writer.write_block(segmentid,
                                    columnid,
                                    colbuf.segment_data[segmentid],
                                    v2_block_impl::block_info());
  colbuf.segment_data[segmentid].clear();

  // update the column buffer counters and estimates the number of elements
  // before the next flush.
  std::lock_guard<simple_spinlock> guard(colbuf.lock);
  colbuf.total_bytes_written += ret;
  colbuf.total_elements_written += write_size;
  colbuf.elements_before_flush = (float)(SFRAME_DEFAULT_BLOCK_SIZE) / (
      (float)(colbuf.total_bytes_written+1) / (float)(colbuf.total_elements_written+1));
  colbuf.elements_before_flush = std::max(colbuf.elements_before_flush,
                                          SARRAY_WRITER_MIN_ELEMENTS_PER_BLOCK);
  colbuf.elements_before_flush = std::min(colbuf.elements_before_flush,
                                          SFRAME_WRITER_MAX_BUFFERED_CELLS / (m_nsegments * m_column_buffers.size()));
  colbuf.elements_before_flush = std::min(colbuf.elements_before_flush,
                                          SFRAME_WRITER_MAX_BUFFERED_CELLS_PER_BLOCK);

}

template <>
inline void sarray_group_format_writer_v2<flexible_type>::write_segment(size_t segmentid,
                                                                        const sframe_rows& rows) {
  DASSERT_LT(segmentid, m_nsegments);
  DASSERT_EQ(m_array_open, true);

  DASSERT_EQ(rows.num_columns(), m_column_buffers.size());
  const auto& cols = rows.cget_columns();
  // reserve space in all buffers
  for (size_t i = 0;i < m_column_buffers.size(); ++i) {
    auto& buffer = m_column_buffers[i].segment_data[segmentid];
    std::copy(cols[i]->begin(), cols[i]->end(), std::back_inserter(buffer));
    if (m_column_buffers[i].segment_data[segmentid].size() >=
        m_column_buffers[i].elements_before_flush) {
      flush_block(i, segmentid);
    }
  }
}

template <typename T>
inline void sarray_group_format_writer_v2<T>::write_segment(size_t segmentid,
                                                     const sframe_rows& rows) {
  ASSERT_MSG(false, "Cannot write to general SArray with sframe_rows");
}

} // namespace turi
#endif
