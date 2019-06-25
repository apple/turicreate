/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml/ml_data/ml_data.hpp>
#include <ml/ml_data/ml_data_iterator.hpp>
#include <ml/ml_data/data_storage/ml_data_row_format.hpp>
#include <ml/ml_data/data_storage/ml_data_row_translation.hpp>
#include <ml/ml_data/data_storage/ml_data_block_manager.hpp>
#include <ml/ml_data/data_storage/util.hpp>
#include <core/util/basic_types.hpp>
#include <core/util/try_finally.hpp>

using namespace turi::ml_data_internal;

namespace turi {

////////////////////////////////////////////////////////////////////////////////
//
//  The ml_data internal stuff.
//
////////////////////////////////////////////////////////////////////////////////

ml_data::ml_data()
    : _metadata(nullptr)
{
}

/**
 *  Construct an ml_data object based on previous ml_data metadata.
 */
ml_data::ml_data(const std::shared_ptr<ml_metadata>& __metadata)
    : _metadata(__metadata)
{
}

// ml_data is cheap to copy.  However, it cannot be copied before
// fill() is called.

ml_data::ml_data(const ml_data& other) {

  // This checks to make sure it's copyable.
  *this = other;
}

const ml_data& ml_data::operator=(const ml_data& other) {

  _metadata                   = other._metadata;
  rm                          = other.rm;
  _row_start                  = other._row_start;
  _row_end                    = other._row_end;
  _original_num_rows          = other._original_num_rows;
  _max_row_size               = other._max_row_size;
  row_block_size              = other.row_block_size;
  data_blocks                 = other.data_blocks;
  block_manager               = other.block_manager;
  untranslated_columns        = other.untranslated_columns;

  return *this;
}

void ml_data::fill(const sframe& raw_data,
                   const std::string& target_column_name,
                   const column_mode_map mode_overrides,
                   bool immutable_metadata,
                   ml_missing_value_action mva) {

  fill(raw_data, {0, raw_data.num_rows()}, target_column_name,
       mode_overrides, immutable_metadata, mva);
}


void ml_data::fill(const sframe& raw_data,
                   const std::pair<size_t, size_t>& row_bounds,
                   const std::string& target_column_name,
                   const column_mode_map mode_overrides,
                   bool immutable_metadata,
                   ml_missing_value_action mva) {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1.  Set up the metadata if need be.

  bool in_training_mode;

  if(_metadata == nullptr) {
    ASSERT_MSG(!immutable_metadata, "immutable_metadata cannot be set for initial metadata building.");
    in_training_mode = true;
    _setup_ml_metadata(raw_data, target_column_name, mode_overrides);
  } else {
    in_training_mode = false;
  }

  if(in_training_mode && mva == ml_missing_value_action::IMPUTE) {
    ASSERT_MSG(false, "missing_value_action impute not allowed on initial fill.");
  }

  ASSERT_LE(row_bounds.first, row_bounds.second);
  ASSERT_LE(row_bounds.second, raw_data.num_rows());


  bool empty_incoming_data = (row_bounds.second == row_bounds.first);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Set up the row start and end.  These are in reference to
  // the seen ml_data, not the raw_sframe.

  _row_start            = 0;
  _row_end              = row_bounds.second - row_bounds.first;
  _original_num_rows    = _row_end - _row_start;

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: Set up metadata and input iterators

  // Set up all the proper data input sources for iterating through the data.
  // The full metadata has the target as well.  This way we can treat it just
  // like a column.  The bookkeeping here is easier, and it gets unpacked on
  // ml_data_iterator end.

  {
    const std::vector<std::string>& raw_column_names = raw_data.column_names();
    std::set<std::string> raw_column_name_set(raw_column_names.begin(), raw_column_names.end());

    std::vector<column_metadata_ptr> full_metadata = _metadata->columns;

    std::vector<std::string> missing_columns;

    for(size_t c_idx = 0; c_idx < full_metadata.size(); ++c_idx) {
      const std::string& column_name = full_metadata[c_idx]->name;
      if(raw_column_name_set.count(column_name) == 0) {
        missing_columns.push_back(column_name);
      } else {
        raw_column_name_set.erase(column_name);
      }
    }

    bool using_target = (_metadata->has_target()
                         && raw_data.contains_column(_metadata->target_column_name()));

    if(using_target)
      raw_column_name_set.erase(_metadata->target_column_name());

    // Now, do we having any missing columns?  Has anything been added
    // wrongly?

    if(!empty_incoming_data && !missing_columns.empty()) {
      std::ostringstream ss;

      ss << "Provided data missing required columns: ";

      for(size_t i = 0; i < missing_columns.size() - 1; ++i) {
        ss << missing_columns[i] << ", ";
      }
      ss << missing_columns.back() << ".";

      log_and_throw(ss.str());
    }

    if(!raw_column_name_set.empty()) {
      std::ostringstream ss;

      ss << "Ignoring columns not present at model construction: ";

      {
        size_t i = 0;
        for(const auto& s : raw_column_name_set) {
          ss << s;
          ++i;
          if(i != raw_column_name_set.size())
            ss << ", ";
        }
      }

      ss << ".";
      logprogress_stream << ss.str() << std::endl;
    }

    if(using_target) {

      std::shared_ptr<sarray<flexible_type> > target
          = raw_data.select_column(_metadata->target_column_name());

      check_type_consistent_with_mode(
          _metadata->target_column_name(), target->get_type(), _metadata->target_column_mode());

      full_metadata.push_back(_metadata->target);
    }

    // Set up the row metadata object.  This is the main thing used
    // below.
    rm.setup(full_metadata, using_target);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 5.  Call the main filling functions.

  bool track_statistics = in_training_mode ? true : false;

  // Get the main sorting functions
  std::set<std::string> sorted_columns;

  if(!immutable_metadata) {
    for(const auto& p : mode_overrides) {
      if(p.second == ml_column_mode::CATEGORICAL_SORTED) {
        sorted_columns.insert(p.first);
      }
    }
  }

  _fill_data_blocks(raw_data, immutable_metadata, track_statistics, mva, row_bounds, sorted_columns);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 7.  Finalize the metadata stuff

  if(in_training_mode) {
    _metadata->set_training_index_sizes_to_current_column_sizes();

    // Set up some of the cached values in the metadata
    _metadata->setup_cached_values();
  } else {

#ifndef NDEBUG

    for(size_t c_idx = 0; c_idx < _metadata->num_columns(); ++c_idx) {
      DASSERT_LE(_metadata->index_size(c_idx), _metadata->column_size(c_idx));
    }

#endif
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 6.  Set up the untranslated columns

  _setup_untranslated_columns(raw_data, row_bounds.first, row_bounds.second);

  ////////////////////////////////////////////////////////////////////////////////
  // Step 8.  Set up the block manager

  _reset_block_manager();
}



/**  Sets the ml metadata for the
 *
 */
void ml_data::_setup_ml_metadata(const sframe& data,
                                 const std::string& target_column_name,
                                 const column_mode_map& mode_overrides) {

  ////////////////////////////////////////////////////////////////////////////////
  // Step 1:  Error testing and easy routines

  ASSERT_MSG(_metadata == nullptr, "Metadata already set!");
  _metadata.reset(new ml_metadata);

  // If we don't have any incoming data, set it up that way and exit.
  if(data.num_columns() == 0) {
    return;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Set up the target column metadata.

  size_t target_column_idx = size_t(-1);

  if(!target_column_name.empty()) {

    if(! data.contains_column(target_column_name))
      log_and_throw(std::string("Required target column '") + target_column_name + "' not found.");

    _metadata->target.reset(new column_metadata);

    _metadata->target->setup(
        true,
        target_column_name,
        data.select_column(target_column_name),
        mode_overrides);

    target_column_idx = data.column_index(target_column_name);

  } else {
    _metadata->target = column_metadata_ptr();
  }

  bool has_target_column = (target_column_idx != size_t(-1));

  ////////////////////////////////////////////////////////////////////////////////
  // Step 4: Choose the columns and column ordering.
  //
  // incoming_data->column_ordering is just a suggestion for the first few columns

  std::vector<std::string> column_names = data.column_names();
  if(has_target_column && !target_column_name.empty()) {
    auto it = std::find(column_names.begin(),column_names.end(),target_column_name);
    DASSERT_TRUE(it != column_names.end());
    column_names.erase(it);
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
        mode_overrides);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 7:  Nothing more.

}

/**
 * Fill the ml_data structure with the raw data in raw_data.
 */
void ml_data::_fill_data_blocks(const sframe& raw_data,
                                bool immutable_metadata,
                                bool track_statistics,
                                ml_missing_value_action mva,
                                const std::pair<size_t, size_t>& row_bounds,
                                const std::set<std::string>& sorted_columns) {

  // Check to make sure that we are
  ASSERT_TRUE(_metadata != nullptr);

  if(rm.metadata_vect.empty()) {
    data_blocks.reset(new sarray<row_data_block>);
    data_blocks->open_for_write(1);
    data_blocks->close();
    return;
  }

  size_t max_num_threads = thread::cpu_count();

  ////////////////////////////////////////////////////////////////////////////////
  // Step 2: Set up the target.

  std::shared_ptr<sarray<flexible_type> > target;

  if(rm.has_target) {

    target = raw_data.select_column(_metadata->target_column_name());

    check_type_consistent_with_mode(
        _metadata->target_column_name(), target->get_type(), _metadata->target_column_mode());
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Step 3: Set up the row sizes

  const size_t num_rows = row_bounds.second - row_bounds.first;
  const size_t row_lb = row_bounds.first;
  const size_t row_ub = row_bounds.second;


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

      auto m = rm.metadata_vect[c_idx];

      if(!m->is_untranslated_column()) {

        ASSERT_MSG(input_data[c_idx]->is_opened_for_read(),
                   "Input data not properly set up for reading.");

        column_readers[c_idx] = input_data[c_idx]->get_reader();

        // If we don't have that many rows, deterministically insert the
        // values into the index in order.  This makes a number of
        // test cases much easier to write.
        if(mode_is_indexed(m->mode)
           && (sorted_columns.empty() || (sorted_columns.count(m->name) == 0))) {
          std::vector<flexible_type> vv;
          column_readers[c_idx]->read_rows(row_lb, std::min(row_lb + 10000, row_ub), vv);
          m->indexer->insert_values_into_index(vv);
        }
      }
    });

  // If it's a sorted column, then get all the possible
  // values, then insert them in sorted order.  Could be
  // optimized.
  if(!sorted_columns.empty()) {
    for(size_t c_idx = 0; c_idx < rm.total_num_columns; ++c_idx) {

      const auto& m = rm.metadata_vect[c_idx];

      if(sorted_columns.count(m->name)) {

        column_indexer _idxr(m->name, m->mode, m->original_column_type);

        {
          _idxr.initialize();
          scoped_finally idx_fin([&](){_idxr.finalize();});

          // Add everything to the index.
          in_parallel([&](size_t thread_idx, size_t num_threads) {

              size_t n_rows = row_ub - row_lb;
              size_t start_idx = row_lb + (thread_idx * n_rows) / num_threads;
              size_t end_idx = row_lb + ((thread_idx + 1) * n_rows) / num_threads;

              std::vector<flexible_type> vv;

              for(size_t row_idx = start_idx; row_idx < end_idx; row_idx += 4096) {

                column_readers[c_idx]->read_rows(row_idx, std::min(row_ub, row_idx + 4096), vv);

                for(const auto& v : vv) {
                  _idxr.map_value_to_index(thread_idx, v);
                }
              }
            });
        }

        // Pull out the values.
        std::vector<flexible_type> values = _idxr.reset_and_return_values();

        // Sort
        std::sort(values.begin(), values.end(),
                  [](const flexible_type& v1, const flexible_type& v2) {
                    if(UNLIKELY(v1.get_type() != v2.get_type())) {
                      return (int(v1.get_type()) < int(v2.get_type()));
                    } else if(UNLIKELY(v1.get_type() == flex_type_enum::UNDEFINED)) {
                      return false;
                    } else {
                      return v1 < v2;
                    }
                  });

        // Insert.
        m->indexer->insert_values_into_index(values);
      }
    }
  }


  ////////////////////////////////////////////////////////////////////////////////
  // Set the number of rows in each row block.

  row_block_size = estimate_row_block_size(num_rows, rm, column_readers);

  // Open the output writers.  In the case of shuffled output, create
  // more segments than we need, then write each block to a random
  // segment.  This fulfills a pretty decent type of shuffling.
  data_blocks.reset(new sarray<row_data_block>);

  size_t num_output_segments = max_num_threads;
  data_blocks->open_for_write(num_output_segments);

  std::vector<typename sarray<row_data_block>::iterator> output_iterators(num_output_segments);
  std::vector<mutex> output_iterator_locks(num_output_segments);

  for(size_t i = 0; i < output_iterators.size(); ++i)
    output_iterators[i] = data_blocks->get_output_iterator(i);

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
              column_readers[c_idx]->read_rows(row_lb + block_row_index_start,
                                               row_lb + block_row_index_end,
                                               buffers[c_idx]);

          DASSERT_EQ(n_rows_returned, block_size);
        }

        // Run the buffer
        size_t max_row_size =
            fill_row_buffer_from_column_buffer( row2data_idx_map,
                                                block_output,
                                                rm, buffers,
                                                thread_idx,
                                                track_statistics,
                                                immutable_metadata,
                                                mva);

        max_row_size_by_segment[thread_idx] = std::max(max_row_size,
                                                       max_row_size_by_segment[thread_idx]);


        // Write the output.
        auto& it_out = output_iterators[thread_idx];
        *it_out = block_output;
        ++it_out;

        // Increment
        block_row_index_start = block_row_index_end;

      } // End loop over buffered blocks

    }); // End parallel looping

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
void ml_data::_setup_untranslated_columns(const sframe& original_data, size_t row_lb, size_t row_ub) {

  untranslated_columns.clear();

  for(size_t c_idx = 0; c_idx < metadata()->num_columns(); ++c_idx) {
    if(metadata()->is_untranslated_column(c_idx)) {
      const std::string& name = metadata()->column_name(c_idx);
      untranslated_columns.push_back(original_data.select_column(name));
    }
  }

  // Now, we need to prune those columns to the correct row

  if(row_lb != 0 || row_ub != original_data.num_rows()) {

    size_t num_segments = thread::cpu_count();

    std::vector<std::shared_ptr<sarray<flexible_type>::reader_type> > column_readers
        (untranslated_columns.size());

    for(size_t i = 0; i < untranslated_columns.size(); ++i) {
      column_readers[i] = untranslated_columns[i]->get_reader();
      untranslated_columns[i].reset(new sarray<flexible_type>);
      untranslated_columns[i]->open_for_write(num_segments);
    }

    // Now, copy them all out there.
    parallel_for(0, num_segments * untranslated_columns.size(), [&](size_t idx) {

        size_t col_idx = idx / num_segments;
        size_t segment_idx = idx % num_segments;

        size_t row_start = row_lb + ((row_ub - row_lb) * segment_idx) / num_segments;
        size_t row_end = row_lb + ((row_ub - row_lb) * (segment_idx+1)) / num_segments;

        auto it_out = untranslated_columns[col_idx]->get_output_iterator(segment_idx);

        std::vector<flexible_type> buffer;
        buffer.reserve(100);

        for(size_t i = row_start; i < row_end; i += 100) {

          size_t n_read = column_readers[col_idx]->read_rows(i, i + 100, buffer);

          for(size_t i = 0; i < n_read; ++i, ++it_out) {
            *it_out = std::move(buffer[i]);
          }
        }
      });

    for(size_t i = 0; i < untranslated_columns.size(); ++i) {
      untranslated_columns[i]->close();
    }
  }
}


/** Return an iterator over part of the data.  See
 *  iterators/ml_data_iterator.hpp for documentation on the returned
 *  iterator.
 */
ml_data_iterator ml_data::get_iterator(size_t thread_idx, size_t num_threads) const {

  ASSERT_MSG(_metadata != nullptr,
             "ml_data is not iterable if uninitialized.");

  ml_data_iterator it;

  it.setup(*this, rm, thread_idx, num_threads);

  return it;
}

/**
 * Create a subsampled copy of the current ml_data structure.  This
 * allows us quickly create a subset of the data to be used for things
 * like sgd, etc.
 *
 * If n_rows < size(), exactly n_rows are sampled IID from the
 * dataset.  Otherwise, a copy of the current ml_data is returned.
 *
 */
ml_data ml_data::create_subsampled_copy(size_t n_rows, size_t random_seed) const{
  size_t data_size = num_rows();

  if(n_rows >= num_rows())
    return *this;

  // Sample without replacement.  Do this in a hacktastic way
  std::vector<size_t> samples(n_rows);

  // Choose them evenly
  for(size_t i = 0; i < n_rows; ++i) {
    samples[i] = (i * data_size) / n_rows;
  }

  random::seed(random_seed);

  // Now randomize this.
  for(size_t i = 0; i < n_rows; ++i) {
    size_t lb = (i > 0) ? (samples[i - 1] + 1) : 0;
    size_t ub = (i < n_rows - 1) ? (samples[i + 1] - 1) : data_size - 1;
    samples[i] = random::fast_uniform<size_t>(lb, ub);
  }

  // Break them up into groups
  DASSERT_TRUE(std::is_sorted(samples.begin(), samples.end()));

  return select_rows(samples);
}

/**
 *  Create a copy of the current ml_data structure, selecting the rows
 *  given by selection_indices.
 *
 *  \param selection_indices A vector of row indices that must be in
 *  sorted order.  Duplicates are allowed.  The returned ml_data
 *  contains all the rows given by selection_indices.
 *
 *  \return A new ml_data object with containing only the rows given
 *  by selection_indices.
 */
ml_data ml_data::select_rows(const std::vector<size_t>& selection_indices) const{

  if(!std::is_sorted(selection_indices.begin(), selection_indices.end())) {
    ASSERT_MSG(false, "selection_indices argument needs to be in sorted order.");
  }

  size_t n_rows = selection_indices.size();

  ml_data out = *this;

  const size_t n_full_blocks = n_rows / row_block_size;
  const size_t n_remaining_rows = n_rows % row_block_size;
  const size_t n_total_blocks = n_full_blocks + ((n_remaining_rows > 0) ? 1 : 0);
  const size_t data_size = num_rows();

  size_t max_n_threads = thread::cpu_count();

  out.data_blocks.reset(new sarray<row_data_block>);
  out.data_blocks->open_for_write(max_n_threads);

  // Sample retreival function.
  auto get_sample = [&](size_t idx) {
    DASSERT_LE(idx, n_rows);
    size_t sample_idx =  (idx < n_rows) ? selection_indices[idx] : data_size;

    if(idx < n_rows)
      DASSERT_LT(sample_idx, data_size);

    return sample_idx;
  };

  // Break them up into groups

  in_parallel([&, selection_indices](size_t thread_idx, size_t num_threads) GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {

      size_t out_block_start_idx = (thread_idx * n_total_blocks) / num_threads;
      size_t out_block_end_idx = ((thread_idx + 1) * n_total_blocks) / num_threads;

      size_t samples_row_start = out_block_start_idx * row_block_size;
      size_t samples_row_end = std::min(out_block_end_idx * row_block_size, n_rows);

      size_t sample_first = get_sample(samples_row_start);
      size_t sample_end = samples_row_end <= n_rows ? get_sample(samples_row_end) : data_size + 1;

      size_t ml_data_row_start = sample_first;

      size_t ml_data_row_end = std::min(this->size(), sample_end + 1);

      DASSERT_LE(ml_data_row_start, ml_data_row_end);

      size_t n_rows_needed = samples_row_end - samples_row_start;

      DASSERT_TRUE(thread_idx + 1 == num_threads
                   || n_rows_needed % row_block_size == 0);

      DASSERT_LE(ml_data_row_start, ml_data_row_end);

      auto it_out = out.data_blocks->get_output_iterator(thread_idx);

      // Set up these values
      ml_data_internal::row_data_block block;

      ml_data sliced_data = this->slice(ml_data_row_start, ml_data_row_end);
      size_t rows_in_block = 0;
      size_t row_count = 0;
      size_t sample_index = samples_row_start;

      for(ml_data_iterator it = sliced_data.get_iterator();!it.done();) {

        ////////////////////////////////////////////////////////////
        //
        // Step 1: Advance to the next chosen row

        size_t selection_index = get_sample(sample_index);

        size_t unsliced_row_index = ml_data_row_start + it.row_index();

        DASSERT_LT(unsliced_row_index, ml_data_row_end);
        DASSERT_LE(ml_data_row_start, unsliced_row_index);

        // An optimization -- if the next index is not in this block,
        // then do a seek -- this will massively speed up sparse
        // selections.
        if(row_count < n_rows_needed) {

          DASSERT_TRUE(!it.done());

          if(selection_index > unsliced_row_index
             && size_t(selection_index / row_block_size) > size_t(unsliced_row_index / row_block_size)) {
            it.seek(it.row_index() + (selection_index - unsliced_row_index));
          }

          while(unsliced_row_index < selection_index) {
            ++it;
            DASSERT_TRUE(!it.done());
          }
        } else {
          break;
        }

        ////////////////////////////////////////////////////////////
        // Step 2: Write that row out.

        while(unsliced_row_index == get_sample(sample_index)
              && row_count < n_rows_needed) {

          entry_value_iterator row_start = it.current_data_iter();

          append_row_to_row_data_block(rm, block, row_start);

          ++rows_in_block, ++row_count, ++sample_index;

          if(rows_in_block == row_block_size || row_count == n_rows_needed) {
            *it_out = block;
            ++it_out;
            block.entry_data.clear();
            rows_in_block = 0;
          }
        }
      }

      DASSERT_EQ(row_count, n_rows_needed);
      DASSERT_TRUE(block.entry_data.empty());
    });

  out.data_blocks->close();

  // Set up the block manager in the target thing.
  out._reset_block_manager();

  // Clean up some of the other things.
  out._row_start         = 0;
  out._row_end           = n_rows;
  out._original_num_rows = n_rows;

  return out;
}

/**
 *  Create a sliced copy of the current ml_data structure.  This
 *  copy is cheap.
 */
ml_data ml_data::slice(size_t slice_row_start, size_t slice_row_end) const {
  ASSERT_LE(slice_row_start, num_rows());
  ASSERT_LE(slice_row_end, num_rows());

  ml_data out = *this;

  out._row_start = _row_start + slice_row_start;
  out._row_end   = _row_start + slice_row_end;

  return out;
}

/** Convenience function to create the block manager given the current
 *  data in the model.
 */
void ml_data::_reset_block_manager() {
  block_manager.reset(
      new ml_data_block_manager(
          metadata(), rm, row_block_size, data_blocks, untranslated_columns));
}

void ml_data::_reindex_blocks(const std::vector<std::vector<size_t> >& reindex_maps) {

  // Step 1: For each block, rewrite the block

  auto new_data_blocks = std::make_shared<sarray<ml_data_internal::row_data_block> >();

  size_t num_output_segments = thread::cpu_count();
  new_data_blocks->open_for_write(num_output_segments);

  size_t num_blocks = data_blocks->size();

  auto reader = block_manager->get_reader();

  in_parallel([&](size_t thread_idx, size_t num_threads) {
      size_t start_idx = (thread_idx * num_blocks) / num_threads;
      size_t end_idx = ((thread_idx + 1) * num_blocks) / num_threads;
      auto out_it = new_data_blocks->get_output_iterator(thread_idx);

      std::vector<row_data_block> rdb_v(1);

      for(size_t i = start_idx; i < end_idx; ++i, ++out_it) {
        reader->read_rows(i, i+1, rdb_v);
        reindex_block(rm, rdb_v[0], reindex_maps);
        *out_it = rdb_v[0];
      }
    });

  new_data_blocks->close();

  data_blocks = new_data_blocks;

  // Reset the block manager.
  _reset_block_manager();

}



}
