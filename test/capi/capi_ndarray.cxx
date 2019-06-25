/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

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
#include <core/data/flexible_type/ndarray.hpp>

#include <capi/TuriCreate.h>
#include <capi/impl/capi_wrapper_structs.hpp>

using namespace turi;
using namespace flexible_type_impl;
namespace tt = boost::test_tools;

template <typename T>
void nd_assert_equal(const ndarray<T>& a, const ndarray<T>& b) {
  BOOST_CHECK(a.is_valid());
  BOOST_CHECK(b.is_valid());

  BOOST_TEST(a.num_elem() == b.num_elem());
  BOOST_TEST(a.shape() == b.shape());
  if (a.shape().size() > 0) {
    std::vector<size_t> idx(a.shape().size(), 0);
    do {
      double aval = a.at(a.index(idx));
      double bval = b.at(b.index(idx));
      BOOST_TEST(aval == bval);
    } while(a.increment_index(idx));
  }
}

static void test_array_path(const ndarray<double>& a) { 

  // Construct an ndarray through the C-API with these. 
  tc_error* error = nullptr; 

  std::vector<uint64_t> _shape(a.shape().begin(), a.shape().end()); 
  std::vector<int64_t> _stride(a.stride().begin(), a.stride().end()); 


  tc_ndarray* X = tc_ndarray_create_from_data(
      a.shape().size(),
      _shape.data(),
      _stride.data(),
      (a.empty() ? nullptr : &(a[0])), 
      &error); 

  BOOST_TEST(error == nullptr); 

  nd_assert_equal(a, X->value);

  size_t n_dim = tc_ndarray_num_dimensions(X, &error);
  const uint64_t* shape = tc_ndarray_shape(X, &error);
  const int64_t* strides = tc_ndarray_strides(X, &error);
  const double* data = tc_ndarray_data(X, &error);

  // Construct another tc_ndarray object with these.
 tc_ndarray* X2 = tc_ndarray_create_from_data(n_dim, shape, strides, data, &error);
  BOOST_TEST(error == nullptr);

  nd_assert_equal(a, X2->value);
}


BOOST_AUTO_TEST_CASE(test_empty) {
 ndarray<double> i;
 BOOST_TEST(i.is_valid());
 BOOST_TEST(i.is_full());
 test_array_path(i);
}

BOOST_AUTO_TEST_CASE(test_canonical) {
 ndarray<double> fortran({0,5,1,6,2,7,3,8,4,9},
                      {2,5},
                      {1,2});
 BOOST_TEST(fortran.is_valid());
 BOOST_TEST(fortran.is_full());
 ndarray<double> c = fortran.canonicalize();

 test_array_path(fortran);
 test_array_path(c);
}


BOOST_AUTO_TEST_CASE(test_subarray) {
 ndarray<double> subarray({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},
                      {2,2},
                      {1,4}); // top left corner of array
 BOOST_TEST(subarray.is_valid());
 BOOST_TEST(subarray.is_full() == false);
 BOOST_TEST(subarray.is_canonical() == false);
 ndarray<double> c = subarray.canonicalize();

 test_array_path(subarray);
 test_array_path(c);
}


BOOST_AUTO_TEST_CASE(test_subarray2) {
 ndarray<double> subarray({0,1,2,3,
                        4,5,6,7,
                        8,9,10,11,
                        12,13,14,15,16},
                      {2,2},
                      {1,4},
                      2); // top right corner of array
 BOOST_TEST(subarray.is_valid());
 BOOST_TEST(subarray.is_full() == false);
 BOOST_TEST(subarray.is_canonical() == false);
 ndarray<double> c = subarray.canonicalize();
 test_array_path(subarray);
 test_array_path(c);

}

BOOST_AUTO_TEST_CASE(test_odd_stride) {
 // a stride of 0 is technically valid
 // though a little odd
 {
   ndarray<double> zero_stride({0,1,2,3,4,5,6,7,8,9},
                           {2,5},
                           {1,0});
   BOOST_TEST(zero_stride.is_valid() == true);
   BOOST_TEST(zero_stride.is_full() == false);
   BOOST_TEST(zero_stride.is_canonical() == false);
   ndarray<double> zero_stride_c = zero_stride.canonicalize();
   test_array_path(zero_stride);
   test_array_path(zero_stride_c);
 }

 // test dim 1
 {
   ndarray<double> dim1(std::vector<double>{0,1,2},
                     {1,1,3},
                     {0,0,1}); 
   BOOST_TEST(dim1.is_valid() == true);
   BOOST_TEST(dim1.is_full() == true);
   BOOST_TEST(dim1.is_canonical() == false);
   ndarray<double> dim1_c = dim1.canonicalize();
   test_array_path(dim1);
   test_array_path(dim1_c);
 }
 // another test dim 1
 {
   ndarray<double> dim1({0,2,4,1,3,5},
                     {3,1,1,2},
                     {1,0,0,3}); 
   BOOST_TEST(dim1.is_valid() == true);
   BOOST_TEST(dim1.is_full() == true);
   BOOST_TEST(dim1.is_canonical() == false);
   ndarray<double> dim1_c = dim1.canonicalize();
   test_array_path(dim1);
   test_array_path(dim1_c);
 }
}

