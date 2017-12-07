/*
 * Copyright (c) 2013 Turi Inc.
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
 *      http://turicreate.com
 *
 */

#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <util/test_macros.hpp>
#include <iostream>
#include <typeinfo>       // operator typeid
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <iterator>


#include <unity/toolkits/sparse_similarity/sliced_itemitem_matrix.hpp>
#include <util/cityhash_tc.hpp>
#include <parallel/lambda_omp.hpp>

using namespace turi;
using namespace turi::sparse_sim;

////////////////////////////////////////////////////////////////////////////////

struct test_itemitem_matrix  {
 public:

  void _test_basic_access(size_t n, size_t m) {
    dense_triangular_itemitem_container<size_t> X(n, m);

    TS_ASSERT_EQUALS(X.rows(), n);
    TS_ASSERT_EQUALS(X.cols(), m);

    // Ensure things are contiguous
    size_t *next_ptr = &(X(0,1));    
    for(size_t i = 0; i < n; ++i) {
      for(size_t j = i + 1; j < m; ++j) {
        DASSERT_TRUE(next_ptr == &(X(i,j)));
        ++next_ptr;
      }
    }

    // Now set them to a value. 
    for(size_t i = 0; i < n; ++i) {
      for(size_t j = i + 1; j < m; ++j) {
        X(i, j) = hash64(i, j);
      }
    }

    // Make sure that works
    for(size_t i = 0; i < n; ++i) {
      for(size_t j = i + 1; j < m; ++j) {
        DASSERT_EQ(hash64(i, j), X(i, j));
      }
    }

    // Make sure the apply all works. 
    std::vector<int> hit(n * m, 0);
    
    X.apply_all([&](size_t i, size_t j, size_t value) {
        DASSERT_LT(i, n);
        DASSERT_LT(j, m);
        DASSERT_LT(i, j);
        
        ++hit[i * m + j];
        DASSERT_EQ(value, hash64(i, j));
      });

    // Now, check the hit vector. 
    for(size_t i = 0; i < n; ++i) {
      for(size_t j = 0; j < m; ++j) {
        if(i < j) {
          DASSERT_EQ(hit[i * m + j], 1);
        } else {
          DASSERT_EQ(hit[i * m + j], 0);
        }
      }
    }
  }

  void test_symmetric_2() {
    _test_basic_access(2, 2);
  }

  void test_symmetric_20() {
    _test_basic_access(20, 20);
  }

  void test_nonsymmetric_1_20() {
    _test_basic_access(1, 20);
  }

  void test_nonsymmetric_10_20() {
    _test_basic_access(10, 20);
  }

  void test_nonsymmetric_19_20() {
    _test_basic_access(19, 20);
  }
  
  void test_parallel_access() {
    dense_triangular_itemitem_container<size_t> X(19, 43);

    parallel_for(size_t(0), X.rows(), [&](size_t i) {
        for(size_t j = i+1; j < 43; ++j) {
          X.apply(i, j, [&](size_t& value) { value = i + j; });
        }
      });

    X.apply_all([](size_t i, size_t j, size_t value) {
        DASSERT_EQ(value, i + j);
      });
  }
};

BOOST_FIXTURE_TEST_SUITE(_test_itemitem_matrix, test_itemitem_matrix)
BOOST_AUTO_TEST_CASE(test_symmetric_2) {
  test_itemitem_matrix::test_symmetric_2();
}
BOOST_AUTO_TEST_CASE(test_symmetric_20) {
  test_itemitem_matrix::test_symmetric_20();
}
BOOST_AUTO_TEST_CASE(test_nonsymmetric_1_20) {
  test_itemitem_matrix::test_nonsymmetric_1_20();
}
BOOST_AUTO_TEST_CASE(test_nonsymmetric_10_20) {
  test_itemitem_matrix::test_nonsymmetric_10_20();
}
BOOST_AUTO_TEST_CASE(test_nonsymmetric_19_20) {
  test_itemitem_matrix::test_nonsymmetric_19_20();
}
BOOST_AUTO_TEST_CASE(test_parallel_access) {
  test_itemitem_matrix::test_parallel_access();
}
BOOST_AUTO_TEST_SUITE_END()
