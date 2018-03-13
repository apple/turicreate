/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/toolkits/ml_data_2/metadata.hpp>
#include <unity/toolkits/ml_data_2/statistics/basic_column_statistics.hpp>
#include <serialization/serialization_includes.hpp>
#include <sframe/sframe.hpp>
#include <globals/globals.hpp>

namespace turi { namespace v2 { namespace ml_data_internal {

size_t ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD = 1024*1024;

REGISTER_GLOBAL(int64_t, ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD, true);


////////////////////////////////////////////////////////////////////////////////
// Initialize the statistics
void basic_column_statistics::initialize() {

  size_t num_threads = thread::cpu_count();

  total_row_count = 0;
  statistics.clear();

  // Initialize the statistics accumulators.
  by_thread_element_counts.resize(num_threads);
  by_thread_row_counts.assign(num_threads, 0);
  by_thread_mean_var_acc.resize(num_threads);

  // Pull in the parallel_threshhold point.
  parallel_threshhold = ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD;
}


////////////////////////////////////////////////////////////////////////////////

/// Update categorical statistics for a batch of categorical indices.
void basic_column_statistics::update_categorical_statistics(
    size_t thread_idx, const std::vector<size_t>& cat_index_vect) {

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

    ++(counts[idx]);
  }

  if(cv_idx != cat_index_vect.size() ) {
    for(; cv_idx < cat_index_vect.size(); ++cv_idx) {
      size_t idx = cat_index_vect[cv_idx] - parallel_threshhold;
      check_global_array_size(idx, global_element_counts);

      std::lock_guard<simple_spinlock> el_lg(global_element_locks[get_lock_index(idx)]);
      
      ++(global_element_counts[idx]);
    }
  }

  ++by_thread_row_counts[thread_idx];
}

////////////////////////////////////////////////////////////////////////////////

// Update categorical statistics for a batch of real values.
void basic_column_statistics::update_numeric_statistics(
    size_t thread_idx, const std::vector<double>& value_vect) {

  DASSERT_TRUE(mode == ml_column_mode::NUMERIC
               || mode == ml_column_mode::NUMERIC_VECTOR);

  // Silently ignore columns of empty vectors.  Note that all the
  // vectors in a column must be empty for this to work.
  if(value_vect.empty())
    return;

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
void basic_column_statistics::update_dict_statistics(
    size_t thread_idx, const std::vector<std::pair<size_t, double> >& dict) {

  DASSERT_TRUE(mode == ml_column_mode::DICTIONARY);

  // The input array is sorted here.
  DASSERT_TRUE(std::is_sorted(dict.begin(), dict.end(),
                              [](std::pair<size_t, double> p1, std::pair<size_t, double> p2) {
                                return p1.first < p2.first;
                              }));

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

      check_global_array_size(idx, global_element_counts);
      check_global_array_size(idx, global_mean_var_acc);

      std::lock_guard<simple_spinlock> el_lg(global_element_locks[get_lock_index(idx)]);
      
      update_f(global_element_counts[idx],
               global_mean_var_acc[idx].mean,
               global_mean_var_acc[idx].var_sum, v);
    }
  }

  ++row_count;
}


void basic_column_statistics::_finalize_threadlocal(
    size_t in_threads_size, bool using_counts, bool using_mean_std) {

  ////////////////////////////////////////////////////////////////////////////////
  //
  // Calculating STDEV properly.
  //
  // The stdev is temporarily a pooled variance calculation -- each of
  // the individual variances are done with reference to different
  // means, so they must be combined properly to reflect that. See
  //
  // http://stats.stackexchange.com/questions/43159/how-to-calculate-pooled-variance-of-two-groups-given-known-group-variances-mean
  //
  // For the proper way of doing this.
  //
  // In summary, we need to use:
  //
  //   v_total += v_total + count[i] * (mean[i] - m)^2
  //
  // where m is the overall mean, and mean[i] is the mean of section
  // i.

  // Pass 1: total counts,

  in_parallel([&](size_t thread_idx, size_t n_threads) GL_GCC_ONLY(GL_HOT_FLATTEN) {

      const size_t start_idx = (thread_idx * in_threads_size) / n_threads;
      const size_t end_idx = ((thread_idx + 1) * in_threads_size) / n_threads;

      if(using_counts) {
        for(const auto& count_v : by_thread_element_counts) {
          for(size_t i = start_idx; i < std::min(count_v.size(), end_idx); ++i) {
            counts[i] += count_v[i];
          }
        }
      }

      if(using_mean_std) {
        DASSERT_EQ(by_thread_element_counts.size(), by_thread_element_counts.size());

        for(size_t src_idx = 0; src_idx < by_thread_mean_var_acc.size(); ++src_idx) {

          if(using_counts) {
            DASSERT_EQ(by_thread_mean_var_acc.size(), by_thread_element_counts.size());
            DASSERT_EQ(by_thread_mean_var_acc[src_idx].size(), by_thread_element_counts[src_idx].size());
          } else {
            DASSERT_EQ(by_thread_mean_var_acc.size(), by_thread_row_counts.size());
          }

          const auto& mean_std_v = by_thread_mean_var_acc[src_idx];

          for(size_t i = start_idx; i < std::min(mean_std_v.size(), end_idx); ++i) {
            size_t count = (using_counts
                            ? by_thread_element_counts[src_idx][i]
                            : by_thread_row_counts[src_idx]);

            // The mean is now the total.  We'll combine these all later.
            statistics[i].mean += mean_std_v[i].mean * count;
          }
        }

        // Turn the totals into means
        for(size_t i = start_idx; i < end_idx; ++i) {
          size_t count = (using_counts ? counts[i] : total_row_count);
          statistics[i].mean /= count;
        }

        // Now go through the standard deviation part.
        for(size_t src_idx = 0; src_idx < by_thread_mean_var_acc.size(); ++src_idx) {

          const auto& mean_std_v = by_thread_mean_var_acc[src_idx];

          for(size_t i = start_idx; i < std::min(mean_std_v.size(), end_idx); ++i) {

            size_t count = (using_counts
                            ? by_thread_element_counts[src_idx][i]
                            : by_thread_row_counts[src_idx]);
            
            // The difference in mean between this one and previous.
            double m_diff = (mean_std_v[i].mean - statistics[i].mean);

            // statistics[].stdev is temporarily a weighted total
            // variance
            statistics[i].stdev += mean_std_v[i].var_sum + count * std::pow(m_diff, 2);
          }
        }
      } // End if using_mean_std


      // Mode-dependent post-processing
      switch(mode) {
        case ml_column_mode::CATEGORICAL:
        case ml_column_mode::CATEGORICAL_VECTOR:
          {
            DASSERT_TRUE(using_counts);
            DASSERT_FALSE(using_mean_std);

            // We're done; don't need to deal with these here.
            break;
          }

        case ml_column_mode::NUMERIC:
        case ml_column_mode::NUMERIC_VECTOR:
          {
            DASSERT_FALSE(using_counts);
            DASSERT_TRUE(using_mean_std);

            if(total_row_count > 1) {

              for(size_t i = start_idx; i < end_idx; ++i) {
                element_statistics& s = statistics[i];
                s.stdev = std::sqrt(s.stdev / (total_row_count - 1));
              }
            }

            break;
          }

        case ml_column_mode::DICTIONARY:
          {
            DASSERT_TRUE(using_counts);
            DASSERT_TRUE(using_mean_std);

            // Stable aggregation of sample means and variances.
            // --------------------------------------------------------------------
            // See update_dict_statistics for explanation.
            if (total_row_count > 1) {

              for(size_t i = start_idx; i < end_idx; ++i) {
                element_statistics& s = statistics[i];
                double count = counts[i];
                double mean  = s.mean;
                double scale = count / total_row_count;

                s.mean = mean * scale;

                // stdev is the total variance here for the seen elements.
                double var = s.stdev + std::pow(mean, 2) * scale * (total_row_count - count);
                s.stdev = std::sqrt(var / (total_row_count - 1));
              }
            }
            break;
          }
        default:
          ASSERT_TRUE(false);
      }
    });
}

////////////////////////////////////////////////////////////////////////////////

void basic_column_statistics::_finalize_global(
    size_t in_threads_size, bool using_counts, bool using_mean_std) {

  in_parallel([&](size_t thread_idx, size_t n_threads) {

      if(using_counts) {

        size_t start_idx = (thread_idx * global_element_counts.size()) / n_threads;
        size_t end_idx = ((thread_idx + 1) * global_element_counts.size()) / n_threads;

        std::copy(global_element_counts.begin() + start_idx,
                  global_element_counts.begin() + end_idx,
                  counts.begin() + start_idx + parallel_threshhold);
      }

      if(using_mean_std) {

        size_t start_idx = (thread_idx * global_mean_var_acc.size()) / n_threads;
        size_t end_idx = ((thread_idx + 1) * global_mean_var_acc.size()) / n_threads;
        
        for(size_t i = start_idx; i < end_idx; ++i) {
          size_t full_idx = parallel_threshhold + i;

          element_statistics& s = statistics[full_idx];
          element_statistics_accumulator& sa = global_mean_var_acc[i];
          
          if(using_counts) {

            // Need to adjust for the unseen elements here, which are zero.
            
            double count = counts[full_idx];
            double scale = count / total_row_count;

            s.mean = sa.mean * scale;

            // Adjust for many of the elements being zero
            sa.var_sum += std::pow(sa.mean, 2) * scale * (total_row_count - count);
          }
          
          s.stdev = std::sqrt(sa.var_sum / (total_row_count - 1));
        }

      }
    });

}

/// Perform final computations on the different statistics.  Must be
/// called after all the data is filled.
void basic_column_statistics::finalize() {

  bool using_mean_std = true;
  bool using_counts = true;

  switch(mode) {
    case ml_column_mode::CATEGORICAL:
    case ml_column_mode::CATEGORICAL_VECTOR:
      using_mean_std = false;
      using_counts = true;
      break;
    case ml_column_mode::NUMERIC:
    case ml_column_mode::NUMERIC_VECTOR:
      using_mean_std = true;
      using_counts = false;
      break;
    case ml_column_mode::DICTIONARY:
      using_mean_std = true;
      using_counts = true;
      break;
    default:
      // No statistics.   E.g. untranslated column.
      statistics.clear();
      counts.clear();
      return;
  }

  // Ensure the statistics column is the right size before we move
  // everything from the accumulator to it.
  size_t final_size = 0;
  size_t in_threads_size = 0; 
  total_row_count = 0;

  for(size_t row_count : by_thread_row_counts)
    total_row_count += row_count;

  if(using_counts) {

    if(!global_element_counts.empty()) {
      DASSERT_LE(global_size, global_element_counts.size());
      global_element_counts.resize(global_size); 
      final_size      = parallel_threshhold + global_element_counts.size();
      in_threads_size = parallel_threshhold; 
    } else {
      for(const auto& v : by_thread_element_counts) {
        final_size      = std::max(final_size, v.size());
        in_threads_size = std::max(in_threads_size, v.size());
      }
    }
  }

  if(using_mean_std) {

    if(!global_mean_var_acc.empty()) {
      DASSERT_LE(global_size, global_mean_var_acc.size());
      global_mean_var_acc.resize(global_size);
      final_size      = std::max(final_size, parallel_threshhold + global_mean_var_acc.size());
      in_threads_size = parallel_threshhold; 
    } else {
      for(const auto& v : by_thread_mean_var_acc) {
        final_size      = std::max(final_size, v.size());
        in_threads_size = std::max(in_threads_size, v.size());
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Now resize the counts and the statistics.
  
  if(using_counts) {
    counts.assign(final_size, 0);
  }

  if(using_mean_std) {
    statistics.clear();
    statistics.resize(final_size);
  }

  // Finalize the thread local buffers
  _finalize_threadlocal(in_threads_size, using_counts, using_mean_std);

  // Finalize the thread local buffers
  if(!global_mean_var_acc.empty() || !global_element_counts.empty()) {
    _finalize_global(in_threads_size, using_counts, using_mean_std);
  }
  
  // Clear out the thread-local accumulators.
  {
    decltype(by_thread_mean_var_acc) v;
    by_thread_mean_var_acc.swap(v);
  }

  {
    decltype(by_thread_row_counts) v;
    by_thread_row_counts.swap(v);
  }

  {
    decltype(global_mean_var_acc) v;
    global_mean_var_acc.swap(v);
  }

  {
    decltype(global_element_counts) v;
    global_element_counts.swap(v);
  }
}

////////////////////////////////////////////////////////////////////////////////

bool basic_column_statistics::is_equal(const column_statistics* other_ptr) const {

  const basic_column_statistics& other = *dynamic_cast<decltype(this)>(other_ptr);

  if(&other == this)
    return true;

  if(statistics.size() != other.statistics.size())
    return false;

  if(total_row_count != other.total_row_count)
    return false;

  if(counts.empty() != other.counts.empty())
    return false;

  if(statistics.empty() != other.statistics.empty())
    return false;
  
  for(size_t i = 0; i < statistics.size(); ++i) {
    if(!counts.empty() && counts[i] != other.counts[i])
      return false; 

    if(!statistics.empty()) {
      const element_statistics& s1 = statistics[i];
      const element_statistics& s2 = other.statistics[i];
      if(s1.mean != s2.mean || s1.stdev != s2.stdev)
        return false;
    }
  }

  return true;
}

/**
 *  Serialize the object (save).
 */
void basic_column_statistics::save_impl(turi::oarchive& oarc) const {

  std::map<std::string, flexible_type> data;

  data["total_row_count"] = total_row_count;

  variant_deep_save(data, oarc);

  oarc << counts << statistics;
}

/**
 *  Load the object.
 */
void basic_column_statistics::load_version(turi::iarchive& iarc, size_t version) {

  std::map<std::string, flexible_type> data;
  variant_deep_load(data, iarc);

  switch(version) {
    case 1: {

      total_row_count = variant_get_value<size_t>(data.at("total_row_count"));

      // Previous version just stored this large struct; now we need to

      struct alt_element_statistics : public turi::IS_POD_TYPE {
        size_t count = 0;  /**< Count of column. */
        double mean  = 0;  /**< Mean of column. */
        double stdev = 0;  /**< Stdev of column. */
      };

      std::vector<alt_element_statistics> alt_stats;
      iarc >> alt_stats;

      size_t n = alt_stats.size();

      // Now, convert everything over.
      counts.resize(n);
      statistics.resize(n);
      for(size_t i = 0; i < n; ++i) {
        counts[i] = alt_stats[i].count;
        statistics[i].mean = alt_stats[i].mean;
        statistics[i].stdev = alt_stats[i].stdev;
      }

      break;
    }

    default: {
      total_row_count = variant_get_value<size_t>(data.at("total_row_count"));
      iarc >> counts >> statistics;
      break;
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
void basic_column_statistics::set_data(const std::map<std::string, variant_type>& params) {
  
  if(params.count("mean")) {

    ASSERT_TRUE(params.count("stdev"));
    
    std::vector<double> mv;
    mv = variant_get_value<decltype(mv)>(params.at("mean"));
  
    std::vector<double> sv;
    sv = variant_get_value<decltype(sv)>(params.at("stdev"));

    ASSERT_EQ(mv.size(), sv.size());

    statistics.resize(mv.size());
    for(size_t i = 0; i < mv.size(); ++i) {
      statistics[i].mean = mv[i]; 
      statistics[i].stdev = sv[i]; 
    }
  }
  
  if(params.count("counts")) {
    
    counts = variant_get_value<decltype(counts)>(params.at("counts"));

    if(!statistics.empty()) {
      ASSERT_EQ(statistics.size(), counts.size());
    }
  }

  if(params.count("total_row_count")) {
    total_row_count = variant_get_value<size_t>(params.at("total_row_count"));
  }
}


}}}
