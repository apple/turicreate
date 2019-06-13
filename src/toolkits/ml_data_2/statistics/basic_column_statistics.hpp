/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ML2_DATA_BASIC_COLUMN_STATISTICS_H_
#define TURI_ML2_DATA_BASIC_COLUMN_STATISTICS_H_

#include <core/data/flexible_type/flexible_type.hpp>
#include <core/logging/assertions.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <toolkits/ml_data_2/ml_data_column_modes.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <boost/thread/lock_algorithms.hpp>
#include <mutex>

namespace turi { namespace v2 { namespace ml_data_internal {

extern size_t ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD;

/**
 * column_metadata contains "meta data" concerning indexing of a single column
 * of an SFrame. A collection of meta_data column objects is "all" the
 * metadata required in the ml_data container.
 */
class basic_column_statistics : public column_statistics {

 public:

  ////////////////////////////////////////////////////////////
  // Functions to access the statistics

  /** Returns the number of seen by the methods collecting the
   *  statistics.
   */
  size_t num_observations() const {
    return total_row_count;
  }

  /* The count; index here is the index obtained by one of the
   * map_value_to_index functions previously.
   */
  size_t count(size_t index) const {
    // Use this to
    if(mode == ml_column_mode::NUMERIC
       || mode == ml_column_mode::NUMERIC_VECTOR) {

      return total_row_count;
    } else {
      return index < counts.size() ? counts[index] : 0;
    }
  }

  /* The mean; index here is the index obtained by one of the
   * map_value_to_index functions previously.
   */
  double mean(size_t index) const {
    if(mode == ml_column_mode::CATEGORICAL
       || mode == ml_column_mode::CATEGORICAL_VECTOR) {

      double p = double(count(index)) / (std::max(1.0, double(total_row_count)));
      return p;

    } else {

      if(total_row_count)
        DASSERT_TRUE(!statistics.empty());

      return index < statistics.size() ? statistics[index].mean : 0;
    }
  }

  /* The variance; index here is the index obtained by one of the
   * map_value_to_index functions previously.
   */
  double stdev(size_t index) const {
    if(mode == ml_column_mode::CATEGORICAL
       || mode == ml_column_mode::CATEGORICAL_VECTOR) {

      double stdev = 0;
      double p = mean(index);

      if (total_row_count > 1) {
        stdev = std::sqrt(total_row_count * p * (1 - p) / (total_row_count-1));
      } else {
        stdev = 0;
      }

      return stdev;

    } else {

      if(total_row_count)
        DASSERT_TRUE(!statistics.empty());

      return index < statistics.size() ? statistics[index].stdev : 0;
    }
  }

  ////////////////////////////////////////////////////////////
  // Routines for updating the statistics.  This is done online, while
  // new categories are being added, etc., so we have to be

  /// Initialize the statistics -- counting, mean, and stdev
  void initialize();

  /// Update categorical statistics for a batch of categorical indices.
  void update_categorical_statistics(size_t thread_idx, const std::vector<size_t>& cat_index_vect) GL_HOT_FLATTEN;

  /// Update categorical statistics for a batch of real values.
  void update_numeric_statistics(size_t thread_idx, const std::vector<double>& value_vect) GL_HOT_FLATTEN;

  /// Update statistics after observing a dictionary.
  void update_dict_statistics(size_t thread_idx, const std::vector<std::pair<size_t, double> >& dict) GL_HOT_FLATTEN;

  /// Perform final computations on the different statistics.  Must be
  /// called after all the data is filled.
  void finalize();

 private:
  /// Perform finalizations; split between the threadlocal stuff and the global stuff.
  void _finalize_threadlocal(size_t, bool, bool);
  void _finalize_global(size_t, bool, bool);


  ////////////////////////////////////////////////////////////////////////////////
  // Stuff for the saving and loading

  /** Returns the current serialization version of this model.
   */
  size_t get_version() const { return 2; }

  /**
   *  Serialize the object (save).
   */
  void save_impl(turi::oarchive& oarc) const;

  /**
   *  Load the object.
   */
  void load_version(turi::iarchive& iarc, size_t version);

  /** For debugging purposes.
   */
  bool is_equal(const column_statistics* other_ptr) const;

  /** Create a copy with the index cleared.
   */
   std::shared_ptr<column_statistics> create_cleared_copy() const;


 private:

  ////////////////////////////////////////////////////////////////////////////////
  // Functions for updating the categorical variables

  // Put all of these in one single structure, and store a vector of
  // these structures.  Since these are always accessed together, this
  // reduces cache misses on the lookups -- we now only have one fetch
  // instead of three.

  std::vector<size_t> counts;

  struct element_statistics : public turi::IS_POD_TYPE {
    double mean  = 0;   /**< Mean of column. */
    double stdev = 0;  /**< Stdev of column. */
  };

  std::vector<element_statistics> statistics;
  size_t total_row_count = 0;

  // The issue with having seperate accumulators for each thread is
  // that it can take an inordinate amount of memory.  Thus we use
  // parallel access for the first million or so items, which are
  // likely to be the most common.  For the rest, we use a larger one
  // with locking.

  size_t parallel_threshhold = 1024*1024;

  std::vector<size_t> by_thread_row_counts;

  struct element_statistics_accumulator : public turi::IS_POD_TYPE {
    double mean    = 0;  /**< Mean of column. */
    double var_sum = 0;  /**< Stdev of column. */
  };

  std::vector<std::vector<size_t> > by_thread_element_counts;
  std::vector<std::vector<element_statistics_accumulator> > by_thread_mean_var_acc;

  // The locks are done by
  static constexpr size_t n_locks = 64; // Should be power of 2

  std::array<simple_spinlock, n_locks> global_element_locks;
  std::vector<size_t> global_element_counts;
  std::vector<element_statistics_accumulator> global_mean_var_acc;

  volatile size_t global_size = 0;

  /**  Return the index of the appropriate lock.
   *
   */
  inline size_t get_lock_index(size_t idx) const {
    size_t lock_idx = (idx % n_locks);
    return lock_idx;
  }

  /**  Possibly resize the local array.
   */
  template <typename T>
  inline void check_local_array_size(size_t idx, std::vector<T>& v) {
    DASSERT_LT(idx, parallel_threshhold);

      // See if a resize is needed.
    if(idx >= v.size() ) {

      if(UNLIKELY(v.capacity() < idx + 1)) {
        size_t new_capacity = std::min(parallel_threshhold, 3 * (idx + 1) / 2);

        // If it's likely to go to max capacity, just make it that to avoid future resizes.
        if(new_capacity > parallel_threshhold / 2)
          new_capacity = parallel_threshhold;

        v.reserve(new_capacity);
      }

      v.resize(idx + 1);
    }
  }

  /**  Check global array size. Possibly resize it.
   */
  template <typename T>
  inline void check_global_array_size(size_t idx, std::vector<T>& v) {

    // If needed, increase the value of global_size.
    if(global_size <= idx) {

      do {
        size_t g = global_size;

        if(g > idx)
          break;

        bool success = atomic_compare_and_swap(global_size, g, idx+1);

        if(!success)
          continue;

      } while(false);
    }

    // See if a resize is needed.
    if(UNLIKELY(idx >= v.size() )) {

      // Grow aggressively, since a resize is really expensive.
      size_t new_size = 2 * (parallel_threshhold + idx + 1);

      {
        std::array<std::unique_lock<simple_spinlock>, n_locks> all_locks;
        for(size_t i = 0; i < n_locks; ++i)
          all_locks[i] = std::unique_lock<simple_spinlock>(global_element_locks[i], std::defer_lock);

        // Ensure nothing is happening with the vector by locking all
        // locks in a thread safe way.  This prevents any thread from
        // accessing it while we resize it.
        boost::lock(all_locks.begin(), all_locks.end());

        // It's possible that another thread beat us to it.
        if(v.size() < idx + 1)
          v.resize(new_size);

        // The destructor of all_locks takes care of the unlocking.
      }
    }
  }

  /** One way to set the statistics.  Used by the serialization converters.
   *
   *  "counts" -- std::vector<size_t>.  Counts.
   *
   *  "mean" -- std::vector<double>.  Mean.
   *
   *  "stdev" -- std::vector<double>.  std dev.
   *
   *  "total_row_count" -- size_t.  Total row count.
   */
  void set_data(const std::map<std::string, variant_type>& params);

};

}}}

#endif
