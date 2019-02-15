/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
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


#include <generics/gl_string.hpp>
#include <util/testing_utils.hpp>

using namespace turi;

struct test_string_iterators  {
 public:

  void _test_begin(gl_string s) {
    const gl_string& cs = s;
    gl_string::iterator b = s.begin();
    gl_string::const_iterator cb = cs.begin();
    if (!s.empty())
    {
      TS_ASSERT(*b == s[0]);
    }
    TS_ASSERT(b == cb);
  }

  void test_begin() {
    _test_begin(gl_string());
    _test_begin(gl_string("123"));
  }

  void _test_cbegin(const gl_string& s) {
    gl_string::const_iterator cb = s.cbegin();
    if (!s.empty())
    {
      TS_ASSERT(*cb == s[0]);
    }
    TS_ASSERT(cb == s.begin());
  }

  void test_cbegin() {
    _test_cbegin(gl_string());
    _test_cbegin(gl_string("123"));
  }


  void _test_cend(const gl_string& s) {
    gl_string::const_iterator ce = s.cend();
    TS_ASSERT(ce == s.end());
  }

  void test_cend() {
    _test_cend(gl_string());
    _test_cend(gl_string("123"));
  }

  void _test_crbegin(const gl_string& s) {
    gl_string::const_reverse_iterator cb = s.crbegin();
    if (!s.empty())
    {
      TS_ASSERT(*cb == s.back());
    }
    TS_ASSERT(cb == s.rbegin());
  }

  void test_crbegin() {
    _test_crbegin(gl_string());
    _test_crbegin(gl_string("123"));
  }
  
  void _test_crend(const gl_string& s) {
    gl_string::const_reverse_iterator ce = s.crend();
    TS_ASSERT(ce == s.rend());
  }

  void test_crend() {
    _test_crend(gl_string());
    _test_crend(gl_string("123"));
  }
  
  void _test_end(gl_string s) {
    const gl_string& cs = s;
    gl_string::iterator e = s.end();
    gl_string::const_iterator ce = cs.end();
    if (s.empty())
    {
      TS_ASSERT(e == s.begin());
      TS_ASSERT(ce == cs.begin());
    }
    TS_ASSERT_EQUALS(e - s.begin(), s.size());
    TS_ASSERT_EQUALS(ce - cs.begin(), cs.size());
  }

  void test_end() {
    _test_end(gl_string());
    _test_end(gl_string("123"));
  }
  
  void test_iterator_traits() { 
    typedef gl_string C;
    C::iterator ii1{}, ii2{};
    C::iterator ii4 = ii1;
    C::const_iterator cii{};
    TS_ASSERT ( ii1 == ii2 );
    TS_ASSERT ( ii1 == ii4 );
    TS_ASSERT ( ii1 == cii );
    TS_ASSERT ( !(ii1 != ii2 ));
    TS_ASSERT ( !(ii1 != cii ));
  }

  void _test_rbegin(gl_string s) {
    const gl_string& cs = s;
    gl_string::reverse_iterator b = s.rbegin();
    gl_string::const_reverse_iterator cb = cs.rbegin();
    if (!s.empty())
    {
      TS_ASSERT(*b == s.back());
    }
    TS_ASSERT(b == cb);
  }

  void test_rbegin() {
    _test_rbegin(gl_string());
    _test_rbegin(gl_string("123"));
  }
  

  void _test_rend(gl_string s) {
    const gl_string& cs = s;
    gl_string::reverse_iterator e = s.rend();
    gl_string::const_reverse_iterator ce = cs.rend();
    if (s.empty())
    {
      TS_ASSERT(e == s.rbegin());
      TS_ASSERT(ce == cs.rbegin());
    }
    TS_ASSERT_EQUALS(e - s.rbegin(), s.size());
    TS_ASSERT_EQUALS(ce - cs.rbegin(), cs.size());
  }

  void test_rend() {
    _test_rend(gl_string());
    _test_rend(gl_string("123"));
  }

  
};

BOOST_FIXTURE_TEST_SUITE(_test_string_iterators, test_string_iterators)
BOOST_AUTO_TEST_CASE(test_begin) {
  test_string_iterators::test_begin();
}
BOOST_AUTO_TEST_CASE(test_cbegin) {
  test_string_iterators::test_cbegin();
}
BOOST_AUTO_TEST_CASE(test_cend) {
  test_string_iterators::test_cend();
}
BOOST_AUTO_TEST_CASE(test_crbegin) {
  test_string_iterators::test_crbegin();
}
BOOST_AUTO_TEST_CASE(test_crend) {
  test_string_iterators::test_crend();
}
BOOST_AUTO_TEST_CASE(test_end) {
  test_string_iterators::test_end();
}
BOOST_AUTO_TEST_CASE(test_iterator_traits) {
  test_string_iterators::test_iterator_traits();
}
BOOST_AUTO_TEST_CASE(test_rbegin) {
  test_string_iterators::test_rbegin();
}
BOOST_AUTO_TEST_CASE(test_rend) {
  test_string_iterators::test_rend();
}
BOOST_AUTO_TEST_SUITE_END()
