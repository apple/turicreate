/*
 * Copyright (c) 2013 Turi
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.turicreate.com
 *
 */

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <vector>
#include <string>
#include <set>
#include <iostream>
#include <sstream>

#include <core/random/random.hpp>

#include <toolkits/util/algorithmic_utils.hpp>


using namespace turi;

void test_it(const std::vector<size_t>& v1, const std::vector<size_t>& v2) {

  {
  std::vector<size_t> out(std::max(v1.size(), v2.size()));

  auto out_end = std::set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(), out.begin());

  size_t out_count = out_end - out.begin();

  TS_ASSERT_EQUALS(out_count, count_intersection(v1.begin(), v1.end(), v2.begin(), v2.end()));

  // Make sure it's the same with a different iterator
  std::list<size_t> l2(v2.begin(), v2.end());
  TS_ASSERT_EQUALS(out_count, count_intersection(v1.begin(), v1.end(), l2.begin(), l2.end()));

  // Make sure it's the same with a castable type
  std::vector<double> v3(v2.begin(), v2.end());
  TS_ASSERT_EQUALS(out_count, count_intersection(v1.begin(), v1.end(), v3.begin(), v3.end()));
  }


  {
    std::vector<size_t> v1a = v1;
    std::vector<size_t> v2a = v2;

    auto comp_function = [](const size_t& x1, const size_t& x2){ return x1 > x2; };

    std::sort(v1a.begin(), v1a.end(), comp_function);
    std::sort(v2a.begin(), v2a.end(), comp_function);

    std::vector<size_t> out(std::max(v1a.size(), v2a.size()));

    auto out_end = std::set_intersection(v1a.begin(), v1a.end(), v2a.begin(), v2a.end(), out.begin(), comp_function);

    size_t out_count = out_end - out.begin();

    TS_ASSERT_EQUALS(out_count, count_intersection(v1a.begin(), v1a.end(), v2a.begin(), v2a.end(), comp_function));

    // Make sure it's the same with a different iterator
    std::list<size_t> l2(v2a.begin(), v2a.end());
    TS_ASSERT_EQUALS(out_count, count_intersection(v1a.begin(), v1a.end(), l2.begin(), l2.end(), comp_function));

    // Make sure it's the same with a castable type
    std::vector<double> v3(v2a.begin(), v2a.end());
    TS_ASSERT_EQUALS(out_count, count_intersection(v1a.begin(), v1a.end(), v3.begin(), v3.end(), comp_function));
  }
}

struct recsys_algo_model_test  {
 public:

  void test_small() {
    test_it({0,1,2}, {2,3,5} );
  }

  void test_no_overlap() {
    test_it({0,1,2}, {3,3,5} );
  }

  void test_no_overlap_2() {
    test_it({0,1,2}, {3,4,5,6,7,8,9} );
  }

  void test_all_overlap() {
    test_it({0,1,2}, {0,1,2});
  }

  void test_subset() {
    test_it({0,1,2}, {0,1,2,3,4});
  }

  void test_subset_2() {
    test_it({0,1,2}, {0,1,2,2,4});
  }

  void test_subset_3() {
    test_it({0,1,2}, {0,1,1,2,2});
  }

  void test_random() {
    std::vector<size_t> v1(500);
    std::vector<size_t> v2(750);

    random::seed(0);

    for(size_t& v : v1)
      v = random::fast_uniform<size_t>(0, 1000);

    for(size_t& v : v2)
      v = random::fast_uniform<size_t>(0, 1000);

    std::sort(v1.begin(), v1.end());
    std::sort(v2.begin(), v2.end());

    test_it(v1,v2);
  }
};

BOOST_FIXTURE_TEST_SUITE(_recsys_algo_model_test, recsys_algo_model_test)
BOOST_AUTO_TEST_CASE(test_small) {
  recsys_algo_model_test::test_small();
}
BOOST_AUTO_TEST_CASE(test_no_overlap) {
  recsys_algo_model_test::test_no_overlap();
}
BOOST_AUTO_TEST_CASE(test_no_overlap_2) {
  recsys_algo_model_test::test_no_overlap_2();
}
BOOST_AUTO_TEST_CASE(test_all_overlap) {
  recsys_algo_model_test::test_all_overlap();
}
BOOST_AUTO_TEST_CASE(test_subset) {
  recsys_algo_model_test::test_subset();
}
BOOST_AUTO_TEST_CASE(test_subset_2) {
  recsys_algo_model_test::test_subset_2();
}
BOOST_AUTO_TEST_CASE(test_subset_3) {
  recsys_algo_model_test::test_subset_3();
}
BOOST_AUTO_TEST_CASE(test_random) {
  recsys_algo_model_test::test_random();
}
BOOST_AUTO_TEST_SUITE_END()
