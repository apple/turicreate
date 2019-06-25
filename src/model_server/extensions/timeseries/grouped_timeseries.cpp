/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <model_server/extensions/timeseries/grouped_timeseries.hpp>
#include <model_server/extensions/timeseries/timeseries.hpp>

namespace turi {
namespace timeseries {

void gl_grouped_timeseries::group(const gl_sframe &sf,
                                  std::string index_col_name,
                                  const std::vector<std::string> column_names) {
  m_time_index_name = index_col_name;

  // Check that the index is not one of the sort columns. We do this because
  // the time index must be the last group column to keep the output sorted by
  // time value. Also, it is somewhat meaningless to group by the values in the
  // time index itself. Instead, group by constants like "DAY" or "HOUR" (this
  // option is only available from Python).
  auto find_ret = std::find(column_names.begin(),
                            column_names.end(),
                            m_time_index_name);
  if(find_ret != column_names.end()) {
    std::stringstream ss;
    ss << "Cannot group timeseries by the index column."
       << " To group the TimeSeries by a part of it's timestamp, use the special"
       << " types declared in 'turicreate.TimeSeries.date_part'" << std::endl;
    log_and_throw(ss.str());
  }

  // Check that we get at least one column name!
  if(column_names.size() < 1) {
    log_and_throw("Must provide at least one column to group!");
  }
  for(const auto& col: column_names) {
    if (! sf.contains_column(col)) {
      std::stringstream ss;
      ss << "Cannot group on the column '" << col << "'."
         << " Column does not exist in the input data." << std::endl;
      log_and_throw(ss.str());
    }
  }
  m_key_col_names = column_names;
  m_value_col_names = sf.column_names();
  auto it = std::find(m_value_col_names.begin(), m_value_col_names.end(),
                      m_time_index_name);
  m_value_col_names.erase(it); // remove index.

  // Add the time index as the last sort column
  std::vector<std::string> sort_columns(column_names.size() + 1);
  std::copy(column_names.begin(), column_names.end(), sort_columns.begin());
  sort_columns[column_names.size()] = m_time_index_name;
  auto grouped_sf = sf.sort(sort_columns);

  m_grouped_sframe = turi::grouped_sframe();
  m_grouped_sframe.group(grouped_sf, column_names, true);
}

gl_sframe gl_grouped_timeseries::get_group(const std::vector<flexible_type> key) {
  return m_grouped_sframe.get_group(key);
}

std::vector<std::pair<flexible_type,gl_sframe>>
gl_grouped_timeseries::iterator_get_next(size_t num) {
  return m_grouped_sframe.iterator_get_next(num);
}

} // namespace timeseries
} // namespace turi
