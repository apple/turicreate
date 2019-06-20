/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/logging/logger.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <core/storage/sframe_data/sframe_index_file.hpp>
#include <core/storage/sframe_data/sframe_reader.hpp>

namespace turi {

void sframe_reader::init(const sframe& frame, size_t num_segments) {
  Dlog_func_entry();
  ASSERT_MSG(!inited, "SFrame reader already inited");
  index_info = frame.get_index_info();
  // no columns. Just stop.
  if (index_info.column_names.size() == 0) {
    m_num_segments = 0;
    return;
  }
  if (num_segments == (size_t)(-1)) {
    // use the segmentation of the first column
    m_num_segments = frame.columns[0]->get_index_info().nsegments;
    std::vector<size_t> segment_sizes = frame.columns[0]->get_index_info().segment_sizes;
    for (size_t i = 0;i < index_info.column_names.size(); ++i) {
      column_data.emplace_back(frame.columns[i]->get_reader(segment_sizes));
    }
  } else {
    // create num_segments worth of segments
    m_num_segments = num_segments;
    for (size_t i = 0;i < index_info.column_names.size(); ++i) {
      column_data.emplace_back(frame.columns[i]->get_reader(m_num_segments));
    }
  }
}

void sframe_reader::init(const sframe& frame, const std::vector<size_t>& segment_lengths) {
  Dlog_func_entry();
  ASSERT_MSG(!inited, "SFrame reader already inited");

  // Verify that lengths match up
  index_info = frame.get_index_info();
  size_t sum = 0;
  for (size_t s: segment_lengths) sum += s;
  ASSERT_EQ(sum, size());

  m_num_segments = segment_lengths.size();
  for (size_t i = 0;i < index_info.column_names.size(); ++i) {
    column_data.emplace_back(frame.columns[i]->get_reader(segment_lengths));
  }
}

sframe_reader::iterator sframe_reader::begin(size_t segmentid) const {
  ASSERT_LT(segmentid, num_segments());
  if(segmentid >= num_segments()) log_and_throw(std::string("Invalid segment ID"));
  return sframe_iterator(column_data, segmentid, true);
}

sframe_reader::iterator sframe_reader::end(size_t segmentid) const {
  ASSERT_LT(segmentid, num_segments());
  if(segmentid >= num_segments()) log_and_throw(std::string("Invalid segment ID"));
  return sframe_iterator(column_data, segmentid, false);
}


size_t sframe_reader::read_rows(size_t row_start,
                                size_t row_end,
                                std::vector<std::vector<flexible_type> >& out_obj) {
  std::shared_ptr<std::vector<flexible_type> > coldata = column_pool.get_new_buffer();
  for (size_t i = 0;i < column_data.size(); ++i) {
    column_data[i]->read_rows(row_start, row_end, *coldata);
    if (i == 0) {
      // resize the output
      // coldata.size() rows
      // column_data.size() colunms
      if (out_obj.size() != coldata->size()) out_obj.resize(coldata->size());
      for (size_t j = 0; j < coldata->size(); ++j) {
        if (out_obj[j].size() != column_data.size()) {
          out_obj[j].resize(column_data.size());
          out_obj[j].shrink_to_fit();
        }
      }
   }
    ASSERT_EQ(out_obj.size(), coldata->size());
    for (size_t j = 0;j < coldata->size(); ++j) {
      out_obj[j][i] = std::move((*coldata)[j]);
    }
  }
  column_pool.release_buffer(std::move(coldata));
  return out_obj.size();
}

size_t sframe_reader::read_rows(size_t row_start,
                                size_t row_end,
                                sframe_rows& out_obj) {
  // sframe_rows is made up of a collection of columns
  out_obj.resize(column_data.size());
  for (size_t i = 0;i < column_data.size(); ++i) {
    column_data[i]->read_rows(row_start, row_end, *(out_obj.get_columns()[i]));
  }
  return out_obj.num_rows();
}

void sframe_reader::reset_iterators() {
  for (auto& col: column_data) {
    col->reset_iterators();
  }
}

} // end of namespace
