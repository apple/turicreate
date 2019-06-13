/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_GROUPED_TIMESERIES_HPP
#define TURI_GROUPED_TIMESERIES_HPP

#include <model_server/lib/toolkit_class_macros.hpp>
#include <model_server/lib/extensions/model_base.hpp>
#include <model_server/extensions/grouped_sframe.hpp>

namespace turi {
namespace timeseries {

class gl_timeseries;

class EXPORT gl_grouped_timeseries : public model_base {
 public:
  /**
   * Groups a TimeSeries by the distinct values in one or more columns.
   *
   * Logically, this creates a TimeSeries for each "group" of values, where the
   * new TimeSeries' all have the same number of columns as the original
   * TimeSeries.  These are accessed through the interface of this data
   * structure.
   *
   * \param sf The underlying SFrame of the TimeSeries.
   * \param index_col_name The index column of the TimeSeries.
   * \param column_names List of column names to group by.
   *
   * Throws if group has already been called on this object, or the column
   * names are not valid.
   */
  void group(const gl_sframe &sf,
             std::string index_col_name,
             const std::vector<std::string> column_names);

  /**
   * Get the SFrame that corresponds to the group named `key`.
   *
   * Each group's name is its distinct value, including its type. This means
   * that an SFrame grouped by a column of integers that has some 1s and some
   * 2s, the name of the group with ones is the integer 1, not the string '1'.
   * The key is given as a vector because more than one columns can be used to
   * group.
   *
   * \param key Name of group to retrieve.
   * \returns An SFrame that can immediately be interpreted as a TimeSeries
   * (i.e. it is sorted by its time index column.)
   */
  gl_sframe get_group(const std::vector<flexible_type> key);

  /**
   * The number of distinct groups found.
   */
  size_t num_groups() const {
    return m_grouped_sframe.num_groups();
  }

  /**
   * Return an SFrame with group_info i.e key columns + number of rows in each
   * key column.
   */
  gl_sframe group_info() const {
    return m_grouped_sframe.group_info();
  }

  /**
   * A list of all the group names.
   */
  gl_sarray groups() {
    return m_grouped_sframe.groups();
  }

  void begin_iterator() {
    m_grouped_sframe.begin_iterator();
  }

  std::vector<std::pair<flexible_type, gl_sframe>> iterator_get_next(size_t num);

  /**
   * Return the index column name of the time series (not the same as the group
   * column)
  */
  gl_sframe get_sframe() const {
    return m_grouped_sframe.get_sframe();
  }

  /**
   * Return the index column name of the time series (not the same as the group
   * column)
  */
  std::string get_index_column_name() const {
    return m_time_index_name;
  }

  /**
   * Return the value columns in the timeseries.
  */
  std::vector<std::string> get_value_col_names() const {
    return m_value_col_names;
  }

  /**
   * Return the list of columns on which the data is grouped.
   */
  std::vector<std::string> get_key_col_names() const {
    return m_key_col_names;
  }

 private:
  grouped_sframe m_grouped_sframe;
  std::string m_time_index_name;
  std::vector<std::string> m_key_col_names;
  std::vector<std::string> m_value_col_names;

 public:
  BEGIN_CLASS_MEMBER_REGISTRATION("_GroupedTimeseries")

    REGISTER_CLASS_MEMBER_FUNCTION(gl_grouped_timeseries::group, "data",
      "index_name", "column_names")
  REGISTER_CLASS_MEMBER_FUNCTION(gl_grouped_timeseries::num_groups)
  REGISTER_CLASS_MEMBER_FUNCTION(gl_grouped_timeseries::groups)
  REGISTER_CLASS_MEMBER_FUNCTION(gl_grouped_timeseries::group_info)
  REGISTER_CLASS_MEMBER_FUNCTION(gl_grouped_timeseries::begin_iterator)
  REGISTER_CLASS_MEMBER_FUNCTION(gl_grouped_timeseries::iterator_get_next,
      "num_items")
  REGISTER_CLASS_MEMBER_FUNCTION(gl_grouped_timeseries::get_group, "key")

  REGISTER_GETTER("sframe", gl_grouped_timeseries::get_sframe)
  REGISTER_GETTER("index_column_name",
      gl_grouped_timeseries::get_index_column_name)
  REGISTER_GETTER("value_col_names", gl_grouped_timeseries::get_value_col_names)
  REGISTER_GETTER("key_columns", gl_grouped_timeseries::get_key_col_names)

  END_CLASS_MEMBER_REGISTRATION
};

} // namespace timeseries
} // namespace turi
#endif // TURI_GROUPED_TIMESERIES_HPP
