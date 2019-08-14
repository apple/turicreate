/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef CAPI_TEST_UTILS
#define CAPI_TEST_UTILS

#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <core/data/sframe/gl_sframe.hpp>
#include <core/data/sframe/gl_sarray.hpp>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <cmath>
#include <vector>
#include <iostream>

#define CAPI_CHECK_ERROR(error)                                     \
  do {                                                              \
    if (error != nullptr) {                                         \
      std::ostringstream ss;                                        \
      ss << "C-API ERROR: " << __FILE__ << ": " << __LINE__ << ": " \
         << error->value;                                           \
      std::cout << ss.str() << std::endl;                           \
      throw std::runtime_error(ss.str());                           \
    }                                                               \
  } while (false)


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

__attribute__((__unused__))
static tc_flex_list* make_flex_list_double(const std::vector<double>& v) {

  tc_error* error = NULL;

  tc_flex_list* fl = tc_flex_list_create(&error);

  CAPI_CHECK_ERROR(error);

  size_t pos = 0;

  for(double t : v) {
    tc_flexible_type* ft = tc_ft_create_from_double(t, &error);

    CAPI_CHECK_ERROR(error);

    size_t idx = tc_flex_list_add_element(fl, ft, &error);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT_EQUALS(idx, pos);
    ++pos;

    size_t actual_size = tc_flex_list_size(fl);

    TS_ASSERT_EQUALS(pos, actual_size);

    tc_release(ft);
  }

  // Go through and make sure things are equal
  for(size_t i = 0; i < v.size(); ++i) {
    tc_flexible_type* ft = tc_flex_list_extract_element(fl, i, &error);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT(tc_ft_is_double(ft) != 0);

    double val = tc_ft_double(ft, &error);
    CAPI_CHECK_ERROR(error);

    TS_ASSERT(v[i] == val);

    tc_release(ft);
  }

  return fl;
}

__attribute__((__unused__))
static tc_flex_list* make_flex_list_int(const std::vector<int64_t>& v) {

  tc_error* error = NULL;

  tc_flex_list* fl = tc_flex_list_create(&error);

  CAPI_CHECK_ERROR(error);

  size_t pos = 0;

  for(int64_t t : v) {
    tc_flexible_type* ft = tc_ft_create_from_int64(t, &error);

    CAPI_CHECK_ERROR(error);

    size_t idx = tc_flex_list_add_element(fl, ft, &error);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT_EQUALS(idx, pos);
    ++pos;

    size_t actual_size = tc_flex_list_size(fl);

    TS_ASSERT_EQUALS(pos, actual_size);

    tc_release(ft);
  }

  // Go through and make sure things are equal
  for(size_t i = 0; i < v.size(); ++i) {
    tc_flexible_type* ft = tc_flex_list_extract_element(fl, i, &error);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT(tc_ft_is_int64(ft) != 0);

    int64_t val = tc_ft_int64(ft, &error);
    CAPI_CHECK_ERROR(error);

    TS_ASSERT(v[i] == val);

    tc_release(ft);
  }

  return fl;
}

__attribute__((__unused__))
static tc_flex_list* make_flex_list_string(const std::vector<std::string>& v) {

  tc_error* error = NULL;

  tc_flex_list* fl = tc_flex_list_create(&error);

  CAPI_CHECK_ERROR(error);

  size_t pos = 0;

  for(std::string t : v) {
    tc_flexible_type* ft = tc_ft_create_from_cstring(t.c_str(), &error);

    CAPI_CHECK_ERROR(error);

    size_t idx = tc_flex_list_add_element(fl, ft, &error);

    CAPI_CHECK_ERROR(error);

    TS_ASSERT_EQUALS(idx, pos);
    ++pos;

    size_t actual_size = tc_flex_list_size(fl);

    TS_ASSERT_EQUALS(pos, actual_size);

    tc_release(ft);
  }

  return fl;
}


__attribute__((__unused__))
static tc_sarray* make_sarray_double(const std::vector<double>& v) {

  tc_error* error = NULL;

  tc_flex_list* fl = make_flex_list_double(v);

  tc_sarray* sa = tc_sarray_create_from_list(fl, &error);

  CAPI_CHECK_ERROR(error);

  {
    // Make sure it gets out what we want it to.
    for(size_t i = 0; i < v.size(); ++i) {
      tc_flexible_type* ft = tc_sarray_extract_element(sa, i, &error);
      CAPI_CHECK_ERROR(error);

      TS_ASSERT(tc_ft_is_double(ft) != 0);

      double val = tc_ft_double(ft, &error);
      CAPI_CHECK_ERROR(error);

      TS_ASSERT(v[i] == val);

      tc_release(ft);
    }
  }

  return sa;
}

__attribute__((__unused__))
static tc_sarray* make_sarray_integer(const std::vector<int64_t>& v) {

  tc_error* error = NULL;

  tc_flex_list* fl = make_flex_list_int(v);

  tc_sarray* sa = tc_sarray_create_from_list(fl, &error);

  CAPI_CHECK_ERROR(error);

  {
    // Make sure it gets out what we want it to.
    for(size_t i = 0; i < v.size(); ++i) {
      tc_flexible_type* ft = tc_sarray_extract_element(sa, i, &error);
      CAPI_CHECK_ERROR(error);

      TS_ASSERT(tc_ft_is_int64(ft) != 0);

      int64_t val = tc_ft_int64(ft, &error);
      CAPI_CHECK_ERROR(error);

      TS_ASSERT(v[i] == val);

      tc_release(ft);
    }
  }

  return sa;
}

__attribute__((__unused__))
static tc_sarray* make_sarray_string(const std::vector<std::string>& v) {

  tc_error* error = NULL;

  tc_flex_list* fl = make_flex_list_string(v);

  tc_sarray* sa = tc_sarray_create_from_list(fl, &error);

  CAPI_CHECK_ERROR(error);

  {
    // Make sure it gets out what we want it to.
    for(size_t i = 0; i < v.size(); ++i) {
      tc_flexible_type* ft = tc_sarray_extract_element(sa, i, &error);
      CAPI_CHECK_ERROR(error);

      TS_ASSERT(tc_ft_is_string(ft) != 0);

      size_t len = tc_ft_string_length(ft, &error);
      CAPI_CHECK_ERROR(error);
      const char * data = tc_ft_string_data(ft, &error);
      CAPI_CHECK_ERROR(error);

      std::string val(data, len);
      TS_ASSERT(v[i] == val);

      tc_release(ft);
    }
  }

  return sa;
}

__attribute__((__unused__))
static tc_sframe* make_sframe_double(const std::vector<std::pair<std::string, std::vector<double> > >& data ) {

    tc_error* error = NULL;

    tc_sframe* sf = tc_sframe_create_empty(&error);

    CAPI_CHECK_ERROR(error);

    for(auto p : data) {

      tc_sarray* sa = make_sarray_double(p.second);

      tc_sframe_add_column(sf, p.first.c_str(), sa, &error);

      CAPI_CHECK_ERROR(error);

      tc_release(sa);
    }

    // Check everything
    for(auto p : data) {
      // Make sure it gets out what we want it to.
      tc_sarray* sa = tc_sframe_extract_column_by_name(sf, p.first.c_str(), &error);

      CAPI_CHECK_ERROR(error);

      tc_sarray* ref_sa = make_sarray_double(p.second);

      TS_ASSERT(tc_sarray_equals(sa, ref_sa, &error));

      CAPI_CHECK_ERROR(error);

      tc_release(sa);
    }

    return sf;
  }

__attribute__((__unused__))
static tc_sframe* make_sframe_integer(const std::vector<std::pair<std::string, std::vector<int64_t> > >& data ) {

  tc_error* error = NULL;

  tc_sframe* sf = tc_sframe_create_empty(&error);

  CAPI_CHECK_ERROR(error);

  for(auto p : data) {

    tc_sarray* sa = make_sarray_integer(p.second);

    tc_sframe_add_column(sf, p.first.c_str(), sa, &error);

    CAPI_CHECK_ERROR(error);

    tc_release(sa);
  }

  // Check everything
  for(auto p : data) {
    // Make sure it gets out what we want it to.
    tc_sarray* sa = tc_sframe_extract_column_by_name(sf, p.first.c_str(), &error);

    CAPI_CHECK_ERROR(error);

    tc_sarray* ref_sa = make_sarray_integer(p.second);

    TS_ASSERT(tc_sarray_equals(sa, ref_sa, &error));

    CAPI_CHECK_ERROR(error);

    tc_release(sa);
  }

  return sf;
}

__attribute__((__unused__))
static bool check_equality_gl_sframe(
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

__attribute__((__unused__))
static bool check_equality_tc_sframe(
  tc_sframe *sf, tc_sframe *ref, bool check_row_order=true) {

  return check_equality_gl_sframe(sf->value, ref->value, check_row_order);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif
