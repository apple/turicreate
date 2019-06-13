/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_compact.hpp>
namespace turi {

bool sframe_fast_compact(const sframe& sf) {
  bool ret = false;
  for (size_t i = 0;i < sf.num_columns(); ++i) {
    auto cur_column = sf.select_column(i);
    ret |= sarray_fast_compact(*cur_column);
  }
  return ret;
}


void sframe_compact(sframe& sf, size_t segment_threshold) {
  sframe_fast_compact(sf);
  size_t num_above_threshold = 0;
  for (size_t i = 0;i < sf.num_columns(); ++i) {
    auto cur_column = sf.select_column(i);
    if (cur_column->get_index_info().segment_files.size() > segment_threshold) {
      ++num_above_threshold;
    }
  }

  if (num_above_threshold == sf.num_columns()) {
    //rewrite the entire sframe
    sframe ret;
    size_t nsegments = std::min(segment_threshold, thread::cpu_count());
    ret.open_for_write(sf.column_names(), sf.column_types(), "", nsegments);
    auto reader = sf.get_reader(nsegments);
    parallel_for(0, nsegments, [&](size_t segment_id) {
                 auto iter = reader->begin(segment_id);
                 auto end = reader->end(segment_id);
                 auto out = ret.get_output_iterator(segment_id);
                 while (iter != end) {
                   *out = *iter;
                   ++out;
                   ++iter;
                 }
                 });
    ret.close();
    sf = ret;
  } else {
    // just rewrite some columns
    for (size_t i = 0;i < sf.num_columns(); ++i) {
      auto cur_column = sf.select_column(i);
      if (cur_column->get_index_info().segment_files.size() > segment_threshold) {
        sarray_compact(*cur_column, segment_threshold);
      }
    }
  }
}

}
