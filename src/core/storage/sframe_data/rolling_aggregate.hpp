/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_ROLLING_AGGREGATE_HPP
#define TURI_SFRAME_ROLLING_AGGREGATE_HPP

#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_data/sarray.hpp>
#include <core/storage/sframe_data/groupby_aggregate_operators.hpp>

namespace turi {

/**
 * \ingroup sframe_physical
 * \addtogroup groupby_aggregate Groupby Aggregation
 * \{
 */

/**
 * Rolling window aggregators
 */
namespace rolling_aggregate {

typedef boost::circular_buffer<flexible_type>::iterator circ_buffer_iterator_t;
typedef std::function<flexible_type(circ_buffer_iterator_t,circ_buffer_iterator_t)> full_window_fn_type_t;

/**
 * Apply an aggregate function over a moving window.
 *
 * \param input The input SArray (expects to be materialized)
 * \param agg_op The aggregator. These classes are the same as used by groupby.
 * \param window_start The start of the moving window relative to the current
 * value being calculated, inclusive. For example, 2 values behind the current
 * would be -2, and 0 indicates that the start of the window is the current
 * value.
 * \param window_end The end of the moving window relative to the current value
 * being calculated, inclusive. Must be greater than `window_start`. For
 * example, 0 would indicate that the current value is the end of the window,
 * and 2 would indicate that the window ends at 2 data values after the
 * current.
 * \param min_observations The minimum allowed number of non-NULL values in the
 * moving window for the emitted value to be non-NULL. size_t(-1) indicates
 * that all values must be non-NULL.
 *
 * Returns an SArray of the same length as the input, with a type that matches
 * the type output by the aggregation function.
 *
 * Throws an exception if:
 *  - window_end < window_start
 *  - The window size is excessively large (currently hardcoded to UINT_MAX).
 *  - The given function name corresponds to a function that will not operate
 *  on the data type of the input SArray.
 *  - The aggregation function returns more than one non-NULL types.
 */
std::shared_ptr<sarray<flexible_type>> rolling_apply(
    const sarray<flexible_type> &input,
    std::shared_ptr<group_aggregate_value> agg_op,
    ssize_t window_start,
    ssize_t window_end,
    size_t min_observations);


/// Aggregate functions
template<typename Iterator>
flexible_type full_window_aggregate(std::shared_ptr<group_aggregate_value> agg_op,
    Iterator first, Iterator last) {
  auto agg = agg_op->new_instance();
  for(; first != last; ++first) {
    agg->add_element_simple(*first);
  }

  return agg->emit();
}

/**
 * Scans the current window to check for the number of non-NULL values.
 *
 * Returns true if the number of non-NULL values is >= min_observations, false
 * otherwise.
 */
template<typename Iterator>
bool has_min_observations(size_t min_observations,
                          Iterator first,
                          Iterator last) {
  size_t observations = 0;
  size_t count = 0;
  bool need_all = (min_observations == size_t(-1));
  for(; first != last; ++first, ++count) {
    if(first->get_type() != flex_type_enum::UNDEFINED) {
      ++observations;
      if(!need_all && (observations >= min_observations)) {
        return true;
      }
    }
  }

  if(need_all)
    return (observations == count);

  return false;
}

} // namespace rolling_aggregate
} // namespace turi
#endif // TURI_SFRAME_ROLLING_AGGREGATE_HPP
