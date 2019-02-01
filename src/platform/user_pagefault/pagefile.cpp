/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <pch/pch.hpp>

#include <unistd.h>
#include <fcntl.h>
#include <logger/assertions.hpp>
#include <user_pagefault/pagefile.hpp>
#include <user_pagefault/type_heuristic_encode.hpp>
#include <fileio/temp_files.hpp>
#include <lz4/lz4.h>
namespace turi {
pagefile::pagefile() { }

pagefile::~pagefile() { 
  reset();
}

void pagefile::reset() {
  for (size_t i = 0;i < m_num_arenas; ++i) {
    if (m_arenas[i].pagefile_handle != -1) {
      close(m_arenas[i].pagefile_handle);
      m_arenas[i].pagefile_handle = -1;
    }
    m_handle_to_allocation.clear();
    m_next_handle_id = 0;
    m_num_arenas = 0;
    m_max_arena_size = 0;
  }
}

void pagefile::init(const std::vector<size_t>& _arena_sizes) {
  auto arena_sizes = _arena_sizes;
  std::sort(arena_sizes.begin(), arena_sizes.end());
  ASSERT_LE(arena_sizes.size(), NUM_ARENAS);
  m_num_arenas = arena_sizes.size();
  // allocate each arena
  for (size_t i = 0; i < m_num_arenas; ++i) {
    m_arenas[i].arena_size = arena_sizes[i];
    m_arenas[i].current_pagefile_length = 0;
    m_max_arena_size = std::max(m_max_arena_size, arena_sizes[i]);
    // try to allocate the pagefile first
    std::string pagefile_name = get_temp_name();
    // unfortunately we have to go one level lower here 
    // since we need ftruncate. So unfortunately we have to use the POSIX
    // apis instead of the C file or the C++ APIS.
    m_arenas[i].pagefile_handle = open(pagefile_name.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0600);
    // unlink it immediately so we don't leak
    delete_temp_file(pagefile_name);
    // unable to open pagefile 
    if (m_arenas[i].pagefile_handle == -1) {
      log_and_throw("Unable to create pagefile");
    }
  }
}


size_t pagefile::allocate() {
  // find the best matching arena
  std::lock_guard<turi::mutex> guard(m_lock);
  allocation alloc;
  m_handle_to_allocation[m_next_handle_id] = std::make_shared<allocation>(alloc);
  size_t ret = m_next_handle_id;
  ++m_next_handle_id;
  return ret;
}

static std::pair<size_t, size_t> compress(char* start, size_t len, char** out) {
  char* type_encode = nullptr;
  size_t type_encode_length = 0;
  type_heuristic_encode::compress(start, len, 
                                  &type_encode, type_encode_length);
  (*out) = (char*)malloc(LZ4_compressBound(type_encode_length));
  size_t final_length = (size_t)LZ4_compress(type_encode, 
                                             (*out), type_encode_length);
  free(type_encode);
  return {type_encode_length, final_length};
}

static bool decompress(char* start, size_t len, size_t prelz4_length, char* out) {
  char* type_encode = (char*)malloc(prelz4_length);
  int ret = LZ4_decompress_fast(start, type_encode, prelz4_length);
  if (ret == 0) return false;

  type_heuristic_encode::decompress(type_encode,
                                    prelz4_length,
                                    out);
  free(type_encode);
  return true;
}

bool pagefile::write(size_t handle, char* start, size_t len) {
  std::shared_ptr<allocation> allocation_ptr;
  if (len > m_max_arena_size) return false;
  {
    std::lock_guard<turi::mutex> guard(m_lock);
    auto iter = m_handle_to_allocation.find(handle);
    if (iter == m_handle_to_allocation.end()) return false;
    allocation_ptr = iter->second;
  }

  // attempt to compress the input
  size_t compressed_size =  0;
  size_t compressed_prelz4_size =  0;
  char* compressed_buffer = nullptr;
// if we can compress the first TRIAL_COMPRESSION_SIZE bytes to less than
// TRIAL_COMPRESSION_OK_SIZE, then we treat the buffer as compressible
// We should be able to at least half.
#define TRIAL_COMPRESS_SIZE 65536
#define TRIAL_COMPRESS_OK_SIZE (TRIAL_COMPRESS_SIZE / 4) * 3
  char* trial_compressed_buffer = nullptr;
  auto trial_compressed_size = 
      compress(start, std::min<size_t>(len, TRIAL_COMPRESS_SIZE),
               &trial_compressed_buffer);
  free(trial_compressed_buffer);

  if (trial_compressed_size.second <= TRIAL_COMPRESS_OK_SIZE) {
    std::tie(compressed_prelz4_size, compressed_size) = 
        compress(start, len, &compressed_buffer);
  }
  if (compressed_size == 0 && compressed_buffer != nullptr) {
    // failed compression? why?
    free(compressed_buffer);
    compressed_buffer = nullptr;
  } 

  // the buffer to store.
  // we need to decide if we are going to be using the compressed version or 
  // not. Since compression might actually increase the size if data
  // is non-compressible.
  char* stored_buffer = nullptr;
  size_t stored_size = 0;
  bool compressed = false;
  if (compressed_size > len || compressed_buffer == nullptr) {
    // ok. non-compressible.
    stored_buffer = start;
    stored_size = len;
    if (compressed_buffer != nullptr) {
      free(compressed_buffer);
      compressed_buffer = nullptr;
    }
  } else {
    // compressible. we are going to store the compressed buffer.
    stored_buffer = compressed_buffer;
    stored_size = compressed_size;
    compressed = true;
  }

  
  // ok see if I can store it back in the same place
  std::lock_guard<simple_spinlock> guard(allocation_ptr->allocation_lock);
  if (allocation_ptr->arena_number_and_offset.first == (size_t)(-1) ||
      m_arenas[allocation_ptr->arena_number_and_offset.first].arena_size < stored_size) {
    // no arena allocated or old arena too small.
    // we need to allocate a new arena
    // release the old arena if any
    if (allocation_ptr->arena_number_and_offset.first != (size_t)(-1))  {
      m_total_allocated_bytes.dec(allocation_ptr->original_size);
      deallocate_arena(allocation_ptr->arena_number_and_offset);
    }
    // allocate a new arena
    allocation_ptr->arena_number_and_offset = allocate_arena(stored_size);
  }
  write_arena(allocation_ptr->arena_number_and_offset, stored_buffer, stored_size);
  allocation_ptr->compressed = compressed;
  allocation_ptr->stored_size = stored_size;
  if (compressed) allocation_ptr->prelz4_size = compressed_prelz4_size;
  allocation_ptr->original_size = len;
  m_total_allocated_bytes.inc(len);
  if (compressed_buffer) free(compressed_buffer);

  /*
   * if (m_num_allocations_made.inc() % 512 == 0) {
   *   std::cout << "Current compression ratio: " << get_compression_ratio() << std::endl;
   * }
   */
  return true;
}

bool pagefile::read(size_t handle, char* start, size_t len) {
  std::shared_ptr<allocation> allocation_ptr;
  if (len > m_max_arena_size) return false;
  {
    std::lock_guard<turi::mutex> guard(m_lock);
    auto iter = m_handle_to_allocation.find(handle);
    if  (iter == m_handle_to_allocation.end()) return false;
    allocation_ptr = iter->second;
  }


  std::lock_guard<simple_spinlock> guard(allocation_ptr->allocation_lock);
  if (allocation_ptr->arena_number_and_offset.first == (size_t)(-1)) {
    // no write occured. ever. set output to 0 and return.
    memset(start, 0, len);
    return true;
  }

  char* stored_buffer = nullptr;
  size_t stored_size = 0;
  char* compressed_buffer = nullptr;
  // if compressed we need to decompress into an intermediate buffer
  // if not compressed, we decode directly into the output
  if (allocation_ptr->compressed) {
    compressed_buffer = (char*)malloc(allocation_ptr->stored_size);
    stored_buffer = compressed_buffer;
    stored_size = allocation_ptr->stored_size;
  } else {
    stored_buffer = start;
    stored_size = len;
    compressed_buffer = nullptr;
  }
  read_arena(allocation_ptr->arena_number_and_offset, 
             stored_buffer, stored_size);

  if (allocation_ptr->compressed) {
    // we need to decompress.
    bool ret = decompress(stored_buffer, stored_size, allocation_ptr->prelz4_size, start);
    free(compressed_buffer);
    if (ret == false) {
      // failed to decompress?
      return false;
    }
  }
  return true;
}



bool pagefile::release(size_t handle) {
  std::shared_ptr<allocation> allocation_ptr;
  // detach it from the handle map
  {
    std::lock_guard<turi::mutex> guard(m_lock);
    auto iter = m_handle_to_allocation.find(handle);
    if  (iter == m_handle_to_allocation.end()) return false;
    allocation_ptr = iter->second;
    m_handle_to_allocation.erase(iter);
  }

  // deallocate arena if any
  if (allocation_ptr->arena_number_and_offset.first != (size_t)(-1))  {
    m_total_allocated_bytes.dec(allocation_ptr->original_size);
    deallocate_arena(allocation_ptr->arena_number_and_offset);
  }
  return true;
}

std::vector<size_t> pagefile::get_allocation_counts() const {
  std::vector<size_t> ret(NUM_ARENAS);
  for (size_t i = 0;i < NUM_ARENAS; ++i) {
    ret[i] = m_arenas[i].allocations.popcount();
  }
  return ret;
}

double pagefile::get_compression_ratio() const {
  size_t total_bytes = 0;
  size_t total_compressed_bytes = 0;
  for (const auto& v: m_handle_to_allocation) {
    total_bytes += v.second->original_size;
    total_compressed_bytes += v.second->stored_size;
  }
  return (double)(total_compressed_bytes) / (double)(total_bytes);
}


std::vector<size_t> pagefile::get_arena_sizes() const {
  std::vector<size_t> ret(NUM_ARENAS);
  for (size_t i = 0;i < NUM_ARENAS; ++i) {
    ret[i] = m_arenas[i].arena_size;
  }
  return ret;
}


size_t pagefile::total_allocated_bytes() const {
  return m_total_allocated_bytes.value;
}

size_t pagefile::best_fit_arena(size_t size) {
  // find the best matching arena
  size_t arena_number = (size_t)(-1);
  for (size_t i = 0;i < m_num_arenas; ++i) {
    if (m_arenas[i].arena_size >= size) {
      arena_number = i;
      break;
    }
  }
  return arena_number;
}
std::pair<size_t, size_t> pagefile::allocate_arena(size_t size) {
  /*
   * Not very complicated. Find the best matching arena,
   * and if there is a free section within the arena (look at the bitfield),
   * take it. If there is not, we need to extend the file with ftruncate.
   * and resize all the datastructures accordingly).
   */
  ASSERT_LE(size, m_max_arena_size);
  // find the best matching arena
  size_t arena_number = best_fit_arena(size);
  auto& cur_arena = m_arenas[arena_number];

  // acquire lock on the arena
  std::lock_guard<turi::mutex> guard(cur_arena.pagefile_lock);
  // find a free bit if there is one 
  size_t b = 0;
  if (cur_arena.allocations.first_zero_bit(b) == false) {
    // insufficient space. extend the file
    int fret = ftruncate(cur_arena.pagefile_handle, 
                         cur_arena.current_pagefile_length + 
                         cur_arena.arena_size);
    ASSERT_EQ(fret, 0);
    // extend the allocations datastructure
    cur_arena.allocations.resize(cur_arena.allocations.size() + 1);
    cur_arena.current_pagefile_length += cur_arena.arena_size;
    b = cur_arena.allocations.size() - 1;
  }
  cur_arena.allocations.set_bit(b);  
  return {arena_number, b};
}


void pagefile::deallocate_arena(std::pair<size_t, size_t> arena_number_and_offset) {
  /*
   * Deallocation is far easier. We just release the bit.
   */
  size_t arena_number;
  size_t offset;
  std::tie(arena_number, offset) = arena_number_and_offset;
  ASSERT_LT(arena_number, m_max_arena_size);

  auto& cur_arena = m_arenas[arena_number];
  std::lock_guard<turi::mutex> guard(cur_arena.pagefile_lock);
  ASSERT_LT(offset, cur_arena.allocations.size());
  cur_arena.allocations.clear_bit(offset);
}

void pagefile::read_arena(std::pair<size_t, size_t> arena_number_and_offset, 
                          char* data, size_t len) {
  size_t arena_number;
  size_t offset;
  std::tie(arena_number, offset) = arena_number_and_offset;
  ASSERT_LT(arena_number, m_max_arena_size);
  
  auto& cur_arena = m_arenas[arena_number];
  std::lock_guard<turi::mutex> guard(cur_arena.pagefile_lock);
#if __APPLE__
  off_t ret = lseek(cur_arena.pagefile_handle, offset * cur_arena.arena_size, SEEK_SET);
#else
  off64_t ret = lseek64(cur_arena.pagefile_handle, offset * cur_arena.arena_size, SEEK_SET);
  posix_fadvise(cur_arena.pagefile_handle, offset * cur_arena.arena_size, 
                cur_arena.arena_size, POSIX_FADV_NOREUSE);
#endif
  ASSERT_GE(ret, 0);
  int read_ret = ::read(cur_arena.pagefile_handle, data, len);
  ASSERT_GE(read_ret, 0);
}

void pagefile::write_arena(std::pair<size_t, size_t> arena_number_and_offset, 
                          char* data, size_t len) {
  size_t arena_number;
  size_t offset;
  std::tie(arena_number, offset) = arena_number_and_offset;
  ASSERT_LT(arena_number, m_max_arena_size);
  
  auto& cur_arena = m_arenas[arena_number];
  std::lock_guard<turi::mutex> guard(cur_arena.pagefile_lock);
#if __APPLE__
  off_t ret = lseek(cur_arena.pagefile_handle, offset * cur_arena.arena_size, SEEK_SET);
#else
  off64_t ret = lseek64(cur_arena.pagefile_handle, offset * cur_arena.arena_size, SEEK_SET);
  posix_fadvise(cur_arena.pagefile_handle, offset * cur_arena.arena_size, 
          cur_arena.arena_size, POSIX_FADV_NOREUSE);
#endif
  ASSERT_GE(ret, 0);
  int write_ret = ::write(cur_arena.pagefile_handle, data, len);
  if (write_ret < 0) {
    std::cout << "Write failed with " << errno << " " << strerror(errno) << std::endl;
  }
  ASSERT_GE(write_ret, 0);
}


} // turicreate
