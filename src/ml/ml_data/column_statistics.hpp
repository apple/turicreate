/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DML_DATA_COLUMN_STATISTICS_H_
#define TURI_DML_DATA_COLUMN_STATISTICS_H_

#include <core/data/flexible_type/flexible_type.hpp>
#include <core/logging/assertions.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <ml/ml_data/ml_data_column_modes.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <boost/thread/lock_algorithms.hpp>
#include <set>

namespace turi { namespace ml_data_internal {

extern size_t ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD;



/**
 * \ingroup mldata
 * \internal
 * \addtogroup mldata ML Data Internal
 * \{
 */

/**
 * column_metadata contains "meta data" concerning indexing of a single column
 * of an SFrame. A collection of meta_data column objects is "all" the
 * metadata required in the ml_data container.
 */
class column_statistics {

 public:

  column_statistics(){}

  /**
   *  Default constructor.
   */
  column_statistics(std::string column_name,
                    ml_column_mode mode,
                    flex_type_enum original_column_type);

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
  size_t count(size_t index=0) const {
    // Use this to
    if(mode == ml_column_mode::NUMERIC
       || mode == ml_column_mode::NUMERIC_VECTOR) {

      return total_row_count;
    } else {
      size_t ret = index < counts.size() ? counts[index] : 0;
      DASSERT_LE(ret, total_row_count);
      return ret;
    }
  }

  /* The mean; index here is the index obtained by one of the
   * map_value_to_index functions previously.
   */
  double mean(size_t index=0) const {
    if(mode == ml_column_mode::CATEGORICAL
       || mode == ml_column_mode::CATEGORICAL_VECTOR) {

      double p = double(count(index)) / (std::max(1.0, double(total_row_count)));

      DASSERT_LE(p, 1.0);
      DASSERT_GE(p, 0.0);

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
  double stdev(size_t index=0) const {
    if(mode == ml_column_mode::CATEGORICAL
       || mode == ml_column_mode::CATEGORICAL_VECTOR) {

      double stdev = 0;
      double p = mean(index);

      DASSERT_LE(p, 1.0);
      DASSERT_GE(p, 0.0);

      if (total_row_count > 1) {
        stdev = std::sqrt(total_row_count * p * (1 - p) / (total_row_count-1));
      } else {
        stdev = 0;
      }

      DASSERT_FALSE(std::isnan(stdev));

      return stdev;

    } else {

      if(total_row_count) {
        DASSERT_TRUE(!statistics.empty());
      }

      return index < statistics.size() ? statistics[index].stdev : 0;
    }
  }

  ////////////////////////////////////////////////////////////
  // Routines for updating the statistics.  This is done online, while
  // new categories are being added, etc., so we have to be

  /// Initialize the statistics -- counting, mean, and stdev
  void initialize();

  /// Update categorical statistics for a batch of categorical indices.
  void update_categorical_statistics(size_t thread_idx, const std::vector<size_t>& cat_index_vect) GL_HOT {

    DASSERT_TRUE(mode == ml_column_mode::CATEGORICAL
                 || mode == ml_column_mode::CATEGORICAL_VECTOR);

    std::vector<size_t>& counts = by_thread_element_counts[thread_idx];

    // The input indices are assured to be sorted
    DASSERT_TRUE(std::is_sorted(cat_index_vect.begin(), cat_index_vect.end()));

    size_t cv_idx = 0;

    for(cv_idx = 0;
        cv_idx < cat_index_vect.size()
            && cat_index_vect[cv_idx] < parallel_threshhold;
        ++cv_idx) {

      size_t idx = cat_index_vect[cv_idx];

      check_local_array_size(idx, counts);

      if(cv_idx == 0 || (idx != cat_index_vect[cv_idx - 1]) ) {
        ++(counts[idx]);
      }
    }

    if(cv_idx != cat_index_vect.size() ) {
      for(; cv_idx < cat_index_vect.size(); ++cv_idx) {
        size_t idx = cat_index_vect[cv_idx] - parallel_threshhold;

        if(idx >= global_element_counts.size() || idx >= global_size)
          adjust_global_array_size(idx, global_element_counts);

        if(cv_idx == 0 || (idx != cat_index_vect[cv_idx - 1]) ) {
          std::lock_guard<simple_spinlock> el_lg(global_element_locks[get_lock_index(idx)]);
          ++(global_element_counts[idx]);
        }
      }
    }

    ++by_thread_row_counts[thread_idx];
  }

  /// Update categorical statistics for a batch of real values.
  void update_numeric_statistics(size_t thread_idx, const std::vector<double>& value_vect) GL_HOT {

    DASSERT_TRUE(mode == ml_column_mode::NUMERIC
                 || mode == ml_column_mode::NUMERIC_VECTOR
                 || mode == ml_column_mode::NUMERIC_ND_VECTOR);

    // Silently ignore columns of empty vectors.  Note that all the
    // vectors in a column must be empty for this to work.
    if(value_vect.empty()) {
      return;
    }

    // Numeric statistics are always cached on a thread-local basis
    // and ignore the parallel_threshhold parameter.
    size_t& count = by_thread_row_counts[thread_idx];
    auto& stats = by_thread_mean_var_acc[thread_idx];

    if(stats.empty()) {

      DASSERT_EQ(size_t(count), 0);

      stats.resize(value_vect.size());

      for(size_t i = 0; i < value_vect.size(); ++i) {
        stats[i].mean    = value_vect[i];
        stats[i].var_sum = 0;
      }

    } else {
      DASSERT_EQ(stats.size(), value_vect.size());

      // Efficient and Stable Mean+Stdev computation
      // --------------------------------------------------------------------
      // This way of computing stdev goes back to a 1962 paper by B. P.
      // Welford and is presented in Donald Knuth's Art of Computer
      // Programming, Vol 2, page 232, 3rd edition. It is way more accurate
      // and stable than computing it "directly".
      //
      // Online update rule:
      //
      // M_k = M_{k-1} + (x_k - M_{k-1})/k
      // S_k = S_{k-1} + (x_k - M_{k-1})*(x_k - M_k).
      //
      // Here: M_k is the estimate of the mean and S_k/(k-1) is the estimate
      // of the variance:
      //
      for(size_t i = 0; i < value_vect.size(); i++){
        double& mean = stats[i].mean;
        double& var_sum = stats[i].var_sum;

        double old_mean = mean;
        double v = value_vect[i];

        mean += (v - old_mean) / (count + 1);

        // stdev here is actually the variance times the counts; it's
        // converted to the stdev later
        var_sum += (v - old_mean) * (v - mean);
      }
    }

    ++count;
  }

  /// Update statistics after observing a dictionary.
  void update_dict_statistics(size_t thread_idx, const std::vector<std::pair<size_t, double> >& dict) GL_HOT {

    DASSERT_TRUE(mode == ml_column_mode::DICTIONARY);

    // Does not compile on windows; comment out for now
    // // The input array is sorted here.
    // DASSERT_TRUE(std::is_sorted(dict.begin(), dict.end(),
    //                             [](std::pair<size_t, double> p1, std::pair<size_t, double> p2) {
    //                               return p1.first < p2.first;
    //                             }));

    size_t& row_count = by_thread_row_counts[thread_idx];

    // The update function for all this
    auto update_f = [&](size_t& count, double& mean, double& var_sum, double v) {

      // Efficient and Stable Mean+Stdev computation on Sparse data
      // --------------------------------------------------------------------
      // We combine means and variances of 2 samples as follows. The
      // first sample contains all the non-zeros while the second
      // contains only zeros.
      //
      // M_{a+b} = m * M_a / (m + n)
      // S_{a+b} = S_{a} + M_a^2 - M_{a+b}^2 * (a+b)
      //
      // This computation is O(m) (insted of being O(n+m)).
      //

      if(count == 0) {
        count = 1;
        mean  = v;
        var_sum = 0;

      } else {
        double old_mean = mean;

        ++count;
        mean += (v - old_mean) / count;
        var_sum += (v - old_mean) * (v - mean);
      }
    };

    // First, go through and work with the common elements.
    std::vector<size_t>& counts = by_thread_element_counts[thread_idx];
    std::vector<element_statistics_accumulator>& stats = by_thread_mean_var_acc[thread_idx];

    size_t d_idx = 0;

    for(; d_idx < dict.size() && dict[d_idx].first < parallel_threshhold; ++d_idx) {
      size_t idx = dict[d_idx].first;
      double v   = dict[d_idx].second;

      check_local_array_size(idx, counts);
      check_local_array_size(idx, stats);

      update_f(counts[idx], stats[idx].mean, stats[idx].var_sum, v);
    }

    // Okay, there are infrequent elements here, so we need to look at the global update deal.
    if(d_idx != dict.size()) {

      for(; d_idx < dict.size(); ++d_idx) {

        size_t idx = dict[d_idx].first - parallel_threshhold;
        double v   = dict[d_idx].second;

        if(idx >= global_element_counts.size()
           || idx >= global_mean_var_acc.size()
           || idx >= global_size) {
          adjust_global_array_size(idx, global_element_counts);
          adjust_global_array_size(idx, global_mean_var_acc);
        }

        std::lock_guard<simple_spinlock> el_lg(global_element_locks[get_lock_index(idx)]);

        update_f(global_element_counts[idx],
                 global_mean_var_acc[idx].mean,
                 global_mean_var_acc[idx].var_sum, v);
      }
    }

    ++row_count;
  }

  /// Perform final computations on the different statistics.  Must be
  /// called after all the data is filled.
  void finalize();

  /**  Reindex, typically according to a global map of things.
   */
  void reindex(const std::vector<size_t>& new_index_map, size_t new_column_size);

  /**  Merges in statistics from another column_statistics object.
   */
  void merge_in(const column_statistics& other);

 private:
  /// Perform finalizations; split between the threadlocal stuff and the global stuff.
  std::pair<bool, bool> _get_using_flags() const;

  void _finalize_threadlocal(size_t, bool, bool);
  void _finalize_global(size_t, bool, bool);


 public:
  ////////////////////////////////////////////////////////////////////////////////
  // Stuff for the saving and loading

  /** Returns the current serialization version of this model.
   */
  size_t get_version() const { return 3; }

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

  /**
   * Equality testing -- slow!  Use for debugging/testing
   */
  bool operator==(const column_statistics& other) const;

  /**
   * Inequality testing -- slow!  Use for debugging/testing
   */
  bool operator!=(const column_statistics& other) const;

#ifndef NDEBUG
  void _debug_check_is_approx_equal(std::shared_ptr<column_statistics> other) const;
#else
  void _debug_check_is_approx_equal(std::shared_ptr<column_statistics> other) const {}
#endif

 private:

  // Store the basic column data.  This allows us to do error checking
  // and error reporting intelligently.

  std::string column_name;
  ml_column_mode mode;
  flex_type_enum original_column_type;

  ////////////////////////////////////////////////////////////////////////////////
  // The categorical variables

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

  // The locks are done by locking the lock given by the write index
  // modulus this value.
  static constexpr size_t n_locks = 64; // Should be power of 2

  std::array<simple_spinlock, n_locks> global_element_locks;
  std::vector<size_t> global_element_counts;
  std::vector<element_statistics_accumulator> global_mean_var_acc;

  size_t global_size = 0;

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
  void adjust_global_array_size(size_t idx, std::vector<T>& v) {

    // If needed, increase the value of global_size.
    atomic_set_max(global_size, idx + 1);

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

};

/// \}
}}


BEGIN_OUT_OF_PLACE_SAVE(arc, std::shared_ptr<ml_data_internal::column_statistics>, m) {
  if(m == nullptr) {
    arc << false;
  } else {
    arc << true;

    // Save the version number
    size_t version = m->get_version();
    arc << version;

    m->save_impl(arc);
  }

} END_OUT_OF_PLACE_SAVE()


BEGIN_OUT_OF_PLACE_LOAD(arc, std::shared_ptr<ml_data_internal::column_statistics>, m) {
  bool is_not_nullptr;
  arc >> is_not_nullptr;
  if(is_not_nullptr) {

    m.reset(new ml_data_internal::column_statistics);

    size_t version;
    arc >> version;

    m->load_version(arc, version);

  } else {
    m = std::shared_ptr<ml_data_internal::column_statistics>(nullptr);
  }
} END_OUT_OF_PLACE_LOAD()


#endif
