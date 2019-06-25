/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SPARSE_SIMILARITY_NEIGHBOR_SEARCH_H
#define TURI_SPARSE_SIMILARITY_NEIGHBOR_SEARCH_H

#include <core/storage/sframe_data/sarray.hpp>
#include <vector>
#include <core/parallel/pthread_tools.hpp>
#include <core/util/try_finally.hpp>
#include <core/util/dense_bitset.hpp>
#include <toolkits/sparse_similarity/similarities.hpp>
#include <toolkits/sparse_similarity/item_processing.hpp>

namespace turi { namespace sparse_sim {

/** Efficiently perform an all-pairs brute force processing as
 *  possible over sarrays of sparse, sorted vectors.
 *
 *  reference_data and query_data are two sarrays of sparse vectors.
 *  A similarity score is calculated between each entry of
 *  reference_data and each entry of query_data, with process_function
 *  called for each (minus exceptions below).   *
 *
 *  Reference_item_info and query_item_info are obtained from calling
 *  one of the methods in item_processing.hpp.
 *
 *  The SimilarityType similarity function is defined as one of the
 *  classes in similarities.hpp, or a class that also conforms to a
 *  similar interface.
 *
 *  process_function should have the signature
 *
 *    process_function(size_t reference_idx, size_t query_idx, double similarity)
 *
 *  It is called in parallel for each reference and query entry.
 *
 *  num_dimensions is the maximum dimension of each sparse vector. An
 *  error is raised if any index is >= num_dimensions.
 *
 *  max_memory_usage is used to determine the block size for doing the
 *  query; a larger value of this means fewer passes through the
 *  reference set.
 *
 *  skip_pair has the signature
 *
 *     skip_pair(size_t reference_idx, size_t query_idx) -> bool
 *
 *  If true, then the similarity score for that item is not calculated
 *  for that reference_idx and query_idx pair.  Normally, this can be
 *  set to return false, in which case nothing is skipped.  (This is
 *  used, for example, if the reference_data and the query_data are the
 *  same, and only one direction is calculated.
 *
 *  If provided, query_mask is a dense_bitset of the same length as
 *  query_data.  If a particular entry is false, then that row is
 *  skipped in the similarity comparisons.
 */
template <typename SimilarityType,
          typename ProcessFunction,
          typename SkipFunction>
void brute_force_all_pairs_similarity_with_vector_reference(
    std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > > reference_data,
    const std::vector<item_processing_info<SimilarityType> >& reference_item_info,
    std::shared_ptr<sarray<std::vector<std::pair<size_t, double> > > > query_data,
    const std::vector<item_processing_info<SimilarityType> >& query_item_info,
    const SimilarityType& similarity,
    ProcessFunction&& process_function,
    size_t num_dimensions,
    size_t max_memory_usage,
    SkipFunction&& skip_pair,
    const dense_bitset* query_mask = nullptr) {

  // The vertex type is used as reference later on.
  typedef typename SimilarityType::item_data_type item_data_type;
  typedef typename SimilarityType::interaction_data_type interaction_data_type;
  typedef typename SimilarityType::final_item_data_type final_item_data_type;
  typedef typename SimilarityType::final_interaction_data_type final_interaction_data_type;

  // Set constants used later.
  static constexpr bool use_final_item_data = (
      !std::is_same<unused_value_type, final_item_data_type>::value);

  static constexpr bool missing_values_are_zero = SimilarityType::missing_values_are_zero();

  final_item_data_type _unused;

  // constants used later
  size_t max_num_threads = thread::cpu_count();
  size_t num_query_rows = query_data->size();
  size_t num_reference_rows = reference_data->size();
  bool using_mask = (query_mask != nullptr);

  ////////////////////////////////////////////////////////////////////////////////
  // Check input

  DASSERT_NE(num_dimensions, 0);

  DASSERT_EQ(reference_item_info.size(), reference_data->size());
  DASSERT_EQ(query_item_info.size(), query_data->size());

  if(using_mask) {
    DASSERT_EQ(query_mask->size(), query_data->size());
  }

  ////////////////////////////////////////////////////////////////////////////////
  // If we are using a mask, then the number of query rows is
  // calculated from that.
  if(using_mask) {
    num_query_rows = query_mask->popcount();
  }

  // Nothing to do here.
  if(num_query_rows == 0)
    return;

  // Set the block size as a function of the maximum memory usage.
  size_t max_query_rows_per_block = max_memory_usage / (num_dimensions * sizeof(double));
  max_query_rows_per_block = std::max(max_num_threads, max_query_rows_per_block);
  max_query_rows_per_block = std::min(num_query_rows, max_query_rows_per_block);

  // This is needed, as we use an int counter in the inner loop to
  // enable easier autovectorization (see
  // http://www.slideshare.net/linaroorg/using-gcc-autovectorizer).
  max_query_rows_per_block = std::min<size_t>(
      std::numeric_limits<int>::max() / 2, max_query_rows_per_block);

  // Set up the number of blocks.  Number of blocks is the ceiling of
  // all this.
  size_t num_blocks = (num_query_rows + (max_query_rows_per_block - 1)) / max_query_rows_per_block;

  // Now that we have the number of blocks, further minimize memory
  // use by making the number of query rows per block as even as
  // possible.  That way we won't end up with a single block that has
  // like 1 row and the rest that have many more.

  max_query_rows_per_block = (num_query_rows + (num_blocks - 1)) / num_blocks;

  // Get the reader for the query data.
  auto query_reader = query_data->get_reader(max_num_threads);

  // Get the reader for the reference data
  auto reference_reader = reference_data->get_reader(max_num_threads);

  // Set up the query data so that all dimensions are contiguous in
  // memory.  That way, on a query, we can do everything for this
  // element together, thereby optimizing memory access and the
  // increasing the likelihood that the compiler can vectorize it.
  std::vector<double> block_data(max_query_rows_per_block * num_dimensions);
  auto block_data_index = [&](size_t row_idx, size_t element_idx) {
    DASSERT_LT(row_idx, max_query_rows_per_block);
    DASSERT_LT(element_idx, num_dimensions);
    return element_idx * max_query_rows_per_block + row_idx;
  };

  // For all the rows in the current block, this is the actual row
  // index within that block.
  std::vector<size_t> block_query_row_indices(max_query_rows_per_block);

  // The vertex info for each of these rows.
  std::vector<item_data_type> block_item_data(max_query_rows_per_block);

  std::vector<final_item_data_type> block_final_item_data;
  if(use_final_item_data) {
    block_final_item_data.resize(max_query_rows_per_block);
  }

  // Counters indicating where we are within each segment, as each
  // thread reads a new segment.
  std::vector<size_t> query_row_counters(max_num_threads, size_t(-1));

  // Loop over the blocks.
  for(size_t block_idx = 0; block_idx < num_blocks; ++block_idx) {

    // This is the location of the current open slot for dumping one of the rows
    atomic<size_t> block_write_idx = 0;

    // Clear out the data in this block.
    std::fill(block_data.begin(), block_data.end(), missing_values_are_zero ? 0 : NAN);

    // Fill the block with appropriate rows.
    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {

        // This is the segment we are responsible for in this thread.
        size_t query_row_idx_start = (query_data->size() * thread_idx) / num_threads;
        size_t query_row_idx_end = (query_data->size() * (thread_idx+1)) / num_threads;

        // Get the overall current_query_row_index where we are at within the segment
        // this thread is assigned to.
        size_t& current_query_row_index = query_row_counters[thread_idx];

        // Check for initializing it at the appropriate location.
        if(current_query_row_index == size_t(-1)) {
          // It has not been initialized yet; do it at the start of our segment.
          current_query_row_index = query_row_idx_start;
        }

        // Row buffer.
        std::vector<std::vector<std::pair<size_t, double> > > row_v(1);

        // Now, read in rows until we are out of space in this block,
        // or until we are out of rows in this reading segment.
        while(current_query_row_index < query_row_idx_end) {

          // If we are using the query mask, then check if we are in a
          // valid spot.  If not, then advance forward until we are.
          if(using_mask && !query_mask->get(current_query_row_index)) {
            size_t new_q_idx = current_query_row_index;
            bool any_more = query_mask->next_bit(new_q_idx);

            if(UNLIKELY(!any_more || new_q_idx >= query_row_idx_end)) {
              // Done.
              current_query_row_index = query_row_idx_end;
              break;
            } else {
              DASSERT_NE(current_query_row_index, new_q_idx);

              // Next row.
              current_query_row_index = new_q_idx;
            }
          }

          if(using_mask) {
            // Just make sure we've got a live one.
            DASSERT_TRUE(query_mask->get(current_query_row_index));
          }

          // Get the next index.
          size_t internal_block_idx = (++block_write_idx) - 1;

          // Do we have a place in the to put this?  If not, break and
          // leave this position for the next block.
          if(internal_block_idx >= max_query_rows_per_block) {
            break;
          }

          // Assert that we do indeed have a row left.
          DASSERT_LT(current_query_row_index, query_row_idx_end);

          // Now that we know we have a spot in the block, write it
          // out to the block data.
          query_reader->read_rows(current_query_row_index, current_query_row_index + 1, row_v);
          const auto& row = row_v[0];

          // Write
          block_query_row_indices[internal_block_idx] = current_query_row_index;
          block_item_data[internal_block_idx] = query_item_info[current_query_row_index].item_data;

          // Write out the final vertex data.
          if(use_final_item_data) {
            block_final_item_data[internal_block_idx]
                = query_item_info[current_query_row_index].final_item_data;
          }

          // Write the row out to the block data.
          for(size_t i = 0; i < row.size(); ++i) {
            size_t idx = block_data_index(internal_block_idx, row[i].first);
            block_data[idx] = row[i].second;
          }

          // Finally, advance the counter to continue.
          ++current_query_row_index;
        }

        // If we are on the last pass, make sure that we have
        // covered all the query data.
        if(block_idx == num_blocks - 1) {
          DASSERT_EQ(current_query_row_index, query_row_idx_end);
        }
      });

    // Check to make sure our math is correct regarding the number of query
    // rows and the number of blocks.
#ifndef NDEBUG
    {
      size_t _block_write_idx = block_write_idx; // cause of atomic
      if(block_idx < num_blocks - 1) {
        DASSERT_GE(_block_write_idx, max_query_rows_per_block);
      } else {
        DASSERT_LE(_block_write_idx, max_query_rows_per_block);
      }

      // Readjust the size of the block (num_query_rows_in_block) if needed.
      if(block_write_idx < max_query_rows_per_block) {
        // Everything is done, so it must be in the last block
        DASSERT_EQ(block_idx, num_blocks - 1);
      }
    }
#endif

    // Set the number of query rows in this block.  The
    // block_write_idx may have been incremented multiple times by
    // different threads.
    size_t num_query_rows_in_block = std::min<size_t>(block_write_idx, max_query_rows_per_block);

      // If all the math is correct, this block will never be empty.
    DASSERT_GT(num_query_rows_in_block, 0);

    // Now, if we're using a mask, make sure all the indices are
    // masked properly.
#ifndef NDEBUG
    {
      if(using_mask) {
        for(size_t i = 0; i < num_query_rows_in_block; ++i) {
          DASSERT_TRUE(query_mask->get(block_query_row_indices[i]));
        }
      }
    }
#endif

    // Okay, now that we have a specific block of query data, go
    // through and perform the nearest neighbors query on it.
    in_parallel([&](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {
        // This is the segment we are responsible for in this thread.
        size_t reference_row_idx_start = (num_reference_rows * thread_idx) / num_threads;
        size_t reference_row_idx_end = (num_reference_rows * (thread_idx+1)) / num_threads;

        const size_t n_reference_rows_per_block = 16;
        std::vector<std::vector<std::pair<size_t, double> > > reference_rows_v;

        std::vector<interaction_data_type> edges(num_query_rows_in_block);

        // Read it in in blocks of n_reference_rows_per_block rows for efficiency.
        for(size_t outer_idx = reference_row_idx_start;
            outer_idx < reference_row_idx_end;
            outer_idx += n_reference_rows_per_block) {

          ////////////////////////////////////////////////////////////////////////////////

          reference_reader->read_rows(
              outer_idx,
              std::min(outer_idx + n_reference_rows_per_block, reference_row_idx_end),
              reference_rows_v);

          if(reference_rows_v.size() != n_reference_rows_per_block) {
            DASSERT_EQ(outer_idx + reference_rows_v.size(), reference_row_idx_end);
          }

          // Now over rows in the buffer.
          for(size_t inner_idx = 0; inner_idx < reference_rows_v.size(); ++inner_idx) {

            // Now, for each row, go through and calculate the full intersection.
            const size_t ref_idx = outer_idx + inner_idx;
            const auto& row = reference_rows_v[inner_idx];

            // Get the information for this particular vertex.
            item_data_type ref_item_data = reference_item_info[ref_idx].item_data;

            const final_item_data_type& ref_final_item_data
                = reference_item_info[ref_idx].final_item_data;

            // Zero the edges.
            edges.assign(num_query_rows_in_block, interaction_data_type());

            // Get the vertex for this one here.
            for(const auto& p : row) {
              size_t dim_index = p.first;
              double ref_value = p.second;

              if(missing_values_are_zero) {
                // This is in the inner loop, so a lot of time is spent
                // in this computation.  Try to make it as friendly as
                // possible to the vectorizer as possible.

                double* __restrict__ bd_ptr = &(block_data[block_data_index(0, dim_index)]);
                item_data_type* __restrict__ it_data_ptr = block_item_data.data();
                interaction_data_type* __restrict__ int_data_ptr = edges.data();

                for(int i = 0; i < int(num_query_rows_in_block);
                    ++i, ++bd_ptr, ++it_data_ptr, ++int_data_ptr) {

                  similarity.update_interaction_unsafe(
                      *int_data_ptr,
                      ref_item_data, *it_data_ptr,
                      ref_value, *bd_ptr);
                }
              } else {
                for(size_t i = 0; i < num_query_rows_in_block; ++i) {
                  // branching on individual entries, so can't do
                  // vectorization here anyway.
                  double block_data_entry = block_data[block_data_index(i, dim_index)];

                  if(std::isnan(block_data_entry))
                    continue;

                  // Aggregate it along this edge.
                  similarity.update_interaction_unsafe(
                      edges[i],
                      ref_item_data, block_item_data[i],
                      ref_value, block_data_entry);
                }
              }
            }

            // Now, go through, finalize the answers, and record them.
            for(size_t i = 0; i < num_query_rows_in_block; ++i) {
              size_t query_index = block_query_row_indices[i];

              if(skip_pair(query_index, ref_idx))
                continue;

              // Get the vertex and value info for this query row.
              const auto& q_item_data = block_item_data[i];
              const auto& q_final_item_data =
                  (use_final_item_data ? block_final_item_data[i] : _unused);

              // Set up the output value.
              final_interaction_data_type e_out = final_interaction_data_type();

              similarity.finalize_interaction(e_out,
                                              ref_final_item_data, q_final_item_data,
                                              edges[i],
                                              ref_item_data, q_item_data);

              // Now do the meat of the operation -- record the result.
              process_function(ref_idx, query_index, e_out);
            }
          }
        }
      });

    // Now, we're done, so go to the next block.
  }
}

/** An easier-to-use wrapper for the above nearest neighbors search.
 *  reference_data and query_data are two sarrays of sparse vectors.
 *  A similarity score is calculated between each entry of
 *  reference_data and each entry of query_data, with process_function
 *  called for each (minus exceptions below).

 *  The SimilarityType similarity function is defined as one of the
 *  classes in similarities.hpp, or a class that also conforms to a
 *  similar interface.
 *
 *  process_function should have the signature
 *
 *    process_function(size_t reference_idx, size_t query_idx, double similarity)
 *
 *  It is called in parallel for each reference and query entry.
 *
 *  max_memory_usage is used to determine the block size for doing the
 *  query; a larger value of this means fewer passes through the
 *  reference set.
 *
 *  skip_pair has the signature
 *
 *     skip_pair(size_t reference_idx, size_t query_idx) -> bool
 *
 *  If true, then the similarity score for that item is not calculated
 *  for that reference_idx and query_idx pair.  Normally, this can be
 *  set to return false, in which case nothing is skipped.  (This is
 *  used, for example, if the reference_data and the query_data are the
 *  same, and only one direction is calculated.
 *
 *  If provided, query_mask is a dense_bitset of the same length as
 *  query_data.  If a particular entry is false, then that row is
 *  skipped in the similarity comparisons.
 */
template<typename T,
         typename SimilarityType,
         typename ProcessFunction,
         typename SkipFunction>
void all_pairs_similarity(
    std::shared_ptr<sarray<std::vector<std::pair<size_t, T> > > > reference_data,
    std::shared_ptr<sarray<std::vector<std::pair<size_t, T> > > > query_data,
    const SimilarityType& similarity,
    ProcessFunction&& process_function,
    size_t max_memory_usage,
    SkipFunction&& skip_pair,
    const dense_bitset* query_mask = nullptr) {


  std::vector<item_processing_info<SimilarityType> > reference_item_info;
  std::vector<item_processing_info<SimilarityType> > query_item_info;

  size_t reference_num_users = 0;
  size_t query_num_users = 0;

  reference_num_users = calculate_item_processing_rowwise(
      reference_item_info, similarity, reference_data);

  if(query_data.get() != reference_data.get()) {
    query_num_users = calculate_item_processing_rowwise(query_item_info, similarity, query_data);
  }

  size_t num_dimensions = std::max(reference_num_users, query_num_users);

  // Now, we have everything we need to use the above function.
  brute_force_all_pairs_similarity_with_vector_reference(
      reference_data,
      reference_item_info,
      query_data,
      (query_data.get() == reference_data.get()) ? reference_item_info : query_item_info,
      similarity,

      // The process function similarity has has to be translated, so do that here.
      [&](size_t i, size_t j, const typename SimilarityType::final_interaction_data_type& v)
      GL_GCC_ONLY(GL_HOT_INLINE_FLATTEN)
      {
        process_function(i, j, similarity.export_similarity_score(v));
      },
      num_dimensions,
      max_memory_usage,
      skip_pair,
      query_mask);
}

}}


#endif
