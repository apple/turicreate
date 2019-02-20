/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UTIL_SPMAT_H_
#define TURI_UTIL_SPMAT_H_

#include <vector>
#include <map>
#include <random/alias.hpp>
#include <Eigen/Core>


/**
 * \ingroup toolkit_util
 * A simple utility class for representing sparse matrices of counts.
 * It exposes getting particular elements, incrementing elements by a 
 * value, and removing zero elements from the internal data structure.
 * It is row-based, so only exposes get_row and num_rows.
 */
class spmat {
 public:

  /**
   * Create a sparse matrix with a fixed number of rows.
   */
  spmat(size_t num_rows = 0) {
    m = std::vector<std::map<size_t, size_t>>(num_rows);
  }
    
  /**
   * Get a vector of nonzero elements in a single row. 
   */
  const std::map<size_t, size_t>& get_row(size_t i) {
    return m[i];
  }

  /**
   * Get the count at element(i,j).
   */
  size_t get(const size_t i, const size_t j) {
    if (m[i].count(j) == 0) {
      return 0;
    } else {
      return m[i].at(j);
    }
  }
  
  /**
   * Get the number of rows.
   */
  size_t num_rows() const { return m.size(); }

  /**
   * Delete zeros in a single row
   */
  void trim(const size_t i) {
    auto it = m[i].begin(); 
    for( ; it != m[i].end();) {
      if (it->second == 0) {
        it = m[i].erase(it);
      } else {
        ++it;
      }
    }
  }

  /**
   * Increment the element (a, b) by v. 
   */
  void increment(const size_t& a, const size_t& b, const size_t& v) {
    auto it = m[a].find(b);
    if (it == m[a].end()) {
      m[a][b] = v;
    } else {
      it->second += v;
    }
  }

  /** 
   * Convert to Eigen matrix.
   */
  Eigen::MatrixXi as_matrix() {
    size_t nrows = m.size();
    size_t ncols = 0;
    for (size_t i = 0; i < nrows; ++i) {
      auto row = get_row(i); 
      for (auto it = row.begin(); it != row.end(); ++it) {
        auto col = it->first;
        if (col >= ncols) ncols = col+1; // zero-based indexing
      }
    }
    auto ret = Eigen::MatrixXi(nrows, ncols);
    for (size_t i = 0; i < nrows; ++i) {
      auto row = get_row(i); 
      for (auto it = row.begin(); it != row.end(); ++it) {
        auto j = it->first;
        auto v = it->second;
        ret(i, j) = v;
      }
    }
    return ret;
  }
 
 private:
  std::vector<std::map<size_t, size_t>> m;
};

#endif
