/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_QUERY_EVAL_SORT_COMPARATOR_HPP
#define TURI_QUERY_EVAL_SORT_COMPARATOR_HPP

#include <core/data/flexible_type/flexible_type.hpp>

namespace turi {

namespace query_eval {

/**
 * \ingroup sframe_query_engine
 * \addtogroup Algorithms Algorithms
 * \{
 */

/**
 * \internal
 * Comparator that compares two flex_list value with given ascending/descending
 * order. Order value "true" means ascending, "false" means descending
 * it compares all values in the two flex_list types
 **/
struct less_than_full_function
{
  less_than_full_function() {};
  less_than_full_function(const std::vector<bool>& sort_orders):
    m_sort_orders(sort_orders) {}

  inline bool operator() (const flexible_type& v1, const flexible_type& v2) const
  {
    DASSERT_TRUE(v1.get_type() == flex_type_enum::LIST);
    DASSERT_TRUE(v2.get_type() == flex_type_enum::LIST);

    const flex_list& v1_v = v1.get<flex_list>();
    const flex_list& v2_v = v2.get<flex_list>();
    return compare(v1_v, v2_v);
  }


  inline bool operator() (const std::vector<flexible_type>& v1, const std::vector<flexible_type>& v2) const {
    return compare(v1, v2);
  }

  inline bool operator() (const std::pair<std::vector<flexible_type>, std::string>& v1,
                          const std::pair<std::vector<flexible_type>, std::string>& v2) const {
    return compare(v1.first, v2.first);
  }

  inline bool compare(const std::vector<flexible_type>& v1, const std::vector<flexible_type>& v2) const {
    DASSERT_TRUE(v1.size() == v2.size());
    DASSERT_TRUE(v1.size() == m_sort_orders.size());

    for(size_t i = 0; i < m_sort_orders.size(); i++) {
      bool ascending = m_sort_orders[i];

      if (v1[i] == FLEX_UNDEFINED) {
        if (v2[i] == FLEX_UNDEFINED) {
          continue;  // equal
        } else {
          return ascending;
        }
      }

      if (v2[i] == FLEX_UNDEFINED) {
        return !ascending;
      }

      if (v1[i] < v2[i]) return ascending;
      if (v1[i] > v2[i]) return !ascending;
    }

    return false;
  }
  std::vector<bool> m_sort_orders;
};


/**
 * Comparator that compares two flex_list value with given ascending/descending
 * order and given sort columns.
 * This is different from above in that it is only comparing part of the columns
 * in the value, not all the values
 * Order value "true" means ascending, "false" means descending
 **/
struct less_than_partial_function
{
  less_than_partial_function(const std::vector<size_t>& sort_columns,
                             const std::vector<bool>& sort_orders)
    : m_sort_columns(sort_columns), m_sort_orders(sort_orders)
  {
    DASSERT_TRUE(sort_orders.size() == sort_columns.size());
  }

  inline bool operator() (const std::vector<flexible_type>& v1, const std::vector<flexible_type>& v2)
  {
    DASSERT_TRUE(v1.size() == v2.size());

    size_t i = 0;
    for(auto column_idx: m_sort_columns) {
      DASSERT_TRUE(v1.size() > column_idx);
      bool ascending = m_sort_orders[i++];

      if (v1[column_idx] == FLEX_UNDEFINED) {
        if (v2[column_idx] == FLEX_UNDEFINED) continue;
        return ascending;
      }

      if (v2[column_idx] == FLEX_UNDEFINED) {
        return !ascending;
      }

      if (v1[column_idx] < v2[column_idx]) return ascending;
      if (v1[column_idx] > v2[column_idx]) return !ascending;
    }

    return false;
  }
  std::vector<size_t> m_sort_columns;
  std::vector<bool> m_sort_orders;
};

/// \}
} // end query_eval
} // end turicreate

#endif
