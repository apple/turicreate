/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SPARSE_SIMILARITY_UTILITIES_H
#define TURI_SPARSE_SIMILARITY_UTILITIES_H

#include <core/storage/sframe_data/sarray.hpp>
#include <vector>
#include <core/parallel/pthread_tools.hpp>
#include <core/util/try_finally.hpp>
#include <core/logging/table_printer/table_printer.hpp>
#include <core/storage/sframe_data/sarray_iterators.hpp>

namespace turi {

/** Given a sorted sparse vector of (index, value) pairs, efficiently
 *  find and return a pair of indices (i1, i2) in the vector such that
 *  (item_index_lb <= v[i].first < item_index_ub) for all i = (i1,
 *  ..., i2 - 1), and false otherwise.
 */
template <typename T>
GL_HOT_INLINE
std::pair<size_t, size_t>
find_slice_boundary_indices(const std::vector<std::pair<size_t, T> >& v,
                            size_t item_index_lb, size_t item_index_ub) {

  // The comparison function for finding the correct start and
  // end items for the loop.
  auto idx_cmp_f = [](const std::pair<size_t, T>& p1, const std::pair<size_t, T>& p2) {
    return p1.first < p2.first;
  };

  // We only work on sorted arrays, so make sure these indeed work.
  DASSERT_TRUE(std::is_sorted(v.begin(), v.end(), idx_cmp_f));

  // Handle the edge cases.
  if(v.empty()) {
    return {0, 0};
  }

  if(item_index_lb <= v.front().first && v.back().first < item_index_ub) {
    return {0, v.size()};
  }

  // Okay, need to actually find the indices.  Do this with binary
  // searches.
  size_t list_idx_start = 0;
  size_t list_idx_end = v.size();

  // If we don't start at the beginning, move up the list start to
  // find the correct bounds.
  if(!( item_index_lb <= v.front().first) ) {
    auto lb_it = std::lower_bound(v.begin(), v.end(),
                                  std::make_pair(item_index_lb, T()), idx_cmp_f);
    list_idx_start = std::distance(v.begin(), lb_it);
  }

  // Likewise, if we don't start at the end, move back the
  // end to the true element.
  if(!(v.back().first < item_index_ub)) {
    auto ub_it = std::lower_bound(v.begin() + list_idx_start,
                                  v.end(),
                                  std::make_pair(item_index_ub, T()), idx_cmp_f);
    list_idx_end = std::distance(v.begin(), ub_it);
  }

  if(list_idx_start != list_idx_end) {
    // Make sure we have indeed found the correct boundaries.
    DASSERT_LT(v[list_idx_end-1].first, item_index_ub);
    DASSERT_GE(v[list_idx_end-1].first, item_index_lb);
    DASSERT_LT(v[list_idx_start].first, item_index_ub);
    DASSERT_GE(v[list_idx_start].first, item_index_lb);
  }

  return {list_idx_start, list_idx_end};
}


/**  Iterates through a sparse vector sarray by column slices, with
 *   possible optimizations for other functionality.  Each row in
 *   `data` is assumed to be a sorted vector of (index, value) pairs,
 *   and this function does multiple passes through the data, with
 *   each pass handling a contiguous slice of the indices in each row.
 *   These slices are determined by the slice_delimiters vector.
 *
 *   Parameters:
 *   -------------
 *
 *   data -- an sarray of vectors of (index, value) pairs.  This vector
 *         must be sorted by index.
 *
 *   slice_delimiters -- A vector of length (num_slices - 1) that give
 *         the boundaries of the slices of indices.  For example, [0,
 *         5, 10, 12] would have 3 slices, with (0,5), (5, 10), and
 *         (10, 12) being the slices processed.
 *
 *   init_slice -- called at the beginning of each slice, before the
 *         data is processed.  init_slice has the signature
 *
 *             init_slice(size_t slice_idx, size_t item_idx_start, size_t item_idx_end)
 *
 *         where slice_idx is the slice counter (0, 1, ...), and
 *         item_idx_start and item_idx_end are taken from
 *         slice_boundaries[slice_idx] and slice_boundaries[slice_idx - 1].
 *
 *   preprocess_row -- the function called on every row.  It has
 *         the signature:
 *
 *              preprocess_row(size_t thread_idx, size_t row_idx,
 *                             size_t slice_item_idx_start, size_t slice_item_idx_end,
 *                             std::vector<std::pair<size_t, T> >& row)
 *
 *         In this case, row_idx is the row currently being processed,
 *         slice_item_idx_start and slice_item_idx_end are taken from
 *         the current slice, and row is the current row.  The row can
 *         be modified, but any resulting modifications must preserve
 *         the sorted nature. If the row is empty upon return, then
 *         processing all future elements is skipped.
 *
 *   process_element -- the function called on every element.  It has
 *         the signature:
 *
 *              process_element(size_t thread_idx, size_t row_idx,
 *                              size_t item_idx_start, size_t item_idx_end,
 *                              size_t item_idx, const T& value)
 *
 *
 *         In this case, row_idx is the row currently being processed,
 *         item_idx_start and item_idx_end are taken from the slice,
 *         item_idx is the index of the value in the row, and value is
 *         the value.
 *
 *   finalize_slice -- the function called at the end of every slice.
 *         It has the same signature as init_slice:
 *
 *              finalize_slice(size_t slice_idx, size_t item_idx_start, size_t item_idx_end)
 *
 */
template <typename T,
          typename SliceInitFunction,
          typename RowProcessFunction,
          typename ElementProcessFunction,
          typename SliceFinalizeFunction>
void iterate_through_sparse_item_array_by_slice(
    const std::shared_ptr<sarray<std::vector<std::pair<size_t, T> > > >& data,
    const std::vector<size_t>& slice_delimiters,
    SliceInitFunction&& init_slice,
    RowProcessFunction&& preprocess_row,
    ElementProcessFunction&& process_element,
    SliceFinalizeFunction&& finalize_slice) {

  const size_t n = data->size();

  if(n == 0)
    return;

  volatile bool user_cancelation = false;

  auto execute_user_cancelation = [&]() GL_GCC_ONLY(GL_COLD_NOINLINE) {
    user_cancelation = true;
    log_and_throw("Cancelled by user.");
  };

  auto check_user_cancelatation = [&]() GL_GCC_ONLY(GL_HOT_INLINE) {
    if(user_cancelation || cppipc::must_cancel()) {
      execute_user_cancelation();
    }
  };

  for(size_t slice_idx = 0; slice_idx < slice_delimiters.size() - 1; ++slice_idx) {
    check_user_cancelatation();

    // Get the proper slice_delimiters.
    size_t item_idx_start = slice_delimiters[slice_idx];
    size_t item_idx_end = slice_delimiters[slice_idx + 1];

    // Initialize the current slice.
    init_slice(slice_idx, item_idx_start, item_idx_end);

    // Check after the initialization function.
    check_user_cancelatation();

    auto data_it = make_sarray_block_iterator(data);

    // Time to rock and roll.
    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT) {

        // Buffer of elements.
        std::vector<std::vector< std::pair<size_t, T> > > item_buffer_v;

        while(true) {
          // Check in case of cancelation
          check_user_cancelatation();

          size_t block_row_index_start = 0;
          if(data_it.read_next(&block_row_index_start, &item_buffer_v) ) {
            break;
          }

          size_t n_rows_read = item_buffer_v.size();

          for(size_t inner_idx = 0; inner_idx < n_rows_read && !user_cancelation; ++inner_idx) {

            // Check at the start here, before anything happens.
            check_user_cancelatation();

            size_t row_idx = block_row_index_start + inner_idx;
            auto& item_list_nonconst = item_buffer_v[inner_idx];

            // Preprocess the row.
            preprocess_row(thread_idx, row_idx, item_idx_start, item_idx_end, item_list_nonconst);

            // If it's empty, then ignore it.
            if(item_list_nonconst.empty()) {
              continue;
            }

            // Check at the end of processing a row.
            check_user_cancelatation();

            const auto& item_list = item_list_nonconst;

            size_t list_idx_start = 0;
            size_t list_idx_end = item_list.size();

            // Adjust the slice indices if needed.
            std::tie(list_idx_start, list_idx_end)
                = find_slice_boundary_indices(
                    item_list, item_idx_start, item_idx_end);

            // If this one is empty, ignore.
            if(UNLIKELY(list_idx_start == list_idx_end))
              continue;

            for(size_t idx_a = list_idx_start; idx_a < list_idx_end; ++idx_a) {

              DASSERT_GE(item_list[idx_a].first, item_idx_start);
              DASSERT_LT(item_list[idx_a].first, item_idx_end);

              process_element(thread_idx, row_idx,
                              item_idx_start, item_idx_end,
                              item_list[idx_a].first, item_list[idx_a].second);
            }
          } // End inner loop over rows.
        } // End outer loop over rows.
      });

    // Check at the end of processing a slice, before the finalize
    // slice function is called.
    check_user_cancelatation();

    finalize_slice(slice_idx, item_idx_start, item_idx_end);
  }
}

/**  Iterates through a sparse vector sarray efficiently, calling a
 *   prescribed function on each row and on each element.
 *
 *   Parameters:
 *   -------------
 *
 *   data -- an sarray of vectors of (index, value) pairs.  This vector
 *         must be sorted by index.
 *
 *   preprocess_row -- the function called on every row.  It has
 *         the signature:
 *
 *              preprocess_row(size_t thread_idx, size_t row_idx,
 *                             std::vector<std::pair<size_t, T> >& row)
 *
 *         In this case, row_idx is the row currently being processed
 *         and row is the current row.  Any modifications to row are
 *         discarded after this.
 *
 */
template <typename T, typename RowProcessFunction>
void iterate_through_sparse_item_array(
    const std::shared_ptr<sarray<std::vector<std::pair<size_t, T> > > >& data,
    RowProcessFunction&& process_row) {

  // This is a convenience wrapper around the main function,
  // iterate_through_sparse_item_array_by_slice.  It does it by
  // assuming the only slice is just (0, inf) -- i.e. one slice.
  //
  // Because of this, most of the functions will be empty.
  auto empty_slice_function = [&](size_t slice_idx, size_t item_idx_start, size_t item_idx_end)
      GL_GCC_ONLY(GL_HOT_INLINE)
      {};

  // This is the only
  auto _process_row = [&](size_t thread_idx, size_t row_idx,
                            size_t slice_item_idx_start, size_t slice_item_idx_end,
                            std::vector<std::pair<size_t, T> >& row)
      GL_GCC_ONLY(GL_HOT_INLINE) {

    // Pass the appropriate parts to the process_row function
    process_row(thread_idx, row_idx, row);

    // This disables further work on this row.
    row.clear();
  };

  auto empty_process_element = [&](size_t thread_idx, size_t row_idx,
                                   size_t item_idx_start, size_t item_idx_end,
                                   size_t item_idx, const T& value)
      GL_GCC_ONLY(GL_HOT_INLINE)
      {};

  // Now, pass these on to the main utility function.
  iterate_through_sparse_item_array_by_slice(
      data,
      {0, std::numeric_limits<size_t>::max()},
      empty_slice_function,
      _process_row,
      empty_process_element,
      empty_slice_function);
}

/**  Transpose a sparse sarray of sorted vectors of entry pairs.  Does
 *   it effeciently and close to within memory bounds.
 *
 *   The input data is a sarray of vectors of (column_index, value)
 *   pairs.  This is then transposed to an similar sarray of vectors
 *   of (row_index, value) pairs, where each row consists of the
 *   matching values of column_index in the original sarray.  The
 *   resulting vectors are sorted.
 *
 *   To do this efficiently, the number of elements for each
 *   column_index are required ahead of time.  This is passed in as
 *   item_counts, which should be of size equal to the column
 *   dimension.
 *
 *   max_memory_usage gives the maximum memory allowed for the
 *   computation.  The larger max_memory_usage is, the fewer passes
 *   through the data are required.
 */
template <typename T>
std::shared_ptr<sarray<std::vector<std::pair<size_t, T> > > > transpose_sparse_sarray(
    std::shared_ptr<sarray<std::vector<std::pair<size_t, T> > > > data,
    const std::vector<size_t>& item_counts,
    size_t max_memory_usage) {

  size_t num_items = item_counts.size();

  // Figure out how many items on each pass can be stored in memory.
  std::vector<size_t> slice_delimiters = {0};

  size_t mem_usage = 0;

  for(size_t i = 0; i < item_counts.size(); ++i) {
    size_t this_mem_usage = sizeof(std::pair<size_t, T>) * item_counts[i];

    if(mem_usage + this_mem_usage > max_memory_usage) {

      slice_delimiters.push_back(i);
      mem_usage = 0;
    } else {
      mem_usage += this_mem_usage;
    }

    if(slice_delimiters.size() > 256) {
      log_and_throw( ( std::string("Memory limit of ")
                       + std::to_string(max_memory_usage)
                       + " too low to efficiently transpose sparse sarray.").c_str() );
    }
  }

  slice_delimiters.push_back(num_items);

  // Set up the transpose.
  const size_t n = data->size();
  const size_t num_slices = slice_delimiters.size() - 1;

  std::vector<size_t> row_locations;
  std::vector<atomic<size_t> > row_sizes;
  std::vector<std::pair<size_t, T> > slice_t_data;

  // If we have multiple passes, do this so we don't do expensive
  // reallocs later.
  if(num_slices > 1) {
    slice_t_data.reserve(max_memory_usage);
  }

  logprogress_stream << "Transposing user-item lists for use in nearest neighbor search. "
                     << std::endl;

  table_printer table( { {"Elapsed Time (Data Transposition))", 0}, {"% Complete", 0} } );
  table.print_header();

  atomic<size_t> row_count = 0;
  size_t total_rows_to_process = n * num_slices;

  ////////////////////////////////////////////////////////////////////////////////
  // Run through each index and add a cutoff to the

  auto init_slice = [&](size_t slice_idx, size_t item_idx_start, size_t item_idx_end) {
    size_t num_items_in_slice = item_idx_end - item_idx_start;

    row_locations.resize(num_items_in_slice + 1);

    size_t item_cumsum = 0;
    for(size_t i = 0; i < num_items_in_slice; ++i) {
      row_locations[i] = item_cumsum;
      item_cumsum += item_counts[i + item_idx_start];
    }

    row_sizes.resize(num_items_in_slice);
    std::fill(row_sizes.begin(), row_sizes.end(), 0);

    slice_t_data.resize(item_cumsum);
    row_locations[num_items_in_slice] = item_cumsum;
  };

  // nothing to be done for preprocessing a row.
  auto empty_preprocess_row = [&](size_t thread_idx, size_t row_idx,
                                  size_t slice_item_idx_start, size_t slice_item_idx_end,
                                  std::vector<std::pair<size_t, T> >& row)
      GL_GCC_ONLY(GL_HOT_INLINE) {
        size_t cur_row_count = (++row_count) - 1;

        if(UNLIKELY(cur_row_count % 100 == 0)) {
          double percent_complete = double((400 * cur_row_count) / total_rows_to_process) / 4;

          table.print_timed_progress_row(progress_time(), percent_complete);
        }
      };


  ////////////////////////////////////////////////////////////////////////////////
  // Stuff for processing each element within a slice.  This means
  // putting it in it's rightful spot in the transpose line.

  auto process_element = [&](size_t thread_idx, size_t row_idx,
                             size_t item_idx_start, size_t item_idx_end,
                             size_t item_idx, const T& value) {

    size_t internal_idx = item_idx - item_idx_start;
    size_t item_count_idx = (++ (row_sizes[internal_idx]) ) - 1;

    DASSERT_LT(item_count_idx, item_counts[item_idx]);

    slice_t_data[row_locations[internal_idx] + item_count_idx] = {row_idx, value};
  };

  ////////////////////////////////////////////////////////////////////////////////
  // Stuff for writing out the slice.
  auto out_data = std::make_shared<sarray<std::vector<std::pair<size_t, T> > > >();
  out_data->open_for_write(1);
  auto it_out = out_data->get_output_iterator(0);

  auto finalize_slice = [&](size_t slice_idx, size_t item_idx_start, size_t item_idx_end) {
    size_t num_items_in_slice = item_idx_end - item_idx_start;

    DASSERT_EQ(row_locations.size(), num_items_in_slice + 1);

    atomic<size_t> process_idx = 0;

    // To do the writing, we allow one of the threads to write finished rows
    size_t write_idx = 0;
    dense_bitset idx_is_finished(num_items_in_slice);

    std::vector<std::pair<size_t, T> > row_out;
    auto flush_next_row = [&]() GL_GCC_ONLY(GL_HOT_INLINE) {
      DASSERT_LT(write_idx, num_items_in_slice);
      DASSERT_TRUE(idx_is_finished.get(write_idx));

      row_out.assign(slice_t_data.begin() + row_locations[write_idx],
                     slice_t_data.begin() + row_locations[write_idx + 1]);

      *it_out = row_out;
      ++it_out, ++write_idx;
    };

    // First, go through and sort all of the individual slices.
    in_parallel([&](size_t thread_idx, size_t num_threads) {

        while(true) {

          if(thread_idx == 0) {
            // As long as thread 0 has stuff to write, then write it
            // out while the rest of tqhe threads handle the sorting.
            // This gives us a head start on writing the rest out as
            // below
            while(write_idx < num_items_in_slice && idx_is_finished.get(write_idx)) {
              flush_next_row();
            }
          }

          size_t idx = (++process_idx) - 1;

          if(idx >= num_items_in_slice) {
            break;
          }

          std::sort(slice_t_data.begin() + row_locations[idx],
                    slice_t_data.begin() + row_locations[idx + 1],
                    [](const std::pair<size_t, T>& p1,
                       const std::pair<size_t, T>& p2) {
                      return p1.first < p2.first;
                    });

          idx_is_finished.set_bit(idx);
        }
      });

    // Now, flush the remaining rows that may have been missed at the
    // end of the parallel portion.
    while(write_idx < num_items_in_slice) {
      flush_next_row();
    }

  }; // End finalize the slice.

  // Now actually run all of the above.
  iterate_through_sparse_item_array_by_slice(
      data,
      slice_delimiters,
      init_slice,
      empty_preprocess_row,
      process_element,
      finalize_slice);

  out_data->close();

  table.print_row(progress_time(), 100);
  table.print_footer();

  return out_data;
}

}

#endif /* UTILITIES_H */
