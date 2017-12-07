/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_USER_PAGEFAULT_PAGEFILE_HPP
#define TURI_USER_PAGEFAULT_PAGEFILE_HPP
#include <vector>
#include <map>
#include <string>
#include <util/dense_bitset.hpp>
#include <parallel/pthread_tools.hpp>
#include <parallel/atomic.hpp>
namespace turi {

/**
 * \internal
 * \ingroup pagefault
 * Internal Pagefile Implementation.
 *
 * Disk-backed pagefile associated with the user pagefault handler.  Used to
 * support the eviction (and future page in) of dirty pages which are
 * completely maintained by the pagefault handler.
 */
class pagefile {
 public:
  static const size_t NUM_ARENAS = 10;
  pagefile();

  /**
   * Initializes the pagefile handler. This function must be called before
   * anything is done
   */
  void init(const std::vector<size_t>& arena_sizes);

  /**
   * Clears all state.
   */
  void reset();

  /**
   * Allocated a region.
   * returns a handle.
   */
  size_t allocate();

  /**
   * Writes a bunch of data to the handle. len must be smaller than or equal
   * to the original allocated length.
   */
  bool write(size_t handle, char* start, size_t len);

  /**
   * Reads a bunch of data from the handle. len must be smaller than or equal
   * to the original allocated length.
   */
  bool read(size_t handle, char* start, size_t len);

  /**
   * Releases the handle
   */
  bool release(size_t handle);

  /**
   * Returns the number of elements in each arena.
   *
   * Returns a vector of length NUM_ARENAS where element i is the number of 
   * elements stored in each arena.
   */
  std::vector<size_t> get_allocation_counts() const;

  /**
   * Returns a vector of arena sizes.
   *
   * Returns a vector of length NUM_ARENAS where element i is the size 
   * of each element in arena i.
   */
  std::vector<size_t> get_arena_sizes() const;

  /**
   * Returns total number of bytes stored.
   */
  size_t total_allocated_bytes() const;

  /**
   * Returns The effective compression ratio
   */
  double get_compression_ratio() const;

  ~pagefile();
 private:
  /**
   * This structure maintains the arena of size "arena_size". i.e.
   * It only maintains allocations of *exactly* that size. 
   * 
   * Essentially I am interpreting the file as a collection of N consecutive
   * sections of arena_size bytes. A bitfield of length N is used to denote whether
   * a particular section is in use or not.
   */
  struct arena {
    turi::mutex pagefile_lock;
    int pagefile_handle = -1;
    size_t current_pagefile_length = 0;
    size_t arena_size = 0;
    /**
     * bitfield of which positions within the arena are being used
     * and which are not. if allocations.get(i) is false, the section is not
     * being used.
     */
    dense_bitset allocations;
  };

  /** An array of all the arenas we have. These must be sorted by arena_size
   * in increasing order.
   */
  arena m_arenas[NUM_ARENAS];

  /// The number of arenas
  size_t m_num_arenas = 0;

  /// The size of the largest arena
  size_t m_max_arena_size = 0;

  /**
   * The total number of bytes allocated
   */
  turi::atomic<size_t> m_total_allocated_bytes;

  /**
   * Total number of alloc calls
   */
  turi::atomic<size_t> m_num_allocations_made;

  /**
   * Every allocation (accessed by the \ref allocate(), \ref read()
   * \ref write() and \ref free() function), references a handle to one of 
   * these datastructures that tell us what is the size that was allocated,
   * and its current location (which arena it is in, whether it is compressed,
   * etc).
   */
  struct allocation{
    size_t stored_size = 0;
    size_t prelz4_size = 0;
    size_t original_size = 0;
    std::pair<size_t, size_t> arena_number_and_offset = {-1, -1};
    bool compressed = false;
    simple_spinlock allocation_lock;
  };
  /**
   * Since the user just sees integer handles, this is a map from integer 
   * handles to the allocation metadata.
   */
  turi::mutex m_lock;
  std::unordered_map<size_t, std::shared_ptr<allocation>> m_handle_to_allocation;
  size_t m_next_handle_id;

  /*
   * These functions below here handle the lower level with-in arena allocations.
   * Each arena allocation is referenced by an "arena_number_and_offset"
   * address.
   * Basically, the arena number is the index into the m_arenas array; i.e.
   * which arena. The offset is the position within the pagefile. 
   * i.e. to get to the data, you seek to to the offset arena_size * offset
   * within the pagefile.
   */

  /**
   * Given a size, find the best fit arena
   */
  size_t best_fit_arena(size_t size);

  /**
   * allocates an arena range.
   * Return a pair of arena number and offset
   */
  std::pair<size_t, size_t> allocate_arena(size_t size);

  /**
   * deallocates an arena from a number and offset pair.
   */
  void deallocate_arena(std::pair<size_t, size_t> arena_number_and_offset);
  /**
   * Reads the data stored at the arena address arena_number_and_offset 
   */
  void read_arena(std::pair<size_t, size_t> arena_number_and_offset, char* data, size_t len);
  void write_arena(std::pair<size_t, size_t> arena_number_and_offset, char* data, size_t len);

};

}
#endif
