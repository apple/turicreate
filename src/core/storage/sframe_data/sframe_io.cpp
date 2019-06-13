/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include<core/storage/sframe_data/sframe_io.hpp>
#include<core/data/flexible_type/json_util.hpp>
#include<cstdio>

namespace turi{

/**
 * Write a csv string of a vector of flexible_types (as a row in the sframe) to buffer.
 * Return the number of bytes written.
 */
size_t sframe_row_to_csv(const std::vector<flexible_type>& row, char* buf, size_t buflen) {
  flexible_type val;
  char* cur = buf;
  int space_remaining = buflen;
  int last_bytes_written = 0;
  for(size_t i = 0; (i < row.size()) && (space_remaining); ++i) {
    val = row[i];
    switch(val.get_type()) {
      case flex_type_enum::STRING:
        last_bytes_written = snprintf(cur, space_remaining, "\"%s\"", val.get<flex_string>().c_str());
        break;
      case flex_type_enum::FLOAT:
        last_bytes_written = snprintf(cur, space_remaining, "%f", val.get<flex_float>());
        break;
      case flex_type_enum::INTEGER: {
        static_assert(std::is_same<flex_int, int64_t>::value, "flex_int == int64_t");
        long long r = static_cast<long long>(val.get<flex_int>());
        last_bytes_written = snprintf(cur, space_remaining, "%lld", r);
        break;
      }
      case flex_type_enum::UNDEFINED:
        last_bytes_written = 0;
        break;
      default:
        last_bytes_written = snprintf(cur, space_remaining, "%s", ((std::string)val).c_str());
        break;
    }
    if (last_bytes_written < 0 || last_bytes_written >= space_remaining) {
      return buflen;
    } else {
      cur += last_bytes_written;
      space_remaining -= last_bytes_written;
      if (space_remaining) {
        if (i == row.size()-1) {
          *cur++ = '\n';
          --space_remaining;
        } else {
          *cur++ = ',';
          --space_remaining;
        }
      } else {
        break;
      }
    }
  }
  return (cur - buf);
}

/**
 * Write column_names and column_values (as a row in the sframe) to JSONNode.
 */
void sframe_row_to_json(const std::vector<std::string>& column_names,
                        const std::vector<flexible_type>& column_values,
                        JSONNode& node) {
  DASSERT_EQ(column_names.size(), column_values.size());
  for (size_t i = 0; i < column_names.size(); ++i) {
    node.push_back(flexible_type_to_json(column_values[i], column_names[i]));
  }
}

}
