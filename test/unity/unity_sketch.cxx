/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <iostream>
#include <cmath>

#include <unity/lib/unity_sarray.hpp>
#include <unity/lib/unity_sketch.hpp>

using namespace turi;

struct unity_sketch_test {
 public:

  void test_numeric_sketch() {
    std::shared_ptr<unity_sarray_base> dbl(new unity_sarray);
    std::vector<flexible_type> vec;
    double sum = 0;

    // I know the mean is 1.0
    double var = 0;
    for (size_t i = 0;i < 10000; ++i) {
      for (size_t j = 0; j < 3; ++j) {
        vec.emplace_back((double)j);
        sum += j;
        var += ((double)j - 1.0) * ((double)j - 1.0);
      }
      vec.emplace_back(flex_type_enum::UNDEFINED);
    }
    std::static_pointer_cast<unity_sarray>(dbl)->construct_from_vector(vec, flex_type_enum::FLOAT);

    std::shared_ptr<unity_sketch_base> sketch(new unity_sketch);
    std::static_pointer_cast<unity_sketch>(sketch)->construct_from_sarray(dbl);
    var = var / 30000;
    // we get all the basic statistics correct
    TS_ASSERT_EQUALS(sketch->sum(), sum);
    TS_ASSERT_DELTA(sketch->mean(), sum / 30000.0, 1E-7);
    TS_ASSERT_DELTA(sketch->var(), var, 1E-7);
    TS_ASSERT_EQUALS(sketch->num_undefined(), 10000);
    TS_ASSERT_EQUALS(sketch->size(), 40000);
    TS_ASSERT_EQUALS(sketch->min(), 0.0);
    TS_ASSERT_EQUALS(sketch->max(), 2.0);
    

    // approximate count is approximate
    TS_ASSERT_DELTA(sketch->frequency_count((flexible_type(0.0))), 10000, 1000.0);
    TS_ASSERT_DELTA(sketch->frequency_count((flexible_type(1.0))), 10000, 1000.0);
    TS_ASSERT_DELTA(sketch->frequency_count((flexible_type(2.0))), 10000, 1000.0);
    // For the few number of items quantile sketch should be exact
    TS_ASSERT_EQUALS(sketch->get_quantile(0.5), 1.0);
    TS_ASSERT_EQUALS(sketch->get_quantile(0.0), 0.0);
    TS_ASSERT_EQUALS(sketch->get_quantile(1.0), 2.0);
    // Unique count hates small counts. But it should still be approximately close
    TS_ASSERT_DELTA(sketch->num_unique(), 3.0, 100.0);
    // For the few number of items, the frequent items count should be exact
    auto ret = sketch->frequent_items();
    std::sort(ret.begin(), ret.end());
    TS_ASSERT_EQUALS(ret.size(), 3);
    TS_ASSERT_EQUALS((double)ret[0].first, 0.0);
    TS_ASSERT_EQUALS((double)ret[1].first, 1.0);
    TS_ASSERT_EQUALS((double)ret[2].first, 2.0);
  }


  void test_string_sketch() {
    std::shared_ptr<unity_sarray_base> str(new unity_sarray);
    std::vector<flexible_type> vec;
    double sum = 0;

    for (size_t i = 0;i < 10000; ++i) {
      for (size_t j = 0; j < 3; ++j) {
        vec.emplace_back(std::to_string(j));
        sum += j;
      }
      vec.emplace_back(flex_type_enum::UNDEFINED);
    }
    std::static_pointer_cast<unity_sarray>(str)->construct_from_vector(vec, flex_type_enum::STRING);

    std::shared_ptr<unity_sketch_base> sketch(new unity_sketch);
    std::static_pointer_cast<unity_sketch>(sketch)->construct_from_sarray(str);
    // we get all the basic statistics correct
    TS_ASSERT_THROWS_ANYTHING(sketch->sum());
    TS_ASSERT_THROWS_ANYTHING(sketch->mean());
    TS_ASSERT_THROWS_ANYTHING(sketch->min());
    TS_ASSERT_THROWS_ANYTHING(sketch->max());
    TS_ASSERT_THROWS_ANYTHING(sketch->var());
    TS_ASSERT_EQUALS(sketch->num_undefined(), 10000);
    TS_ASSERT_EQUALS(sketch->size(), 40000);


    // approximate count is approximate
    TS_ASSERT_DELTA(sketch->frequency_count((flexible_type("0"))), 10000, 1000.0);
    TS_ASSERT_DELTA(sketch->frequency_count((flexible_type("1"))), 10000, 1000.0);
    TS_ASSERT_DELTA(sketch->frequency_count((flexible_type("2"))), 10000, 1000.0);
    // For the few number of items quantile sketch should be exact
    TS_ASSERT_THROWS_ANYTHING(sketch->get_quantile(0.5));
    // Unique count hates small counts. But it should still be approximately close
    TS_ASSERT_DELTA(sketch->num_unique(), 3.0, 100.0);
    // For the few number of items, the frequent items count should be exact
    auto ret = sketch->frequent_items();
    std::sort(ret.begin(), ret.end());
    TS_ASSERT_EQUALS(ret.size(), 3);
    TS_ASSERT_EQUALS((std::string)ret[0].first, "0");
    TS_ASSERT_EQUALS((std::string)ret[1].first, "1");
    TS_ASSERT_EQUALS((std::string)ret[2].first, "2");
  }


  void test_empty_sketch() {
    std::shared_ptr<unity_sarray_base> dbl(new unity_sarray);
    std::vector<flexible_type> vec;
    std::static_pointer_cast<unity_sarray>(dbl)->construct_from_vector(vec, flex_type_enum::FLOAT);

    std::shared_ptr<unity_sketch_base> sketch(new unity_sketch);
    std::static_pointer_cast<unity_sketch>(sketch)->construct_from_sarray(dbl);
    // we get all the basic statistics correct
    TS_ASSERT_EQUALS(sketch->sum(), 0);
    TS_ASSERT_EQUALS(sketch->mean(), 0);
    TS_ASSERT_EQUALS(sketch->var(), 0);
    TS_ASSERT_EQUALS(sketch->num_undefined(), 0);
    TS_ASSERT_EQUALS(sketch->size(), 0);
    TS_ASSERT(std::isnan(sketch->min()));
    TS_ASSERT(std::isnan(sketch->max()));


    TS_ASSERT_EQUALS(sketch->frequency_count((flexible_type(0.0))), 0.0);
    TS_ASSERT_THROWS_ANYTHING(sketch->get_quantile(0.5));
    TS_ASSERT_EQUALS(sketch->num_unique(), 0.0);
    auto ret = sketch->frequent_items();
    std::sort(ret.begin(), ret.end());
    TS_ASSERT_EQUALS(ret.size(), 0);
  }
  
  void test_nan_handling_1() {
    std::shared_ptr<unity_sarray_base> dbl(new unity_sarray);
    std::vector<flexible_type> vecb = {NAN, 1.0/0.0, 1.0, 2.0, 3.0};
    std::vector<flexible_type> vec;
    vec.reserve(4000);
    for(size_t i = 0; i < 1000; ++i) vec.insert(vec.end(), vecb.begin(), vecb.end());
            
    std::static_pointer_cast<unity_sarray>(dbl)->construct_from_vector(vec, flex_type_enum::FLOAT);
    std::shared_ptr<unity_sketch_base> sketch(new unity_sketch);
    std::static_pointer_cast<unity_sketch>(sketch)->construct_from_sarray(dbl);
  }

  void test_nan_handling_2() {
    std::shared_ptr<unity_sarray_base> dbl(new unity_sarray);
    std::vector<flexible_type> vecb = {flex_vec{NAN, 1.0}, flex_vec{6.0, 1.0/0.0},
                                      flex_vec{1.0}, flex_vec{2.0}, flex_vec{3.0}};
    
    std::vector<flexible_type> vec;
    vec.reserve(5000);
    
    for(size_t i = 0; i < 1000; ++i)
      vec.insert(vec.end(), vecb.begin(), vecb.end());
    
    std::static_pointer_cast<unity_sarray>(dbl)->construct_from_vector(vec, flex_type_enum::VECTOR);
    std::shared_ptr<unity_sketch_base> sketch(new unity_sketch);
    std::static_pointer_cast<unity_sketch>(sketch)->construct_from_sarray(dbl);
  }

  void test_nan_handling_3() {
    std::shared_ptr<unity_sarray_base> dbl(new unity_sarray);
    std::vector<flexible_type> vecb = {flex_dict{{NAN, 5.0}, {1.0, 8.0} },
                                      flex_dict{{1.8, NAN}, {1.0, 8.0} },
                                      flex_dict{{5, 4}, {1.0, 8.0} }};
    std::vector<flexible_type> vec;
    vec.reserve(3000);

    for(size_t i = 0; i < 1000; ++i)
      vec.insert(vec.end(), vecb.begin(), vecb.end());
            
    std::static_pointer_cast<unity_sarray>(dbl)->construct_from_vector(vec, flex_type_enum::DICT);
    std::shared_ptr<unity_sketch_base> sketch(new unity_sketch);
    std::static_pointer_cast<unity_sketch>(sketch)->construct_from_sarray(dbl);
  }


  void test_int_regression_case_1() {

    // This caused an issue a while ago.
    std::shared_ptr<unity_sarray_base> intl(new unity_sarray);
    std::vector<flexible_type> vec = {flex_int(-1), flex_int(0), flex_int(1)};
    vec.reserve(20000 + 3);
    for(long i = 0; i < 10000; ++i) {
      vec.push_back(i);
      vec.push_back(-i);
    }
            
    std::static_pointer_cast<unity_sarray>(intl)->construct_from_vector(vec, flex_type_enum::INTEGER);
    std::shared_ptr<unity_sketch_base> sketch(new unity_sketch);
    std::static_pointer_cast<unity_sketch>(sketch)->construct_from_sarray(intl);
  }
  
};

BOOST_FIXTURE_TEST_SUITE(_unity_sketch_test, unity_sketch_test)
BOOST_AUTO_TEST_CASE(test_numeric_sketch) {
  unity_sketch_test::test_numeric_sketch();
}
BOOST_AUTO_TEST_CASE(test_string_sketch) {
  unity_sketch_test::test_string_sketch();
}
BOOST_AUTO_TEST_CASE(test_empty_sketch) {
  unity_sketch_test::test_empty_sketch();
}
BOOST_AUTO_TEST_CASE(test_nan_handling_1) {
  unity_sketch_test::test_nan_handling_1();
}
BOOST_AUTO_TEST_CASE(test_nan_handling_2) {
  unity_sketch_test::test_nan_handling_2();
}
BOOST_AUTO_TEST_CASE(test_nan_handling_3) {
  unity_sketch_test::test_nan_handling_3();
}
BOOST_AUTO_TEST_CASE(test_int_regression_case_1) {
  unity_sketch_test::test_int_regression_case_1();
}
BOOST_AUTO_TEST_SUITE_END()
