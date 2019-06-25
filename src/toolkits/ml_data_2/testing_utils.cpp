/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <toolkits/ml_data_2/testing_utils.hpp>
#include <core/storage/sframe_data/testing_utils.hpp>
#include <vector>
#include <string>
#include <map>

namespace turi { namespace v2 {

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

std::pair<sframe, ml_data> make_random_sframe_and_ml_data(
    size_t n_rows, std::string column_types,
    bool create_target_column,
    const std::map<std::string, flexible_type>& _options) {

  std::map<std::string, flexible_type> options = _options;


  sframe data = make_random_sframe(n_rows, column_types, create_target_column);

  options["integer_columns_categorical_by_default"] = true;

  if(!(options.count("target_column_always_categorical")
       && options.at("target_column_always_categorical"))) {

    options["target_column_always_numeric"] = true;
  }

  ml_data mdata(options);
  mdata.set_data(data, create_target_column ? "target" : "");
  mdata.fill();


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

      case 's':
      case 'S':
      case 'c':
      case 'C':
        DASSERT_TRUE(mdata.metadata()->column_mode(i) == ml_column_mode::CATEGORICAL);
        DASSERT_TRUE(mdata.metadata()->column_type(i) == flex_type_enum::STRING);
        break;

      case 'v':
      case 'V':
        DASSERT_TRUE(mdata.metadata()->column_mode(i) == ml_column_mode::NUMERIC_VECTOR);
        DASSERT_TRUE(mdata.metadata()->column_type(i) == flex_type_enum::VECTOR);
        break;

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

sframe_and_side_info make_ml_data_with_side_data(
    size_t n_main_rows,
    const std::string& main,
    const std::vector<std::pair<size_t, std::string> >& side,
    bool create_target_column,
    const std::map<std::string, flexible_type>& _options) {

  size_t n_main_columns = main.size();

  sframe_and_side_info ret;

  // Make the main side data.
  ret.main_sframe = make_random_sframe(n_main_rows, main, create_target_column);

  // Make the side data.

  ret.side_sframes.resize(main.size());

  size_t total_num_columns = n_main_columns;

  std::vector<std::unordered_map<flexible_type, std::vector<flexible_type> > > sidedata(n_main_columns);
  std::vector<size_t> side_data_widths(n_main_columns, 0);
  std::vector<std::vector<flex_type_enum> > side_data_types(n_main_columns);

  for(size_t i = 0; i < std::min(side.size(), ret.side_sframes.size()); ++i) {

    std::string run_str = std::string(&(main[i]), 1) + side[i].second;

    sframe sf = make_random_sframe(side[i].first, run_str, false);

    size_t nc = side_data_widths[i] = sf.num_columns() - 1;
    side_data_types[i].resize(nc);

    for(size_t k = 0; k < nc; ++k)
      side_data_types[i][k] = sf.column_types()[k + 1];

    total_num_columns += nc;

    std::vector<std::vector<flexible_type> > raw_data = testing_extract_sframe_data(sf);

    for(const auto& v : raw_data) {
      sidedata[i][v[0]] = std::move(v);
    }

    raw_data.clear();
    for(const auto& p : sidedata[i]) {
      raw_data.push_back(p.second);
    }

    std::vector<std::string> column_names(sf.num_columns());

    column_names[0] = ret.main_sframe.column_name(i);

    for(size_t j = 1; j < sf.num_columns(); ++j)
      column_names[j] = std::string("S-") + std::to_string(j);

    ret.side_sframes[i] = make_testing_sframe(column_names, sf.column_types(), raw_data);
  }

  // Do the join.
  ret.joined_data = testing_extract_sframe_data(ret.main_sframe);

  for(size_t i = 0; i < ret.joined_data.size(); ++i) {

    auto& row = ret.joined_data[i];

    row.resize(total_num_columns);
    size_t write_pos = n_main_columns;

    for(size_t j = 0; j < n_main_columns; ++j) {
      auto side_it = sidedata[j].find(row[j]);

      if(side_it == sidedata[j].end()) {
        // Go through and init the right types

        for(size_t k = 0; k < side_data_widths[j]; ++k, ++write_pos) {
          row[write_pos] = flexible_type(side_data_types[j][k]);
        }
      } else {

        ASSERT_EQ(side_data_widths[j], side_it->second.size() - 1);

        for(size_t k = 0; k < side_data_widths[j]; ++k, ++write_pos)
          row[write_pos] = side_it->second[k + 1];
      }
    }

    ASSERT_EQ(write_pos, total_num_columns);
  }

  std::map<std::string, flexible_type> options = _options;
  options["integer_columns_categorical_by_default"] = true;
  ret.data = v2::ml_data(options);

  if(create_target_column)
    ret.data.set_data(ret.main_sframe, "target");
  else
    ret.data.set_data(ret.main_sframe);

  for(const sframe& s : ret.side_sframes)
    if(s.num_columns() >= 2)
      ret.data.add_side_data(s);

  ret.data.fill();

  ASSERT_EQ(ret.data.num_columns(), total_num_columns);


  return ret;
}


}}


/** Printing out a row.
 *
 */
std::ostream& operator<<(std::ostream& os, const std::vector<turi::v2::ml_data_entry>& v) {
  os << "[ ";

  for(const auto& e : v)
    os << "(" << e.column_index << "," << e.index << "," << e.value << ") ";

  os << "]";

  return os;
}
