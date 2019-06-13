/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SPARSE_SIMILARITY_ITEM_STATISTICS_H
#define TURI_UNITY_SPARSE_SIMILARITY_ITEM_STATISTICS_H

#include <toolkits/sparse_similarity/similarities.hpp>
#include <toolkits/sparse_similarity/utilities.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/util/dense_bitset.hpp>
#include <core/parallel/atomic_ops.hpp>
#include <core/logging/table_printer/table_printer.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <map>
#include <vector>
#include <string>

namespace turi { namespace sparse_sim {

////////////////////////////////////////////////////////////////////////////////

/*  For each item, we track several values and statistics needed for
 *  the processing -- the number of users, an item_data_type entry,
 *  and a final_item_data_type entry.  These latter types are defined
 *  by the SimilarityType structure.
 */
template <typename SimilarityType>
struct item_processing_info {
  typedef typename SimilarityType::item_data_type item_data_type;
  typedef typename SimilarityType::final_item_data_type final_item_data_type;

  size_t num_users = 0;
  item_data_type item_data = item_data_type();
  final_item_data_type final_item_data = final_item_data_type();
};


/**  Creates an array of item_processing_info and populates it with
 *   the appropriate item statistics.  Accepts as input a sparse
 *   SArray in which each row represents a "user" and each column an
 *   item.  This takes an sarray of vectors of (index, value) pairs.
 *
 *   It's expensive if the number of items is not known ahead of time,
 *   and this is typically known, so we require it as a parameter.
 *
 *   items_per_user, if not null, is set to a vector recording the
 *   number of items each user rates.
 */
template <typename SimilarityType>
void calculate_item_processing_colwise(
    std::vector<item_processing_info<SimilarityType> >& item_info,
    const SimilarityType& similarity,
    const std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > >& data,
    size_t num_items,
    std::vector<size_t>* items_per_user = nullptr) {

  item_info.resize(num_items);

  const size_t n = data->size();

  ////////////////////////////////////////////////////////////////////////////////
  // Setup all the containers.

  static constexpr bool use_item_locking = SimilarityType::require_item_locking();

  std::vector<simple_spinlock> item_locks;

  if(use_item_locking) {
    item_locks.resize(num_items);
  }

  if(items_per_user != nullptr) {
    items_per_user->assign(n, 0);
  }

  logprogress_stream << "Gathering per-item and per-user statistics." << std::endl;

  table_printer table( { {"Elapsed Time (Item Statistics)", 0}, {"% Complete", 0} } );

  table.print_header();

  atomic<size_t> rows_processed_total = 0;
  mutex print_lock;

  ////////////////////////////////////////////////////////////////////////////////
  // Now, iterate through the data in parallel.

  auto process_row_f = [&](size_t thread_idx, size_t row_idx,
                           const std::vector<std::pair<size_t, double> >& item_list)
      GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN) {

    if(items_per_user != nullptr) {
      (*items_per_user)[row_idx] = item_list.size();
    }

    for(size_t idx_a = 0; idx_a < item_list.size(); idx_a++) {

      const size_t item_a = item_list[idx_a].first;
      const auto& value_a = item_list[idx_a].second;

      ////////////////////////////////////////////////////////////////////////////////
      // Apply the vertex function of the similarity.
      DASSERT_LT(item_a, item_info.size());

      if(use_item_locking) {
        DASSERT_LT(item_a, item_locks.size());
        std::lock_guard<simple_spinlock> lg(item_locks[item_a]);
        similarity.update_item(item_info[item_a].item_data, value_a);
      } else {
        similarity.update_item(item_info[item_a].item_data, value_a);
      }

      atomic_increment(item_info[item_a].num_users);
    }

    size_t local_rows_processed_total = ++rows_processed_total;

    if(local_rows_processed_total % 1000 == 0) {
      double percent_complete = double((400 * local_rows_processed_total) / n) / 4;
      table.print_timed_progress_row(progress_time(), percent_complete);
    }
  };

  // Now, just do the iteration.
  iterate_through_sparse_item_array(data, process_row_f);

  ////////////////////////////////////////////////////////////////////////////////
  // Now, finalize the vertices.

  in_parallel([&](size_t thread_idx, size_t num_threads) {
      size_t start_idx = (thread_idx * num_items) / num_threads;
      size_t end_idx = ((thread_idx + 1) * num_items) / num_threads;

      for(size_t i = start_idx; i < end_idx; ++i) {
        similarity.finalize_item(item_info[i].final_item_data, item_info[i].item_data);
      }
    });

  table.print_row(progress_time(), 100);
  table.print_footer();
}

/** A version like the previous one, but has the columns and rows
 *  reversed.  That is, each row is an item and each column is a
 *  user.
 *
 *  Currently, this function does not calculate the item_counts and
 *  user_counts.
 *
 *  Returns the total number of users.
 */
template <typename SimilarityType>
size_t calculate_item_processing_rowwise(
    std::vector<item_processing_info<SimilarityType> >& item_info,
    const SimilarityType& similarity,
    const std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > >& data) {

  ////////////////////////////////////////////////////////////////////////////////
  // Setup all the containers.

  const size_t n = data->size();
  size_t num_users = 0;

  item_info.resize(n);

  // Do a single pass through the data to build all of the vertex
  // statistics.
  const size_t max_num_threads = thread::cpu_count();
  auto reader = data->get_reader(max_num_threads);

  // Comparing the indices in the row to verify it is indeed sorted.
  TURI_ATTRIBUTE_UNUSED_NDEBUG auto idx_cmp_f = [](const std::pair<size_t, double>& p1,
                                                   const std::pair<size_t, double>& p2) {
    return p1.first < p2.first;
  };

  in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {

      size_t start_idx = (thread_idx * n) / num_threads;
      size_t end_idx = ((thread_idx+1) * n) / num_threads;

      std::vector<std::vector< std::pair<size_t, double> > > row_buffer_v(1);

      for(size_t row_idx = start_idx; row_idx < end_idx; ++row_idx) {
        reader->read_rows(row_idx, row_idx+1, row_buffer_v);

        const auto& row = row_buffer_v[0];

        DASSERT_TRUE(std::is_sorted(row.begin(), row.end(), idx_cmp_f));

        // Update the number of dimensions with the largest one here.
        if(!row.empty()) {
          atomic_set_max(num_users, row.back().first + 1);
        }

        for(const auto& p : row) {

          ////////////////////////////////////////////////////////////////////////////////
          // Apply the vertex function of the similarity.  Can use the
          // unsafe version as each row is isolated to a thread (this
          // version does not do columns).

          similarity.update_item_unsafe(item_info[row_idx].item_data, p.second);
        }

        similarity.finalize_item(item_info[row_idx].final_item_data, item_info[row_idx].item_data);
      }
    });

  return num_users;
}

}}

#endif
