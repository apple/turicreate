/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_ITEM_SIMILARITY_LOOKUP_CONSTRUCTION_H_
#define TURI_UNITY_ITEM_SIMILARITY_LOOKUP_CONSTRUCTION_H_

#include <unity/toolkits/sparse_similarity/item_processing.hpp>
#include <unity/toolkits/sparse_similarity/index_mapper.hpp>
#include <unity/toolkits/sparse_similarity/sparse_similarity_lookup.hpp>
#include <unity/toolkits/sparse_similarity/similarities.hpp>
#include <unity/toolkits/sparse_similarity/utilities.hpp>
#include <unity/toolkits/sparse_similarity/neighbor_search.hpp>
#include <unity/toolkits/sparse_similarity/sliced_itemitem_matrix.hpp>
#include <table_printer/table_printer.hpp>
#include <generics/sparse_parallel_2d_array.hpp>
#include <sframe/sframe.hpp>
#include <sframe/sframe_iterators.hpp>
#include <util/logit_math.hpp>
#include <util/cityhash_tc.hpp>
#include <util/dense_bitset.hpp>
#include <util/sys_util.hpp>
#include <parallel/pthread_tools.hpp>

namespace turi { namespace sparse_sim {

////////////////////////////////////////////////////////////////////////////////

/** The main class for training and actually implementing the sparse
 *  similarity lookup stuff.
 */
template <typename SimilarityType>
class sparse_similarity_lookup_impl : public sparse_similarity_lookup {
 private:

  ////////////////////////////////////////////////////////////////////////////////
  // Construction routines.

 public:
  sparse_similarity_lookup_impl(SimilarityType&& _similarity,
                                const std::map<std::string, flexible_type>& _options)
      : similarity(_similarity)
      , max_item_neighborhood_size(_options.at("max_item_neighborhood_size"))
      , item_prediction_buffers_by_thread(thread::cpu_count())
  {
    this->options = _options;
  }

  std::string similarity_name() const {
    return SimilarityType::name();
  }

 private:

  ////////////////////////////////////////////////////////////////////////////////
  // Type definitions based on the similarity type.

  typedef typename SimilarityType::item_data_type item_data_type;
  typedef typename SimilarityType::interaction_data_type interaction_data_type;
  typedef typename SimilarityType::final_item_data_type final_item_data_type;
  typedef typename SimilarityType::final_interaction_data_type final_interaction_data_type;
  typedef typename SimilarityType::prediction_accumulation_type prediction_accumulation_type;
  typedef std::vector<item_processing_info<SimilarityType> > item_info_vector;

  // If the final_item_data_type is unused, then we can ignore it.
  static constexpr bool use_final_item_data() {
    return sparse_sim::use_final_item_data<SimilarityType>();
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Global containers needed for the various processing routines.

  // The main similarity type used.
  SimilarityType similarity;
  size_t total_num_items = 0;

  ////////////////////////////////////////////////////////////////////////////////
  // The lookup table data.  This information is used in the
  // prediction and scoring routines.

  typedef std::pair<size_t, final_interaction_data_type> interaction_info_type;

  std::vector<size_t> item_neighbor_boundaries;
  std::vector<interaction_info_type> item_interaction_data;

  size_t max_item_neighborhood_size = 0;

  // This is pulled in from the final_item_data.  Some similarity
  // types use this for processing.
  std::vector<final_item_data_type> final_item_data;

  ////////////////////////////////////////////////////////////////////////////////
  // Stuff for proper progress printing and tracking.

  /** Tracks the progress of the entire process.
   *
   */
  struct _progress_tracker {
    
    _progress_tracker(size_t _num_items)
        : num_items(_num_items)
        , table({{"Elapsed Time (Constructing Lookups)", 0},
            {"Total % Complete", 0},
            {"Items Processed", 0}})
    {
    }
    
    void print_header() { table.print_header(); }
    void print_break()  { table.print_line_break(); }
    void print_footer() {
      item_pair_count = num_items * num_items;
      double percent_complete = 100.0;
      
      table.print_row(
          progress_time(), 
          percent_complete,
          num_items);

      table.print_footer();
    }
        
    GL_HOT_INLINE
    void increment_item_counter(size_t counter = 1) { 
      item_pair_count += counter;
      
      if(UNLIKELY(table.time_for_next_row() && !in_print_next_row)) {
        // Because this function can be called a lot, we just set a
        // flag to tell other threads to hold off.  It will still be
        // ordered correctly -- if we miss a few entries, that's fine.
        // The situation we want to prevent is every thread suddenly
        // calling _print_next_row because time_for_next_row() is
        // true; While _print_next_row is fine with this, it this
        // would slow things down in an inner loop.  Thus we use the
        // atomic in_print_next_row flag to deter this situation.
        in_print_next_row = true;
        _print_next_row();
        in_print_next_row = false; 
      }
    }
    
   private:

    GL_HOT_NOINLINE
    void _print_next_row() {

      // This lock prevents multiple threads from accessing this at
      // the same time, which means things will always be in order.
      std::unique_lock<simple_spinlock> lg(_print_next_row_counter_lock, std::defer_lock);

      bool acquired_lock = lg.try_lock();

      if(!acquired_lock) {
        return; 
      }
      
      size_t _item_pair_count = item_pair_count;
      size_t items_processed = _item_pair_count / num_items;

      double n_total_items = double(num_items) * num_items;
      double prop_complete = std::min(1.0, double(_item_pair_count) / n_total_items);

      // Approximate to the nearest 0.25%
      double percent_complete = std::floor(4 * 100.0 * prop_complete) / 4;
      
      table.print_timed_progress_row(
          progress_time(), 
          percent_complete,
          items_processed);
    }
    
    size_t num_items = 0; 
    atomic<size_t> item_pair_count;
    atomic<int> in_print_next_row;
    simple_spinlock _print_next_row_counter_lock;
    table_printer table;
  };
  
  ////////////////////////////////////////////////////////////////////////////////
  // Management functions for building the lookup tables.

  // Stuff needed for building the intermediate structures.
  std::vector<uint32_t> item_neighbor_counts;
  std::vector<simple_spinlock> item_interaction_locks;

  /**  Initialize the item lookup tables.  Call before the item
   *   lookups are used.
   */
  void init_item_lookups(size_t num_items, const std::vector<final_item_data_type>& _final_item_data) {

    // Assign these. 
    total_num_items = num_items;
    item_neighbor_counts.assign(total_num_items, 0);
    item_interaction_locks.resize(total_num_items);

 ALLOCATION_RETRY:
    try {
      item_interaction_data.reserve(total_num_items * max_item_neighborhood_size);
        
    } catch(const std::bad_alloc& e) {
      // Attempt to handle the allocations in this realm properly.
      // If it drops to ridiculously low item similarity numbers,
      // then we've got a problem, but usually when this happens the
      // user has set max_item_neighborhood_size to way too large of
      // a number, like the total number of items.
      if(max_item_neighborhood_size >= 16) {

        size_t new_max_item_neighborhood_size = std::min<size_t>(64, max_item_neighborhood_size / 2); 
          
        logstream(LOG_ERROR)
            << "Error allocating proper lookup tables with max_item_neighborhood_size = "
            << max_item_neighborhood_size << "; reattempting with max_item_neighborhood_size = "
            << new_max_item_neighborhood_size << "." << std::endl;
              
        max_item_neighborhood_size = new_max_item_neighborhood_size;
        options.at("max_item_neighborhood_size") = max_item_neighborhood_size;
        goto ALLOCATION_RETRY;
        
      } else {
        std::ostringstream ss;
        ss << "Out-of-Memory error allocating proper lookup tables with max_item_neighborhood_size = "
           << max_item_neighborhood_size << ".  This currently requires a lookup table of " 
           << (max_item_neighborhood_size * total_num_items * 16)
           << " bytes.  Please attempt with fewer items or use a machine with more memory."
           << std::endl;
        log_and_throw(ss.str().c_str());
      }
    }

    item_interaction_data.assign(total_num_items * max_item_neighborhood_size,
                                 {0, final_interaction_data_type()});
    
    // Copy over the item vertex data.
    if(use_final_item_data()) {
      ASSERT_EQ(_final_item_data.size(), num_items);
      final_item_data = _final_item_data;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////

  /**  Insert a new item into the lookups.  Is completely threadsafe.
   */
  GL_HOT_INLINE_FLATTEN
  void insert_into_lookup(
      size_t item_a, size_t item_b,
      const final_interaction_data_type& value) {

    std::pair<size_t, final_interaction_data_type> p = {item_b, value};
    auto& count = item_neighbor_counts[item_a];

    final_item_data_type _unused;

    auto item_comparitor = [&](const interaction_info_type& p1, const interaction_info_type& p2)
        GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

      DASSERT_LT(item_a, total_num_items);
      DASSERT_LT(p1.first, total_num_items);
      DASSERT_LT(p2.first, total_num_items);

      return similarity.compare_interaction_values(
          p1.second,
          p2.second,
          use_final_item_data() ? final_item_data[item_a] : _unused,
          use_final_item_data() ? final_item_data[p1.first] : _unused,
          use_final_item_data() ? final_item_data[p2.first] : _unused);
    };

    // Before locking the item, make sure that the new item is likely
    // to actually go on the heap.
    if(count == max_item_neighborhood_size
       && !(item_comparitor(p, item_interaction_data[item_a * max_item_neighborhood_size + count - 1]))) {
      return;
    } else {

      // Put this in a separate function so the above is much more
      // easy to inline.  Below involes a lot of math.
      auto insert_on_heap = [&]() GL_GCC_ONLY(GL_HOT_NOINLINE) { 
        
        // Now, lock the item.
        std::lock_guard<simple_spinlock> lg(item_interaction_locks[item_a]);

        // Make sure this item is not already in there.
#ifdef NDEBUG
        {
          for(size_t i = 0; i < count; ++i) {
            DASSERT_NE(item_interaction_data[item_a*max_item_neighborhood_size + i].first, p.first);
          }
        }
#endif

        if(count < max_item_neighborhood_size) {
          item_interaction_data[item_a*max_item_neighborhood_size + count] = p;
          ++count;

          if(count == max_item_neighborhood_size) {
            std::make_heap(item_interaction_data.begin() + (item_a*max_item_neighborhood_size),
                           item_interaction_data.begin() + ((item_a + 1)*max_item_neighborhood_size),
                           item_comparitor);
          }
        } else {
          if(LIKELY(item_comparitor(p, item_interaction_data[item_a * max_item_neighborhood_size + count - 1]))) {

            item_interaction_data[item_a * max_item_neighborhood_size + count - 1] = p;
            std::push_heap(item_interaction_data.begin() + (item_a*max_item_neighborhood_size),
                           item_interaction_data.begin() + ((item_a + 1)*max_item_neighborhood_size),
                           item_comparitor);
            std::pop_heap(item_interaction_data.begin() + (item_a*max_item_neighborhood_size),
                          item_interaction_data.begin() + ((item_a + 1)*max_item_neighborhood_size),
                          item_comparitor);

            // Check that the popped item is not better than the new one.
            DASSERT_FALSE(item_comparitor(item_interaction_data[item_a * max_item_neighborhood_size + count - 1], p));
          }
        }
      };

      insert_on_heap(); 
    }
  }
  
  /** Finalize the lookup tables.  After calling this, things are
   *  ready to be used.
   */
  GL_HOT_NOINLINE_FLATTEN
  void finalize_lookups() {
    double threshold = options.at("threshold");

    // First, go through and remove all the empty space.
    size_t current_position = 0;
    item_neighbor_boundaries.resize(total_num_items + 1);
    for(size_t i = 0; i < total_num_items; ++i) {
      item_neighbor_boundaries[i] = current_position;

      // Apply threshholding.
      size_t write_pos = 0;
      for(size_t j = 0; j < item_neighbor_counts[i]; ++j) {
        if(item_interaction_data[i * max_item_neighborhood_size + j].second > threshold) {
          item_interaction_data[current_position + write_pos]
              = item_interaction_data[i * max_item_neighborhood_size + j];
          ++write_pos;
        }
      }

      current_position += write_pos;
    }

    item_neighbor_boundaries[total_num_items] = current_position;
    item_interaction_data.resize(current_position);
    item_neighbor_counts.clear();
    item_neighbor_counts.shrink_to_fit();

    // Now, sort each max_item_neighborhood_size spot.
    atomic<size_t> current_idx = 0;

    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {
        while(true) {
          size_t idx = (++current_idx) - 1;
          if(idx >= total_num_items) {
            break;
          }

          std::sort(item_interaction_data.begin() + item_neighbor_boundaries[idx],
                    item_interaction_data.begin() + item_neighbor_boundaries[idx + 1],
                    [](const std::pair<size_t, final_interaction_data_type>& p1,
                       const std::pair<size_t, final_interaction_data_type>& p2) {
                      return p1.first < p2.first;
                    });
        }
      });
  }

  ////////////////////////////////////////////////////////////////////////////////

  /** An internal routine
   *
   */
  void setup_by_raw_similarity(
      size_t num_items,
      const flex_list& item_data,
      const sframe& _interaction_data,
      const std::string& item_column,
      const std::string& similar_item_column,
      const std::string& similarity_column,
      bool add_reverse = false) {

    total_num_items = num_items;

    {
      std::vector<final_item_data_type> _final_item_data;

      if(use_final_item_data()) {
        ASSERT_EQ(item_data.size(), total_num_items);

        _final_item_data.resize(total_num_items);
        for(size_t i = 0; i < total_num_items; ++i) {
          similarity.import_final_item_value(_final_item_data[i], item_data[i]);
        }
      }

      init_item_lookups(num_items, _final_item_data);
    }

    // Pretty much no magic here.  Just read it all out and dump it
    // into the item_interaction_data lookup.

    sframe interaction_data = _interaction_data.select_columns(
        {item_column, similar_item_column, similarity_column});

    if(interaction_data.column_type(0) != flex_type_enum::INTEGER) {
      log_and_throw("Items in provided data must be integers in the set {0, ..., num_items}.");
    }

    if(interaction_data.column_type(1) != flex_type_enum::INTEGER) {
      log_and_throw("Similar items in provided data must be integers in the set {0, ..., num_items}.");
    }

    parallel_sframe_iterator_initializer it_init(interaction_data);

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        final_interaction_data_type final_interaction_data = final_interaction_data_type();

        for(parallel_sframe_iterator it(it_init, thread_idx, num_threads); !it.done(); ++it) {
          size_t item_a = it.value(0);
          size_t item_b = it.value(1);
          if(item_a == item_b) continue;
          
          const flexible_type& sim_value = it.value(2);

          if(item_a >= num_items || item_b >= num_items) {
            auto raise_error = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {
              std::ostringstream ss;
              ss << "Out of range item index encountered in row " << it.row_index()
                 << "; item index = " << std::max(item_a, item_b) << " >= " << num_items << " = num_items."
                 << std::endl;
              log_and_throw(ss.str().c_str());
            };

            raise_error();
          }
          
          similarity.import_final_interaction_value(final_interaction_data, sim_value);
          insert_into_lookup(item_a, item_b, final_interaction_data);
          
          if(add_reverse) {
            insert_into_lookup(item_b, item_a, final_interaction_data);
          }
        }
      });

    // Now, finalize the lookups and we're done!
    finalize_lookups();
  }

  ////////////////////////////////////////////////////////////////////////////////

  /** Estimates the density of the matrix, so we can get an accurate
   *  picture of how many passes will be needed to properly fit
   *  everything in memory.
   */
   double estimate_sparse_matrix_density(
      const item_info_vector& item_info,
      const std::vector<size_t>& items_per_user) {

    random::seed(0);

    size_t degree_approximation_threshold = options.at("degree_approximation_threshold");
    size_t num_items = item_info.size();

    // This is a somewhat challenging one to figure out, and the
    // critical value is the expected density of the matrix.  To get
    // this, we choose a number of edges as specific points, then go
    // through and calculate the probability that each of these has
    // been hit on a pass through the data.
    //
    // We have the exact marginal probability an item i is chosen from
    // the item counts.  Call this p_i.
    //
    // For a given user that has rated $n_u$ items, we assume that the
    // items are chosen iid according the probability an item is
    // chosen is sampling p_i without replacement.  For ease of use,
    // we relax the "without replacement", and just assume it's chosen
    // with replacement.  Then the probability a given user has chosen
    // item i in his full collection is $1 - (1 - p_i)^n_u$.
    //
    // Now, we are actually looking at interactions, so a given edge
    // has two parts -- (i, j).  Furthermore, we limit the number of
    // edges considered by one user to degree_approximation_threshold;
    // which we need to account for as well.  Thus, the probability
    // that this edge is hit by user u is:
    //
    //     r_{iju} (1 - (1 - p_i)^{n_u}) (1 - (1 - p_j)^{n*_u})
    //
    // where {n^*_u} = min(degree_approximation_threshold, n_u).
    //
    // Then, the probability q_{ij} that a given edge (i, j) is hit is
    // given by:
    //
    //     q_{ij} = 1 - \prod_u (1 - r_{iju})
    //
    // Using logs for numerical stability:
    //
    // log(r_{iju}) = (log(1 - exp(n_u * log(1 - p_i) ) )
    //                 + log(1 - exp(n^*_u * log(1 - p_j) ) ) )
    //
    //              = log1me(n_u * log(1 - p_i) ) + log1me(n*_u * log(1 - p_j) )
    //
    // where log1me is the numerically stable version of log(1 - exp(x)).
    //
    // Thus log(1 - q_ij) = sum_u log(1 - exp(log(r_{iju})) )
    //                    = sum_u log1me( log1me(n_u * log(1 - p_i) )
    //                                      + log1me(n*_u * log(1 - p_j) ) )
    //

    ////////////////////////////////////////////////////////////////////////////////
    // Calculate log(1 - p_i) for all items.

    std::vector<double> item_log1mp(num_items);

    size_t total_item_counts = 0;

    for(const auto& ii : item_info) {
      total_item_counts += ii.num_users;
    }

    // Calculate log(1 - p_i) for all i.
    parallel_for(size_t(0), num_items, [&](size_t i) {
        DASSERT_GT(item_info[i].num_users, 0);
        double hit_p = double(item_info[i].num_users) / total_item_counts;
        item_log1mp[i] = std::log1p(-hit_p);
      });

    ////////////////////////////////////////////////////////////////////////////////
    // create function to calculate log(1 - r_iju) given i, j, u.
    // This is what is accumulated as things grow.

    // This is where all the math happens
    auto calc_log_hit_prob_accumulation = [&](
        size_t i, size_t j, size_t user_item_count) {

      size_t clipped_user_item_count
      = std::min(user_item_count, degree_approximation_threshold);

      double log_riju = (
          log1me(std::min<double>(-1e-16, clipped_user_item_count * item_log1mp[i]))
          + log1me(std::min<double>(-1e-16, clipped_user_item_count * item_log1mp[j])));

      // std::ostringstream ss;
      // ss << "hit_probability (" << i << ", " << j << ", " << user_item_count << ") = "
      //    << std::exp(log_riju) << "; log prob miss = " << log1me(log_riju)
      //    << "; item_log1mp[i] = " << item_log1mp[i]
      //    << "; item_log1mp[j] = " << item_log1mp[j]
      //    << "; user_item_count[j] = " << user_item_count << std::endl;

      // std::cerr << ss.str() << std::endl;

      return log1me(std::min<double>(1e-16, log_riju));
    };

    struct _sample {
      size_t i = 0, j = 0;
      double log_1_m_q = 0;
      double estimated_prob = 0;
    };

    size_t num_samples = options.at("sparse_density_estimation_sample_size");

    // Don't need an insane number of samples if we don't have that
    // many passes through the data to consider.
    num_samples = std::min(num_samples, num_items * num_items);

    // Sample a number of locations here.
    std::vector<_sample> samples(num_samples);

    // Go through and do the estimation
    in_parallel(
        [&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {

          size_t sample_start_idx = (thread_idx * samples.size()) / num_threads;
          size_t sample_end_idx = ((thread_idx + 1) * samples.size()) / num_threads;

          for(size_t s_idx = sample_start_idx; s_idx < sample_end_idx; ++s_idx) {
            auto& s = samples[s_idx];
            s.i = random::fast_uniform<size_t>(0, num_items - 1);
            s.j = random::fast_uniform<size_t>(0, num_items - 1);
            s.log_1_m_q = 0;
          }

          double mult_factor = 1;

          std::vector<size_t> item_count_distribution;

          static constexpr size_t user_count_dist_sample_size = 5000;

          // If we have a ton of users, then we subsample this part here.
          if(items_per_user.size() > user_count_dist_sample_size) {
            // Do an iid sample here.
            item_count_distribution.resize(user_count_dist_sample_size);
            for(size_t i = 0; i < user_count_dist_sample_size; ++i) {
              size_t idx = random::fast_uniform<size_t>(0, items_per_user.size() - 1);
              item_count_distribution[i] = items_per_user[idx];
            }
            mult_factor = double(items_per_user.size()) / item_count_distribution.size();
          } else {
            item_count_distribution = items_per_user;
          }

          for(size_t idx = 0; idx < item_count_distribution.size(); ++idx) {
            for(size_t s_idx = sample_start_idx; s_idx < sample_end_idx; ++s_idx) {
              auto& s = samples[s_idx];
              size_t count = item_count_distribution[idx];
              s.log_1_m_q += mult_factor * calc_log_hit_prob_accumulation(s.i, s.j, count);
            }
          }

          for(size_t s_idx = sample_start_idx; s_idx < sample_end_idx; ++s_idx) {
            auto& s = samples[s_idx];
            s.estimated_prob = -std::expm1(s.log_1_m_q);
            DASSERT_LE(s.estimated_prob, 1.0 + 1e-6);
            DASSERT_GE(s.estimated_prob, 0.0 - 1e-6);
          }
        });

    double total_prob = 0;

    for(const _sample& s : samples) {
      total_prob += s.estimated_prob;
    }

    double estimated_density = total_prob / samples.size();

    return estimated_density;
  }

  /**  Calculate the slice structure of the full matrix.
   *
   */
  std::vector<size_t> calculate_slice_structure(
      size_t num_items, size_t max_slices, double bytes_per_interaction) {

    size_t target_memory_usage = options.at("target_memory_usage");

    size_t target_num_items_per_slice
        = std::ceil(target_memory_usage / bytes_per_interaction);

    // Make sure it's at least 1.
    target_num_items_per_slice = std::max(num_items, target_num_items_per_slice);

    auto slice_boundaries = calculate_upper_triangular_slice_structure(
        num_items, target_num_items_per_slice, max_slices);

    return slice_boundaries;
  }

  /** Bytes per item in the dense case.
   */
  double bytes_per_item_dense() {
    return sizeof(interaction_data_type);
  }

  /** Bytes per item in the sparse case.
   */
  double bytes_per_item_sparse(const item_info_vector& item_info,
                             const std::vector<size_t>& items_per_user) {

    logstream(LOG_INFO) << "Estimating relative cost of doing sparse lookups vs. dense lookups."
                        << std::endl;

    double estimated_density = estimate_sparse_matrix_density(item_info, items_per_user);

    logstream(LOG_INFO) << "Estimated sparse matrix density at "
                        << estimated_density << ". " << std::endl;

    // The 1.7 here comes from the average memory usage per element factor of
    // google's dense_hash_set table.  We store 1 index and 1 edge per element.
    double estimated_memory_usage_per_element
        = estimated_density * (1.7 * (sizeof(size_t) + sizeof(interaction_data_type) ));

    return estimated_memory_usage_per_element;
  }

  ////////////////////////////////////////////////////////////////////////////////

  /** Get the threshold user count value above which we assume the
   *  individual effect of a single edge is negligible.  This allows
   *  us to prune a user's items to something more manageable.
   */
  size_t get_item_count_threshold_for_user(
      const item_info_vector& item_info,
      const std::vector<std::pair<size_t, double> >& item_list) {

    size_t degree_approximation_threshold =
        options.at("degree_approximation_threshold");

    DASSERT_GT(item_list.size(), degree_approximation_threshold);

    std::vector<size_t> items(item_list.size());
    for(size_t i = 0; i < item_list.size(); ++i) {
      items[i] = item_list[i].first;
    }

    // For all the users that have over
    // degree_approximation_threshold ratings, we register the
    // least frequently occuring items and only look at the
    // incoming edges to those.
    std::nth_element(
        items.begin(),
        items.begin() + degree_approximation_threshold,
        items.end(),
        [&](size_t i, size_t j) { return item_info[i].num_users < item_info[j].num_users; });

    size_t item_count_threshold = item_info[items[degree_approximation_threshold]].num_users;
    
    // Two checks to make sure our math is indeed correct.
    // We want to make sure that approximately
    // degree_approximation_threshold items with the
    // fewest hit counts -- therefore, the items most
    // likely to be influenced by this user -- are the
    // ones we hit below.  If the math in determining this
    // threshhold is correct, then there should be at
    // least degree_approximation_threshold items with
    // equal to or fewer than item_count_threshold users,
    // and fewer than degree_approximation_threshold items
    // with fewer than item_count_threshold users, as
    // item_count_threshold should be the count of the
    // degree_approximation_threshold item if they are
    // sorted by user count.

#ifndef NDEBUG
    {
      size_t n1 = std::count_if(
          item_list.begin(), item_list.end(),
          [&](std::pair<size_t, double> p) {
            return item_info[p.first].num_users <= item_count_threshold;
          });
    
      DASSERT_TRUE(n1 > degree_approximation_threshold);

      size_t n2 = std::count_if(
          item_list.begin(), item_list.end(),
          [&](std::pair<size_t, double> p) {
            return item_info[p.first].num_users < item_count_threshold;
          });
    
      DASSERT_TRUE(n2 <= degree_approximation_threshold);
    }
#endif
    
    return item_count_threshold;
  }

  ////////////////////////////////////////////////////////////////////////////////
  //

  /** A utility to run nearest neighbors to eliminate some of the items.
   */
  size_t preprocess_nearest_neighbors(
      dense_bitset& item_in_nearest_neighbors,
      const std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > >& data,
      const item_info_vector& item_info,
      const std::vector<size_t>& items_per_user,
      std::shared_ptr<_progress_tracker> progress_tracker) {

    // Check some of the input terms.
    DASSERT_EQ(item_in_nearest_neighbors.size(), item_info.size());
    DASSERT_EQ(item_in_nearest_neighbors.popcount(), 0);

    size_t num_items = item_info.size();
    size_t num_users = items_per_user.size();

    std::string force_mode = options.at("training_method");
    DASSERT_TRUE(force_mode == "auto" || force_mode.substr(0,2) == "nn");

    // The minimum number of users that hit an item has to be at least
    // num_users / nearest_neighbors_user_count_ratio_threshhold.  In
    // that case, the nearest_neighbors_user_count_ratio_threshhold is
    // fixed.
    size_t nearest_neighbors_user_count_ratio_threshhold = 32;

    // To make nearest neighbors worth it, we should actually run it
    // on as many items as are feasible to do.  This is per thread, as
    // each thread has an iterator which then reads local query rows,
    // but the most expensive part is reading in the data.
    size_t nearest_neighbors_min_num_items = 32;
    nearest_neighbors_min_num_items = std::min(
        item_info.size(), nearest_neighbors_min_num_items);

    // Now, do we have enough to run nearest neighbors on the data?
    // Not worth it if we don't have enough items to run through, but
    // removing a single common item can have significant impact on
    // the later performance.
    size_t n_in_nearest_neighbors = 0;

    // Any items with more users than this we allocate to nearest neighbors.
    size_t user_count_threshold
        = num_users / nearest_neighbors_user_count_ratio_threshhold;

    for(size_t i = 0; i < num_items; ++i) {
      if(item_info[i].num_users > user_count_threshold) {
        item_in_nearest_neighbors.set_bit(i);
        ++n_in_nearest_neighbors;
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Clip to make sure we are fitting within the number of items.

    // Now, handle the edge cases.
    if(force_mode == "auto") {

      // If there is no advantage to running anything here, then just
      // skip this step.
      if(n_in_nearest_neighbors == 0) {
        return 0;
      }

      // Finally, if we can efficiently handle all the items, then we
      // should.
      if(nearest_neighbors_min_num_items == item_info.size()) {
        item_in_nearest_neighbors.fill();
        n_in_nearest_neighbors = item_info.size();
      }

    } else {

      if(force_mode == "nn") {
        // Force everything to be done with nearest neighbors.
        item_in_nearest_neighbors.fill();
        n_in_nearest_neighbors = item_info.size();
      } else if(force_mode.substr(0, 2) == "nn") {

        // We are forced to do nearest neighbors here, but we need to
        // make sure that we don't actually do it on all of them so
        // some are left over for testing the next stage.

        // Make sure that the minimum does not force us to do all the
        // items.
        if(nearest_neighbors_min_num_items == item_info.size()) {
          nearest_neighbors_min_num_items /= 2;
        }

        // If this is the case, the nth_element operation below will
        // fill in the items.
        if(n_in_nearest_neighbors == 0) {
          // Pass.  The minimum number was not reached.  This is just
          // to make this case explicit -- it will be continued below.
        }

        // In this case, we reset the mask and let the nth_element
        // operation below pick off the top items.
        if(n_in_nearest_neighbors == item_info.size()) {
          item_in_nearest_neighbors.clear();

          // Guaranteed to hit the nth_element mode below.
          n_in_nearest_neighbors = nearest_neighbors_min_num_items - 1;
        }
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Step 2: Check to make sure the ne nearest neighbors worth it,
    // take the transpose and subsequent nearest neighbors operations
    // are worth it -- so pick the nearest_neighbors_min_num_items
    // number most expensive number of items and run them through
    // nearest neighbors.

    if(n_in_nearest_neighbors < nearest_neighbors_min_num_items) {

      // Okay, we need to make the nearest neighbors worth it, so move
      // some more items over there.

      std::vector<std::pair<size_t, size_t> > count_buffer(item_info.size());
      for(size_t i = 0; i < item_info.size(); ++i) {
        count_buffer[i] = {item_info[i].num_users, i};
      }

      std::nth_element(count_buffer.begin(),
                       count_buffer.begin() + nearest_neighbors_min_num_items,
                       count_buffer.end(),
                       [](const std::pair<size_t, size_t>& p1,
                          const std::pair<size_t, size_t>& p2) {
                         return p1.first > p2.first;
                       });

      DASSERT_GE(count_buffer.at(nearest_neighbors_min_num_items - 1).first,
                 count_buffer.at(nearest_neighbors_min_num_items).first);

      // Add these to the mask.
      for(size_t i = 0; i < nearest_neighbors_min_num_items; ++i) {
        item_in_nearest_neighbors.set_bit(count_buffer[i].second);
      }

      n_in_nearest_neighbors = nearest_neighbors_min_num_items;
    }

    DASSERT_EQ(item_in_nearest_neighbors.popcount(), n_in_nearest_neighbors);

    logprogress_stream << "Processing the " << n_in_nearest_neighbors
                       << " most common items by brute force search." << std::endl;

    ////////////////////////////////////////////////////////////////////////////////
    // Step 3: Transpose the array so it's a by-item list of the users
    // for each item.

    std::vector<size_t> users_per_item(item_info.size());
    for(size_t i = 0; i < item_info.size(); ++i) {
      users_per_item[i] = item_info[i].num_users;
    }

    std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > > transposed_data
        = transpose_sparse_sarray(
            data, users_per_item, options.at("target_memory_usage"));

    ////////////////////////////////////////////////////////////////////////////////
    // Step 4: Run it through a nearest neighbors brute-force search.

    /**  Function to process the pairs coming in.
     */
    auto process_item_pair = [&](size_t item_a, size_t item_b, const final_interaction_data_type& value)
        GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

      DASSERT_NE(item_a, item_b);

      insert_into_lookup(item_a, item_b, value);
      insert_into_lookup(item_b, item_a, value);

      // Increment this by one.
      progress_tracker->increment_item_counter();
    };

    /**  A function that allows certain pairs to be skipped and not
     *   processed.  We skip the transpose of pairs already in the
     *   query list, and identical indices.
     */
    auto skip_pair = [&](size_t query_idx, size_t ref_idx) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

      if(query_idx == ref_idx)
        return true;

      if(query_idx < ref_idx) {
        return item_in_nearest_neighbors.get(ref_idx);
      }

      return false;
    };

    progress_tracker->print_header();
    
    /** Now send it off to the brute force similarity lookup tables.
     */
    brute_force_all_pairs_similarity_with_vector_reference(
        /* Reference data. */
        transposed_data,
        item_info,

        /* Query data. Same set. */
        transposed_data,
        item_info,

        /* The similarity. */
        similarity,

        /* Process function. */
        process_item_pair,

        /* metadata -- the number of dimensions; in this case, it's the number of users. */
        num_users,
        options.at("target_memory_usage"),

        /* Skip processing of items we don't want to. */
        skip_pair,

        /* the mask. */
        &item_in_nearest_neighbors);

    // And we're done!
    return n_in_nearest_neighbors;
  }

  /**  The main function that processes the sparse matrix sarray.
   */
  template <typename ItemItemContainer>
  void _train_with_sparse_matrix_sarray(
      ItemItemContainer&& interaction_data,
      const std::vector<size_t>& slice_boundaries,
      const item_info_vector& item_info,
      const std::vector<size_t>& items_per_row,
      const vector_index_mapper& index_mapper,
      std::shared_ptr<_progress_tracker> progress_tracker,
      const std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > >& data) {

    ////////////////////////////////////////////////////////////////////////////////
    // Step 1.  Define constants needed later on, along with common lookup tables. 
    
    // Locks, in case they are needed.  Just hash to a particular
    // point in this array to avoid contention.
    static constexpr bool use_interaction_locks = (
        // If we require edge locking for the similarity types...
        SimilarityType::require_interaction_locking()
        // ... and we aren't using the sparse_parallel_2d_array, which
        // has edge locking built in.
        && !std::is_same<ItemItemContainer,
                         sparse_parallel_2d_array<interaction_data_type> >::value);

    static constexpr size_t n_interaction_locks = (use_interaction_locks ? 1024 : 1);
    std::array<simple_spinlock, n_interaction_locks> interaction_locks;
    
    // Now that that is set up, get the rest.
    const size_t num_items = item_info.size();
    TURI_ATTRIBUTE_UNUSED_NDEBUG const size_t n = data->size();
    DASSERT_EQ(items_per_row.size(), n);
    
    const size_t random_seed = (options.count("random_seed")
                                ? size_t(options.at("random_seed"))
                                : size_t(0));
    
    size_t degree_approximation_threshold = options.at("degree_approximation_threshold");

    simple_spinlock pruned_user_item_count_thresholds_lock;
    std::map<size_t, size_t> pruned_user_item_count_thresholds;
    
    ////////////////////////////////////////////////////////////////////////////////
    // Variables for the progress tracking.
    
    progress_tracker->print_header();

    // Calculate the total number of operations registered through a
    // pass through the data (ignoring sampling).

    size_t total_interactions_to_register = 0;

    // These quantities are in the original, non-index-mapped values,
    // so the actual updates will have to be in terms of that as well.
    for(size_t m : items_per_row) {
      total_interactions_to_register += std::min(degree_approximation_threshold, m) * m;
    }

    // For each item, we register this quantity after processing each
    // row, so multiply this by the number of passes.
    total_interactions_to_register *= (slice_boundaries.size() - 1);

    // Now, what we actually report is in terms of the number of
    // item-item interactions, so each row needs to be scaled by this
    // amount.

    double progress_register_scale = (double(num_items) * num_items) / (total_interactions_to_register); 

    ////////////////////////////////////////////////////////////////////////////////
    // At the beginning of each slice, this function is called.  We
    // use that to set up the information and lookups for process_row.
    
    auto init_slice = [&](size_t slice_idx, size_t item_idx_start, size_t item_idx_end) {

      // The matrix starts from (item_idx_start, item_idx_start).
      size_t slice_height = item_idx_end - item_idx_start;
      size_t slice_width = num_items - item_idx_start;
      DASSERT_GE(slice_height, 1);
      
      // Now reset the edge data container.
      interaction_data.clear();
      interaction_data.resize(slice_height, slice_width);
    };   

    ////////////////////////////////////////////////////////////////////////////////
    // The workhorse function; called for every row in the data and
    // for every slice.  This processes all the items in this row,
    // then clears the row so
    // iterate_through_sparse_item_array_by_slice continues to the
    // next row after this.
    
    auto process_row = [&](size_t thread_idx, size_t row_idx, 
                           size_t item_idx_start, size_t item_idx_end,
                           std::vector<std::pair<size_t, double> >& item_list)
        GL_GCC_ONLY(GL_HOT_INLINE) {

      ////////////////////////////////////////////////////////////////////////////////
      // If the index mapping is enabled, remap the indices to
      // the current internal ones.

      do {
        // First, report this to the progress tracker based on progress size.
        {
          size_t m = item_list.size();
          size_t n_interactions_to_register
             = size_t(progress_register_scale * std::min(m, degree_approximation_threshold) * m);

          progress_tracker->increment_item_counter(n_interactions_to_register);
        }
          
        index_mapper.remap_sparse_vector(item_list);
      
        // It may be that the above cleared out all the items.
        if(item_list.empty()) {
          break;
        }

        ////////////////////////////////////////////////////////////////////////////////
        // Check if we need to threshold this one to make things
        // computationally feasible.

        size_t item_count_threshold = std::numeric_limits<size_t>::max();

        // Data structures for the sampling.  We just use a
        // simple hash-based sampling that is deterministic by
        // row, so the entire thing is deterministic by
        // random_seed.
        size_t rng_gen_value = 0;

        // items with hash above this are excluded.
        size_t rng_64bit_threshhold = std::numeric_limits<size_t>::max();
      
        // Exclusion function.
        auto exclude_item_by_sampling = [&](size_t idx) {
          rng_gen_value = hash64(rng_gen_value, idx);
          return rng_gen_value >= rng_64bit_threshhold;
        };

        // Do we need to approximate this interaction?
        bool approximation_enabled = (item_list.size() > degree_approximation_threshold);

        if(approximation_enabled) {
          // The approximation used here has two part:
          //
          // - For incoming edges -- that is, vertices who's
          //   nearest neighors we are choosing, we pick the top
          //   degree_approximation_threshold items that have
          //   the lowest counts.  The rational is that
          //   processing single row will have the most impact
          //   on these items, since the fewest other items
          //   process them.
          //
          // - For outgoing edges -- that is, vertices that we
          //   process here -- we sample down to
          //   degree_approximation_threshold items.  That means
          //   that we will always have at most
          //   degree_approximation_threshold**2 items processed
          //   per user.

          ////////////////////////////////////////////////////////////////////////////////
          // Step 1: Set up the pruning part.

          // Get the pruning threshold
          {
            std::lock_guard<simple_spinlock> lg(pruned_user_item_count_thresholds_lock);

            auto it = pruned_user_item_count_thresholds.find(row_idx);

            if(it == pruned_user_item_count_thresholds.end()) {
              item_count_threshold
                  = get_item_count_threshold_for_user(item_info, item_list);
              pruned_user_item_count_thresholds[row_idx] = item_count_threshold;
            } else {
              item_count_threshold = it->second;
            }
          }

          ////////////////////////////////////////////////////////////////////////////////
          // Step 2: Set up the random sampling for the inner part.

          rng_gen_value = hash64(random_seed, row_idx);
          rng_64bit_threshhold = ((std::numeric_limits<size_t>::max() / item_list.size())
                                  * degree_approximation_threshold);
        }

        // Set the iteration bounds on the incoming list of
        // items based on which slice we are in.  Requires a
        // search if they are not at the end.
        size_t list_idx_start, list_idx_end;

        std::tie(list_idx_start, list_idx_end)
        = find_slice_boundary_indices(item_list, item_idx_start,item_idx_end);

        // If this one is empty, ignore.
        if(UNLIKELY(list_idx_start == list_idx_end)) {
          break;
        }

        // Make sure we have indeed found the correct boundaries.
        DASSERT_LT(item_list[list_idx_end-1].first, item_idx_end);
        DASSERT_GE(item_list[list_idx_end-1].first, item_idx_start);
        DASSERT_LT(item_list[list_idx_start].first, item_idx_end);
        DASSERT_GE(item_list[list_idx_start].first, item_idx_start);

        ////////////////////////////////////////////////////////////////////////////////
        // Now that list_idx_start and list_idx_end are set, iterate
        // over the elements in it.
        for(size_t idx_a = list_idx_start; idx_a < list_idx_end; ++idx_a) {

          size_t item_a = item_list[idx_a].first;
          const auto& value_a = item_list[idx_a].second;

          // If this is one of the ones we've determined not to
          // worry about by threshold count, then skip it.
          if(item_info[item_a].num_users > item_count_threshold)
            continue;

          // Only do the upper slice of the triangle --
          // everything is assumed to be symmetric.
          for(size_t idx_b = idx_a + 1; idx_b < item_list.size(); ++idx_b) {
            if(exclude_item_by_sampling(idx_b))
              continue;

            size_t item_b = item_list[idx_b].first;
            const auto& value_b = item_list[idx_b].second;

            size_t row_idx = item_a - item_idx_start;
            size_t col_idx = item_b - item_idx_start;

            DASSERT_LT(row_idx, col_idx);

            auto update_interaction_f = [&](interaction_data_type& edge)
                GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

              // Apply the edge.
              similarity.update_interaction(
                  edge,
                  item_info[item_a].item_data, item_info[item_b].item_data,
                  value_a, value_b);
            };

            if(use_interaction_locks) {
              std::lock_guard<simple_spinlock> lg(
                  interaction_locks[hash64(row_idx, col_idx) % n_interaction_locks]);

              interaction_data.apply(row_idx, col_idx, update_interaction_f);
            } else {
              interaction_data.apply(row_idx, col_idx, update_interaction_f);
            }
          }
        }

      } while(false); 
      
      // Clear the item list so the iteration function doesn't iterate
      // through the resulting elements. (see docs for
      // iterate_through_sparse_item_array_by_slice).
      item_list.clear();
    };

    ////////////////////////////////////////////////////////////////////////////////
    // The function to process a given element.  We're not using that
    // one here, so just declare it to be empty.
    
    auto empty_process_element = [](size_t thread_idx, size_t row_idx,
                                    size_t item_idx_start, size_t item_idx_end,
                                    size_t item_idx, double value)
        GL_GCC_ONLY(GL_HOT_INLINE)
        {};

    
    ////////////////////////////////////////////////////////////////////////////////
    // At the end of every slice, go through and process all the
    // lookup tables that process_row filled.  Between init_slice and
    // finalize_slice was one complete run through the data.
    
    auto finalize_slice = [&](size_t slice_idx, size_t item_idx_start, size_t item_idx_end) {
      
      ////////////////////////////////////////////////////////////////////////////////
      // Process all of these edges, then go back and do another pass
      // on the next slice if necessary.  Note that there is
      // guaranteed to be no thread accessing the same data at once.

      interaction_data.apply_all(
        [&](size_t row_idx, size_t col_idx, const interaction_data_type& edge)
        GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

          final_interaction_data_type final_interaction_data = final_interaction_data_type();

          size_t item_a = item_idx_start + row_idx;
          size_t item_b = item_idx_start + col_idx;

          DASSERT_LT(item_a, item_b);

          this->similarity.finalize_interaction(
              final_interaction_data,
              item_info[item_a].final_item_data,
              item_info[item_b].final_item_data,
              edge,
              item_info[item_a].item_data,
              item_info[item_b].item_data);

          // Remap the indices if needed.
          size_t abs_item_a = index_mapper.map_internal_index_to_data_index(item_a);
          size_t abs_item_b = index_mapper.map_internal_index_to_data_index(item_b);

          // Insert into the central lookup.
          insert_into_lookup(abs_item_a, abs_item_b, final_interaction_data);
          insert_into_lookup(abs_item_b, abs_item_a, final_interaction_data);
        });
    };

    // Now, run the above functions!
    iterate_through_sparse_item_array_by_slice(
        data,
        slice_boundaries,
        init_slice,
        process_row,
        empty_process_element,
        finalize_slice);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // This works okay.

  /** Algorithm for implementation of the sparse_similarity similarities.
   */
  std::map<std::string, flexible_type>
  train_from_sparse_matrix_sarray(
      size_t num_items,
      const std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > >& data) {

    turi::timer total_timer;
    total_timer.start();
    auto progress_tracker = std::make_shared<_progress_tracker>(num_items); 

    std::map<std::string, flexible_type> ret;

    // For debugging purposes, this
    std::string force_mode = options.at("training_method");

    ////////////////////////////////////////////////////////////////////////////////
    // Step 1 -- set up the vertices.  Based on this, we end up
    // dividing some items into nearest neighbors, some into
    // dense symmetric similarity, some into dense non-symmetric similarity,
    // and some into sparse similarity depending on the item counts of each.

    item_info_vector item_info;
    vector_index_mapper index_mapper;
    std::vector<size_t> items_per_user;
    bool nearest_neighbors_run = false;

    calculate_item_processing_colwise(
        item_info, similarity, data, num_items, &items_per_user);
    
    size_t num_items_remaining = item_info.size();

    logprogress_stream << "Setting up lookup tables." << std::endl;
    {
      std::vector<final_item_data_type> _final_item_data(num_items, final_item_data_type());

      for(size_t i = 0; i < item_info.size(); ++i) {
        _final_item_data[i] = item_info[i].final_item_data;
      }

      init_item_lookups(num_items, _final_item_data);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Since the dense pass option can be called in multiple places, put

    auto attempt_dense_pass = [&](size_t pass_limit) {

      std::vector<size_t> dense_slice_structure
         = calculate_slice_structure(num_items_remaining, pass_limit, bytes_per_item_dense());

      if(dense_slice_structure.empty()) {
        return false;
      }

      size_t num_dense_passes = dense_slice_structure.size() - 1;

      // If we can do it in a couple passes, then just do it and don't
      // do any of the preprocessing steps.
      if(num_dense_passes <= pass_limit) {

        if(num_dense_passes == 1) {
          logprogress_stream
              << "Processing data in one pass using dense lookup tables."
              << std::endl;
        } else {
          logprogress_stream
              << "Processing data in " << num_dense_passes << " passes using dense lookup tables."
              << std::endl;
        }
        
        typedef dense_triangular_itemitem_container<interaction_data_type> matrix_type;

        {
          matrix_type dense_container;

          // Reserve to avoid expensive reallocations that cause bad allocs.
          if(num_dense_passes != 1) {
            size_t target_memory_usage = options.at("target_memory_usage");
            dense_container.reserve(target_memory_usage / sizeof(interaction_data_type));
          }
        
          // Set up the slices for the edge processing.
          _train_with_sparse_matrix_sarray(
              dense_container, dense_slice_structure, item_info,
              items_per_user, index_mapper, progress_tracker, data);
        }
        
        return true;
      } else {
        return false;
      }
    };

    ////////////////////////////////////////////////////////////////////////////////
    // Step 1: See if we can do it all with one pass of the max_num_items.
    size_t max_data_passes = options.at("max_data_passes");

    if(force_mode == "auto") {
      bool success = attempt_dense_pass(4);
      if(success) {

        // Record the actual training method into the dense.
        options["training_method"] = "dense";
        goto ITEM_SIM_DONE;
      }
    }

    if(force_mode == "dense") {
      bool success = attempt_dense_pass(max_data_passes);
      if(success) {
        goto ITEM_SIM_DONE;
      } else {
        std::ostringstream ss;
        ss <<"Not enough allowed memory to use training_method = \"dense\" with "
           << "max_data_passes = " << max_data_passes
           << "; consider increasing target_memory_usage "
           << " or max_data_passes." << std::endl;
        log_and_throw(ss.str().c_str());
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Step 2: First do a nearest neighbors preprocessing step to
    // handle the most expensive items.

    if(force_mode == "auto" || force_mode.substr(0, 2) == "nn") {

      dense_bitset item_in_nearest_neighbors(num_items);

      size_t n_in_nearest_neighbors = preprocess_nearest_neighbors(
          item_in_nearest_neighbors, data, item_info, items_per_user, progress_tracker);

      if(n_in_nearest_neighbors == num_items) {
        // The nearest neighbors has taken care of everything.  We're
        // done.
        
        // Record the actual training method into the dense.
        options["training_method"] = "nn";
        
        goto ITEM_SIM_DONE;
      } else if(n_in_nearest_neighbors == 0) {
        // nearest neighbors did nothing.  So we don't need to remap
        // the indices.
        nearest_neighbors_run = false;
      } else {
        
        // Apply this mapping to the vertex data
        item_in_nearest_neighbors.invert();
        num_items_remaining = index_mapper.set_index_mapping_from_mask(item_in_nearest_neighbors);
        index_mapper.remap_vector(item_info);
        DASSERT_EQ(num_items_remaining, item_info.size());
        nearest_neighbors_run = true;

        // Register we're going on to other methods. 
        progress_tracker->print_break();
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Step 3: Are we forced to do a dense pass after this?

    if(force_mode == "nn:dense") {
      bool success = attempt_dense_pass(max_data_passes);
      if(success) {
        goto ITEM_SIM_DONE;
      } else {
        std::ostringstream ss;
        ss <<"Not enough allowed memory to use training_method = \"nn:dense\" with "
           << "max_data_passes = " << max_data_passes
           << "; consider increasing target_memory_usage "
           << " or max_data_passes." << std::endl;
        log_and_throw(ss.str().c_str());
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Step 3:  Pick how many passes wo


    // This is just to make sure we don't get in an infinite loop
    // here.
    for(size_t attempt = 0; ; ++attempt) {

      auto error_out = []() {
        log_and_throw("Unable to determine reasonable way to run "
                      "item_similarity given constrained running parameters. "
                      "To fix, try: (1) increasing target_memory_usage, "
                      "(2) increasing max_data_passes, or (3) forcing nearest "
                      "neighbors mode with training_method='nn'.");
      };

      // A safegaurd against there being an infinite loop.
      if(attempt >= 16) {
        error_out();
      }

      // First, try to do a sparse pass.
      double bpi_sparse = bytes_per_item_sparse(item_info, items_per_user);

      logstream(LOG_INFO) << "Bytes per item in sparse matrix = " << bpi_sparse << std::endl;

      std::vector<size_t> sparse_slice_structure
          = calculate_slice_structure(num_items_remaining, max_data_passes, bpi_sparse);

      bool sparse_possible = !sparse_slice_structure.empty();

      size_t num_sparse_passes = (sparse_possible
                                  ? sparse_slice_structure.size() - 1
                                  : std::numeric_limits<size_t>::max());

      if(sparse_possible) { 
        logstream(LOG_INFO) << "Estimated " << num_sparse_passes
                            << " passes needed for sparse matrix." << std::endl;
      } else {
        logstream(LOG_INFO) << "Number of data passes too high for sparse matrix. " << std::endl;
      }
      
      // Are we disabling the dense mode by forcing the sparse mode?
      // If so, then we keep trying until it works in the sparse mode.
      bool disable_dense = (force_mode == "sparse" || force_mode == "nn:sparse");

      // If we are not disabling the dense mode, then attempt to do it
      // by allocating enough of the 
      if(!disable_dense) {

        size_t dense_mode_allowed_passes = max_data_passes;

        // By rough guestimation and some benchmarking, it seems that
        // the sparse mode occurs a 4-8x-ish penalty over the dense
        // mode. Furthermore, the memory usage in the dense mode is
        // more predictable, so we should favor it slightly. Thus, if
        // we are able to do the dense mode in less than 4 times the
        // passes it would take to do the sparse mode, just run with
        // that.
       
        if(sparse_possible) {
          dense_mode_allowed_passes = std::min(8*num_sparse_passes, max_data_passes);
        }

        bool success = attempt_dense_pass(dense_mode_allowed_passes);

        if(success) {
          if(nearest_neighbors_run) {
            options["training_method"] = "nn:dense";
          } else {
            options["training_method"] = "dense";
          }
          goto ITEM_SIM_DONE;
        }
      }

      // Okay, dense didn't work.  So let's do sparse if possible.
      if(sparse_possible) {

        if(num_sparse_passes == 1) {
          logprogress_stream
              << "Processing data in one pass using sparse lookup tables."
              << std::endl;
        } else {
          logprogress_stream
              << "Processing data in " << num_sparse_passes
              << " passes using sparse lookup tables."
              << std::endl;
        }

        typedef sparse_parallel_2d_array<interaction_data_type> matrix_type;

        _train_with_sparse_matrix_sarray(
            matrix_type(), sparse_slice_structure, item_info,
            items_per_user, index_mapper, progress_tracker, data);

        // Record what we actually did for future reference. 
        if(nearest_neighbors_run) {
          options["training_method"] = "nn:sparse";
        } else {
          options["training_method"] = "sparse";
        }

        goto ITEM_SIM_DONE;
      }

      ////////////////////////////////////////////////////////////////////////////////
      // Okay, it appears that neither method above helps.  Time to
      // apply approximations and do it again.

      // If we are doing it with training_method == "auto", then lower
      // the degree approximation threshold. This may help us do it
      // approximately.

      size_t degree_approximation_threshold = options.at("degree_approximation_threshold");

      // This number is somewhat arbitrary, but not sure what else to do here...
      if(degree_approximation_threshold > 8) {
        logprogress_stream
            << "Unable to fit dataset processing into limit of max_data_passes="
            << size_t(options.at("max_data_passes")) << " and target_memory_usage="
            << size_t(options.at("target_memory_usage")) << " "
            << "bytes.  Employing more aggressive approximations; "
            << "increase target_memory_usage, "
            << "nearest_neighbors_interaction_proportion_threshold, "
            << "or max_data_passes to avoid this. "
            << std::endl;

        logprogress_stream
            << "  Setting degree_approximation_threshold="
            << degree_approximation_threshold << std::endl;

        options["degree_approximation_threshold"] /= 2;
        continue;
      } else {
        error_out();
      }
    }

 ITEM_SIM_DONE:

    ////////////////////////////////////////////////////////////////////////////////
    // Finalize it!  Put in data.
    progress_tracker->print_footer();

    logprogress_stream << "Finalizing lookup tables." << std::endl;
    
    finalize_lookups();

    ret.insert(options.begin(), options.end());
    ret["training_time"] = total_timer.current_time();
    return ret;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Score item for the update

  mutable std::vector<std::vector<prediction_accumulation_type> >
  item_prediction_buffers_by_thread;

  /**  Score all items in a list of item predictions given a list of
   *   user-item interactions.
   */
  size_t score_items(std::vector<std::pair<size_t, double> >& item_predictions,
                     const std::vector<std::pair<size_t, double> >& user_item_data) const {

    final_item_data_type _unused;

    // Use this in case we are already inside an in_parallel loop.
    size_t outer_thread_idx = thread::thread_id();

    DASSERT_LT(outer_thread_idx, item_prediction_buffers_by_thread.size());

    // Get the buffer we'll be using here
    auto& item_prediction_buffer = item_prediction_buffers_by_thread[outer_thread_idx];
    item_prediction_buffer.assign(total_num_items, prediction_accumulation_type());

    atomic<size_t> num_updates = 0;
    
    // The function that actually does the similarity calculations.
    auto _run_scoring = [&](
        size_t user_item_data_start, size_t user_item_data_end,
        bool use_unsafe_update_method) GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

      for(size_t i = user_item_data_start; i < user_item_data_end; ++i) {
        size_t item = user_item_data[i].first;
        double score = user_item_data[i].second;

        if(item >= total_num_items) {
          continue;
        }

        for(size_t i = item_neighbor_boundaries[item]; i < item_neighbor_boundaries[item+1]; ++i) {
          const auto& item_neighbor = item_interaction_data[i];

          ++num_updates;
          
          if(use_unsafe_update_method) {
            similarity.update_prediction_unsafe(
                item_prediction_buffer[item_neighbor.first],
                item_neighbor.second,
                use_final_item_data() ? final_item_data[item] : _unused,
                use_final_item_data() ? final_item_data[item_neighbor.first] : _unused,
                score);
          } else {
            similarity.update_prediction(
                item_prediction_buffer[item_neighbor.first],
                item_neighbor.second,
                use_final_item_data() ? final_item_data[item] : _unused,
                use_final_item_data() ? final_item_data[item_neighbor.first] : _unused,
                score);
          }
        }
      }
    };

    // If possible, do the above in parallel to get accurate
    // recommendations.
    in_parallel([&](size_t thread_idx, size_t num_threads) {
        bool parallel_here = (num_threads != 1);

        if(parallel_here) {
          // Not inside an in_parallel call outside of this one; we
          // can totally parallelize this.
          size_t user_item_data_start = (thread_idx * user_item_data.size()) / num_threads;
          size_t user_item_data_end = ((thread_idx+1) * user_item_data.size()) / num_threads;

          _run_scoring(user_item_data_start, user_item_data_end, false);
        } else {
          // Means likely that we are already in an in_parallel call.
          _run_scoring(0, user_item_data.size(), true);
        }
    });

    in_parallel([&](size_t thread_idx, size_t num_threads) {
        size_t item_index_start = (thread_idx * item_predictions.size()) / num_threads;
        size_t item_index_end = ((thread_idx+1) * item_predictions.size()) / num_threads;

        for(size_t i = item_index_start; i < item_index_end; ++i) {
          // Finalize the result, dumping the scores into the main scoring function.
          auto& p = item_predictions[i];
          size_t item = p.first;

          if(item >= total_num_items) {
            p.second = 0;
            continue;
          }

          p.second = similarity.finalize_prediction(
              item_prediction_buffer[item],
              use_final_item_data() ? final_item_data[item] : _unused,
              user_item_data.size());

          DASSERT_TRUE(std::isfinite(p.second));
        }
      });

    // We're done!
    return size_t(num_updates);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Routines for loading and serialization.

  size_t get_version() const { return 1; }

  void save(turi::oarchive& oarc) const {

    oarc << get_version();

    oarc << total_num_items
         << final_item_data
         << item_neighbor_boundaries
         << item_interaction_data;
  }

  /** Load things.
   */
  void load(turi::iarchive& iarc) {
    size_t version = 0;

    iarc >> version;

    ASSERT_MSG(version == 1,
               "Item similarity lookup does not support loading from this version.");

    iarc >> total_num_items
         >> final_item_data
         >> item_neighbor_boundaries
         >> item_interaction_data;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Utilites for other operations

  /** For a given item, return the recorded closest nearest items.
   *
   */
  void get_similar_items(
      std::vector<std::pair<size_t, flexible_type> >& similar_items,
      size_t item,
      size_t top_k) const {

    final_item_data_type _unused;

    auto item_comparitor = [&](const interaction_info_type& p1, const interaction_info_type& p2) -> bool {

      return similarity.compare_interaction_values(
          p1.second,
          p2.second,
          use_final_item_data() ? final_item_data[item] : _unused,
          use_final_item_data() ? final_item_data[p1.first] : _unused,
          use_final_item_data() ? final_item_data[p2.first] : _unused);
    };

    if(item >= total_num_items) {
      similar_items.clear();
      return;
    }

    // Put them in a buffer that can be sorted by the item_comparitor function.
    std::vector<std::pair<size_t, final_interaction_data_type> > item_buffer(
        item_interaction_data.begin() + item_neighbor_boundaries[item],
        item_interaction_data.begin() + item_neighbor_boundaries[item + 1]);

    if(top_k < item_buffer.size()) {
      std::nth_element(item_buffer.begin(),
                       item_buffer.begin() + top_k,
                       item_buffer.end(),
                       item_comparitor);
      item_buffer.resize(top_k);
    }

    std::sort(item_buffer.begin(), item_buffer.end(), item_comparitor);

    similar_items.resize(item_buffer.size());
    for(size_t i = 0; i < item_buffer.size(); ++i) {
      similar_items[i].first = item_buffer[i].first;
      similar_items[i].second = similarity.export_similarity_score(item_buffer[i].second);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Debugging

  bool _debug_check_equal(const sparse_similarity_lookup& _other) const {
    auto other = dynamic_cast<const sparse_similarity_lookup_impl*>(&_other);

    std::map<std::pair<size_t, size_t>, final_interaction_data_type> edges_this, edges_other;

    for(size_t i = 0; i < total_num_items; ++i) {
      for(size_t j = item_neighbor_boundaries[i]; j < item_neighbor_boundaries[i + 1]; ++j) {
        edges_this[{i, item_interaction_data[j].first}] = item_interaction_data[j].second;
      }
    }

    for(size_t i = 0; i < total_num_items; ++i) {
      for(size_t j = other->item_neighbor_boundaries[i];
          j < other->item_neighbor_boundaries[i + 1]; ++j) {

        edges_other[{i, other->item_interaction_data[j].first}] = other->item_interaction_data[j].second;
      }
    }

    std::vector<std::pair<std::pair<size_t, size_t>, final_interaction_data_type> > in_this_not_that;
    std::vector<std::pair<std::pair<size_t, size_t>, final_interaction_data_type> > in_that_not_this;
    std::vector<std::pair<std::pair<size_t, size_t>,
                          std::pair<final_interaction_data_type, final_interaction_data_type> > > diff_values;

    for(const auto& p : edges_this) {
      if(edges_other.count(p.first) == 0) {
        in_this_not_that.push_back(p);
      } else if(std::abs(edges_other.at(p.first) - p.second) > 1e-6) {
        diff_values.push_back( {p.first, {p.second, edges_other.at(p.first)} } );
      }
    }

    for(const auto& p : edges_other) {
      if(edges_this.count(p.first) == 0) {
        in_that_not_this.push_back(p);
      }
    }

    bool failed = false;

    if(!in_this_not_that.empty()) {
      std::cout << "IN THIS, NOT OTHER: " << std::endl;
      for(const auto& p : in_this_not_that) {
        std::cout << "     (" << p.first.first << ", " << p.first.second << "): "
                  << p.second << std::endl;
      }
      failed = true;
    }

    if(!in_that_not_this.empty()) {
      std::cout << "IN OTHER, NOT THIS: " << std::endl;
      for(const auto& p : in_that_not_this) {
        std::cout << "     (" << p.first.first << ", " << p.first.second << "): "
                  << p.second << std::endl;
      }
      failed = true;
    }

    if(!diff_values.empty()) {
      std::cout << "Differing Values: " << std::endl;
      for(const auto& p : diff_values) {
        std::cout << "     (" << p.first.first << ", " << p.first.second << "): "
                  << "(this = " << p.second.first << ", other = " << p.second.second << ")"
                  << std::endl;
      }
      failed = true;
    }

    return !failed;
  }
};

}}

#endif /* _ITEM_SIMILARITY_LOOKUP_H_ */
