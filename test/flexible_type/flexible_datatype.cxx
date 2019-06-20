/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <vector>
#include <iostream>
#include <typeinfo>       // operator typeid


#include <core/data/flexible_type/flexible_type.hpp>
#include <core/data/flexible_type/flexible_type_converter.hpp>

using namespace turi;


struct legacy_flex_date_time {
  legacy_flex_date_time() = default;
  /**
   * Constructs a flex_date_time object from a posix timestamp, and and time zone offset.
   * \param _posix_timestamp Timestamp value since 1st Jan 1970
   * \param _half_hour_offset Additional offset for timezone. In integral increments
   *                          of half-hour.
   */
  explicit inline legacy_flex_date_time(int64_t _posix_timestamp, int32_t _half_hour_offset = 0) { 
    first = _posix_timestamp;
    second = _half_hour_offset;
  }

  int64_t first : 56;
  int64_t second : 8;

  /**
   * Returns the timestamp value.
   */
  inline int64_t posix_timestamp() const {
    return first;
  }

  /**
   * Returns the time zone value in integral increments of half-hour.
   */
  inline int32_t time_zone_offset() const {
    return second;
  }

  void save(oarchive& oarc) const {
    oarc << *reinterpret_cast<const int64_t*>(this);
  }

  void load(iarchive& iarc) {
    union {
      legacy_flex_date_time dt;
      int64_t val;
    } date_time_serialized;
    iarc >> date_time_serialized.val;
    (*this) = date_time_serialized.dt;
  }
};


struct flexible_datatype_test  {

  public:
   void test_size() {
     TS_ASSERT_EQUALS(sizeof(flexible_type), 16);
     TS_ASSERT_EQUALS(sizeof(flex_date_time), 12);
   }

   void test_containers() {
     std::vector<flexible_type> f;
     f.emplace_back(123);
     f.emplace_back("hello world");
     std::map<flexible_type, std::vector<flexible_type> > m;
     m["123"].push_back(123);

     flexible_type e("234");
     m[e].push_back(e);

   }
   void test_constructors() {
     flexible_type f = flex_string("hello world");
     flexible_type g = f;
     TS_ASSERT_EQUALS(g.get_type(), flex_type_enum::STRING);
     g = std::move(f);
     TS_ASSERT_EQUALS(g.get_type(), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(g.get<flex_string>(), "hello world");
     TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::INTEGER);

     f = g;
     TS_ASSERT_EQUALS(g.get_type(), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(g.get<flex_string>(), "hello world");
     TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(f.get<flex_string>(), "hello world");

     flexible_type h = std::move(g);
     TS_ASSERT_EQUALS(h.get_type(), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(h.get<flex_string>(), "hello world");
     TS_ASSERT_EQUALS(g.get_type(), flex_type_enum::INTEGER);
    
     std::swap(h, g);
     TS_ASSERT_EQUALS(g.get_type(), flex_type_enum::STRING);
     TS_ASSERT_EQUALS(g.get<flex_string>(), "hello world");
     TS_ASSERT_EQUALS(h.get_type(), flex_type_enum::INTEGER);
   } 
  void test_types_long() {
    flexible_type f = 1;
    flexible_type f2 = 2;

    TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::INTEGER);

    TS_ASSERT_EQUALS(f, f);
    TS_ASSERT_EQUALS(f, 1);

    TS_ASSERT_DIFFERS(f, f2);
    TS_ASSERT_DIFFERS(f2, 1);

    long x = f;
    TS_ASSERT_EQUALS(x, 1);

    double xd = f;
    TS_ASSERT_EQUALS(xd, 1);
  }

  void test_types_double() {
    flexible_type f = 1.0;
    flexible_type f2 = 2.0;

    TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::FLOAT);

    TS_ASSERT_EQUALS(f, f);
    TS_ASSERT_EQUALS(f, 1.0);
    TS_ASSERT_DIFFERS(f, f2);
    TS_ASSERT_DIFFERS(f2, 1.0);

    double x = f;

    TS_ASSERT_EQUALS(x, 1.0);
  }

  void test_types_string() {
    flexible_type f = "Hey man!";
    flexible_type f2 = "Hay man!";

    TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::STRING);

    TS_ASSERT_EQUALS(f, f);
    TS_ASSERT_EQUALS(f, "Hey man!");
    TS_ASSERT_DIFFERS(f, f2);
    TS_ASSERT_DIFFERS(f2, "Hey man!");

    std::string s = f;

    TS_ASSERT_EQUALS(s, "Hey man!");
  }

  void test_types_vector() {
    std::vector<double> v = {1.0, 2.0};
    std::vector<double> v2 = {2.0, 1.0};

    flexible_type f = v;
    flexible_type f2 = v2;

    TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::VECTOR);

    TS_ASSERT_EQUALS(f, f);
    TS_ASSERT_EQUALS(f[0], 1.0);
    TS_ASSERT_EQUALS(f[1], 2.0);
    TS_ASSERT_DIFFERS(f, f2);

    std::vector<double> v3 = f;
    TS_ASSERT(v == v3);
  }
  void test_types_recursive() {
    std::vector<flexible_type> v = {flexible_type(1.0), flexible_type("hey")};
    std::vector<flexible_type> v2 = {flexible_type("hey"), flexible_type(1.0)};

    flexible_type f = v;
    flexible_type f2 = v2;

    TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::LIST);

    if (f != f) {
      std::cout << f << std::endl;
    }
    TS_ASSERT_EQUALS(f, f);
    TS_ASSERT(f(0) == 1.0);
    TS_ASSERT(f(1) == std::string("hey") );
    TS_ASSERT_DIFFERS(f, f2);

    std::vector<flexible_type> v3 = f;

    TS_ASSERT(v == v3);

    v = {flexible_type(1.0), flexible_type("hey")};
    v2 = {flexible_type(2.0), flexible_type("hoo")};
    f = v;
    f2 = v2;
    TS_ASSERT(f < f2);
    TS_ASSERT(!(f2 < f));

    v = {flexible_type(1.0), flexible_type("hey")};
    v2 = {flexible_type(1.0), flexible_type("hey")};
    f = v;
    f2 = v2;
    TS_ASSERT(f == f2);
    TS_ASSERT(!(f2 < f));
    TS_ASSERT(!(f2 > f));

    v = {flexible_type(1.0), flexible_type("hey")};
    v2 = {flexible_type(1.0), flexible_type("hoo")};
    f = v;
    f2 = v2;
    TS_ASSERT(f != f2);
    TS_ASSERT(f < f2);
    TS_ASSERT(!(f > f2));

    v = {flexible_type(1.0), flexible_type("hey")};
    v2 = {flexible_type(1.0)};
    f = v;
    f2 = v2;
    TS_ASSERT(f != f2);
    TS_ASSERT(f > f2);
    TS_ASSERT(!(f < f2));

    v = {flexible_type(1.0)};
    v2 = {flexible_type(1.0), flexible_type("hey")};
    f = v;
    f2 = v2;
    TS_ASSERT(f != f2);
    TS_ASSERT(f < f2);
    TS_ASSERT(!(f > f2));

    v = {flexible_type(1.0)};
    v2 = {flexible_type(1.0)};
    f = v;
    f2 = v2;
    TS_ASSERT(f == f2);
    TS_ASSERT(!(f < f2));
    TS_ASSERT(!(f > f2));
  }

  void test_types_dict() {
    flexible_type vector_v = flex_vec{1,2,3};

    std::vector<std::pair<flexible_type, flexible_type>> m{
      std::make_pair(flexible_type("foo"), flexible_type(1.0)),
      std::make_pair(flexible_type(123), flexible_type("string")),
      std::make_pair(vector_v, vector_v)
    };

    // same as m but different order
    std::vector<std::pair<flexible_type, flexible_type>> m2{
      std::make_pair(vector_v, vector_v),
      std::make_pair(flexible_type(123), flexible_type("string")),
      std::make_pair(flexible_type("foo"), flexible_type(1.0))
    };

    // different length
    std::vector<std::pair<flexible_type, flexible_type>> m3{
      std::make_pair(flexible_type("foo"), flexible_type(1.0)),
    };

    // same length but different key
    std::vector<std::pair<flexible_type, flexible_type>> m4{
      std::make_pair(flexible_type("fooo"), flexible_type(2.0)),
      std::make_pair(flexible_type(1234), flexible_type("string2")),
      std::make_pair(vector_v, vector_v)
    };

    // same key but different value
    std::vector<std::pair<flexible_type, flexible_type>> m5{
      std::make_pair(flexible_type("foo"), flexible_type(2.0)),
      std::make_pair(flexible_type(123), flexible_type("string2")),
      std::make_pair(vector_v, flexible_type(1))
    };


    flexible_type f = m;
    flexible_type f2 = m2;
    flexible_type f3 = m3;
    flexible_type f4 = m4;
    flexible_type f5 = m5;

    TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::DICT);

    TS_ASSERT_EQUALS(f, f);
    TS_ASSERT_EQUALS(f2, f2);
    TS_ASSERT_EQUALS(f3, f3);
    TS_ASSERT_EQUALS(f4, f4);
    TS_ASSERT_EQUALS(f5, f5);

    TS_ASSERT_EQUALS(f, f2);

    TS_ASSERT_DIFFERS(f, f3);
    TS_ASSERT_DIFFERS(f, f4);
    TS_ASSERT_DIFFERS(f, f5);

    std::vector<std::pair<flexible_type, flexible_type>> new_f = f.get<flex_dict>();

    TS_ASSERT(new_f == m);

    flexible_type v1 = f.dict_at("foo");
    flexible_type v2 = f.dict_at(123);
    flexible_type v3 = f.dict_at(vector_v);
    TS_ASSERT_THROWS_ANYTHING(f.dict_at("non exist key"));

    TS_ASSERT_EQUALS(v1, 1.0);
    TS_ASSERT_EQUALS(v2, "string");
    TS_ASSERT_EQUALS(v3, vector_v);

    TS_ASSERT_EQUALS(v1.get_type(), flex_type_enum::FLOAT);
    TS_ASSERT_EQUALS(v2.get_type(), flex_type_enum::STRING);
    TS_ASSERT_EQUALS(v3.get_type(), flex_type_enum::VECTOR);

    // erase
    f.erase("foo");
    TS_ASSERT_EQUALS(f.dict_at(123), "string");
    TS_ASSERT_THROWS_ANYTHING(f.dict_at("foo"));
    TS_ASSERT_THROWS_ANYTHING(f.dict_at("123"));

  }


  void test_date_time() {
    flex_date_time dt;
    // basic timestamp storage
    dt.set_posix_timestamp(441964800);
    TS_ASSERT_EQUALS(dt.posix_timestamp(), 441964800);
    TS_ASSERT_EQUALS(dt.microsecond(), 0);
    TS_ASSERT_EQUALS(dt.microsecond_res_timestamp(), 441964800.0);

    dt.set_posix_timestamp(0);
    TS_ASSERT_EQUALS(dt.posix_timestamp(), 0);
    TS_ASSERT_EQUALS(dt.shifted_posix_timestamp(), 0);
    dt.set_posix_timestamp(-1000);
    TS_ASSERT_EQUALS(dt.posix_timestamp(), -1000);
    TS_ASSERT_EQUALS(dt.shifted_posix_timestamp(), -1000);
    dt.set_posix_timestamp(1LL << 54);
    TS_ASSERT_EQUALS(dt.posix_timestamp(), 1LL << 54);
    TS_ASSERT_EQUALS(dt.shifted_posix_timestamp(), 1LL << 54);
    dt.set_posix_timestamp(-(1LL << 54));
    TS_ASSERT_EQUALS(dt.posix_timestamp(), -(1LL << 54));
    TS_ASSERT_EQUALS(dt.shifted_posix_timestamp(), -(1LL << 54));

    // 15 min timezones
    dt.set_posix_timestamp(441964800);
    dt.set_time_zone_offset(0);
    TS_ASSERT_EQUALS(dt.posix_timestamp(), 441964800);
    TS_ASSERT_EQUALS(dt.shifted_posix_timestamp(), 441964800);

    dt.set_time_zone_offset(1);
    TS_ASSERT_EQUALS(dt.posix_timestamp(), 441964800);
    TS_ASSERT_EQUALS(dt.shifted_posix_timestamp(), 
                     441964800 + flex_date_time::TIMEZONE_RESOLUTION_IN_SECONDS);

    dt.set_time_zone_offset(-1);
    TS_ASSERT_EQUALS(dt.posix_timestamp(), 441964800);
    TS_ASSERT_EQUALS(dt.shifted_posix_timestamp(), 
                     441964800 - flex_date_time::TIMEZONE_RESOLUTION_IN_SECONDS);

    dt.set_time_zone_offset(48);
    TS_ASSERT_EQUALS(dt.posix_timestamp(), 441964800);
    TS_ASSERT_EQUALS(dt.shifted_posix_timestamp(), 
                     441964800 + 48 * flex_date_time::TIMEZONE_RESOLUTION_IN_SECONDS);

    dt.set_time_zone_offset(-48);
    TS_ASSERT_EQUALS(dt.posix_timestamp(), 441964800);
    TS_ASSERT_EQUALS(dt.shifted_posix_timestamp(), 
                     441964800 - 48 * flex_date_time::TIMEZONE_RESOLUTION_IN_SECONDS);

    dt.set_time_zone_offset(flex_date_time::EMPTY_TIMEZONE);
    TS_ASSERT_EQUALS(dt.posix_timestamp(), 441964800);
    TS_ASSERT_EQUALS(dt.shifted_posix_timestamp(), 441964800);

    // out of limit time zones
    // 12 hours + 1 tick
    size_t out_of_limit = 12 * 60 / flex_date_time::TIMEZONE_RESOLUTION_IN_MINUTES + 1;
    TS_ASSERT_THROWS_ANYTHING(dt.set_time_zone_offset(out_of_limit));
    TS_ASSERT_THROWS_ANYTHING(dt.set_time_zone_offset(-out_of_limit));

    // check microsecond values
    dt.set_posix_timestamp(441964800);
    dt.set_time_zone_offset(flex_date_time::EMPTY_TIMEZONE);
    dt.set_microsecond(500000);
    TS_ASSERT_DELTA(dt.microsecond_res_timestamp(), (double)(441964800.5),
                    flex_date_time::MICROSECOND_EPSILON);
    // out of limit microsecond values
    TS_ASSERT_THROWS_ANYTHING(dt.set_microsecond(-1));
    TS_ASSERT_THROWS_ANYTHING(dt.set_microsecond(1000001));

    TS_ASSERT(flex_date_time(441964800) < flex_date_time(441964801));
    TS_ASSERT(flex_date_time(441964801) > flex_date_time(441964800));
    TS_ASSERT(flex_date_time(441964800) == flex_date_time(441964800));
    TS_ASSERT(flex_date_time(441964800) < flex_date_time(441964800, 0, 1));
    TS_ASSERT(flex_date_time(441964800, 0, 1) > flex_date_time(441964800));
    TS_ASSERT(flex_date_time(441964800, 0, 1) == flex_date_time(441964800, 0, 1));
    TS_ASSERT(flex_date_time(441964800, 0, 1) == flex_date_time(441964800, 10, 1));

    dt.set_microsecond_res_timestamp(441964800.5);
    TS_ASSERT_DELTA(dt.microsecond_res_timestamp(), 441964800.5,
                    flex_date_time::MICROSECOND_EPSILON);
    dt.set_microsecond_res_timestamp(-441964800.5);
    TS_ASSERT_DELTA(dt.microsecond_res_timestamp(), -441964800.5,
                    flex_date_time::MICROSECOND_EPSILON);
    TS_ASSERT_EQUALS(dt.microsecond(), 500000);

    // make sure that limit values are stored correctly
    // in a flexible_type
    dt = flex_date_time(1LL << 54, flex_date_time::EMPTY_TIMEZONE, 999999);
    flexible_type f = dt;
    TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::DATETIME);
    TS_ASSERT_EQUALS(f.get_date_time_microsecond(), 999999);
    TS_ASSERT_EQUALS(f.get_date_time_as_timestamp_and_offset().first, 1LL << 54);
    TS_ASSERT_EQUALS(f.get_date_time_as_timestamp_and_offset().second, flex_date_time::EMPTY_TIMEZONE);
    TS_ASSERT(f.get<flex_date_time>()  == dt);
    TS_ASSERT(f.get<flex_date_time>() == dt);

    f = 0; // reset to interget
    f.set_date_time_from_timestamp_and_offset(
        {1LL << 54, flex_date_time::EMPTY_TIMEZONE}, 999999);
    TS_ASSERT_EQUALS(f.get_date_time_microsecond(), 999999);
    TS_ASSERT_EQUALS(f.get_date_time_as_timestamp_and_offset().first, 1LL << 54);
    TS_ASSERT_EQUALS(f.get_date_time_as_timestamp_and_offset().second, 
                     flex_date_time::EMPTY_TIMEZONE);
    TS_ASSERT(f.get<flex_date_time>()  == dt);
    TS_ASSERT(f.get<flex_date_time>()  == dt);
    TS_ASSERT_EQUALS(flex_date_time::TIMEZONE_RESOLUTION_IN_SECONDS / 60,
                     flex_date_time::TIMEZONE_RESOLUTION_IN_MINUTES);
    TS_ASSERT_EQUALS(double(flex_date_time::TIMEZONE_RESOLUTION_IN_SECONDS) / 60 / 60,
                     flex_date_time::TIMEZONE_RESOLUTION_IN_HOURS);

    // test addition
    // reset the state
    dt.set_posix_timestamp(441964800);
    dt.set_microsecond(0);

    f = dt;
    double expected_val;
    flexible_type x = 1.04566;
    for (size_t i = 0;i < 100000; ++i)  f += x;
    expected_val = dt.posix_timestamp() + 100000 * 1.04566;
    TS_ASSERT_DELTA(f.get<flex_date_time>().microsecond_res_timestamp(),
                    expected_val, 1E-5);



    // test addition with negative timestamp
    dt.set_posix_timestamp(-441964800);
    dt.set_microsecond(0);

    f = dt;
    x = 1.04566;
    for (size_t i = 0;i < 100000; ++i) f += x;
    expected_val = dt.posix_timestamp() + 100000 * 1.04566;
    TS_ASSERT_DELTA(f.get<flex_date_time>().microsecond_res_timestamp(),
                    expected_val, 1E-5);


    // test subtraction
    dt.set_posix_timestamp(441964800);
    dt.set_microsecond(0);
    f = dt;
    x = 1.04566;
    for (size_t i = 0;i < 100000; ++i)  f -= x;
    expected_val = dt.posix_timestamp() - 100000 * 1.04566;
    TS_ASSERT_DELTA(f.get<flex_date_time>().microsecond_res_timestamp(),
                    expected_val, 1E-5);



    // test subtraction with negative timestamp
    dt.set_posix_timestamp(-441964800);
    dt.set_microsecond(0);

    f = dt;
    x = 1.04566;
    for (size_t i = 0;i < 100000; ++i) f -= x;
    expected_val = dt.posix_timestamp() - 100000 * 1.04566;
    TS_ASSERT_DELTA(f.get<flex_date_time>().microsecond_res_timestamp(),
                    expected_val, 1E-5);

  }

  void test_date_time_serialization() {
    oarchive oarc;
    oarc << flex_date_time(1LL << 54, flex_date_time::EMPTY_TIMEZONE, 999999);
    oarc << flex_date_time(1LL << 54, 0, 999999);
    oarc << flex_date_time(1LL << 54, 1, 999999);
    oarc << flex_date_time(1LL << 54, -1, 999999);
    oarc << flex_date_time(1LL << 54, flex_date_time::TIMEZONE_LOW, 999999);
    oarc << flex_date_time(1LL << 54, flex_date_time::TIMEZONE_HIGH, 999999);
    oarc << legacy_flex_date_time(1LL << 54, 0);
    oarc << legacy_flex_date_time(1LL << 54, 24);
    oarc << legacy_flex_date_time(1LL << 54, -24);
    oarc << legacy_flex_date_time(1LL << 54, 1);
    oarc << legacy_flex_date_time(1LL << 54, -1);
    oarc << legacy_flex_date_time(1LL << 54, 12);
    oarc << legacy_flex_date_time(1LL << 54, -12);

    iarchive iarc(oarc.buf, oarc.off);
    flex_date_time dt;

    iarc >> dt;
    TS_ASSERT(
        dt.identical(flex_date_time(1LL << 54, flex_date_time::EMPTY_TIMEZONE, 999999)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, 0, 999999)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, 1, 999999)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, -1, 999999)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, flex_date_time::TIMEZONE_LOW, 999999)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, flex_date_time::TIMEZONE_HIGH, 999999)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, 0, 0)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, 48, 0)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, -48, 0)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, 2, 0)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, -2, 0)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, 24, 0)));
    iarc >> dt;
    TS_ASSERT(dt.identical(flex_date_time(1LL << 54, -24, 0)));
  }

  void test_types_enum() {

    // For use in variant_type 
    
    enum class TestEnum {A, B, C};

    flexible_type_converter<TestEnum> converter; 
    
    flexible_type f = converter.set(TestEnum::A);
    flexible_type f2 = converter.set(TestEnum::A);
    flexible_type f3 = converter.set(TestEnum::B);

    TS_ASSERT_EQUALS(f.get_type(), flex_type_enum::INTEGER);

    TS_ASSERT(f == f2);
    TS_ASSERT(f != f3);

    TestEnum x = converter.get(f);
    TestEnum x2 = converter.get(f2);
    TestEnum x3 = converter.get(f3);

    TS_ASSERT_EQUALS(int(x), int(TestEnum::A));
    TS_ASSERT_EQUALS(int(x2), int(TestEnum::A));
    TS_ASSERT_EQUALS(int(x3), int(TestEnum::B));
  }

}; // class


BOOST_FIXTURE_TEST_SUITE(_flexible_datatype_test, flexible_datatype_test)
BOOST_AUTO_TEST_CASE(test_size) {
  flexible_datatype_test::test_size();
}
BOOST_AUTO_TEST_CASE(test_containers) {
  flexible_datatype_test::test_containers();
}
BOOST_AUTO_TEST_CASE(test_constructors) {
  flexible_datatype_test::test_constructors();
}
BOOST_AUTO_TEST_CASE(test_types_long) {
  flexible_datatype_test::test_types_long();
}
BOOST_AUTO_TEST_CASE(test_types_double) {
  flexible_datatype_test::test_types_double();
}
BOOST_AUTO_TEST_CASE(test_types_string) {
  flexible_datatype_test::test_types_string();
}
BOOST_AUTO_TEST_CASE(test_types_vector) {
  flexible_datatype_test::test_types_vector();
}
BOOST_AUTO_TEST_CASE(test_types_recursive) {
  flexible_datatype_test::test_types_recursive();
}
BOOST_AUTO_TEST_CASE(test_types_dict) {
  flexible_datatype_test::test_types_dict();
}
BOOST_AUTO_TEST_CASE(test_date_time) {
  flexible_datatype_test::test_date_time();
}
BOOST_AUTO_TEST_CASE(test_date_time_serialization) {
  flexible_datatype_test::test_date_time_serialization();
}
BOOST_AUTO_TEST_CASE(test_types_enum) {
  flexible_datatype_test::test_types_enum();
}
BOOST_AUTO_TEST_SUITE_END()
