/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/ml_data_2/ml_data.hpp>
#include <toolkits/ml_data_2/data_storage/ml_data_row_format.hpp>
#include <toolkits/ml_data_2/side_features.hpp>
#include <toolkits/ml_data_2/data_storage/util.hpp>
#include <model_server/lib/variant_deep_serialize.hpp>
#include <core/util/basic_types.hpp>
#include <core/util/try_finally.hpp>

using namespace turi::v2::ml_data_internal;

namespace turi { namespace v2 {

////////////////////////////////////////////////////////////////////////////////
//
//  The primary filling and setup functions.
//
////////////////////////////////////////////////////////////////////////////////

/**  Sets the ml metadata for the
 *
 */
void ml_data::_setup_ml_metadata() {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1:  Error testing and easy routines

  ASSERT_MSG(_metadata == nullptr, "Metadata already set!");
  _metadata.reset(new ml_metadata);

  ASSERT_MSG(incoming_data != nullptr, "Incoming data not available -- fill() called out of order?");

  _metadata->options = incoming_data->options;

  // If we don't have any incoming data, set it up that way and exit.
  if(incoming_data->data.num_columns() == 0)
    return;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Set up the different column modes

  const auto& target_column_name = incoming_data->target_column_name;
  const auto& data               = incoming_data->data;
  auto mode_overrides            = incoming_data->mode_overrides;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Set up the target column metadata.

  size_t target_column_idx = size_t(-1);

  if(!target_column_name.empty()) {

    bool target_column_always_numeric =
        _metadata->options.at("target_column_always_numeric");

    bool target_column_always_categorical =
        _metadata->options.at("target_column_always_categorical");

    ASSERT_MSG(!(target_column_always_categorical && target_column_always_numeric),
               "Conflicting type constraints given for target column.");

    if(target_column_always_numeric)
      mode_overrides[target_column_name] = ml_column_mode::NUMERIC;

    if(target_column_always_categorical)
      mode_overrides[target_column_name] = ml_column_mode::CATEGORICAL;

    if(! data.contains_column(target_column_name))
      log_and_throw(std::string("Required target column '") + target_column_name + "' not found.");

    _metadata->target.reset(new column_metadata);

    _metadata->target->setup(
        true,
        target_column_name,
        data.select_column(target_column_name),
        mode_overrides,
        _metadata->options);

    target_column_idx = data.column_index(target_column_name);

  } else {
    _metadata->target = column_metadata_ptr();
  }

  bool has_target_column = (target_column_idx != size_t(-1));

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: Choose the columns and column ordering.
  //
  // incoming_data->column_ordering is just a suggestion for the first few columns

  std::vector<std::string> column_names = incoming_data->column_ordering;

  const std::set<std::string> fixed_column_name_set(column_names.begin(), column_names.end());

  // Check that the column ordering is okay.
  ASSERT_EQ(column_names.size(), fixed_column_name_set.size());

  // Check that all the columns present normally are indeed present.
  for(const std::string& n : column_names) {
    if(!data.contains_column(n)) {
      ASSERT_MSG(false,
                 (std::string("Column ")
                  + n + " requested in ordering, but not present in data.").c_str());
    }

    if(n == target_column_name) {
      ASSERT_MSG(false,
                 (std::string("Column ")
                  + n + " requested in ordering, but conflicts with target column.").c_str());
    }
  }

  for(size_t c_idx = 0; c_idx < data.num_columns(); ++c_idx) {

    if(has_target_column && c_idx == target_column_idx)
      continue;

    if(fixed_column_name_set.count(data.column_name(c_idx)))
      continue;

    column_names.push_back(data.column_name(c_idx));
  }

  // Remember the original column ordering for translating things back as needed.
  _metadata->original_column_names = data.column_names();

  ////////////////////////////////////////////////////////////////////////////////
  // Step 6: Set up the indexers and the statistics trackers for each
  // of the columns.

  _metadata->columns.resize(column_names.size());

  for(size_t c_idx = 0; c_idx < column_names.size(); ++c_idx) {

    _metadata->columns[c_idx].reset(new column_metadata);

    _metadata->columns[c_idx]->setup(
        false,
        column_names[c_idx],
        data.select_column(column_names[c_idx]),
        mode_overrides,
        _metadata->options);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 6: Set up the side data if present

  if(!incoming_data->incoming_side_features.empty()) {

    _metadata->side_features.reset(new ml_data_side_features(_metadata->columns));

    for(const auto& p : incoming_data->incoming_side_features) {

      _metadata->side_features->add_and_index_side_data(
          p.data,
          p.mode_overrides,
          _metadata->options,
          true,
          false,
          p.forced_join_column);
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 7:  Nothing more.

}



/**
 * Fill the ml_data structure with the raw data in raw_data.
 */
void ml_data::_fill_data_blocks(bool in_training_mode) {

  // Check to make sure that we are
  ASSERT_TRUE(incoming_data != nullptr);
  ASSERT_TRUE(_metadata != nullptr);

  if(rm.metadata_vect.empty()) {
    data_blocks.reset(new sarray<row_data_block>);
    data_blocks->open_for_write(1);
    data_blocks->close();
    return;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1: Set up all the variables relevant to controlling the
  // filling.

  size_t max_num_threads = thread::cpu_count();

  /////////////////////////////////////////////////////////////
  // Step 1.1: Set up the other binary variables here

  bool track_statistics = in_training_mode ? true : false;
  bool immutable_metadata = (!in_training_mode) && incoming_data->immutable_metadata;

  ////////////////////////////////////////////////////////////
  // Step 1.2: Set the missing value (flex_type_enum::UNDEFINED)
  // action.

  missing_value_action none_action = get_missing_value_action(_metadata->options, in_training_mode);

  ////////////////////////////////////////////////////////////
  // Step 1.3: Set up the creation flags.

  const bool shuffle_output_data = _metadata->options.at("shuffle_rows");

  const bool sort_by_first_two_columns_always
      = _metadata->options.at("sort_by_first_two_columns");

  const bool sort_by_first_two_columns_on_train
      = _metadata->options.at("sort_by_first_two_columns_on_train");

  const bool sort_by_first_two_columns
      = (sort_by_first_two_columns_always
         || (in_training_mode && sort_by_first_two_columns_on_train));


  // Perform checks appropriate for the flags
  if(sort_by_first_two_columns) {
    ASSERT_MSG(_metadata->column_mode(0) == ml_column_mode::CATEGORICAL,
               "Mode of first column must be categorical for sorted_output to apply.");

    ASSERT_MSG(_metadata->column_mode(1)== ml_column_mode::CATEGORICAL,
               "Mode of second column must be categorical for sorted_output to apply.");
  }


  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Set up the target.

  std::shared_ptr<sarray<flexible_type> > target;

  const sframe& raw_data = incoming_data->data;

  if(rm.has_target) {

    target = raw_data.select_column(_metadata->target_column_name());

    check_type_consistent_with_mode(
        _metadata->target_column_name(), target->get_type(), _metadata->target_column_mode());
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Set up the row sizes

  _row_start = 0;
  _row_end = raw_data.num_rows();

  const size_t num_rows = _row_end - _row_start;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3.1: Check for an empty sframe.  In this case, just clear
  // everything and exit.  (We need to handle this case explicitly
  // since the user is allowed to pass in sframe() to signal that
  // ml_data is going to be created as an empty data.

  if(num_rows == 0) {
    data_blocks.reset(new sarray<row_data_block>);
    data_blocks->open_for_write(1);
    data_blocks->close();

    _max_row_size = 0;

    return;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: Set up metadata and input iterators

  // Set up all the proper data input sources for iterating through the data.
  // The full metadata has the target as well.  This way we can treat it just
  // like a column.  The bookkeeping here is easier, and it gets unpacked on
  // ml_data_iterator end.

  std::vector<std::shared_ptr<sarray<flexible_type> > > input_data;

  for(const auto& m : rm.metadata_vect) {
    input_data.push_back(raw_data.select_column(m->name));
    check_type_consistent_with_mode(m->name, raw_data.column_type(m->name), m->mode);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 5: Initialize all of the indexing and statistics classes

  scoped_finally indexer_finalizer;
  scoped_finally statistics_finalizer;

  // Initialize the metadata and key parts of ml_data
  //
  // The problem is that we always must make sure that things
  for(const auto& m : rm.metadata_vect) {
    std::shared_ptr<column_indexer> indexer = m->indexer;

    indexer->initialize();
    indexer_finalizer.add([indexer](){indexer->finalize();});

    if(track_statistics) {
      std::shared_ptr<column_statistics> statistics = m->statistics;

      statistics->initialize();
      statistics_finalizer.add([statistics](){statistics->finalize();});
    }
  }

  // Track the maximum row size; simple front loading of things for
  // efficient allocation.
  std::vector<size_t> max_row_size_by_segment(max_num_threads, 0);


  ////////////////////////////////////////////////////////////////////////////////
  // Step 5: Open the readers. Slow and independent, so do it in
  // parallel.

  std::vector<std::shared_ptr<sarray<flexible_type>::reader_type> > column_readers(rm.total_num_columns);

  parallel_for(0, rm.total_num_columns, [&](size_t c_idx) {

      if(!rm.metadata_vect[c_idx]->is_untranslated_column()) {

        ASSERT_MSG(input_data[c_idx]->is_opened_for_read(),
                   "Input data not properly set up for reading.");

        column_readers[c_idx] = input_data[c_idx]->get_reader();

        // If we don't have that many rows, deterministically insert the
        // values into the index in order.  This makes a number of
        // test cases much easier to write.

        if(!shuffle_output_data && !immutable_metadata) {
          std::vector<flexible_type> vv;
          column_readers[c_idx]->read_rows(0, std::min<size_t>(10000, num_rows), vv);
          rm.metadata_vect[c_idx]->indexer->insert_values_into_index(vv);
        }
      }
    });

  ////////////////////////////////////////////////////////////////////////////////
  // Step 6: Prepare the shuffling, if needed.
  //
  // If the output data needs to be shuffled, AND the output needs to
  // be sorted, then the only way we do the shuffling is to index the
  // first two columns in random order.  As the sorting is done on the
  // resulting indices, the net result will be that the original data
  // is shuffled widely on the far end.

  if(shuffle_output_data && sort_by_first_two_columns) {

    const size_t shuffle_block_size = 32*1024;

    // 1. Choose blocks of 32K indices; at each step, pick one of the
    // 32K index blocks at random, then add those values in random
    // order.

    std::vector<size_t> blocks(ceil_divide(num_rows, shuffle_block_size));

    for(size_t i = 0; i < blocks.size(); ++i)
      blocks[i] = i;

    random::shuffle(blocks.begin(), blocks.end());

    for(size_t col_idx : {0, 1}) {

      auto& col_reader = column_readers[col_idx];

      in_parallel([&](size_t thread_idx, size_t num_threads) {
          size_t block_idx_run_start = (thread_idx * blocks.size()) / num_threads;
          size_t block_idx_run_end = ((thread_idx + 1) * blocks.size()) / num_threads;

          std::vector<flexible_type> col_data;

          for(size_t block_idx = block_idx_run_start; block_idx < block_idx_run_end; ++block_idx) {

            size_t row_idx_start = blocks[block_idx] * shuffle_block_size;
            size_t row_idx_end   = std::min(num_rows, (blocks[block_idx] + 1) * shuffle_block_size);

            col_reader->read_rows(row_idx_start, row_idx_end, col_data);

            random::shuffle(col_data.begin(), col_data.end());

            for(const flexible_type& f : col_data)
              rm.metadata_vect[col_idx]->indexer->insert_values_into_index(f);
          }
        });
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Set the number of rows in each row block.

  row_block_size = estimate_row_block_size(
      num_rows, rm, column_readers);

  // Open the output writers.  In the case of shuffled output, create
  // more segments than we need, then write each block to a random
  // segment.  This fulfills a pretty decent type of shuffling.
  data_blocks.reset(new sarray<row_data_block>);

  size_t num_output_segments;

  if(shuffle_output_data && !sort_by_first_two_columns) {
    num_output_segments = std::max(size_t(13), 2*max_num_threads + 3);
  } else {
    num_output_segments = max_num_threads;
  }

  data_blocks->open_for_write(num_output_segments);

  std::vector<typename sarray<row_data_block>::iterator> output_iterators(num_output_segments);
  std::vector<mutex> output_iterator_locks(num_output_segments);

  for(size_t i = 0; i < output_iterators.size(); ++i)
    output_iterators[i] = data_blocks->get_output_iterator(i);

  // If we are shuffling the data, the final_shuffled_block_saved_row
  // needs to be held back and put at the end, as it isn't full
  // length. This holds this value; it gets set.
  row_data_block final_shuffled_block_saved_row;

  // Run through the original data in parallel, mapping indices and tracking
  // everything as needed for the metadata statistics.
  in_parallel([&](size_t thread_idx, size_t num_threads) {

      // Set up start points for the segments.  To make the indexing sane, the
      // rows are stored in blocks of row_block_size rows. This must be true
      // across segments.  Thus we must set up the row indexing so that each
      // thread starts processing at one of the block boundaries.

      // Equal to flooring (i * num_rows / num_threads)
      // to the nearest multiple of row_block_size.
      size_t segment_row_index_start =
          row_block_size * size_t( ((thread_idx * num_rows) / num_threads)
                                   / row_block_size);

      size_t segment_row_index_end =
          ( (thread_idx == num_threads - 1)
            ? num_rows
            : row_block_size * size_t( (((thread_idx + 1) * num_rows)
                                        / num_threads) / row_block_size));

      // The data block into which we write everything
      row_data_block block_output;

      // Set up a buffered block of each of the columns
      std::vector<std::vector<flexible_type> > buffers(rm.total_num_columns);

      // The row index map; gets filled with every block.
      std::vector<size_t> row2data_idx_map;

      // The index remapping; this allows the use of sorting and
      // shuffling to reorder rows within a block.
      std::vector<size_t> index_remapping;
      std::vector<std::pair<size_t, size_t> > column_sorting_values;

      // Loop over blocks of rows, writing each one.
      size_t block_row_index_start = segment_row_index_start;
      DASSERT_EQ(block_row_index_start % row_block_size, 0);

      while(block_row_index_start != segment_row_index_end) {

        size_t block_row_index_end =
            std::min(segment_row_index_end,
                     block_row_index_start + row_block_size);

        size_t block_size = (block_row_index_end - block_row_index_start);

        if(block_size != row_block_size) {
          DASSERT_LT(block_size, row_block_size);
          DASSERT_EQ(segment_row_index_end, num_rows);
        }

        // Read all the rows into the buffer.  If we can't read in every one
        // from all the columns (????), then, we read in as much as we can and
        // then treat that amount as the block.
        for(size_t c_idx = 0; c_idx < rm.total_num_columns; ++c_idx) {

          // skip the untranslated_columns columns
          if(rm.metadata_vect[c_idx]->is_untranslated_column())
            continue;

#ifndef NDEBUG
          size_t n_rows_returned =
#endif
              column_readers[c_idx]->read_rows(block_row_index_start,
                                               block_row_index_end,
                                               buffers[c_idx]);

          DASSERT_EQ(n_rows_returned, block_size);
        }

        // Set up an index mapping as needed.
        if(sort_by_first_two_columns) {

          // index these two rows as needed to get the sorted values.
          // This may slow things down slightly, but currently it
          // isn't the bottleneck.
          column_sorting_values.resize(block_size);

          for(size_t r_idx = 0; r_idx < block_size; ++r_idx) {
            size_t c1_idx, c2_idx;

            if(!immutable_metadata) {
              c1_idx = rm.metadata_vect[0]->indexer->map_value_to_index(thread_idx, buffers[0][r_idx]);
              c2_idx = rm.metadata_vect[1]->indexer->map_value_to_index(thread_idx, buffers[1][r_idx]);
            } else {
              c1_idx = rm.metadata_vect[0]->indexer->immutable_map_value_to_index(buffers[0][r_idx]);
              c2_idx = rm.metadata_vect[1]->indexer->immutable_map_value_to_index(buffers[1][r_idx]);
            }

            column_sorting_values[r_idx] = {c1_idx, c2_idx};
          }

          if(index_remapping.size() != block_size) {
            index_remapping.resize(block_size);
            for(size_t k = 0; k < block_size; ++k)
              index_remapping[k] = k;
          }

          // Get the index_mapping for a sorted list.
          std::sort(index_remapping.begin(), index_remapping.end(),
                    [&column_sorting_values](size_t i1, size_t i2) {
                      return column_sorting_values[i1] < column_sorting_values[i2];
                    });

        } else if(shuffle_output_data) {

          // Shuffle the index map.
          if(index_remapping.size() != block_size) {
            index_remapping.resize(block_size);
            for(size_t k = 0; k < block_size; ++k)
              index_remapping[k] = k;
          }

          random::shuffle(index_remapping.begin(), index_remapping.end());
        }

        // Run the buffer
        size_t max_row_size =
            fill_row_buffer_from_column_buffer( row2data_idx_map,
                                                block_output,
                                                rm, buffers,
                                                thread_idx,
                                                track_statistics,
                                                immutable_metadata,
                                                none_action,
                                                index_remapping);

        max_row_size_by_segment[thread_idx] = std::max(max_row_size,
                                                       max_row_size_by_segment[thread_idx]);

        // Write the output block to one of the segments.
        if(shuffle_output_data && !sort_by_first_two_columns) {

          if(block_size == row_block_size) {

            while(true) {

              // If it's a full block, write it to a random location.
              size_t write_out_segment = random::fast_uniform<size_t>(0, output_iterators.size()-1);

              if(output_iterator_locks[write_out_segment].try_lock()) {
                auto& it_out = output_iterators[write_out_segment];
                *it_out = block_output;
                ++it_out;
                output_iterator_locks[write_out_segment].unlock();
                break;
              }
            }
          } else {

            // If it's not a full block, save it for later -- it needs
            // to be written last, to the final block.
            // Also, this will only happen in the final thread.
            DASSERT_TRUE(final_shuffled_block_saved_row.entry_data.empty());
            DASSERT_EQ(thread_idx, num_threads-1);
            final_shuffled_block_saved_row = block_output;
          }

        } else {
          auto& it_out = output_iterators[thread_idx];
          *it_out = block_output;
          ++it_out;
        }

        // Increment
        block_row_index_start = block_row_index_end;

      } // End loop over buffered blocks

    }); // End parallel looping

  // Do we have a residual last block?  If so write it out to the last
  // segment.
  if(shuffle_output_data && !final_shuffled_block_saved_row.entry_data.empty()) {
    auto& it_out = output_iterators.back();
    *it_out = final_shuffled_block_saved_row;
    ++it_out;
  }

  // Close the data out writers
  data_blocks->close();

  DASSERT_EQ(data_blocks->size(), ceil_divide(num_rows, row_block_size));

  // Finalize the lookups and statistics
  indexer_finalizer.execute_and_clear();
  if(track_statistics)
    statistics_finalizer.execute_and_clear();

  // Set overall max row size.  Subtract one if the target is present,
  // as the row size does not include that.
  _max_row_size = *std::max_element(max_row_size_by_segment.begin(),
                                    max_row_size_by_segment.end());

  if(rm.has_target)
    _max_row_size -= 1;
}

/**  Set up the untranslated columns.
 */
void ml_data::_setup_untranslated_columns(const sframe& original_data) {

  untranslated_columns.clear();

  for(size_t c_idx = 0; c_idx < metadata()->num_columns(false); ++c_idx) {
    if(metadata()->is_untranslated_column(c_idx)) {
      const std::string& name = metadata()->column_name(c_idx);
      untranslated_columns.push_back(original_data.select_column(name));
    }
  }
}

}}
