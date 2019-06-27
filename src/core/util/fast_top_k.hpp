/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FAST_TOP_K_H_
#define TURI_FAST_TOP_K_H_

#include <vector>
#include <array>
#include <algorithm>

namespace turi {

template <typename T, typename LessThan>
GL_HOT_NOINLINE_FLATTEN
void __run_top_k_small_k(std::vector<T>& v, LessThan less_than, size_t k) {

  std::sort(v.begin(), v.begin() + k, less_than);

  for(size_t i = k; i < v.size(); ++i) {
    if(less_than(v[0], v[i])) {

#ifndef NDEBUG
      // Preserve all the elements so the debug routines below can check things.
      std::swap(v[0], v[i]);
#else
      // Just do an assignment.
      v[0] = v[i];
#endif

      for(size_t j = 1; j < k; ++j) {
        if(!less_than(v[j-1], v[j])) {
          std::swap(v[j], v[j-1]);
        } else {
          break;
        }
      }
    }
  }

#ifndef NDEBUG

  // Run checking code here to make sure this is equivalent to
  // nth_element + sort.
  std::vector<T> va;
  va.assign(v.begin(), v.begin() + k);

  auto gt_sorter = [&](const T& t1, const T& t2) {
    return less_than(t2, t1);
  };

  std::nth_element(v.begin(), v.begin() + k, v.end(), gt_sorter);

  std::sort(v.begin(), v.begin() + k, gt_sorter);
  for(size_t j = 0; j < k; ++j) {
    // test for equality using the less_than operator
    ASSERT_TRUE(!less_than(v[j], va[k - 1 - j]) && !less_than(va[k - 1 - j], v[j]));
  }

  for(size_t i = k; i < v.size(); ++i) {
    for(size_t j = 0; j < k; ++j) {
      ASSERT_TRUE(bool(!less_than(v[j], v[i])));
    }
  }

  // Copy them back in sorted decreasing order.
  for(size_t i = 0; i < k; ++i) {
    v[k - 1 - i] = va[i];
  }
#else
  std::reverse(v.begin(), v.begin() + k);
#endif

  DASSERT_TRUE(bool(std::is_sorted(v.begin(), v.begin() + k, gt_sorter)));

  v.resize(k);
}

/**
 * \ingroup util
 * Goes through and extracts the top k out of all the elements in v,
 * then resizes v to be of size top_k.  After running this, the
 * elements of v are in sorted descending order.
 */
template <typename T, typename LessThan>
void extract_and_sort_top_k(
    std::vector<T>& v, size_t top_k, LessThan less_than) {

  auto gt_sorter = [&](const T& t1, const T& t2) {
    return less_than(t2, t1);
  };

  if(v.size() <= top_k) {
    std::sort(v.begin(), v.end(), gt_sorter);
    return;
  }

  if(top_k <= 10) {
    __run_top_k_small_k(v, less_than, top_k);
    return;
  }

  std::nth_element(v.begin(), v.begin() + top_k, v.end(), gt_sorter);
  v.resize(top_k);
  std::sort(v.begin(), v.end(), gt_sorter);
  return;
}

/**
 * \ingroup util
 * Goes through and extracts the top k out of all the elements in v,
 * then resizes v to be of size top_k.  After running this, the
 * elements of v are in sorted descending order.
 */
template <typename T>
void extract_and_sort_top_k(
    std::vector<T>& v, size_t top_k) {
  extract_and_sort_top_k(v, top_k, std::less<T>());
}


}

#endif /* _FAST_TOP_K_H_ */
