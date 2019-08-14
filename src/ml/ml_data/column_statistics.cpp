/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/ml_data/metadata.hpp>
#include <ml/ml_data/column_statistics.hpp>
#include <core/storage/serialization/serialization_includes.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/globals/globals.hpp>
#include <model_server/lib/variant.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>

namespace turi { namespace ml_data_internal {

size_t ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD = 1024*1024;

REGISTER_GLOBAL(int64_t, ML_DATA_STATS_PARALLEL_ACCESS_THRESHOLD, true);

/**
 *  Default constructor; does nothing;
 *  Initialize from a serialization stream
 */
column_statistics::column_statistics(std::string _column_name,
                                     ml_column_mode _mode,
                                     flex_type_enum _original_column_type)
    : column_name(_column_name)
    , mode(_mode)
    , original_column_type(_original_column_type)
{}

////////////////////////////////////////////////////////////////////////////////
// Initialize the statistics
void column_statistics::initialize() {

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

void column_statistics::_finalize_threadlocal(
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
          statistics[i].mean /= std::max<size_t>(1, count);
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
        case ml_column_mode::NUMERIC_ND_VECTOR:
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
                double var = s.stdev + std::pow(mean, 2) * count * (1 - scale);
                s.stdev = std::sqrt(var / (total_row_count - 1));
              }
            }
            break;
          }
        default:
          break;
      }
    });
}

////////////////////////////////////////////////////////////////////////////////

void column_statistics::_finalize_global(
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
            sa.var_sum += std::pow(sa.mean, 2) * count * (1 - scale);
          }
          if (total_row_count > 1) {
            s.stdev = std::sqrt(sa.var_sum / (total_row_count - 1));
          } else {
            s.stdev = 0;
          }
          DASSERT_FALSE(std::isnan(s.stdev));
        }

      }
    });

}

std::pair<bool, bool> column_statistics::_get_using_flags() const {

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
    case ml_column_mode::NUMERIC_ND_VECTOR:
      using_mean_std = true;
      using_counts = false;
      break;
    case ml_column_mode::DICTIONARY:
      using_mean_std = true;
      using_counts = true;
      break;
    default:
      using_mean_std = false;
      using_counts = false;
      break;
  }

  return {using_mean_std, using_counts};
}




/// Perform final computations on the different statistics.  Must be
/// called after all the data is filled.
void column_statistics::finalize() {

  bool using_mean_std, using_counts;

  std::tie(using_mean_std, using_counts) = _get_using_flags();

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

void column_statistics::reindex(const std::vector<size_t>& new_index_map, size_t new_column_size) {

  bool using_mean_std, using_counts;
  std::tie(using_mean_std, using_counts) = _get_using_flags();

  std::vector<size_t> new_counts;
  std::vector<element_statistics> new_statistics;

  if(using_counts) {
    new_counts.assign(new_column_size, 0);
  }

  if(using_mean_std) {
    new_statistics.assign(new_column_size, element_statistics());
  }

  in_parallel([&](size_t thread_idx, size_t num_threads) {

      size_t start_index = (thread_idx * new_index_map.size()) / num_threads;
      size_t end_index = ((thread_idx + 1) * new_index_map.size()) / num_threads;

      DASSERT_EQ(counts.size(), new_index_map.size());

      for(size_t i = start_index; i < end_index; ++i) {
        size_t new_index = new_index_map[i];
        DASSERT_LT(new_index, new_column_size);

        if(using_counts)
          new_counts[new_index] = counts[i];

        if(using_mean_std)
          new_statistics[new_index] = statistics[i];
      }
    });

  counts = std::move(new_counts);
  statistics = std::move(new_statistics);
}

void column_statistics::merge_in(const column_statistics& other) {

  // The reduce stage of something.  Assumes consistent indices.
  if(this->total_row_count == 0) {
    this->counts = other.counts;
    this->statistics = other.statistics;
    this->total_row_count = other.total_row_count;
    return;
  } else if(other.total_row_count == 0) {
    return;
  }

  bool using_mean_std, using_counts;
  std::tie(using_mean_std, using_counts) = _get_using_flags();

  if(!using_counts && !using_mean_std) {
    this->total_row_count += other.total_row_count;
    return;
  }

  in_parallel([&](size_t thread_idx, size_t num_threads) {
      size_t n = std::max(this->counts.size(), this->statistics.size());

      size_t start_index = (thread_idx * n) / num_threads;
      size_t end_index = ((thread_idx + 1) * n) / num_threads;

      if(using_mean_std && other.total_row_count != 0) {

        ASSERT_EQ(n, this->statistics.size());
        ASSERT_EQ(n, other.statistics.size());

        auto sq = [](double v) { return v*v; };

        size_t count_1 = this->total_row_count;
        size_t count_2 = other.total_row_count;

        size_t new_count = this->total_row_count + other.total_row_count;

        DASSERT_GE(count_1, 1);
        DASSERT_GE(count_2, 1);

        for(size_t i = start_index; i < end_index; ++i) {

          const auto& s1 = this->statistics[i];
          const auto& s2 = other.statistics[i];

          element_statistics out;

          out.mean = (s1.mean * count_1 + s2.mean * count_2) / new_count;
          DASSERT_GE(new_count, 2);

          double m_diff_1 = s1.mean - out.mean;
          double m_diff_2 = s2.mean - out.mean;

          out.stdev = std::sqrt(
              (( sq(s1.stdev) * (count_1 - 1) + count_1 * sq(m_diff_1))
               + (sq(s2.stdev) * (count_2 - 1) + count_2 * sq(m_diff_2)))
              / (new_count - 1) );

          DASSERT_FALSE(std::isnan(out.stdev));

          this->statistics[i] = out;
        }
      }

      if(using_counts) {

        ASSERT_EQ(n, this->counts.size());
        ASSERT_EQ(n, other.counts.size());

        for(size_t i = start_index; i < end_index; ++i) {
          this->counts[i] += other.counts[i];
        }
      }

    });

  this->total_row_count += other.total_row_count;
}



////////////////////////////////////////////////////////////////////////////////

bool column_statistics::is_equal(const column_statistics* other_ptr) const {

  const column_statistics& other = *dynamic_cast<decltype(this)>(other_ptr);

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
 * Equality testing -- slow!  Use for debugging/testing
 */
bool column_statistics::operator==(const column_statistics& other) const {
  if(mode != other.mode
     || original_column_type != other.original_column_type
     || column_name != other.column_name)
    return false;

  return is_equal(&other);
}

/**
 * Inequality testing -- slow!  Use for debugging/testing
 */
bool column_statistics::operator!=(const column_statistics& other) const {
  return (!(*this == other));
}

#ifndef NDEBUG
void column_statistics::_debug_check_is_approx_equal(std::shared_ptr<column_statistics> other) const {

  DASSERT_TRUE(column_name == other->column_name);
  DASSERT_TRUE(mode == other->mode);
  DASSERT_TRUE(original_column_type == other->original_column_type);

  DASSERT_EQ(counts.size(), other->counts.size());

  // Now, we simply test that everything is correct.
  for(size_t i = 0; i < counts.size(); ++i) {
    ASSERT_EQ(counts[i], other->counts[i]);
  }

  DASSERT_EQ(statistics.size(), other->statistics.size());

  // Now, we simply test that everything is correct.
  for(size_t i = 0; i < statistics.size(); ++i) {
    ASSERT_DELTA(statistics[i].mean, other->statistics[i].mean, 1e-6);
    ASSERT_DELTA(statistics[i].stdev, other->statistics[i].stdev, 1e-6);
  }

  ASSERT_EQ(total_row_count, other->total_row_count);
}
#endif

/**
 *  Serialize the object (save).
 */
void column_statistics::save_impl(turi::oarchive& oarc) const {

  oarc << column_name << mode << original_column_type << total_row_count;
  oarc << counts << statistics;
}

/**
 *  Load the object.
 */
void column_statistics::load_version(turi::iarchive& iarc, size_t version) {

  if(version == 3) {
    iarc >> column_name >> mode >> original_column_type >> total_row_count;
    iarc >> counts >> statistics;

  } else if(version == 2) {

    std::map<std::string, variant_type> creation_options;
    variant_deep_load(creation_options, iarc);

    std::string statistics_type = variant_get_value<std::string>(creation_options.at("statistics_type"));

    ASSERT_TRUE(statistics_type == "basic-dense");

    {
      size_t version;  // The sub version used in unpacking things here.

#define __EXTRACT(var)                                                  \
      var = variant_get_value<decltype(var)>(creation_options.at(#var));

      __EXTRACT(version);
      __EXTRACT(column_name);
      __EXTRACT(mode);
      __EXTRACT(original_column_type);

#undef __EXTRACT

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
  } else {
    log_and_throw("Error loading statistics saved with this version.");
  }
}

}}
