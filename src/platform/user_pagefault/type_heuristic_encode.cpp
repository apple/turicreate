/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <vector>
#include <limits>
#include <sframe/sarray_v2_type_encoding.hpp>
namespace turi {
namespace type_heuristic_encode {

constexpr size_t COMPRESSION_PROBE_NUM_VALUES = 8192;
constexpr size_t COMPRESSION_PROBE_MAX_COLUMNS = 16;
constexpr size_t COMPRESSION_PROBE_NUM_COLUMNS_MAX_COLUMNS = 256;

static bool test_is_integers(int64_t* start, size_t numel) {
  for (size_t i = 0;i < numel; ++i) {
    if (start[i] >= std::numeric_limits<uint32_t>::max()) return false;
  }
  return true;
}


template <typename T>
size_t infer_num_columns(T* start, size_t numel) {
  // We fist bump all the values down by a log factor.
  // value_range[i] = {log(abs(start[i])), i}
  //
  // This will be used to quickly "cluster" potential columns together.
  std::vector<size_t> value_ranges(numel);
  for (size_t i = 0;i < numel; ++i) {
    value_ranges[i] = std::log2(std::fabs(start[i]));
  }
  size_t max_columns = std::min(numel, COMPRESSION_PROBE_MAX_COLUMNS);
  max_columns = std::min(max_columns, numel / 2);
  
  size_t best_num_col = 1;
  double best_score = std::numeric_limits<double>::max();

  for (size_t num_col = 1; num_col < max_columns; ++num_col) {
    /*
     * The score of a certain number of columns is basically the sum of all
     * gaps of num_cols inside value_ranges.
     */
    double score = 0;
    for (size_t i = num_col; i < numel; ++i) {
      score += std::abs<T>(value_ranges[i] - value_ranges[i - num_col]);
    }
    // get a normalized score. 
    // Since the number of sums I perform vary with num_col
    // plus a little regularizer so we prefer fewer number of columns.
    //
    // (otherwise considering something with 2 columns, all values in column 0
    // are identical, and all values in column 1 are identical. The score for
    // #columns = 2 and #columns = 4 and #columns = 6 .... will be the same.
    //
    // But we want to prefer #columns = 2
    score = score / (numel - num_col); 
    score += 0.01 * num_col;
    if (score < best_score) {
      best_score = score;
      best_num_col = num_col;
      // std::cout << "     current best #cols" << best_num_col << " " << best_score << std::endl;
    }
  }
  // std::cout << "Num Columns infered as " << best_num_col << " " << best_score << std::endl;
  return best_num_col;
}

void compress(char* start, size_t length, 
              char** output, size_t& output_length) {
  // ok. first. are the contents integers? Or doubles
  size_t numel = length / sizeof(int64_t);
  int64_t* input = reinterpret_cast<int64_t*>(start);

  bool is_integers = test_is_integers(input,
                                      std::min(numel, COMPRESSION_PROBE_NUM_VALUES));

  size_t ncols;
  if (is_integers) {
    ncols = infer_num_columns<int64_t>(input,
                                       std::min(numel, 
                                                COMPRESSION_PROBE_NUM_COLUMNS_MAX_COLUMNS));
  } else {
    ncols = infer_num_columns<double>(reinterpret_cast<double*>(input),
                                       std::min(numel, 
                                                COMPRESSION_PROBE_NUM_COLUMNS_MAX_COLUMNS));
  }

  oarchive oarc;
  oarc.buf = *output;
  oarc.off = 0;
  oarc.len = output_length;

  oarc << (unsigned char)(is_integers) 
       << (unsigned char)(ncols) 
       << numel;
  std::vector<std::vector<flexible_type> > column_buffer(ncols);
  size_t elem_per_col = numel / ncols;
  // the elem_per_col may not divide perfectly.
  // there may be some overrun
  // ex: 5 values, 2 columns, there is an overrun of 1. 
  // column 0 must take 3 values
  size_t overrun = ncols - elem_per_col * ncols;
  for (size_t i = 0; i < ncols; ++i) {
    if (is_integers) {
      column_buffer[i].resize(elem_per_col + (i < overrun),
                              flexible_type(flex_type_enum::INTEGER));
    } else {
      column_buffer[i].resize(elem_per_col + (i < overrun), 
                              flexible_type(flex_type_enum::FLOAT));
    }
  }

  // loop through the array, fill in the column buffers and encode
  // when the column buffers are full (128)
  size_t cur_row = 0;
  size_t cur_column = 0;
  for (size_t i = 0;i < numel; ++i) {
    // we store the raw value whether or not it is an integer.
    // without trying to interpret it
    column_buffer[cur_column][cur_row].reinterpret_mutable_get<flex_int>() = input[i];
    ++cur_column;
    if (cur_column >= ncols) {
      cur_column = 0;
      ++cur_row;
    }
  }
  // begin encoding
  // we are going to use the sarray v2 type encoders and they want a block_info
  // structure. We just fake one.
  v2_block_impl::block_info info;
  if (is_integers) {
    for (size_t i = 0;i < ncols; ++i) {
      v2_block_impl::encode_number(info, oarc, column_buffer[i]);
    }
  } else {
    for (size_t i = 0;i < ncols; ++i) {
      v2_block_impl::encode_double(info, oarc, column_buffer[i]);
    }
  }
  (*output) = oarc.buf;
  output_length = oarc.off;
}


void decompress(char* start, size_t length, char* output) {
  int64_t* output_values = reinterpret_cast<int64_t*>(output);
  bool is_integers = false;
  size_t ncols = 0;
  size_t numel = 0;

  unsigned char c;
  iarchive iarc(start, length);
  iarc >> c;  is_integers = c;
  iarc >> c;  ncols = c;
  iarc >> numel;

  std::vector<std::vector<flexible_type> > column_buffer(ncols);
  size_t elem_per_col = numel / ncols;
  size_t overrun = ncols - elem_per_col * ncols;

  std::vector<flexible_type> f;
  f.reserve(elem_per_col + 1);

  for (size_t i = 0;i < ncols; ++i) {
    f.clear();
    if (is_integers) {
      f.resize(elem_per_col + (i < overrun), 
               flexible_type(flex_type_enum::INTEGER));
      v2_block_impl::decode_number(iarc, f, 0);
    } else {
      f.resize(elem_per_col + (i < overrun), 
               flexible_type(flex_type_enum::FLOAT));
      v2_block_impl::decode_double(iarc, f, 0);
    }
    // scatter
    size_t ctr = 0;
    for (size_t j = i; j < numel; j += ncols) {
      output_values[j] = f[ctr].reinterpret_get<flex_int>();
      ++ctr;
    }
  }
}

} // type_heuristic_encode
} // turicreate

