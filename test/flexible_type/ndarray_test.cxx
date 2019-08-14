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

template <typename T>
void test_save_load(const ndarray<T>& a) {
  oarchive oarc;
  oarc << a;
  iarchive iarc(oarc.buf, oarc.off);
  ndarray<T> b;
  iarc >> b;
  nd_assert_equal(a, b);
  BOOST_TEST(b.is_valid() == true);
  BOOST_TEST(b.is_full() == true);
}


BOOST_AUTO_TEST_CASE(test_empty) {
 ndarray<int> i;
 BOOST_TEST(i.is_valid());
 BOOST_TEST(i.is_full());
 test_save_load(i);

 ndarray<int> array1({1,1,2,3,
                      4,5,6,7},
                      {2,4},
                      {4,1});
 BOOST_TEST(array1.empty() == false);
 ndarray<int> array2;
 BOOST_TEST(array2.empty() == true);
}

BOOST_AUTO_TEST_CASE(test_canonical) {
 ndarray<int> fortran({0,5,1,6,2,7,3,8,4,9},
                      {2,5},
                      {1,2});
 std::cout << fortran << "\n";
 BOOST_TEST(fortran.is_valid());
 BOOST_TEST(fortran.is_full());
 ndarray<int> c = fortran.canonicalize();

 std::vector<size_t> desired_stride{5,1};
 std::vector<size_t> desired_shape{2,5};
 std::vector<int> desired_elements{0,1,2,3,4,5,6,7,8,9};
 BOOST_TEST(c.stride() == desired_stride, tt::per_element());
 BOOST_TEST(c.shape() == desired_shape, tt::per_element());
 BOOST_TEST(c.elements() == desired_elements, tt::per_element());
 BOOST_TEST(c.is_valid() == true);
 BOOST_TEST(c.is_full() == true);
 BOOST_TEST(c.is_canonical() == true);
 nd_assert_equal(c, fortran);

 test_save_load(fortran);
 test_save_load(c);
}


BOOST_AUTO_TEST_CASE(test_subarray) {
 ndarray<int> subarray({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},
                      {2,2},
                      {1,4}); // top left corner of array
 std::cout << subarray << "\n";
 BOOST_TEST(subarray.is_valid());
 BOOST_TEST(subarray.is_full() == false);
 BOOST_TEST(subarray.is_canonical() == false);
 ndarray<int> c = subarray.canonicalize();

 std::vector<int> desired_elements{0,4,1,5};
 std::vector<size_t> desired_shape{2,2};
 std::vector<size_t> desired_stride{2,1};
 BOOST_TEST(c.elements() == desired_elements, tt::per_element());
 BOOST_TEST(c.shape() == desired_shape, tt::per_element());
 BOOST_TEST(c.stride() == desired_stride, tt::per_element());
 BOOST_TEST(c.is_valid() == true);
 BOOST_TEST(c.is_full() == true);
 BOOST_TEST(c.is_canonical() == true);
 nd_assert_equal(c, subarray);

 test_save_load(subarray);
 test_save_load(c);
}

BOOST_AUTO_TEST_CASE(test_print) {
 ndarray<int> array({0,1,2,3,4,5},
                      {2,3},
                      {3,1}); // top left corner of array
 std::cout << "print test " << array << "\n";
}


BOOST_AUTO_TEST_CASE(test_subarray2) {
 ndarray<int> subarray({0,1,2,3,
                        4,5,6,7,
                        8,9,10,11,
                        12,13,14,15,16},
                      {2,2},
                      {1,4},
                      2); // top right corner of array
 BOOST_TEST(subarray.is_valid());
 BOOST_TEST(subarray.is_full() == false);
 BOOST_TEST(subarray.is_canonical() == false);
 ndarray<int> c = subarray.canonicalize();

 std::vector<int> desired_elements{2,6,3,7};
 std::vector<size_t> desired_shape{2,2};
 std::vector<size_t> desired_stride{2,1};
 BOOST_TEST(c.elements() == desired_elements, tt::per_element());
 BOOST_TEST(c.shape() == desired_shape, tt::per_element());
 BOOST_TEST(c.stride() == desired_stride, tt::per_element());
 BOOST_TEST(c.is_valid() == true);
 BOOST_TEST(c.is_full() == true);
 BOOST_TEST(c.is_canonical() == true);
 nd_assert_equal(c, subarray);

 test_save_load(subarray);
 test_save_load(c);
}

BOOST_AUTO_TEST_CASE(test_invalid) {
 TS_ASSERT_THROWS(ndarray<int>({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},
                               {2,3},
                               {2,8}), std::string);

 TS_ASSERT_THROWS(ndarray<int>({0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16},
                               {3,8},
                               {1,1}), std::string);
}


BOOST_AUTO_TEST_CASE(test_bad_shapes) {
 ndarray<int> a({0,1,2,3,4,5,6,7,8,9},
                {0,0},
                {1,5});
 BOOST_TEST(a.elements().size() == 0);
 ndarray<int> b({0,1,2,3,4,5,6,7,8,9},
                {1,0},
                {1,5});
 BOOST_TEST(b.elements().size() == 0);
}
BOOST_AUTO_TEST_CASE(test_odd_stride) {
 // a stride of 0 is technically valid
 // though a little odd
 {
   ndarray<int> zero_stride({0,1,2,3,4,5,6,7,8,9},
                           {2,5},
                           {1,0});
   BOOST_TEST(zero_stride.is_valid() == true);
   BOOST_TEST(zero_stride.is_full() == false);
   BOOST_TEST(zero_stride.is_canonical() == false);
   ndarray<int> zero_stride_c = zero_stride.canonicalize();
   std::vector<int> desired_elements{0,0,0,0,0,1,1,1,1,1};
   std::vector<size_t> desired_shape{2,5};
   std::vector<size_t> desired_stride{5,1};
   BOOST_TEST(zero_stride_c.elements() == desired_elements, tt::per_element());
   BOOST_TEST(zero_stride_c.shape() == desired_shape, tt::per_element());
   BOOST_TEST(zero_stride_c.stride() == desired_stride, tt::per_element());
   test_save_load(zero_stride);
 }

 // test dim 1
 {
   ndarray<int> dim1(std::vector<int>{0,1,2},
                     {1,1,3},
                     {0,0,1}); 
   BOOST_TEST(dim1.is_valid() == true);
   BOOST_TEST(dim1.is_full() == true);
   BOOST_TEST(dim1.is_canonical() == false);
   ndarray<int> dim1_c = dim1.canonicalize();
   std::vector<int> desired_elements{0,1,2};
   std::vector<size_t> desired_shape{1,1,3};
   std::vector<size_t> desired_stride{3,3,1};
   BOOST_TEST(dim1_c.elements() == desired_elements, tt::per_element());
   BOOST_TEST(dim1_c.shape() == desired_shape, tt::per_element());
   BOOST_TEST(dim1_c.stride() == desired_stride, tt::per_element());
   test_save_load(dim1);
 }
 // another test dim 1
 {
   ndarray<int> dim1({0,2,4,1,3,5},
                     {3,1,1,2},
                     {1,0,0,3}); 
   std::cout << dim1 << "\n";
   BOOST_TEST(dim1.is_valid() == true);
   BOOST_TEST(dim1.is_full() == true);
   BOOST_TEST(dim1.is_canonical() == false);
   ndarray<int> dim1_c = dim1.canonicalize();
   std::vector<int> desired_elements{0,1,2,3,4,5};
   std::vector<size_t> desired_shape{3,1,1,2};
   std::vector<size_t> desired_stride{2,2,2,1};
   BOOST_TEST(dim1_c.elements() == desired_elements, tt::per_element());
   BOOST_TEST(dim1_c.shape() == desired_shape, tt::per_element());
   BOOST_TEST(dim1_c.stride() == desired_stride, tt::per_element());
   test_save_load(dim1);
   test_save_load(dim1_c);
 }
}


BOOST_AUTO_TEST_CASE(test_add) {
 ndarray<int> array1({1,1,2,3,
                      4,5,6,7},
                      {2,4},
                      {4,1});
 ndarray<int> array2({1,4,
                      1,5,
                      2,6,
                      3,7},
                      {2,4},
                      {1,2});
 std::cout << array1 << "\n";
 std::cout << array2 << "\n";
 array1 += array2;
 std::vector<int> desired_elements{1+1,1+1,2+2,3+3,4+4,5+5,6+6,7+7};
 std::vector<size_t> desired_shape{2,4};
 std::vector<size_t> desired_stride{4,1};
 BOOST_TEST(array1.elements() == desired_elements, tt::per_element());
 BOOST_TEST(array1.shape() == desired_shape, tt::per_element());
 BOOST_TEST(array1.stride() == desired_stride, tt::per_element());

 array2 += 5;
 std::vector<int> desired_elements2{1+5,4+5, 1+5,5+5, 2+5,6+5,3+5,7+5} ;
 BOOST_TEST(array2.elements() == desired_elements2, tt::per_element());
}

BOOST_AUTO_TEST_CASE(test_sub) {
 ndarray<int> array1({1,1,2,3,
                      4,5,6,7},
                      {2,4},
                      {4,1});
 ndarray<int> array2({1,4,
                      1,5,
                      2,6,
                      3,7},
                      {2,4},
                      {1,2});
 std::cout << array1 << "\n";
 std::cout << array2 << "\n";
 array1 -= array2;
 std::vector<int> desired_elements{1-1,1-1,2-2,3-3,4-4,5-5,6-6,7-7};
 std::vector<size_t> desired_shape{2,4};
 std::vector<size_t> desired_stride{4,1};
 BOOST_TEST(array1.elements() == desired_elements, tt::per_element());
 BOOST_TEST(array1.shape() == desired_shape, tt::per_element());
 BOOST_TEST(array1.stride() == desired_stride, tt::per_element());

 array2 -= 5;
 std::vector<int> desired_elements2{1-5,4-5, 1-5,5-5, 2-5,6-5,3-5,7-5} ;
 BOOST_TEST(array2.elements() == desired_elements2, tt::per_element());
}

BOOST_AUTO_TEST_CASE(test_multiply) {
 ndarray<int> array1({1,1,2,3,
                      4,5,6,7},
                      {2,4},
                      {4,1});
 ndarray<int> array2({1,4,
                      1,5,
                      2,6,
                      3,7},
                      {2,4},
                      {1,2});
 array1 *= array2;
 std::vector<int> desired_elements{1*1,1*1,2*2,3*3,4*4,5*5,6*6,7*7};
 std::vector<size_t> desired_shape{2,4};
 std::vector<size_t> desired_stride{4,1};
 BOOST_TEST(array1.elements() == desired_elements, tt::per_element());
 BOOST_TEST(array1.shape() == desired_shape, tt::per_element());
 BOOST_TEST(array1.stride() == desired_stride, tt::per_element());

 array2 *= 5;
 std::vector<int> desired_elements2{1*5,4*5, 1*5,5*5, 2*5,6*5,3*5,7*5} ;
 BOOST_TEST(array2.elements() == desired_elements2, tt::per_element());
}


BOOST_AUTO_TEST_CASE(test_divide) {
 ndarray<int> array1({1,1,2,3,
                      4,5,6,7},
                      {2,4},
                      {4,1});
 ndarray<int> array2({1,4,
                      1,5,
                      2,6,
                      3,7},
                      {2,4},
                      {1,2});
 array1 /= array2;
 std::vector<int> desired_elements{1/1,1/1,2/2,3/3,4/4,5/5,6/6,7/7};
 std::vector<size_t> desired_shape{2,4};
 std::vector<size_t> desired_stride{4,1};
 BOOST_TEST(array1.elements() == desired_elements, tt::per_element());
 BOOST_TEST(array1.shape() == desired_shape, tt::per_element());
 BOOST_TEST(array1.stride() == desired_stride, tt::per_element());

 array2 /= 5;
 std::vector<int> desired_elements2{1/5,4/5, 1/5,5/5, 2/5,6/5,3/5,7/5} ;
 BOOST_TEST(array2.elements() == desired_elements2, tt::per_element());
}


BOOST_AUTO_TEST_CASE(test_mod) {
 ndarray<int> array1({1,1,2,3,
                      4,5,6,7},
                      {2,4},
                      {4,1});
 ndarray<int> array2({1,4,
                      1,5,
                      2,6,
                      3,7},
                      {2,4},
                      {1,2});
 array1 %= array2;
 std::vector<int> desired_elements{1%1,1%1,2%2,3%3,4%4,5%5,6%6,7%7};
 std::vector<size_t> desired_shape{2,4};
 std::vector<size_t> desired_stride{4,1};
 BOOST_TEST(array1.elements() == desired_elements, tt::per_element());
 BOOST_TEST(array1.shape() == desired_shape, tt::per_element());
 BOOST_TEST(array1.stride() == desired_stride, tt::per_element());

 array2 %= 5;
 std::vector<int> desired_elements2{1%5,4%5, 1%5,5%5, 2%5,6%5,3%5,7%5} ;
 BOOST_TEST(array2.elements() == desired_elements2, tt::per_element());
}

BOOST_AUTO_TEST_CASE(test_mod_float) {
 ndarray<double> array1({1,1,2,3,
                      4,5,6,7},
                      {2,4},
                      {4,1});
 double c = 0.6;
 array1 %= c;
 std::vector<double> desired_elements{fmod(1,c),fmod(1,c),fmod(2,c),fmod(3,c),
                                      fmod(4,c),fmod(5,c),fmod(6,c),fmod(7,c)};
 std::vector<size_t> desired_shape{2,4};
 std::vector<size_t> desired_stride{4,1};
 BOOST_TEST(array1.elements() == desired_elements, tt::per_element());
 BOOST_TEST(array1.shape() == desired_shape, tt::per_element());
 BOOST_TEST(array1.stride() == desired_stride, tt::per_element());
}


BOOST_AUTO_TEST_CASE(test_flexible_type_conversions1) {
 flex_list array1{flexible_type(flex_vec{1,1,1,1}),
                  flexible_type(flex_vec{2,2,2,2}),
                  flexible_type(flex_vec{3,3,3,3}),
                  flexible_type(flex_vec{4,4,4,4})};
 flexible_type f1(array1);
 flex_nd_vec target1({1,1,1,1,
                      2,2,2,2,
                      3,3,3,3,
                      4,4,4,4},
                      {4,4});
 flex_nd_vec fconv1 = f1.to<flex_nd_vec>();

 BOOST_TEST(fconv1.elements() == target1.elements(), tt::per_element());
 BOOST_TEST(fconv1.shape() == target1.shape(), tt::per_element());
 BOOST_TEST(fconv1.stride() == target1.stride(), tt::per_element());
 test_save_load(fconv1);
}

BOOST_AUTO_TEST_CASE(test_flexible_type_conversions2) {
 flexible_type two(2.0);
 flexible_type four(4);
 flex_list array1{flexible_type(flex_vec{1,1,1,1}),
                  flexible_type(flex_list{two,two,two,two}),
                  flexible_type(flex_nd_vec({3,3,3,3})),
                  flexible_type(flex_list{four,four,four,four})};
 flexible_type f1(array1);
 flex_nd_vec target1({1,1,1,1,
                      2,2,2,2,
                      3,3,3,3,
                      4,4,4,4},
                      {4,4});
 flex_nd_vec fconv1 = f1.to<flex_nd_vec>();

 BOOST_TEST(fconv1.elements() == target1.elements(), tt::per_element());
 BOOST_TEST(fconv1.shape() == target1.shape(), tt::per_element());
 BOOST_TEST(fconv1.stride() == target1.stride(), tt::per_element());
 test_save_load(fconv1);
}


BOOST_AUTO_TEST_CASE(test_flexible_type_conversions_3d) {
 flex_nd_vec nd1({1,1,1,1,
                  2,2,2,2,
                  3,3,3,3},
                  {3,4});
 flex_list array1{flexible_type(nd1),
                  flexible_type(nd1),
                  flexible_type(nd1),
                  flexible_type(nd1)};
 flexible_type f1(array1);

 flex_nd_vec target1({1,1,1,1,
                      2,2,2,2,
                      3,3,3,3,
                      1,1,1,1,
                      2,2,2,2,
                      3,3,3,3,
                      1,1,1,1,
                      2,2,2,2,
                      3,3,3,3,
                      1,1,1,1,
                      2,2,2,2,
                      3,3,3,3},
                      {4,3,4});
 flex_nd_vec fconv1 = f1.to<flex_nd_vec>();

 BOOST_TEST(fconv1.elements() == target1.elements(), tt::per_element());
 BOOST_TEST(fconv1.shape() == target1.shape(), tt::per_element());
 BOOST_TEST(fconv1.stride() == target1.stride(), tt::per_element());
 test_save_load(fconv1);
}


BOOST_AUTO_TEST_CASE(test_flexible_type_conversions_fail1) {
 flexible_type two(2.0);
 flexible_type four(4);
 flex_list array1{flexible_type(flex_vec{1,1,1,1}),
                  flexible_type(flex_list{two,two,two,two}),
                  flexible_type(flex_vec{3,3,3}),
                  flexible_type(flex_list{four,four,four,four})};
 flexible_type f1(array1);
 TS_ASSERT_THROWS(f1.to<flex_nd_vec>(), std::string);

 flex_list array2{flexible_type(flex_vec{1,1,1,1}),
                  flexible_type(flex_list{two,two,two,two, two})};
 flexible_type f2(array2);
 TS_ASSERT_THROWS(f2.to<flex_nd_vec>(), std::string);

 flex_list array3{flexible_type(flex_list{two,two,two,two, two}),
                  flexible_type(flex_vec{1,1,1,1})};
 flexible_type f3(array3);
 TS_ASSERT_THROWS(f3.to<flex_nd_vec>(), std::string);

 flex_list array4{flexible_type(flex_list{two,two,two,two, two}),
                  flexible_type(1)};
 flexible_type f4(array4);
 TS_ASSERT_THROWS(f4.to<flex_nd_vec>(), std::string);
}


BOOST_AUTO_TEST_CASE(test_image_conversion) {
 flex_nd_vec nd1({1,1,1,1,
                 2,2,2,2,
                 3,3,3,3,
                 1,1,1,1,
                 2,2,2,2,
                 3,3,3,3,
                 1,1,1,1,
                 2,2,2,2,
                 3,3,3,3,
                 1,1,1,1,
                 2,2,2,2,
                 3,3,3,3},
                 {4,3,4});
 auto rt1 = flexible_type(flexible_type(nd1).to<flex_image>()).to<flex_nd_vec>();
 BOOST_TEST(nd1.elements() == rt1.elements(), tt::per_element());
 BOOST_TEST(nd1.shape() == rt1.shape(), tt::per_element());
 BOOST_TEST(nd1.stride() == rt1.stride(), tt::per_element());

 flex_nd_vec nd2({1,1,1,
                 2,2,2,
                 3,3,3,
                 1,1,1,
                 2,2,2,
                 3,3,3,
                 1,1,1,
                 2,2,2,
                 3,3,3,
                 1,1,1,
                 2,2,2,
                 3,3,3,},
                 {4,3,3});
 auto rt2 = flexible_type(flexible_type(nd2).to<flex_image>()).to<flex_nd_vec>();
 BOOST_TEST(nd2.elements() == rt2.elements(), tt::per_element());
 BOOST_TEST(nd2.shape() == rt2.shape(), tt::per_element());
 BOOST_TEST(nd2.stride() == rt2.stride(), tt::per_element());

 flex_nd_vec nd3({1, 2, 3, 
                 1, 2, 3,
                 1, 2, 3,
                 1, 2, 3,},
                 {4,3});
 auto rt3 = flexible_type(flexible_type(nd3).to<flex_image>()).to<flex_nd_vec>();
 BOOST_TEST(nd3.elements() == rt3.elements(), tt::per_element());
 BOOST_TEST(nd3.shape() == rt3.shape(), tt::per_element());
 BOOST_TEST(nd3.stride() == rt3.stride(), tt::per_element());
}


BOOST_AUTO_TEST_CASE(test_equality) {
 flex_nd_vec array1({1,1,2,3,
                      4,5,6,7},
                      {2,4},
                      {4,1});
 flex_nd_vec array2({1,4,
                      1,5,
                      2,6,
                      3,7},
                      {2,4},
                      {1,2});
 flexible_type f1(array1);
 flexible_type f2(array2);
 BOOST_TEST(array1 == array2);
 BOOST_TEST(f1 == f2);
}

BOOST_AUTO_TEST_CASE(test_equality_subarray) {
 flex_nd_vec array1({1,1,2,3,
                      4,5,6,7},
                      {1,2},
                      {4,1});
 flex_nd_vec array2({1,4,
                      1,5,
                      2,6,
                      3,7},
                      {1,2},
                      {1,2});
 flexible_type f1(array1);
 flexible_type f2(array2);
 BOOST_TEST(array1 == array2);
 BOOST_TEST(f1 == f2);
}

BOOST_AUTO_TEST_CASE(test_equality_fail) {
 flex_nd_vec array1({1,1,2,3,
                      4,5,6,7},
                      {1,2},
                      {4,1});
 flex_nd_vec array2({1,4,
                      1,5,
                      2,6,
                      3,7},
                      {2,1},
                      {2,1});
 flexible_type f1(array1);
 flexible_type f2(array2);
 BOOST_TEST(array1 != array2);
 BOOST_TEST(f1 != f2);
 // check both == and != 
 bool t = f1 == f2;
 BOOST_TEST(t == false);
}
