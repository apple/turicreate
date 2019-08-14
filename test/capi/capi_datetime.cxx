/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE capi_datetime

#include <boost/test/unit_test.hpp>
#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>
#include <core/data/flexible_type/flexible_type.hpp>
#include <core/util/test_macros.hpp>

BOOST_AUTO_TEST_CASE(test_tc_datetime_create_empty) {
  tc_error* error = nullptr;
  tc_datetime* dt = tc_datetime_create_empty(&error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_DIFFERS(dt, nullptr);

  TS_ASSERT(dt->value == turi::flex_date_time());
}

BOOST_AUTO_TEST_CASE(test_tc_datetime_create_from_posix_timestamp) {
  tc_error* error = nullptr;

  static constexpr int64_t TIMESTAMP = 1234567;

  tc_datetime* dt = tc_datetime_create_from_posix_timestamp(TIMESTAMP, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_DIFFERS(dt, nullptr);

  TS_ASSERT(dt->value == turi::flex_date_time(TIMESTAMP));
}

BOOST_AUTO_TEST_CASE(test_tc_datetime_create_from_posix_highres_timestamp) {
  tc_error* error = nullptr;

  static constexpr double TIMESTAMP = 1234567.89;

  tc_datetime* dt =
      tc_datetime_create_from_posix_highres_timestamp(TIMESTAMP, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_DIFFERS(dt, nullptr);

  turi::flex_date_time expected;
  expected.set_microsecond_res_timestamp(TIMESTAMP);
  TS_ASSERT(dt->value == expected);
}

#include <iostream>
BOOST_AUTO_TEST_CASE(test_tc_datetime_create_from_string) {
  tc_error* error = nullptr;

  static constexpr int64_t TIMESTAMP = 1234567;
  const turi::flexible_type expected = turi::flex_date_time(TIMESTAMP);

  tc_datetime* dt = tc_datetime_create_from_string(
      expected.to<turi::flex_string>().c_str(), nullptr, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_DIFFERS(dt, nullptr);

  TS_ASSERT(dt->value == expected.get<turi::flex_date_time>());
}

BOOST_AUTO_TEST_CASE(test_tc_datetime_set_time_zone_offset) {
  tc_error* error = nullptr;
  tc_datetime* dt = tc_datetime_create_empty(&error);

  int64_t hour_offset = 3;
  int64_t quarter_hour_offsets = 0;
  tc_datetime_set_time_zone_offset(
      dt, hour_offset, quarter_hour_offsets, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_EQUALS(dt->value.time_zone_offset(),
                   hour_offset * 4 + quarter_hour_offsets);

  hour_offset = 0;
  quarter_hour_offsets = 10;
  tc_datetime_set_time_zone_offset(
      dt, hour_offset, quarter_hour_offsets, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_EQUALS(dt->value.time_zone_offset(),
                   hour_offset * 4 + quarter_hour_offsets);
}

BOOST_AUTO_TEST_CASE(test_tc_datetime_get_time_zone_offset_minutes) {
  tc_error* error = nullptr;
  tc_datetime* dt = tc_datetime_create_empty(&error);
  dt->value.set_time_zone_offset(5);
  int64_t minutes = tc_datetime_get_time_zone_offset_minutes(dt, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_EQUALS(minutes, 5 * 15);
}

BOOST_AUTO_TEST_CASE(test_tc_datetime_set_microsecond) {
  static constexpr uint64_t MICROS = 123456;

  tc_error* error = nullptr;
  tc_datetime* dt = tc_datetime_create_empty(&error);
  tc_datetime_set_microsecond(dt, MICROS, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_EQUALS(dt->value.microsecond(), MICROS);
}

BOOST_AUTO_TEST_CASE(test_tc_datetime_get_microsecond) {
  static constexpr uint64_t MICROS = 123456;

  tc_error* error = nullptr;
  tc_datetime* dt = tc_datetime_create_empty(&error);
  dt->value.set_microsecond(MICROS);
  uint64_t micros = tc_datetime_get_microsecond(dt, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_EQUALS(micros, MICROS);
}

BOOST_AUTO_TEST_CASE(test_tc_datetime_set_timestamp) {
  static constexpr int64_t TIMESTAMP = 1234567;

  tc_error* error = nullptr;
  tc_datetime* dt = tc_datetime_create_empty(&error);
  tc_datetime_set_timestamp(dt, TIMESTAMP, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_EQUALS(dt->value.posix_timestamp(), TIMESTAMP);
}

BOOST_AUTO_TEST_CASE(test_tc_datetime_get_timestamp) {
  static constexpr int64_t TIMESTAMP = 1234567;

  tc_error* error = nullptr;
  tc_datetime* dt = tc_datetime_create_empty(&error);
  dt->value.set_posix_timestamp(TIMESTAMP);
  int64_t ts = tc_datetime_get_timestamp(dt, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_EQUALS(ts, TIMESTAMP);
}

BOOST_AUTO_TEST_CASE(test_tc_datetime_set_highres_timestamp) {
  static constexpr double TIMESTAMP = 1234567.89;

  tc_error* error = nullptr;
  tc_datetime* dt = tc_datetime_create_empty(&error);
  tc_datetime_set_highres_timestamp(dt, TIMESTAMP, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_DELTA(dt->value.microsecond_res_timestamp(), TIMESTAMP, 0.000002);
}


BOOST_AUTO_TEST_CASE(test_tc_datetime_get_highres_timestamp) {
  static constexpr double TIMESTAMP = 1234567.89;

  tc_error* error = nullptr;
  tc_datetime* dt = tc_datetime_create_empty(&error);
  dt->value.set_microsecond_res_timestamp(TIMESTAMP);
  double ts = tc_datetime_get_highres_timestamp(dt, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_DELTA(ts, TIMESTAMP, 0.000002);
}

BOOST_AUTO_TEST_CASE(test_tc_datetime_less_than) {
  tc_error* error = nullptr;

  tc_datetime* dt1 = tc_datetime_create_from_posix_timestamp(1234567, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_DIFFERS(dt1, nullptr);

  tc_datetime* dt2 = tc_datetime_create_from_posix_timestamp(1234576, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_DIFFERS(dt2, nullptr);

  TS_ASSERT(tc_datetime_less_than(dt1, dt2, &error));
  TS_ASSERT_EQUALS(error, nullptr);

  TS_ASSERT(!tc_datetime_less_than(dt2, dt1, &error));
  TS_ASSERT_EQUALS(error, nullptr);

  TS_ASSERT(!tc_datetime_less_than(dt1, dt1, &error));
  TS_ASSERT_EQUALS(error, nullptr);
}

BOOST_AUTO_TEST_CASE(test_tc_datetime_equal) {
  tc_error* error = nullptr;

  tc_datetime* dt1 = tc_datetime_create_from_posix_timestamp(1234567, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_DIFFERS(dt1, nullptr);

  tc_datetime* dt2 = tc_datetime_create_from_posix_timestamp(1234576, &error);
  TS_ASSERT_EQUALS(error, nullptr);
  TS_ASSERT_DIFFERS(dt2, nullptr);

  TS_ASSERT(tc_datetime_equal(dt1, dt1, &error));
  TS_ASSERT_EQUALS(error, nullptr);

  TS_ASSERT(!tc_datetime_equal(dt1, dt2, &error));
  TS_ASSERT_EQUALS(error, nullptr);
}

