/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/util/sframe_utils.hpp>
#include <core/storage/sframe_data/sframe.hpp>
#include <limits>

namespace turi {

std::pair<sframe,sframe> split_sframe_on_index(const sframe& src, std::function<bool(size_t)> switch_function) {

  sframe s0, s1;

  std::vector<std::string> column_names(src.num_columns());
  std::vector<flex_type_enum> column_types(src.num_columns());

  for(size_t i = 0; i < src.num_columns(); ++i) {
    column_names[i] = src.column_name(i);
    column_types[i] = src.column_type(i);
  }

  s0.open_for_write(column_names, column_types);
  s1.open_for_write(column_names, column_types);

  auto reader = src.get_reader();

  size_t num_segments = src.num_segments();

  std::vector<size_t> index_offsets(num_segments);

  {
    size_t acc_idx = 0;
    for(size_t sidx = 0; sidx < num_segments; ++sidx) {
      index_offsets[sidx] = acc_idx;
      acc_idx += src.segment_length(sidx);
    }
  }

  parallel_for(0, num_segments, [&](size_t sidx) {
      size_t idx = index_offsets[sidx];
      auto s0_out = s0.get_output_iterator(sidx);
      auto s1_out = s1.get_output_iterator(sidx);

      auto it = reader->begin(sidx);
      auto it_end = reader->end(sidx);

      for(;it != it_end; ++it) {

        if(switch_function(idx)) {
          *s1_out = *it;
          ++s1_out;
        } else {
          *s0_out = *it;
          ++s0_out;
        }

        ++idx;
      }
    });

  s0.close();
  s1.close();

  DASSERT_EQ(s0.size() + s1.size(), src.size());

  return {s0, s1};
}

/**
 * Write the provided matrix as an SArray, where each row is
 * an element in the SArray with vector type.
 */
std::shared_ptr<sarray<flexible_type>>
matrix_to_sarray(const Eigen::MatrixXd& m) {

  // Write the probabilities to an SArray of vector type.
  std::shared_ptr<sarray<flexible_type> > sa(new sarray<flexible_type>);
  size_t num_segments = thread::cpu_count();
  sa->open_for_write(num_segments);
  sa->set_type(flex_type_enum::VECTOR);

  size_t num_rows = m.rows();
  size_t num_cols = m.cols();
  in_parallel([&](size_t thread_idx, size_t num_threads) {
    auto it_out = sa->get_output_iterator(thread_idx);
    size_t segment_row_start = size_t(thread_idx * num_rows / num_segments);
    size_t segment_row_end = (thread_idx == num_segments - 1) ?
           num_rows : size_t((thread_idx + 1) * num_rows / num_segments);

    std::vector<double> v(num_cols);
    for (size_t i = segment_row_start; i < segment_row_end; ++i) {
      for (size_t j = 0; j < num_cols; ++j) {
        v[j] = m(i, j);
      }
      *it_out = v;
      ++it_out;
    }
  });

  sa->close();
  return sa;
}



}
