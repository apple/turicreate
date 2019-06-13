/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/sarray.hpp>
#include <iostream>

namespace turi {


/**
 * \ingroup sframe_physical
 * \addtogroup sframe_main Main SFrame Objects
 * \{
 */

/**
 *
 * Class for fast, sequential iteration through an SArray.  Avoids copies
 * and buffering, and the size of each chunk of data is given by a single
 * block.
 *
 * Example:
 *
 *   auto it = make_sarray_block_iterator(data);
 *
 *   in_parallel([&](size_t thread_idx, size_t num_threads) {
 *
 *       std::vector<T> v;
 *
 *       while(true) {
 *
 *         size_t row_start;
 *
 *         if(it.read_next(&row_start, &v)) {
 *            break;
 *         }
 *
 *         //...
 *         // v contains elements row_start + 0, row_start + 1, ..., row_start + (v.size()-1)
 *         //...
 *       }
 *     });
 *
 */
template <typename DataType>
class sarray_block_iterator {
 public:

  sarray_block_iterator(const std::shared_ptr<sarray<DataType> >& _data)
      : data(_data)
      , block_manager(v2_block_impl::block_manager::get_instance())
      , num_segments(data->get_index_info().segment_files.size())
  {
    initialize();
  }

  ~sarray_block_iterator() {
  }

  /** True if we're done.
   */
  inline bool done() const {
    return _is_done;
  }

  /** Reads the next block of data, writing the row number of the
   *  starting block to row_number_start, and all the data within that
   *  block to the vector pointed to by read_data.  After this call,
   *  read_data contains elements row_number_start + 0,
   *  row_number_start + 1, ..., row_number_start + (read_data.size()-1)
   *
   *  Returns true if all data has been read.  In this case,
   *  row_number_start is set to the number of elements in the sarray.
   *  Otherwise, returns false.
   *
  */
  inline bool read_next(size_t *row_number_start, std::vector<DataType>* read_data) {

    // The information of what we're currently reading.  Keep the
    // segment alive here in read_segment so it doesn't get closed by
    // another thread before this one gets done actually reading it.
    v2_block_impl::block_address read_block_address;
    std::shared_ptr<segment> read_segment;

    size_t n_elem = 0;

    // Check once here for a CTRL-C interupt as the following
    // interface does not naturally do that.
    if(cppipc::must_cancel()) {
      log_and_throw("Canceled by user.");
    }

    {
      std::lock_guard<simple_spinlock> lg(block_read_lock);

      if(_is_done) {
        *row_number_start = data->size();
        return true;
      }

      // Pull this into what we are currently
      read_segment = segment_of_next_block;
      DASSERT_TRUE(read_segment != nullptr);

      DASSERT_LT(block_number_of_next_block, read_segment->num_blocks());

      // First, set the current block address and block rows,
      // then advance the counter to the next level.
      read_block_address
          = v2_block_impl::block_address{std::get<0>(read_segment->address()),
                                         std::get<1>(read_segment->address()),
                                         block_number_of_next_block};

      n_elem = block_manager.get_block_info(read_block_address).num_elem;
      *row_number_start = row_start_idx_of_next_block;
      row_start_idx_of_next_block += n_elem;

      // Now, advance to the next block or segment.
      _is_done = load_next_block(true);
    } // Release lock.

    // Now, read the current block
    block_manager.read_block(read_block_address, *read_data);
    DASSERT_EQ(read_data->size(), n_elem);

    // Check once once for a CTRL-C interupt.
    if(cppipc::must_cancel()) {
      log_and_throw("Canceled by user.");
    }

    // We are done.
    return false;
  }

 private:

  // The original sarray.
  std::shared_ptr<sarray<DataType> > data;

  // Global block manager.
  v2_block_impl::block_manager& block_manager;

  // Number of segments.
  const size_t num_segments = 0;

  bool _is_done = false;

  simple_spinlock block_read_lock;

  size_t segment_index_of_next_block = 0;
  size_t block_number_of_next_block  = 0;
  size_t row_start_idx_of_next_block = 0;

  // The segment information.  Kept as a shared_ptr to a struct so
  // that things don't get closed when out of line.
  struct segment {
   public:
    // Constructor -- opens the segment.
    segment(const std::shared_ptr<sarray<DataType> >& data, size_t segment_index)
        : block_manager(v2_block_impl::block_manager::get_instance())
    {

      const auto& column_index = data->get_index_info();
      DASSERT_LT(segment_index, column_index.segment_files.size());

      const auto& segment_file = column_index.segment_files[segment_index];

      // Open the next segment.
      segment_address = block_manager.open_column(segment_file);
      _num_blocks = block_manager.num_blocks_in_column(segment_address);
    }

    // Destructor -- important -- closes the segment.
    ~segment() {
      v2_block_impl::block_manager::get_instance().close_column(segment_address);
    }

    // Address of the segment.
    const v2_block_impl::column_address& address() const {
      return segment_address;
    }

    size_t num_blocks() const {
      return _num_blocks;
    }

   private:
    index_file_information column_index;
    v2_block_impl::column_address segment_address;
    v2_block_impl::block_manager& block_manager;

    size_t _num_blocks = 0;
  };

  std::shared_ptr<segment> segment_of_next_block;

  void initialize() {

    if(data->size() == 0) {
      _is_done = true;
      return;
    }

    DASSERT_NE(num_segments, 0);
    segment_index_of_next_block = 0;
    block_number_of_next_block = 0;
    row_start_idx_of_next_block = 0;

    segment_of_next_block.reset(new segment(data, 0));

    // With no advancement, this just makes sure the first segment has
    // one or more blocks.
    load_next_block(false);
  }

  bool load_next_block(bool advance_from_current_position) {

    if(advance_from_current_position) {
      ++block_number_of_next_block;
    }

    while(block_number_of_next_block == segment_of_next_block->num_blocks()) {
      ++segment_index_of_next_block;

      // If we are done,
      if(segment_index_of_next_block == num_segments) {
        return true;
      } else {
        segment_of_next_block.reset(new segment(data, segment_index_of_next_block));
        block_number_of_next_block = 0;
      }
    }

    return false;
  }
};

/** Creates a sarray block iterator; convenience function using
 * automatic template matching.
 *
 */
template <typename T>
sarray_block_iterator<T> make_sarray_block_iterator(const std::shared_ptr<sarray<T> >& data) {
  return sarray_block_iterator<T>(data);
}

/// \}
}
