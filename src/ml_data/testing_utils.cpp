/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <ml_data/testing_utils.hpp>
#include <sframe/testing_utils.hpp>
#include <vector>
#include <string>
#include <map>

namespace turi {

////////////////////////////////////////////////////////////////////////////////


std::pair<sframe, ml_data> make_random_sframe_and_ml_data(
    size_t n_rows, std::string column_types,
    bool create_target_column,
    bool target_column_categorical) {

  sframe data = make_random_sframe(n_rows, column_types, create_target_column);

  std::map<std::string, ml_column_mode> mode_overides;
  for(size_t i = 0; i < data.num_columns(); ++i) {
    if(data.column_type(i) == flex_type_enum::INTEGER)
      mode_overides[data.column_name(i)] = ml_column_mode::CATEGORICAL;
  }

  if(create_target_column && target_column_categorical)
    mode_overides["target"] = ml_column_mode::CATEGORICAL;
  else
    mode_overides["target"] = ml_column_mode::NUMERIC;

  ml_data mdata;
  mdata.fill(data, create_target_column ? "target" : "", mode_overides);

  // Now go through and test all the types to make sure that we have
  // what we want.

  DASSERT_EQ(mdata.metadata()->num_columns(), column_types.size());

  for(size_t i = 0; i < mdata.num_columns(); ++i) {

    switch(column_types[i]) {

      case 'n':
        DASSERT_TRUE(mdata.metadata()->column_mode(i) == ml_column_mode::NUMERIC);
        DASSERT_TRUE(mdata.metadata()->column_type(i) == flex_type_enum::FLOAT);
        break;

      case 'b':
      case 'z':
      case 'Z':
        DASSERT_TRUE(mdata.metadata()->column_mode(i) == ml_column_mode::CATEGORICAL);
        DASSERT_TRUE(mdata.metadata()->column_type(i) == flex_type_enum::INTEGER);
        break;

      case 'c':
      case 'C':
      case 's':
      case 'S':
        DASSERT_TRUE(mdata.metadata()->column_mode(i) == ml_column_mode::CATEGORICAL);
        DASSERT_TRUE(mdata.metadata()->column_type(i) == flex_type_enum::STRING);
        break;

      case 'v':
      case 'V':
        DASSERT_TRUE(mdata.metadata()->column_mode(i) == ml_column_mode::NUMERIC_VECTOR);
        DASSERT_TRUE(mdata.metadata()->column_type(i) == flex_type_enum::VECTOR);
        break;

      case 'l':
      case 'L':
      case 'm':
      case 'M':
      case 'u':
      case 'U':
        DASSERT_TRUE(mdata.metadata()->column_mode(i) == ml_column_mode::CATEGORICAL_VECTOR);
        DASSERT_TRUE(mdata.metadata()->column_type(i) == flex_type_enum::LIST);
        break;

      case 'd':
      case 'D':
        DASSERT_TRUE(mdata.metadata()->column_mode(i) == ml_column_mode::DICTIONARY);
        DASSERT_TRUE(mdata.metadata()->column_type(i) == flex_type_enum::DICT);
        break;
      default:
        break;
    }
  }
  return {data, mdata};

}

}


/** Printing out a row.
 *
 */
std::ostream& operator<<(std::ostream& os, const std::vector<turi::ml_data_entry>& v) {
  os << "[ ";

  for(const auto& e : v)
    os << "(" << e.column_index << "," << e.index << "," << e.value << ") ";

  os << "]";

  return os;
}
