/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <algorithm>
#include <core/data/flexible_type/flexible_type.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/algorithm/string.hpp>
#include <core/storage/sframe_data/rolling_aggregate.hpp>

namespace turi {
namespace rolling_aggregate {

ssize_t clip(ssize_t val, ssize_t lower, ssize_t upper) {
  return std::min(upper, std::max(lower, val));
}

// Calculate the size of an inclusive range
size_t calculate_window_size(ssize_t start, ssize_t end) {
  if((start < 0) && (end >= 0)) {
    return size_t(std::abs(start) + end + 1);
  }

  auto the_size = size_t(std::abs(end - start) + 1);

  return the_size;
}

std::shared_ptr<sarray<flexible_type>> rolling_apply(
    const sarray<flexible_type> &input,
    std::shared_ptr<group_aggregate_value> agg_op,
    ssize_t window_start,
    ssize_t window_end,
    size_t min_observations) {
  /// Sanity checks
  if(window_start > window_end) {
    log_and_throw("Start of window cannot be > end of window.");
  }

  // Check type is supported by aggregate operator
  if(!agg_op->support_type(input.get_type())) {
    log_and_throw(agg_op->name() + std::string(" does not support input type."));
  }

  agg_op->set_input_type(input.get_type());

  // Get window size given inclusive range
  size_t total_window_size = calculate_window_size(window_start, window_end);
  if(total_window_size > uint32_t(-1)) {
    log_and_throw("Window size cannot be larger than " +
        std::to_string(uint32_t(-1)));
  }

  bool check_num_observations = (min_observations != 0);

  if(min_observations > total_window_size) {
    if(min_observations != size_t(-1))
      logprogress_stream << "Warning: min_observations (" << min_observations <<
        ") larger than window size (" << total_window_size <<
        "). Continuing with min_observations=" << total_window_size << "." <<
        std::endl;
    min_observations = total_window_size;
  }

  auto num_segments = thread::cpu_count();

  // sarray_reader as shared_ptr so we can use sarray_reader_buffer. Segments
  // are not used to actually iterate, just to evenly split up the array.
  std::shared_ptr<sarray_reader<flexible_type>> reader(
      input.get_reader(num_segments));
  auto ret_sarray = std::make_shared<sarray<flexible_type>>();
  ret_sarray->open_for_write(num_segments);

  // Calculate the valid range of data that each segment will need to do its
  // full rolling aggregate calculation.
  size_t running_length = 0;
  std::vector<std::pair<size_t,size_t>> seg_ranges(num_segments);
  std::vector<size_t> seg_starts(num_segments);
  for(size_t i = 0; i < num_segments; ++i) {
    seg_starts[i] = running_length;
    ssize_t beg = clip((window_start+running_length), 0, reader->size());
    running_length += reader->segment_length(i);
    ssize_t end = clip(window_end+(running_length-1), 0, reader->size());
    seg_ranges[i] = std::make_pair(size_t(beg), size_t(end));
  }

  // Store the type returned by the aggregation function of each segment
  std::vector<flex_type_enum> fn_returned_types(num_segments,
                                                flex_type_enum::UNDEFINED);

  parallel_for(0, num_segments, [&](size_t segment_id) {
    auto range = seg_ranges[segment_id];

    // Create buffer for the window
    auto window_buf = boost::circular_buffer<flexible_type>(total_window_size,
        flex_undefined());
    auto out_iter = ret_sarray->get_output_iterator(segment_id);

    sarray_reader_buffer<flexible_type> buf_reader(reader,
                                                   range.first,
                                                   range.second+1);

    // The esteemed "current" value that all the documentation talks about
    ssize_t logical_pos = ssize_t(seg_starts[segment_id]);
    // The last row for which this thread should calculate the aggregate
    ssize_t logical_end = ssize_t(seg_starts[segment_id] +
        reader->segment_length(segment_id));

    // The "fake" row numbers that comprise the window of the current value.
    // This can have negative numbers and numbers past the end.
    std::pair<ssize_t,ssize_t> my_logical_window =
      std::make_pair(window_start+logical_pos, window_end+logical_pos);

    // Initially fill the window buffer
    for(ssize_t i = my_logical_window.first;
        i <= my_logical_window.second;
        ++i) {
      if(i >= 0 && buf_reader.has_next()) {
        window_buf.push_back(buf_reader.next());
      } else {
        // If this is a "fake" section of the logical window, just fill with
        // NULL values
        window_buf.push_back(flex_undefined());
      }
    }

    // Go through array with window
    while(logical_pos < logical_end) {
      // First check if we have the minimum non-NULL observations. This is here
      // to remove the burden of checking from every aggregation function.
      if(check_num_observations &&
          !has_min_observations(min_observations,
            window_buf.begin(), window_buf.end())) {
        *out_iter = flex_undefined();
      } else {
        auto result = full_window_aggregate(agg_op, window_buf.begin(), window_buf.end());
        // Record the emitted type from the function. We just take the first
        // one that is non-NULL.
        if(fn_returned_types[segment_id] == flex_type_enum::UNDEFINED &&
            result.get_type() != flex_type_enum::UNDEFINED) {
          fn_returned_types[segment_id] = result.get_type();
        }
        *out_iter = result;
      }

      // Update logical window
      ++logical_pos;
      ++my_logical_window.first;
      ++my_logical_window.second;

      // Get the next value in the SArray
      if(my_logical_window.second >= 0 && buf_reader.has_next()) {
        window_buf.push_back(buf_reader.next());
      } else {
        // If this is a "fake" section of the logical window, just fill with
        // NULL values
        window_buf.push_back(flex_undefined());
      }
    }
  }
  );

  // Set output type of SArray based on what the aggregation function returned
  flex_type_enum array_type = flex_type_enum::UNDEFINED;
  for(const auto &i : fn_returned_types) {
    // Error out if the aggregation function outputs values with more than one
    // type (not counting NULL)
    if((i != array_type) &&
        (array_type != flex_type_enum::UNDEFINED) &&
        (i != flex_type_enum::UNDEFINED)) {
      log_and_throw("Aggregation function returned two different non-NULL "
          "types!");
    }
    if(i != flex_type_enum::UNDEFINED) {
      array_type = i;
    }
  }

  ret_sarray->set_type(array_type);

  ret_sarray->close();
  return ret_sarray;
}

} // namespace rolling_aggregate
} // namespace turi
