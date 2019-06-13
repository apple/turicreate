/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ALGORITHMIC_UTILS_H_
#define TURI_ALGORITHMIC_UTILS_H_

#include <algorithm>
#include <core/logging/assertions.hpp>

namespace turi {

/**
 * \ingroup toolkit_util
 *
 * Calls an accumulator on all intersections in two sorted ranges.
 * This behavior is analogous to std::set_intersection, except that
 * the intersections are simply accumulated. Matching is performed
 * through the use of less_than_operator.
 *
 * \tparam InputIterator1 The type of an input iterator over the first range.
 *
 * \tparam InputIterator2 The type of an input iterator over the second range.
 *
 * \param first1 The begin() iterator of the first range.
 *
 * \param last1 The end() iterator of the first range.
 *
 * \param first2 The begin() iterator of the second range.
 *
 * \param last2 The end() iterator of the second range.
 *
 * \param less_than_operator A comparison function that determines the
 * ordering.
 */
template <typename InputIterator1, typename InputIterator2,
          typename ComparisonFunction, typename AccumulateFunction>
static inline void accumulate_intersection(InputIterator1 first1, const InputIterator1& last1,
                                           InputIterator2 first2, const InputIterator2& last2,
                                           const ComparisonFunction& less_than_operator,
                                           const AccumulateFunction& accumulate_matching_pair) {

  DASSERT_TRUE(std::is_sorted(first1, last1, less_than_operator));
  DASSERT_TRUE(std::is_sorted(first2, last2, less_than_operator));

  while (first1 != last1 && first2 != last2) {
    if (less_than_operator(*first1, *first2) ) {
      ++first1;
    } else if (less_than_operator(*first2, *first1) ) {
      ++first2;
    } else {
      accumulate_matching_pair(*first1, *first2);
      ++first1;
      ++first2;
    }
  }
}

/**
 * \ingroup toolkit_util
 * Calls an accumulator on all intersections in two sorted ranges.
 * This behavior is analogous to std::set_intersection, except that
 * the intersections are simply accumulated.
 *
 * \tparam InputIterator1 The type of an input iterator over the first range.
 *
 * \tparam InputIterator2 The type of an input iterator over the second range.
 *
 * \param first1 The begin() iterator of the first range.
 *
 * \param last1 The end() iterator of the first range.
 *
 * \param first2 The begin() iterator of the second range.
 *
 * \param last2 The end() iterator of the second range.
 *
 */
template <typename InputIterator1, typename InputIterator2, typename AccumulateFunction>
static inline void accumulate_intersection(InputIterator1 first1, const InputIterator1& last1,
                                           InputIterator2 first2, const InputIterator2& last2,
                                           const AccumulateFunction& accumulate_matching_pair) {

  typedef typename std::remove_reference<decltype(*first1)>::type value1_type;
  typedef typename std::remove_reference<decltype(*first2)>::type value2_type;

  accumulate_intersection(first1, last1, first2, last2,
                          [](const value1_type& v1,
                             const value2_type& v2)
                          { return v1 < v2; },
                          accumulate_matching_pair);
}

/**
 * \ingroup toolkit_util
 * Counts the number of intersections in two sorted ranges.  This
 * behavior is analogous to std::set_intersection, except that the
 * intersections are simply stored and not output.
 *
 * \tparam InputIterator1 The type of an input iterator over the first range.
 *
 * \tparam InputIterator2 The type of an input iterator over the second range.
 *
 * \param first1 The begin() iterator of the first range.
 *
 * \param last1 The end() iterator of the first range.
 *
 * \param first2 The begin() iterator of the second range.
 *
 * \param last2 The end() iterator of the second range.
 *
 */
template <typename InputIterator1, typename InputIterator2>
static size_t count_intersection(InputIterator1 first1, const InputIterator1& last1,
                                 InputIterator2 first2, const InputIterator2& last2) {

  typedef typename std::remove_reference<decltype(*first1)>::type value1_type;
  typedef typename std::remove_reference<decltype(*first2)>::type value2_type;

  size_t count = 0;
  accumulate_intersection(first1, last1, first2, last2,
                          [&](const value1_type&, const value2_type&)
                          { ++count; });

  return count;
}


/**
 * \ingroup toolkit_util
 * Counts the number of intersections in two sorted ranges.  This
 * behavior is analogous to std::set_intersection, except that the
 * intersections are simply stored and not output.  Matching is
 * performed through the use of less_than_operator.
 *
 * \tparam InputIterator1 The type of an input iterator over the first range.
 *
 * \tparam InputIterator2 The type of an input iterator over the second range.
 *
 * \param first1 The begin() iterator of the first range.
 *
 * \param last1 The end() iterator of the first range.
 *
 * \param first2 The begin() iterator of the second range.
 *
 * \param last2 The end() iterator of the second range.
 *
* \param less_than_operator A comparison function that determines the
 * ordering.
 */
template <typename InputIterator1, typename InputIterator2, typename ComparisonFunction>
static inline size_t count_intersection(InputIterator1 first1, const InputIterator1& last1,
                                        InputIterator2 first2, const InputIterator2& last2,
                                        const ComparisonFunction& less_than_operator) {

  typedef typename std::remove_reference<decltype(*first1)>::type value1_type;
  typedef typename std::remove_reference<decltype(*first2)>::type value2_type;

  size_t count = 0;
  accumulate_intersection(first1, last1, first2, last2,
                          less_than_operator,
                          [&](const value1_type&, const value2_type&)
                          { ++count; });

  return count;
}

}

#endif /* TURI_ALGORITHMIC_UTILS_H_ */
