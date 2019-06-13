/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <core/system/platform/timer//timer.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_constants.hpp>
#include <core/storage/sframe_data/sframe_config.hpp>
#include <core/storage/sframe_data/sarray_v2_block_manager.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
namespace turi {
namespace query_eval {
using sframe_config::SFRAME_SORT_BUFFER_SIZE;

/**
* This returns the number of bytes after LZ4 decode needed for each column.
*
* This is used as a proxy for the size of the column. It does not represent
* at all, the actual size of the column in memory, but is a reasonable proxy.
*
* For instance, integers can be compressed to as low as 1 bit per integer,
* while in memory it will require 16 bytes.
*
* However, for complex values such as dicts, arrays, lists, this should be
* closer to the true in memory size; probably within a factor of 2 or 4.
* \param values The sframe
* \return a list of length ncolumns. Element i is the number of bytes on disk
*         used to store the column without LZ4 encoding.
*/
static std::vector<size_t> num_bytes_per_column(sframe& values) {
  std::vector<size_t> column_num_bytes(values.num_columns(), 0);
  auto& block_manager = v2_block_impl::block_manager::get_instance();

  // for each column
  for (size_t i = 0;i < values.num_columns(); ++i) {
    auto cur_column = values.select_column(i);
    auto column_index = cur_column->get_index_info();
    // for each segment in the column
    for(auto segment_file: column_index.segment_files) {
      auto segment_address = block_manager.open_column(segment_file);
      auto num_blocks_in_current_segment =
          block_manager.num_blocks_in_column(segment_address);
      // for each block in the segment
      for (size_t block_number = 0;
           block_number < num_blocks_in_current_segment;
           ++block_number) {
        v2_block_impl::block_address block_address{
                                std::get<0>(segment_address),
                                std::get<1>(segment_address),
                                block_number};
        column_num_bytes[i] += block_manager.get_block_info(block_address).block_size;
      }
      block_manager.close_column(segment_address);
    }
  }
  return column_num_bytes;
}

/**
 * Given the storage requirements of a column (via num_bytes_per_column),
 * and its type, return an estimate of the number of bytes of memory required
 * per value.
 */
static size_t column_bytes_per_value_estimate(size_t column_num_bytes,
                                              size_t num_rows,
                                              flex_type_enum column_type) {
  // initial estimate
  size_t bytes_per_value = (column_num_bytes + num_rows - 1) / num_rows;
  // fix up the estimate based on the type
  switch(column_type) {
   case flex_type_enum::INTEGER:
   case flex_type_enum::FLOAT:
   case flex_type_enum::DATETIME:
     // these are stored entirely within the flexible_type
     bytes_per_value = sizeof(flexible_type);
     break;
   case flex_type_enum::STRING:
     // these incur some constant
     bytes_per_value += (sizeof(flexible_type) + sizeof(flex_string));
     break;
   case flex_type_enum::VECTOR:
     // these incur some constant
     bytes_per_value += (sizeof(flexible_type) + sizeof(flex_vec));
     break;
   default:
     // everything else is complicated to estimate
     // so we just scale it up by a slack factor of 2
     bytes_per_value = bytes_per_value * 2 + sizeof(flexible_type);
     break;
  }
  return bytes_per_value;
}



/**
 * An SArray is stored internally as blocks, this returns the block boundaries.
 *
 * Given an SArray, open the column and enumerate all the blocks, returning
 * the block boundaries. This is the optimal read order.
 */
static std::vector<size_t> column_row_boundaries(std::shared_ptr<sarray<flexible_type>> cur_column) {
  std::vector<size_t> row_boundaries;
  auto column_index = cur_column->get_index_info();
  auto& block_manager = v2_block_impl::block_manager::get_instance();

  // keep track of the number of rows
  size_t row_number = 0;
  row_boundaries.push_back(row_number);

  // for each segment in the column
  for(auto segment_file: column_index.segment_files) {
    // open the segment
    auto segment_address = block_manager.open_column(segment_file);
    auto num_blocks_in_current_segment =
        block_manager.num_blocks_in_column(segment_address);
    // for each block in the segment
    for (size_t block_number = 0;
         block_number < num_blocks_in_current_segment;
         ++block_number) {
      // get the block, and count the number of rows
      v2_block_impl::block_address block_address{
                              std::get<0>(segment_address),
                              std::get<1>(segment_address),
                              block_number};
      size_t nrows = block_manager.get_block_info(block_address).num_elem;
      row_number += nrows;
      row_boundaries.push_back(row_number);
    }
    // remember to close the segment
    block_manager.close_column(segment_address);
  }
  return row_boundaries;
}
/**
 * A subroutine of ec_permute.
 *
 * Scatters the input into a collection of buckets using the forward_map.
 * The forward_map must be an SArray of the same length as the input and contain
 * every integer from 0 to len-1.
 *
 * Returns an sframe. The last column of the sframe is the per-bucket
 * forward map.
 */
static sframe ec_scatter_partitions(sframe input,
                                    size_t rows_per_bucket,
                                    const std::vector<bool>& indirect_column,
                                    std::shared_ptr<sarray<flexible_type> > forward_map) {
  //  - For each (c,r,v) in data:
  //        Write (c,v) to bucket `bucket of forward_map(r)`
  //  - For each (c,r,v) in forward_map:
  //        Write (c,v) to bucket `bucket of forward_map(r)`
  //
  //  In the implementation, each bucket corresponds to one segment.
  logstream(LOG_INFO) << "input size " << input.size() << std::endl;
  logstream(LOG_INFO) << "forward map size " << forward_map->size() << std::endl;
  input = input.add_column(forward_map);
  auto num_buckets = (input.size() + rows_per_bucket - 1)/ rows_per_bucket;
  sframe output;
  auto out_column_types = input.column_types();
  for (size_t i = 0;i < out_column_types.size(); ++i) {
    if (i < indirect_column.size() &&
        indirect_column[i]) out_column_types[i] = flex_type_enum::INTEGER;
  }
  output.open_for_write(input.column_names(), out_column_types, "", num_buckets);
  auto writer = output.get_internal_writer();

  // prepare all the readers
  std::vector<std::shared_ptr<sarray<flexible_type>::reader_type>> readers(input.num_columns());
  for (size_t i = 0;i < input.num_columns(); ++i) {
    auto cur_column = input.select_column(i);
    readers[i] = std::shared_ptr<sarray<flexible_type>::reader_type>(cur_column->get_reader());
  }
  // now. the challenge here is that the natural order of the sframe is not
  // necessarily good for forward map lookups. The forward map lookup has to
  // really fast. Instead we are going to do it this way:
  //  - Use the SFRAME_SORT_BUFFER_SIZE to estimate how much forward map
  //  we can keep in memory at any one time. Then in parallel over
  //  columns of the sframe.
  size_t max_forward_map_in_memory = SFRAME_SORT_BUFFER_SIZE / sizeof(flexible_type);
  auto forward_map_reader = forward_map->get_reader();
  std::vector<flexible_type> forward_map_buffer;
  logstream(LOG_INFO) << "Beginning Scatter"  << std::endl;
  logstream(LOG_INFO) << "Maximum forward map in memory "
                      << max_forward_map_in_memory
                      << std::endl;

  for (size_t forward_map_start = 0;
       forward_map_start < forward_map->size();
       forward_map_start += max_forward_map_in_memory) {
    size_t forward_map_end = std::min(forward_map_start + max_forward_map_in_memory,
                                      forward_map->size());
    logstream(LOG_INFO) << "Processing rows "
                        << forward_map_start
                        << " to " << forward_map_end << std::endl;
    forward_map_reader->read_rows(forward_map_start, forward_map_end,
                                  forward_map_buffer);

    // now in parallel over columns.
    atomic<size_t> col_number = 0;
    in_parallel(
        [&](size_t unused, size_t unused2) {
          while(1) {
            // this is the column we are now processing
            size_t column_id = col_number.inc_ret_last();
            if (column_id >= input.num_columns()) return;

            if (column_id < indirect_column.size() && indirect_column[column_id]) {
              // if this is to be an indirect column,
              // we use write the row number as the value
              for (size_t actual_row = forward_map_start; actual_row < forward_map_end; ++actual_row) {
                size_t output_row =
                    forward_map_buffer[actual_row - forward_map_start].get<flex_int>();
                size_t output_segment = output_row / rows_per_bucket;
                if (output_segment >= num_buckets) output_segment = num_buckets - 1;
                writer->write_segment(column_id, output_segment, flexible_type(actual_row));
              }
            } else {
              // ok. we need to actually read the column values.
              // For performance reasons, we shall try to read it on the
              // natural boundaries (that actually has the effect
              // of minimizing copies)
              std::vector<size_t> boundaries =
              column_row_boundaries(input.select_column(column_id));
              std::vector<flexible_type> buffer;
              auto boundary_start = std::lower_bound(boundaries.begin(),
                                                     boundaries.end(),
                                                     forward_map_start);
              // std::lower_bound returns the first element that is >= the sort_key
              // On the other hand for this, I need the last element that is <= the sort key
              // So sometimes I need to decrement by one
              if (boundary_start == boundaries.end() ||
                  (boundary_start != boundaries.begin() &&
                   forward_map_start < (*boundary_start))) {
                --boundary_start;
              }

              for (size_t i = std::distance(boundaries.begin(), boundary_start);
                   i < boundaries.size() - 1;
                   ++i) {
                size_t row = boundaries[i];
                size_t row_end = boundaries[i + 1];
                row = std::max(row, forward_map_start);
                row_end = std::min(row_end, forward_map_end);
                // we have ended on the right hand side. quit
                if (row >= row_end) break;
                readers[column_id]->read_rows(row, row_end, buffer);
                // scatter
                for (size_t i= 0;i < buffer.size(); ++i) {
                  size_t actual_row = row + i;
                  ASSERT_LT(actual_row - forward_map_start, forward_map_buffer.size());
                  size_t output_row =
                      forward_map_buffer[actual_row - forward_map_start].get<flex_int>();
                  size_t output_segment = output_row / rows_per_bucket;
                  writer->write_segment(column_id, output_segment, buffer[i]);
                }
              }
            }
          }
        });
  }
  output.close();
  ASSERT_EQ(output.size(), input.size());
  return output;
}

/**
 * A subroutine of ec_permute.
 *
 * Scatters the input into a collection of buckets using the last
 * column of the input as the forward_map.
 *
 * The forward_map must be an SArray of the same length as the input and contain
 * every integer from 0 to len-1.
 *
 * Returns the permuted sframe without the forward map.
 *
 * \param input : the result of ec_scatter_partitions
 * \param original_input : The original original input passed into ec_scatter_partitions
 */
sframe ec_permute_partitions(sframe input,
                             sframe& original_input,
                             size_t rows_per_bucket,
                             const std::vector<size_t>& column_bytes_per_value,
                             const std::vector<bool>& indirect_column) {
//     For each Bucket b:
//         Allocate Output vector of (Length of bucket) * (#columns)
//         Let S be the starting index of bucket b (i.e. b*N/k)
//         Let T be the ending index of bucket b (i.e. (b+1)*N/k)
//         Load forward_map[S:T] into memory
//         For each (c,r,v) in bucket b
//             Output[forward_map(r) - S][c] = v
//         Dump Output to an SFrame
//
  auto num_input_columns = input.num_columns() - 1; // last column is the forward map
  auto num_buckets = (input.size() + rows_per_bucket - 1)/ rows_per_bucket;

  ASSERT_GE(input.num_columns(), 1);
  ASSERT_EQ(input.num_columns(), original_input.num_columns() + 1);
  ASSERT_EQ(input.size(), original_input.size());
  logstream(LOG_INFO) << "Final permute " << input.size() << " rows" << std::endl;
  logstream(LOG_INFO) << "Rows per bucket: " << rows_per_bucket << std::endl;
  logstream(LOG_INFO) << "Num buckets: " << num_buckets << std::endl;

  // readers from the original input. We need to read from this for each
  // indirect column
  std::vector<std::shared_ptr<sarray<flexible_type>::reader_type>>
      original_readers(original_input.num_columns());
  for (size_t i = 0;i < original_input.num_columns(); ++i) {
    if (indirect_column[i]) {
      auto cur_column = original_input.select_column(i);
      original_readers[i] =
          std::shared_ptr<sarray<flexible_type>::reader_type>(cur_column->get_reader());
    }
  }

  auto& block_manager = v2_block_impl::block_manager::get_instance();

  // prepare the output
  sframe output;
  output.open_for_write(original_input.column_names(),
                        original_input.column_types(), "", num_buckets);
  auto writer = output.get_internal_writer();


  auto forward_map_reader = input.select_column(num_input_columns)->get_reader();
  size_t MAX_SORT_BUFFER = SFRAME_SORT_BUFFER_SIZE / thread::cpu_count();

  atomic<size_t> atomic_bucket_id = 0;
  // for each bucket
  in_parallel([&](size_t unused, size_t unused2) {
    while(1) {
      size_t bucket_id = atomic_bucket_id.inc_ret_last();
      if (bucket_id >= num_buckets) return;
      // these are the rows I am processing in this bucket
      size_t row_start = bucket_id * rows_per_bucket;
      size_t row_end = std::min(input.size(), row_start + rows_per_bucket);
      size_t num_rows = row_end - row_start;

      logstream(LOG_INFO) << "Processing bucket " << bucket_id << ": "
                          << row_start << " - " << row_end << std::endl;

      // read in the forward map corresponding to these set of rows
      std::vector<flexible_type> forward_map_buffer;
      forward_map_buffer.resize(num_rows);
      forward_map_reader->read_rows(row_start, row_end, forward_map_buffer);

      // loop through the columns
      size_t col_start = 0;
      while(col_start < num_input_columns) {
        // use the column_bytes_per_value to estimate how many columns to permute
        // at once. Minimum of one column.
        size_t col_end = col_start + 1;
        size_t memory_estimate = column_bytes_per_value[col_start] * num_rows;
        while(memory_estimate < MAX_SORT_BUFFER &&
              col_end < num_input_columns) {
          size_t next_col_memory_estimate = column_bytes_per_value[col_end] * num_rows;
          // if we can fit the next column in memory. good. Otherwise break;
          if (next_col_memory_estimate + memory_estimate < MAX_SORT_BUFFER) {
            memory_estimate += next_col_memory_estimate;
            ++col_end;
          } else {
            break;
          }
        }

        logstream(LOG_INFO) << "  Columns " << col_start << " to " << col_end << std::endl;

        std::vector<std::vector<flexible_type> >
            permute_buffer(col_end - col_start,
                           std::vector<flexible_type>(num_rows)); // buffer after permutation

        // this is the order I am going to read the blocks in this bucket
        std::vector<v2_block_impl::block_address> block_read_order;
        std::map<v2_block_impl::column_address, size_t> column_id_from_column_address;
        std::vector<size_t> cur_row_number(col_end - col_start, 0);
        timer ti;
        for (size_t column_id = col_start; column_id < col_end ; ++column_id) {
          // look in the segment file and list all the blocks
          const auto column_index = input.select_column(column_id)->get_index_info();
          auto segment_address = block_manager.open_column(column_index.segment_files[bucket_id]);
          column_id_from_column_address[segment_address] = column_id;

          auto num_blocks_in_current_segment =
              block_manager.num_blocks_in_column(segment_address);
          // for each block in the segment
          for (size_t block_number = 0;
               block_number < num_blocks_in_current_segment;
               ++block_number) {
            block_read_order.push_back(v2_block_impl::block_address{
                                       std::get<0>(segment_address),
                                       std::get<1>(segment_address),
                                       block_number});
          }
        }
        // now sort the block_read_order by the block info offset
        std::sort(block_read_order.begin(), block_read_order.end(),
                  [&](const v2_block_impl::block_address& left,
                      const v2_block_impl::block_address& right) {
                    return block_manager.get_block_info(left).offset <
                                block_manager.get_block_info(right).offset;
                  });
        ti.start();
        // good. now we fetch the blocks in that order.
        std::vector<flexible_type> buffer;
        for (auto& block: block_read_order) {
          block_manager.read_typed_block(block, buffer);
          v2_block_impl::column_address col_address{std::get<0>(block), std::get<1>(block)};
          size_t column_id = column_id_from_column_address.at(col_address);

          ASSERT_LT(column_id - col_start, cur_row_number.size());
          size_t& row_number = cur_row_number[column_id - col_start];
          for (size_t i = 0; i < buffer.size(); ++i) {
            ASSERT_LT(row_number, forward_map_buffer.size());
            ASSERT_GE(forward_map_buffer[row_number].get<flex_int>(), row_start);
            ASSERT_LT(forward_map_buffer[row_number].get<flex_int>(), row_end);
            size_t target = forward_map_buffer[row_number].get<flex_int>() - row_start;
            DASSERT_LT(column_id - col_start, permute_buffer.size());
            DASSERT_LT(target, permute_buffer[column_id - col_start].size());
            permute_buffer[column_id - col_start][target] = std::move(buffer[i]);
            ++row_number;
          }
        }

        for(auto kv: column_id_from_column_address) block_manager.close_column(kv.first);
        logstream(LOG_INFO) << "Permute buffer fill in " << ti.current_time() << std::endl;
        ti.start();
        // write the permute buffer.
        for (size_t column_id = col_start; column_id < col_end ; ++column_id) {
          if (indirect_column[column_id]) {
            // this is an indirect column. We have to fetch the value from
            // the original input.
            std::vector<flexible_type> indirect_buffer;
            // this column here contains integers. We need to read the values
            // from the original input. We do this one value at a time
            DASSERT_LT(column_id - col_start, permute_buffer.size());
            for (auto& value: permute_buffer[column_id - col_start]) {
              ASSERT_EQ((int)(value.get_type()), (int)(flex_type_enum::INTEGER));
              flex_int row_number = value.get<flex_int>();
              original_readers[column_id]->read_rows(row_number, row_number + 1, indirect_buffer);
              ASSERT_EQ(indirect_buffer.size(), 1);
              writer->write_segment(column_id, bucket_id, indirect_buffer[0]);
            }
          } else {
            writer->write_column(column_id, bucket_id, std::move(permute_buffer[column_id - col_start]));
          }
        }
        logstream(LOG_INFO) << "write columns in " << ti.current_time() << std::endl;
        ti.start();
        writer->flush_segment(bucket_id);
        col_start = col_end;
      }
    }
  });

  output.close();
  return output;
}

/**
 * Permutes an sframe.
 * forward_map must be an SArray of the same length as the values_sframe,
 * containing every integer in the range 0 to len-1. Row i of the input sframe
 * is moved to row forward_map[i] of the output sframe.
 * The result is an SFrame of the same size as the input sframe, but with
 * its elements permuted.
 */
sframe permute_sframe(sframe &values_sframe,
                      std::shared_ptr<sarray<flexible_type> > forward_map) {
  auto num_rows = values_sframe.size();
  auto value_column_names = values_sframe.column_names();
  auto value_column_types = values_sframe.column_types();
  auto num_value_columns = values_sframe.num_columns();
  timer ti;
  // column_bytes_per_value: The average number of bytes of memory required for
  //                         a value in each columns
  // indirect_column : If true, we write a row number in scatter, and pick it
  //                   up again later.
  std::vector<size_t> column_bytes_per_value(num_value_columns, 0);
  std::vector<bool> indirect_column(num_value_columns, false);
  size_t num_buckets = 0;
  {
    // First lets get an estimate of the column sizes and we use that
    // to estimate the number of buckets needed.
    std::vector<size_t> column_num_bytes = num_bytes_per_column(values_sframe);
    for (size_t i = 0;i < column_bytes_per_value.size();++i) {
      column_bytes_per_value[i] =
              column_bytes_per_value_estimate(column_num_bytes[i],
                                              num_rows,
                                              value_column_types[i]);
      // if bytes_per_value exceeds 256K, we use the indirect write.
      logstream(LOG_INFO) << "Est. bytes per value for column "
                          << value_column_names[i] << ": "
                          << column_bytes_per_value[i] << std::endl;
      if (column_bytes_per_value[i] > 256 * 1024) {
        indirect_column[i] = true;
        column_bytes_per_value[i] = sizeof(flexible_type);
        logstream(LOG_INFO) << "Using indirect access for column "
                            << value_column_names[i] << std::endl;
      }
      // update the number of bytes for the whole column
      column_num_bytes[i] = column_bytes_per_value[i] * num_rows;
    }


    ASSERT_GT(column_num_bytes.size(), 0);
    size_t max_column_num_bytes = *max_element(column_num_bytes.begin(),
                                               column_num_bytes.end());
    // maximum size of column / sort buffer size. round up
    // at least 1 bucket
    size_t HALF_SORT_BUFFER = SFRAME_SORT_BUFFER_SIZE / 2;
    num_buckets = (max_column_num_bytes + HALF_SORT_BUFFER - 1) / HALF_SORT_BUFFER;
    num_buckets = std::max<size_t>(1, num_buckets);
    num_buckets *= thread::cpu_count();
    if (num_buckets > num_rows) {
      // we are going to have less than 1 row per bucket.
      // i.e. we have *really* few rows. lets just do 1 bucket.
      num_buckets = 1;
    }

    logstream(LOG_INFO) << "Generating " << num_buckets << " buckets" << std::endl;

    size_t max_column_bytes_per_value = *max_element(column_bytes_per_value.begin(),
                                                     column_bytes_per_value.end());
    /*
     * There is a theoretical maximum number of rows we can sort, given
     * max_column_bytes_per_value. We can contain a maximum of
     * SFRAME_SORT_BUFFER_SIZE / max_column_bytes_per_value values per segment, and
     * we can only construct SFRAME_SORT_MAX_SEGMENTS number of segments.
     */
    size_t max_sort_rows =
            ((HALF_SORT_BUFFER * SFRAME_SORT_MAX_SEGMENTS) / max_column_bytes_per_value);
    logstream(LOG_INFO) << "Maximum sort rows: " << max_sort_rows << std::endl;
    if (num_rows > max_sort_rows) {
      logstream(LOG_WARNING)
        << "With the current configuration of SFRAME_SORT_BUFFER_SIZE "
        << "and SFRAME_SORT_MAX_SEGMENTS "
        << "we can sort an SFrame of up to " << max_sort_rows << " elements\n"
        << "The size of the current SFrame exceeds this length. We will proceed anyway "
        << "If this fails, either of these constants need to be increased.\n"
        << "SFRAME_SORT_MAX_SEGMENTS can be increased by increasing the number of n"
        << "file handles via ulimit -n\n"
        << "SFRAME_SORT_BUFFER_SIZE can be increased with tc.set_runtime_config()"
        << std::endl;
    }
  }

  //
  // Pivot Generation
  // ----------------
  // - Now we have a forward map, we can get exact buckets. Of N/K
  // length. I.e. row r is written to bucket `Floor(K \ forward_map(r) / N)`
  // ok. how many columns / rows can we fit in memory?
  // The limiter is going to be the largest column.
  //
  // due rows_per_bucket being an integer, a degree of imbalance
  // (up to num_buckets) is expected. That's fine.
  size_t rows_per_bucket = size_t(num_rows) / num_buckets;
  logstream(LOG_INFO) << "Rows per bucket: " << rows_per_bucket << std::endl;

  ti.start();
  logstream(LOG_INFO) << "Beginning scatter " << std::endl;
  // Scatter
  // -------
  //  - For each (c,r,v) in data:
  //    Write (c,v) to bucket `Floor(K \ forward_map(r) / N)`
  sframe scatter_sframe = ec_scatter_partitions(values_sframe,
                                                rows_per_bucket,
                                                indirect_column,
                                                forward_map);
  logstream(LOG_INFO) << "Scatter finished in " << ti.current_time() << std::endl;

  sframe sorted_values_sframe = ec_permute_partitions(scatter_sframe,
                                                      values_sframe,
                                                      rows_per_bucket,
                                                      column_bytes_per_value,
                                                      indirect_column);
  return sorted_values_sframe;
}

} // query_eval
} // turicreate
