/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_SFRAME_TEST_UTIL_H_
#define TURI_UNITY_SFRAME_TEST_UTIL_H_

#include <boost/test/unit_test.hpp>
#include <unity/lib/gl_sarray.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <util/test_macros.hpp>

#include <string>

__attribute__((__unused__))
inline bool check_equality_gl_sframe(
  turi::gl_sframe sf_gl, turi::gl_sframe ref_gl, bool check_row_order=true) {

  size_t num_columns_sf = sf_gl.num_columns();
  size_t num_columns_ref = ref_gl.num_columns();

  TS_ASSERT_EQUALS(num_columns_sf, num_columns_ref);

  std::vector<std::string> column_names_sf = sf_gl.column_names();
  std::vector<std::string> column_names_ref = ref_gl.column_names();
  TS_ASSERT(column_names_sf == column_names_ref);

  if (!check_row_order) {
      sf_gl = sf_gl.sort(column_names_sf);
      ref_gl = ref_gl.sort(column_names_ref);
  }

  for (size_t column_index=0; column_index < num_columns_sf; column_index++) {
    // go through all columns and check for sarray equality one by one

    turi::gl_sarray column_sf = sf_gl.select_column(
      column_names_sf[column_index]);
    turi::gl_sarray column_ref = ref_gl.select_column(
      column_names_ref[column_index]);

    TS_ASSERT_EQUALS(column_sf.size(), column_ref.size());
    TS_ASSERT_EQUALS(column_sf.dtype(), column_ref.dtype());

    for (size_t i = 0; i < column_sf.size(); i++) {
      if (column_sf.dtype() == turi::flex_type_enum::FLOAT) {
        if ((std::isnan(column_sf[i].get<turi::flex_float>()))
          && (std::isnan(column_ref[i].get<turi::flex_float>()))) {
          continue;
        }
        if ((std::isinf(column_sf[i].get<turi::flex_float>()))
          && (std::isinf(column_ref[i].get<turi::flex_float>()))) {
          // check for both positive or both negative
          TS_ASSERT_EQUALS(column_sf[i] > 0, column_ref[i] > 0);
          TS_ASSERT_EQUALS(column_sf[i] < 0, column_ref[i] < 0);
        }
      }
      TS_ASSERT_EQUALS(column_sf[i], column_ref[i]);
    }
  }

  return true;
}

#endif /* TURI_UNITY_SFRAME_TEST_UTIL_H_ */
