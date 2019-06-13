/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_2D_SPARSE_PARALLEL_ARRAY
#define TURI_2D_SPARSE_PARALLEL_ARRAY

#include <core/util/bitops.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/parallel/atomic.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <sparsehash/dense_hash_set>

namespace turi {

/**
 * \ingroup turi
 * A sparse 2d array structure for holding items accessed by multiple
 *  threads.  Besides metadata operations, this structure essentially
 *  provides two operations -- apply and apply_all.  apply takes as
 *  input a row index, a column index, and a function taking a
 *  reference to an item.  The item is created if it does not exist,
 *  is locked, and then the function is called.  The reference is
 *  invalid as soon as the function exits. apply_all takes a function
 *  that takes as input a row_index, a column_index, and a reference
 *  to the item.  apply_all calls this function on every entry of the
 *  matrix in parallel, with the added gaurantee that each row is
 *  called by the same thread.
 */
template <typename T>
class sparse_parallel_2d_array {
public:

  typedef T value_type;

  sparse_parallel_2d_array(size_t n_rows = 0, size_t n_cols = 0)
      : kv_temp_container_v(thread::cpu_count())
  {
    resize(n_rows, n_cols);
  }

  size_t rows() const { return n_rows; }
  size_t cols() const { return n_cols; }

  /** Provides concurrant access to a particular element.  The access
   *  must be done through the apply_f function, which should have the
   *  signature apply_f(T&). It is assumed that all changes to the
   *  element are completed when the element exits.
   */
  template <typename ApplyFunction>
  GL_HOT
  void apply(size_t i, size_t j, ApplyFunction&& apply_f) {
    DASSERT_LT(i, rows());
    DASSERT_LT(j, cols());

    size_t thread_idx = thread::thread_id();

    kv_container& kv = kv_temp_container_v[thread_idx];
    kv.set_key(i, j, n_col_bits);

    size_t base_idx = get_first_level_hash(i, j, kv);

    auto& hs = hash_maps[base_idx];

    std::lock_guard<simple_spinlock> lg(hs.access_lock);

    auto ret = hs.hash_map.insert(std::move(kv));

    apply_f(ret.first->value);
  }

  /** Provides non-locking access to a particular element.  Cannot be
   * used in parallel.
   */
  GL_HOT_INLINE_FLATTEN
  T& operator()(size_t i, size_t j) {
    DASSERT_LT(i, rows());
    DASSERT_LT(j, cols());

    kv_container& kv = kv_temp_container_v[0];
    kv.set_key(i, j, n_col_bits);

    size_t base_idx = get_first_level_hash(i, j, kv);

    auto& hs = hash_maps[base_idx];
    auto ret = hs.hash_map.insert(std::move(kv));

    return ret.first->value;
  }

  /** Calls apply_f, in parallel, for every value currently in the
   *  table.  The signature of the apply function is assumed to be
   *  apply_f(size_t i, size_t j, const T& t); Note this is the const
   *  overload.
   *
   *  The storage and scheduling gaurantees that each unique value of
   *  i is called within the same thread. In other words, there are
   *  never two simultaneous calls to apply_f with the same value of
   *  i.
   */
  template <typename ApplyFunction>
  void apply_all(ApplyFunction&& apply_f) const {

    atomic<size_t> current_block_idx = 0;

    in_parallel([&](size_t thread_idx, size_t num_threads)
                GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {

        while(true) {
          size_t block_idx = (++current_block_idx) - 1;

          if(block_idx >= n_thread_blocks) {
            break;
          }

          size_t start_idx = n_levels_per_block * block_idx;
          size_t end_idx = n_levels_per_block * (block_idx + 1);

          for(size_t i = start_idx; i < end_idx; ++i) {
            const hash_block& hb = hash_maps[i];

            for(auto it = hb.hash_map.begin(); it != hb.hash_map.end(); ++it) {
              const kv_container& kv = *it;
              const auto idx_pair = kv.get_indices(n_col_bits);
              const auto& value = kv.value;
              apply_f(idx_pair.first, idx_pair.second, value);
            }
          }
        }
      });
  }

  /** Calls apply_f, in parallel, for every value currently in the
   *  table.  The signature of the apply function is assumed to be
   *  apply_f(size_t i, size_t j, T& t);
   *
   *  The storage and scheduling gaurantees that each unique value of
   *  i is called within the same thread. In other words, there are
   *  never two simultaneous calls to apply_f with the same value of
   *  i.
   *
   *  mutable overload.
   */
  template <typename ApplyFunction>
  void apply_all(ApplyFunction&& apply_f) {

    atomic<size_t> current_block_idx = 0;

    in_parallel([&](size_t thread_idx, size_t num_threads)
                GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {
        while(true) {
          size_t block_idx = (++current_block_idx) - 1;

          if(block_idx >= n_thread_blocks) {
            break;
          }

          size_t start_idx = n_levels_per_block * block_idx;
          size_t end_idx = n_levels_per_block * (block_idx + 1);

          for(size_t i = start_idx; i < end_idx; ++i) {
            const hash_block& hb = hash_maps[i];

            for(auto it = hb.hash_map.begin(); it != hb.hash_map.end(); ++it) {
              const kv_container& kv = *it;
              const auto idx_pair = kv.get_indices(n_col_bits);
              apply_f(idx_pair.first, idx_pair.second, kv.value);
            }
          }
        }
      });
  }


  void clear() {
    parallel_for(size_t(0), hash_maps.size(), [&](size_t i) {
        hash_maps[i].hash_map.clear();
      });
  }

  void resize(size_t _n_rows, size_t _n_cols) {
    n_cols = _n_cols;
    n_rows = _n_rows;
    n_col_bits = bitwise_log2_ceil(n_cols + 1);
  }

////////////////////////////////////////////////////////////////////////////////

 private:

  ////////////////////////////////////////////////////////////////////////////////
  // The internal data structures to make this efficient.

  struct kv_container {
    size_t key = 0;
    mutable T value;

    // For the empty key, use the hash of
    static kv_container as_empty() {
      kv_container kv;
      kv.key = index_hash(0); // Will never occur in practice, as we
                              // add 1 to the key.
      kv.value = T();
      return kv;
    }

    ////////////////////////////////////////

    inline bool operator==(const kv_container& kv) const {
      return key == kv.key;
    }

    /** Sets the key.
     */
    void set_key(size_t i, size_t j, size_t n_col_bits) GL_HOT_INLINE_FLATTEN {
      key = index_hash((i << n_col_bits) + j + 1);

#ifndef NDEBUG
      auto p = get_indices(n_col_bits);
      DASSERT_EQ(p.first, i);
      DASSERT_EQ(p.second, j);
#endif
    }

    // Get the indices
    inline std::pair<size_t, size_t> get_indices(size_t n_col_bits) const GL_HOT_INLINE_FLATTEN {
      size_t idx = reverse_index_hash(key) - 1;
      return std::pair<size_t, size_t>{(idx >> n_col_bits), idx & bit_mask<size_t>(n_col_bits)};
    }
  };

  kv_container empty_container;

  // The goal of this two-level system is to both allow us to have
  // each row index be always called within the same thread, and to
  // minimize collisions among writing threads.
  static constexpr size_t n_thread_block_bits = 6;
  static constexpr size_t n_levels_per_block_bits = 5;
  static constexpr size_t n_thread_blocks = (size_t(1) << n_thread_block_bits);
  static constexpr size_t n_levels_per_block = (size_t(1) << n_levels_per_block_bits);
  static constexpr size_t n_level_bits = n_thread_block_bits + n_levels_per_block_bits;
  static constexpr size_t n_levels = (size_t(1) << n_level_bits);

  GL_HOT_INLINE_FLATTEN
  inline size_t get_first_level_hash(size_t i, size_t j, const kv_container& kv) const {

    // The first index points to the thread block we end up using.
    // All values within each block will be accessed by the same
    // thread in the apply_all call.  After that, it's randomized to
    // reduce thread contention.
    size_t first_idx = i & bit_mask<size_t>(n_thread_block_bits);
    DASSERT_LT(first_idx, n_thread_blocks);

    size_t second_idx = kv.key >> (bitsizeof(size_t) - n_levels_per_block_bits);
    DASSERT_LT(second_idx, n_levels_per_block);

    size_t base_idx = first_idx * n_levels_per_block + second_idx;
    DASSERT_LT(base_idx, n_levels);
    return base_idx;
  }

  size_t n_rows = 0, n_cols = 0;
  size_t n_col_bits = 0;

  /** Sets the key.
   */
  struct hash_block {
    hash_block() {
      hash_map.set_empty_key(kv_container::as_empty());
    }

    simple_spinlock access_lock;

    struct trivial_kv_container_hash {
      GL_HOT_INLINE_FLATTEN size_t operator()(const kv_container& k) const {
        return k.key;
      }
    };

    google::dense_hash_set<kv_container, trivial_kv_container_hash> hash_map;
  };

  // The first level table for this.
  std::array<hash_block, n_levels> hash_maps;

  // Temporary things to avoid potential reallocations and stuff.
  std::vector<kv_container> kv_temp_container_v;
};

}
#endif /* TURI_2D_SPARSE_PARALLEL_ARRAY
 */
