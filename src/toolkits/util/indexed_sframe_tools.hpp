/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_INDEXED_SFRAME_TOOLS_H_
#define TURI_UNITY_INDEXED_SFRAME_TOOLS_H_

#include <vector>
#include <map>
#include <memory>

namespace turi {

class flexible_type;
template <typename T> class sarray;

/**
 * \ingroup toolkit_util
 * Constructs a vector of the unique values present in an sframe
 *  column having integer type.  The resulting vector is in sorted order,
 *  so membership can be queried using std::binary_search.  When the
 *  0, ..., n condition is met, this is faster than .unique().
 */
std::vector<size_t> get_unique_values(std::shared_ptr<sarray<flexible_type> > indexed_column);

/**
 * \ingroup toolkit_util
 * Convenience function: Same as get_unique_values, but returns the
 *  result as an sarray.
 */
std::shared_ptr<sarray<flexible_type> > make_unique(std::shared_ptr<sarray<flexible_type> > indexed_column);

/**
 * \ingroup toolkit_util
 * Creates an in-memory group lookup table from one integer column to
 * another.  In this representation, all the items in one column
 * associated with a given value in the other column can be retrieved
 * by that value.
 */
class indexed_column_groupby {
 public:

  /** Builds a mapping of integer values in src_column to a vector of
   *  all the associated values in dest_column.  This can be queried
   *  using dest_group.
   *
   *  If sort is true, then the returned elements are sorted.
   *
   *  If uniquify is true, then the returned vector has at most one of
   *  any given element.
   */
  indexed_column_groupby(std::shared_ptr<sarray<flexible_type> > src_column,
                         std::shared_ptr<sarray<flexible_type> > dest_column,
                         bool sort,
                         bool uniquify);

  /** Returns a vector with all the associated values in dest_column
   * that have src_value in the corresponding location of src_column.
   * If src_value does not appear in src_column, then the returned
   * vector is empty.
   */
  const std::vector<size_t>& dest_group(size_t src_value) const;

 private:
  std::vector<size_t> empty_vector;
  std::map<size_t, std::vector<size_t> > group_lookup;
};


}

#endif
