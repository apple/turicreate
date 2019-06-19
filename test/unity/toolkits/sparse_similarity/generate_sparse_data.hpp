/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TESTING_GENERATE_SPARSE_DATA_H
#define TESTING_GENERATE_SPARSE_DATA_H

#include <vector>
#include <core/random/random.hpp>

using namespace turi;

static std::vector<std::vector<std::pair<size_t, double> > > generate(
    size_t n, size_t m, double p, bool allow_negative, bool binary) {

  std::vector<std::vector<std::pair<size_t, double> > > data;

  data.resize(n);

  for(size_t i = 0; i < size_t(n * m * p); ++i) {
    size_t row_idx = random::fast_uniform<size_t>(0, n-1);
    size_t col_idx = random::fast_uniform<size_t>(0, m-1);
    double value = random::fast_uniform<double>(0, 1);

    if(binary) {
      value = (value < 0.1) ? 0 : 1;
    } else if(allow_negative) {
      value = 2 * (value - 0.5);
    }

    data[row_idx].push_back({col_idx, value});
  }

  auto idx_cmp_f = [](const std::pair<size_t, double>& p1,
                      const std::pair<size_t, double>& p2) {
    return p1.first < p2.first;
  };

  auto idx_eq_f = [](const std::pair<size_t, double>& p1,
                     const std::pair<size_t, double>& p2) {
    return p1.first == p2.first;
  };

  for(auto& row : data) {
    std::sort(row.begin(), row.end(), idx_cmp_f);

    auto it = std::unique(row.begin(), row.end(), idx_eq_f);
    row.resize(it - row.begin());
  }

  return data;
}

#endif /* GENERATE_SPARSE_SARRAY_H */
