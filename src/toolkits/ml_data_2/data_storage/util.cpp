/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/ml_data_2/data_storage/util.hpp>
#include <toolkits/ml_data_2/data_storage/ml_data_row_format.hpp>

namespace turi { namespace v2 { namespace ml_data_internal {

/** Estimate the number of rows to stick into one block.  The goal is
 *  to have it about ML_DATA_TARGET_ROW_BYTE_MINIMUM bytes per block.
 *
 */
size_t estimate_row_block_size(
    size_t original_sframe_num_rows,
    const row_metadata& rm,
    const std::vector<std::shared_ptr<sarray<flexible_type>::reader_type> >& column_readers) {

  DASSERT_EQ(rm.metadata_vect.size(), column_readers.size());

  const size_t TARGET_NUM_ELEMENTS = ML_DATA_TARGET_ROW_BYTE_MINIMUM / sizeof(entry_value);

  size_t median_row_size = 0;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1.  First see if all the rows have constant size.  If they
  // do, we know the median!

  if(rm.data_size_is_constant) {
    median_row_size = rm.constant_data_size;
  } else {

    ////////////////////////////////////////////////////////////////////////////////
    // Step 2.1.  First see how many rows we have with known sizes
    // known without loading the data.  That makes the determination
    // of the number of rows easy.

    size_t base_row_size = 0;

    for(const column_metadata_ptr& m : rm.metadata_vect) {
      if(m->mode_has_fixed_size())
        base_row_size += m->fixed_column_size();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Step 2.2.  Go through the remaining rows and figure out the
    // sizes from estimates.  The tricky thing here is that we need to
    // be able to handle both extremely sparse data and things like
    // extremely large dictionaries.

    size_t n_sizes = 1000;
    n_sizes = std::min(n_sizes, original_sframe_num_rows);

    std::vector<size_t> row_sizes(n_sizes, base_row_size);

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        std::vector<flexible_type> buffer;

        size_t start_row = (thread_idx * n_sizes) / num_threads;
        size_t end_row = ((thread_idx + 1) * n_sizes) / num_threads;

        for(size_t c_idx = 0; c_idx < rm.total_num_columns; ++c_idx) {
          const column_metadata_ptr& m = rm.metadata_vect[c_idx];

          // This is already included.
          if(m->mode_has_fixed_size())
            continue;

          // Read off in blocks of 16.  That way we are somewhat fast,
          // but don't implode on ginourmous dictionaries.
          for(size_t row = start_row; row < end_row; row += 16) {
            size_t r_start = row;
            size_t r_end = std::min(row + 16, end_row);

            column_readers[c_idx]->read_rows(r_start, r_end, buffer);

            for(size_t i = 0; i < buffer.size(); ++i)
              row_sizes[r_start + i] += estimate_num_data_entries(m, buffer[i]);
          }
        }
      });

    size_t n_over_2 = row_sizes.size() / 2;

    std::nth_element(row_sizes.begin(), row_sizes.begin() + n_over_2, row_sizes.end());

    median_row_size = row_sizes[n_over_2];
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3:  Set the target number of rows per block.

  median_row_size = std::max(size_t(1), median_row_size);

  size_t target_rows_per_block =
      size_t(std::exp2(std::ceil(1 + std::log2(1 + double(TARGET_NUM_ELEMENTS) / median_row_size))));

  // Absolute guard against this being 0
  return std::max(size_t(1), target_rows_per_block);
}

}}}
