/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <iostream>
#include <typeinfo>       // operator typeid
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <iterator>


#include <core/generics/gl_string.hpp>
#include <core/util/testing_utils.hpp>

using namespace turi;

struct test_string_find  {
 public:
  void _test_char_size(const gl_string& s, char c, size_t pos, size_t x) {
    TS_ASSERT(s.find(c, pos) == x);
    if (x != gl_string::npos)
      TS_ASSERT(pos <= x && x + 1 <= s.size());
  }


  void _test_char_size(const gl_string& s, char c, size_t x) {
    TS_ASSERT(s.find(c) == x);
    if (x != gl_string::npos)
      TS_ASSERT(0 <= x && x + 1 <= s.size());
  }

  void test_char() {
    _test_char_size(gl_string(""), 'c', 0, gl_string::npos);
    _test_char_size(gl_string(""), 'c', 1, gl_string::npos);
    _test_char_size(gl_string("abcde"), 'c', 0, 2);
    _test_char_size(gl_string("abcde"), 'c', 1, 2);
    _test_char_size(gl_string("abcde"), 'c', 2, 2);
    _test_char_size(gl_string("abcde"), 'c', 4, gl_string::npos);
    _test_char_size(gl_string("abcde"), 'c', 5, gl_string::npos);
    _test_char_size(gl_string("abcde"), 'c', 6, gl_string::npos);
    _test_char_size(gl_string("abcdeabcde"), 'c', 0, 2);
    _test_char_size(gl_string("abcdeabcde"), 'c', 1, 2);
    _test_char_size(gl_string("abcdeabcde"), 'c', 5, 7);
    _test_char_size(gl_string("abcdeabcde"), 'c', 9, gl_string::npos);
    _test_char_size(gl_string("abcdeabcde"), 'c', 10, gl_string::npos);
    _test_char_size(gl_string("abcdeabcde"), 'c', 11, gl_string::npos);
    _test_char_size(gl_string("abcdeabcdeabcdeabcde"), 'c', 0, 2);
    _test_char_size(gl_string("abcdeabcdeabcdeabcde"), 'c', 1, 2);
    _test_char_size(gl_string("abcdeabcdeabcdeabcde"), 'c', 10, 12);
    _test_char_size(gl_string("abcdeabcdeabcdeabcde"), 'c', 19, gl_string::npos);
    _test_char_size(gl_string("abcdeabcdeabcdeabcde"), 'c', 20, gl_string::npos);
    _test_char_size(gl_string("abcdeabcdeabcdeabcde"), 'c', 21, gl_string::npos);

    _test_char_size(gl_string(""), 'c', gl_string::npos);
    _test_char_size(gl_string("abcde"), 'c', 2);
    _test_char_size(gl_string("abcdeabcde"), 'c', 2);
    _test_char_size(gl_string("abcdeabcdeabcdeabcde"), 'c', 2);
  }



void
_test_string_size(const gl_string& s, const gl_string& str, size_t pos, size_t x)
{
    TS_ASSERT(s.find(str, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(pos <= x && x + str.size() <= s.size());
}


void
_test_string_size(const gl_string& s, const gl_string& str, size_t x)
{
    TS_ASSERT(s.find(str) == x);
    if (x != gl_string::npos)
        TS_ASSERT(0 <= x && x + str.size() <= s.size());
}


void test_string_size0()
{
    _test_string_size(gl_string(""), gl_string(""), 0, 0);
    _test_string_size(gl_string(""), gl_string("abcde"), 0, gl_string::npos);
    _test_string_size(gl_string(""), gl_string("abcdeabcde"), 0, gl_string::npos);
    _test_string_size(gl_string(""), gl_string("abcdeabcdeabcdeabcde"), 0, gl_string::npos);
    _test_string_size(gl_string(""), gl_string(""), 1, gl_string::npos);
    _test_string_size(gl_string(""), gl_string("abcde"), 1, gl_string::npos);
    _test_string_size(gl_string(""), gl_string("abcdeabcde"), 1, gl_string::npos);
    _test_string_size(gl_string(""), gl_string("abcdeabcdeabcdeabcde"), 1, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string(""), 0, 0);
    _test_string_size(gl_string("abcde"), gl_string("abcde"), 0, 0);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 0, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 0, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string(""), 1, 1);
    _test_string_size(gl_string("abcde"), gl_string("abcde"), 1, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 1, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 1, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string(""), 2, 2);
    _test_string_size(gl_string("abcde"), gl_string("abcde"), 2, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 2, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 2, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string(""), 4, 4);
    _test_string_size(gl_string("abcde"), gl_string("abcde"), 4, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 4, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 4, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string(""), 5, 5);
    _test_string_size(gl_string("abcde"), gl_string("abcde"), 5, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 5, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 5, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string(""), 6, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcde"), 6, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 6, gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 6, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string(""), 0, 0);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 0, 0);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 0, 0);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 0, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string(""), 1, 1);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 1, 5);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 1, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 1, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string(""), 5, 5);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 5, 5);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 5, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 5, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string(""), 9, 9);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 9, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 9, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 9, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string(""), 10, 10);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 10, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 10, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 10, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string(""), 11, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 11, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 11, gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 11, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 0, 0);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 0, 0);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 0, 0);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 0, 0);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 1, 1);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 1, 5);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 1, 5);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 1, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 10, 10);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 10, 10);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 10, 10);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 10, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 19, 19);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 19, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 19, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 19, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 20, 20);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 20, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 20, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 20, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 21, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 21, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 21, gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 21, gl_string::npos);
}


void test_string_size1()
{
    _test_string_size(gl_string(""), gl_string(""), 0);
    _test_string_size(gl_string(""), gl_string("abcde"), gl_string::npos);
    _test_string_size(gl_string(""), gl_string("abcdeabcde"), gl_string::npos);
    _test_string_size(gl_string(""), gl_string("abcdeabcdeabcdeabcde"), gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string(""), 0);
    _test_string_size(gl_string("abcde"), gl_string("abcde"), 0);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcde"), gl_string::npos);
    _test_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), gl_string::npos);
    _test_string_size(gl_string("abcdeabcde"), gl_string(""), 0);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 0);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 0);
    _test_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), gl_string::npos);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 0);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 0);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 0);
    _test_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 0);
}

  
void
_test_pointer_size(const gl_string& s, const char* str, size_t pos,
     size_t x)
{
    TS_ASSERT(s.find(str, pos) == x);
    if (x != gl_string::npos)
    {
        size_t n = std::strlen(str);
        TS_ASSERT(pos <= x && x + n <= s.size());
    }
}


void
_test_pointer_size(const gl_string& s, const char* str, size_t x)
{
    TS_ASSERT(s.find(str) == x);
    if (x != gl_string::npos)
    {
      size_t n = std::strlen(str);
        TS_ASSERT(0 <= x && x + n <= s.size());
    }
}


void test_pointer_size0()
{
    _test_pointer_size(gl_string(""), "", 0, 0);
    _test_pointer_size(gl_string(""), "abcde", 0, gl_string::npos);
    _test_pointer_size(gl_string(""), "abcdeabcde", 0, gl_string::npos);
    _test_pointer_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, gl_string::npos);
    _test_pointer_size(gl_string(""), "", 1, gl_string::npos);
    _test_pointer_size(gl_string(""), "abcde", 1, gl_string::npos);
    _test_pointer_size(gl_string(""), "abcdeabcde", 1, gl_string::npos);
    _test_pointer_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "", 0, 0);
    _test_pointer_size(gl_string("abcde"), "abcde", 0, 0);
    _test_pointer_size(gl_string("abcde"), "abcdeabcde", 0, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "", 1, 1);
    _test_pointer_size(gl_string("abcde"), "abcde", 1, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcde", 1, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "", 2, 2);
    _test_pointer_size(gl_string("abcde"), "abcde", 2, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcde", 2, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "", 4, 4);
    _test_pointer_size(gl_string("abcde"), "abcde", 4, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcde", 4, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "", 5, 5);
    _test_pointer_size(gl_string("abcde"), "abcde", 5, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcde", 5, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "", 6, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcde", 6, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcde", 6, gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "", 0, 0);
    _test_pointer_size(gl_string("abcdeabcde"), "abcde", 0, 0);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 0);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "", 1, 1);
    _test_pointer_size(gl_string("abcdeabcde"), "abcde", 1, 5);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 1, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "", 5, 5);
    _test_pointer_size(gl_string("abcdeabcde"), "abcde", 5, 5);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 5, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "", 9, 9);
    _test_pointer_size(gl_string("abcdeabcde"), "abcde", 9, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 9, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "", 10, 10);
    _test_pointer_size(gl_string("abcdeabcde"), "abcde", 10, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 10, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "", 11, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "abcde", 11, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 11, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 0, 0);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 0);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 0);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 0);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 1, 1);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 5);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 5);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 10, 10);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 10);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 10);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 19, 19);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 20, 20);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 21, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, gl_string::npos);
}


void test_pointer_size1()
{
    _test_pointer_size(gl_string(""), "", 0);
    _test_pointer_size(gl_string(""), "abcde", gl_string::npos);
    _test_pointer_size(gl_string(""), "abcdeabcde", gl_string::npos);
    _test_pointer_size(gl_string(""), "abcdeabcdeabcdeabcde", gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "", 0);
    _test_pointer_size(gl_string("abcde"), "abcde", 0);
    _test_pointer_size(gl_string("abcde"), "abcdeabcde", gl_string::npos);
    _test_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcde"), "", 0);
    _test_pointer_size(gl_string("abcdeabcde"), "abcde", 0);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 0);
    _test_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", gl_string::npos);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 0);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0);
    _test_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0);
}

  

void
_test_pointer_size_size(const gl_string& s, const char* str, size_t pos,
     size_t n, size_t x)
{
    TS_ASSERT(s.find(str, pos, n) == x);
    if (x != gl_string::npos)
        TS_ASSERT(pos <= x && x + n <= s.size());
}


void test_pointer_size_size0()
{
    _test_pointer_size_size(gl_string(""), "", 0, 0, 0);
    _test_pointer_size_size(gl_string(""), "abcde", 0, 0, 0);
    _test_pointer_size_size(gl_string(""), "abcde", 0, 1, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcde", 0, 2, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcde", 0, 4, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcde", 0, 5, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcde", 0, 0, 0);
    _test_pointer_size_size(gl_string(""), "abcdeabcde", 0, 1, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcde", 0, 5, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcde", 0, 9, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcde", 0, 10, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, 0, 0);
    _test_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, 1, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, 10, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, 19, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, 20, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "", 1, 0, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcde", 1, 0, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcde", 1, 1, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcde", 1, 2, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcde", 1, 4, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcde", 1, 5, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcde", 1, 0, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcde", 1, 1, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcde", 1, 5, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcde", 1, 9, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcde", 1, 10, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, 0, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, 1, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, 10, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, 19, gl_string::npos);
    _test_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 0, 1, 0);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 0, 2, 0);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 0, 4, 0);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 0, 5, 0);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 0, 1, 0);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 0, 5, 0);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 0, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 0, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, 1, 0);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 1, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 1, 2, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 1, 4, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 1, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 1, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 1, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 1, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 1, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "", 2, 0, 2);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 2, 0, 2);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 2, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 2, 2, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 2, 4, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 2, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 2, 0, 2);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 2, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 2, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 2, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 2, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, 0, 2);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "", 4, 0, 4);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 4, 0, 4);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 4, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 4, 2, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 4, 4, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 4, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 4, 0, 4);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 4, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 4, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 4, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 4, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, 0, 4);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "", 5, 0, 5);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 5, 0, 5);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 5, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 5, 2, gl_string::npos);
}


void test_pointer_size_size1()
{
    _test_pointer_size_size(gl_string("abcde"), "abcde", 5, 4, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 5, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 5, 0, 5);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 5, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 5, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 5, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 5, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, 0, 5);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "", 6, 0, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 6, 0, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 6, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 6, 2, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 6, 4, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcde", 6, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 6, 0, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 6, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 6, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 6, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcde", 6, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, 0, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 0, 1, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 0, 2, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 0, 4, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 0, 5, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 1, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 5, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 9, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 10, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, 1, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, 10, 0);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 1, 1, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 1, 2, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 1, 4, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 1, 5, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 1, 1, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 1, 5, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 1, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 1, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, 1, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "", 5, 0, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 5, 0, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 5, 1, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 5, 2, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 5, 4, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 5, 5, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 5, 0, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 5, 1, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 5, 5, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 5, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 5, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, 0, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, 1, 5);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "", 9, 0, 9);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 9, 0, 9);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 9, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 9, 2, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 9, 4, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 9, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 9, 0, 9);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 9, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 9, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 9, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 9, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, 0, 9);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "", 10, 0, 10);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 10, 0, 10);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 10, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 10, 2, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 10, 4, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 10, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 10, 0, 10);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 10, 1, gl_string::npos);
}


void test_pointer_size_size2()
{
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 10, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 10, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 10, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, 0, 10);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "", 11, 0, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 11, 0, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 11, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 11, 2, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 11, 4, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcde", 11, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 11, 0, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 11, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 11, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 11, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 11, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, 0, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 1, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 2, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 4, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 5, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 1, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 5, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 9, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 10, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 0, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 1, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 10, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 19, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 20, 0);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 1, 5);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 2, 5);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 4, 5);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 5, 5);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 1, 5);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 5, 5);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 9, 5);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 10, 5);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, 0, 1);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, 1, 5);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, 10, 5);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 10, 0, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 0, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 1, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 2, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 4, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 5, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 0, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 1, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 5, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 9, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 10, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, 0, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, 1, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, 10, 10);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 19, 0, 19);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, 0, 19);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, 2, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, 4, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, 0, 19);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, 0, 19);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, 19, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, 20, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 20, 0, 20);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, 0, 20);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, 2, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, 4, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, 0, 20);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, 1, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, 5, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, 9, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, 10, gl_string::npos);
    _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, 0, 20);
}


void test_pointer_size_size3()
{
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, 1, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, 10, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, 19, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, 20, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 21, 0, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, 0, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, 1, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, 2, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, 4, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, 5, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, 0, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, 1, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, 5, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, 9, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, 10, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, 0, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, 1, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, 10, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, 19, gl_string::npos);
  _test_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, 20, gl_string::npos);
}

  
void _test_string_find_first_not_of_char_size(const gl_string& s, char c, size_t pos, size_t x) {
  TS_ASSERT(s.find_first_not_of(c, pos) == x);
  if (x != gl_string::npos)
    TS_ASSERT(pos <= x && x < s.size());
}


void _test_string_find_first_not_of_char_size(const gl_string& s, char c, size_t x)
{
  TS_ASSERT(s.find_first_not_of(c) == x);
  if (x != gl_string::npos)
    TS_ASSERT(x < s.size());
}

void test_string_find_first_not_of_char_size() { 
  _test_string_find_first_not_of_char_size(gl_string(""), 'q', 0, gl_string::npos);
  _test_string_find_first_not_of_char_size(gl_string(""), 'q', 1, gl_string::npos);
  _test_string_find_first_not_of_char_size(gl_string("kitcj"), 'q', 0, 0);
  _test_string_find_first_not_of_char_size(gl_string("qkamf"), 'q', 1, 1);
  _test_string_find_first_not_of_char_size(gl_string("nhmko"), 'q', 2, 2);
  _test_string_find_first_not_of_char_size(gl_string("tpsaf"), 'q', 4, 4);
  _test_string_find_first_not_of_char_size(gl_string("lahfb"), 'q', 5, gl_string::npos);
  _test_string_find_first_not_of_char_size(gl_string("irkhs"), 'q', 6, gl_string::npos);
  _test_string_find_first_not_of_char_size(gl_string("gmfhdaipsr"), 'q', 0, 0);
  _test_string_find_first_not_of_char_size(gl_string("kantesmpgj"), 'q', 1, 1);
  _test_string_find_first_not_of_char_size(gl_string("odaftiegpm"), 'q', 5, 5);
  _test_string_find_first_not_of_char_size(gl_string("oknlrstdpi"), 'q', 9, 9);
  _test_string_find_first_not_of_char_size(gl_string("eolhfgpjqk"), 'q', 10, gl_string::npos);
  _test_string_find_first_not_of_char_size(gl_string("pcdrofikas"), 'q', 11, gl_string::npos);
  _test_string_find_first_not_of_char_size(gl_string("nbatdlmekrgcfqsophij"), 'q', 0, 0);
  _test_string_find_first_not_of_char_size(gl_string("bnrpehidofmqtcksjgla"), 'q', 1, 1);
  _test_string_find_first_not_of_char_size(gl_string("jdmciepkaqgotsrfnhlb"), 'q', 10, 10);
  _test_string_find_first_not_of_char_size(gl_string("jtdaefblsokrmhpgcnqi"), 'q', 19, 19);
  _test_string_find_first_not_of_char_size(gl_string("hkbgspofltajcnedqmri"), 'q', 20, gl_string::npos);
  _test_string_find_first_not_of_char_size(gl_string("oselktgbcapndfjihrmq"), 'q', 21, gl_string::npos);

  _test_string_find_first_not_of_char_size(gl_string(""), 'q', gl_string::npos);
  _test_string_find_first_not_of_char_size(gl_string("q"), 'q', gl_string::npos);
  _test_string_find_first_not_of_char_size(gl_string("qqq"), 'q', gl_string::npos);
  _test_string_find_first_not_of_char_size(gl_string("csope"), 'q', 0);
  _test_string_find_first_not_of_char_size(gl_string("gfsmthlkon"), 'q', 0);
  _test_string_find_first_not_of_char_size(gl_string("laenfsbridchgotmkqpj"), 'q', 0);
}


void
_test_string_find_first_not_of_pointer_size(const gl_string& s, const char* str, size_t pos,
     size_t x)
{
    TS_ASSERT(s.find_first_not_of(str, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(pos <= x && x < s.size());
}


void
_test_string_find_first_not_of_pointer_size(const gl_string& s, const char* str, size_t x)
{
    TS_ASSERT(s.find_first_not_of(str) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x < s.size());
}


void test_string_find_first_not_of_pointer_size0()
{
    _test_string_find_first_not_of_pointer_size(gl_string(""), "", 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string(""), "laenf", 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string(""), "pqlnkmbdjo", 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string(""), "qkamfogpnljdcshbreti", 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string(""), "", 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string(""), "bjaht", 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string(""), "hjlcmgpket", 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string(""), "htaobedqikfplcgjsmrn", 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("fodgq"), "", 0, 0);
    _test_string_find_first_not_of_pointer_size(gl_string("qanej"), "dfkap", 0, 0);
    _test_string_find_first_not_of_pointer_size(gl_string("clbao"), "ihqrfebgad", 0, 0);
    _test_string_find_first_not_of_pointer_size(gl_string("mekdn"), "ngtjfcalbseiqrphmkdo", 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("srdfq"), "", 1, 1);
    _test_string_find_first_not_of_pointer_size(gl_string("oemth"), "ikcrq", 1, 1);
    _test_string_find_first_not_of_pointer_size(gl_string("cdaih"), "dmajblfhsg", 1, 3);
    _test_string_find_first_not_of_pointer_size(gl_string("qohtk"), "oqftjhdmkgsblacenirp", 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("cshmd"), "", 2, 2);
    _test_string_find_first_not_of_pointer_size(gl_string("lhcdo"), "oebqi", 2, 2);
    _test_string_find_first_not_of_pointer_size(gl_string("qnsoh"), "kojhpmbsfe", 2, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("pkrof"), "acbsjqogpltdkhinfrem", 2, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("fmtsp"), "", 4, 4);
    _test_string_find_first_not_of_pointer_size(gl_string("khbpm"), "aobjd", 4, 4);
    _test_string_find_first_not_of_pointer_size(gl_string("pbsji"), "pcbahntsje", 4, 4);
    _test_string_find_first_not_of_pointer_size(gl_string("mprdj"), "fhepcrntkoagbmldqijs", 4, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("eqmpa"), "", 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("omigs"), "kocgb", 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("onmje"), "fbslrjiqkm", 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("oqmrj"), "jeidpcmalhfnqbgtrsko", 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("schfa"), "", 6, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("igdsc"), "qngpd", 6, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("brqgo"), "rodhqklgmb", 6, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("tnrph"), "thdjgafrlbkoiqcspmne", 6, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("hcjitbfapl"), "", 0, 0);
    _test_string_find_first_not_of_pointer_size(gl_string("daiprenocl"), "ashjd", 0, 2);
    _test_string_find_first_not_of_pointer_size(gl_string("litpcfdghe"), "mgojkldsqh", 0, 1);
    _test_string_find_first_not_of_pointer_size(gl_string("aidjksrolc"), "imqnaghkfrdtlopbjesc", 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("qpghtfbaji"), "", 1, 1);
    _test_string_find_first_not_of_pointer_size(gl_string("gfshlcmdjr"), "nadkh", 1, 1);
    _test_string_find_first_not_of_pointer_size(gl_string("nkodajteqp"), "ofdrqmkebl", 1, 4);
    _test_string_find_first_not_of_pointer_size(gl_string("gbmetiprqd"), "bdfjqgatlksriohemnpc", 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("crnklpmegd"), "", 5, 5);
    _test_string_find_first_not_of_pointer_size(gl_string("jsbtafedoc"), "prqgn", 5, 5);
    _test_string_find_first_not_of_pointer_size(gl_string("qnmodrtkeb"), "pejafmnokr", 5, 6);
    _test_string_find_first_not_of_pointer_size(gl_string("cpebqsfmnj"), "odnqkgijrhabfmcestlp", 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("lmofqdhpki"), "", 9, 9);
    _test_string_find_first_not_of_pointer_size(gl_string("hnefkqimca"), "rtjpa", 9, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("drtasbgmfp"), "ktsrmnqagd", 9, 9);
    _test_string_find_first_not_of_pointer_size(gl_string("lsaijeqhtr"), "rtdhgcisbnmoaqkfpjle", 9, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("elgofjmbrq"), "", 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("mjqdgalkpc"), "dplqa", 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("kthqnfcerm"), "dkacjoptns", 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("dfsjhanorc"), "hqfimtrgnbekpdcsjalo", 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("eqsgalomhb"), "", 11, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("akiteljmoh"), "lofbc", 11, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("hlbdfreqjo"), "astoegbfpn", 11, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("taqobhlerg"), "pdgreqomsncafklhtibj", 11, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("snafbdlghrjkpqtoceim"), "", 0, 0);
    _test_string_find_first_not_of_pointer_size(gl_string("aemtbrgcklhndjisfpoq"), "lbtqd", 0, 0);
    _test_string_find_first_not_of_pointer_size(gl_string("pnracgfkjdiholtbqsem"), "tboimldpjh", 0, 1);
    _test_string_find_first_not_of_pointer_size(gl_string("dicfltehbsgrmojnpkaq"), "slcerthdaiqjfnobgkpm", 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("jlnkraeodhcspfgbqitm"), "", 1, 1);
    _test_string_find_first_not_of_pointer_size(gl_string("lhosrngtmfjikbqpcade"), "aqibs", 1, 1);
    _test_string_find_first_not_of_pointer_size(gl_string("rbtaqjhgkneisldpmfoc"), "gtfblmqinc", 1, 3);
    _test_string_find_first_not_of_pointer_size(gl_string("gpifsqlrdkbonjtmheca"), "mkqpbtdalgniorhfescj", 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("hdpkobnsalmcfijregtq"), "", 10, 10);
    _test_string_find_first_not_of_pointer_size(gl_string("jtlshdgqaiprkbcoenfm"), "pblas", 10, 11);
    _test_string_find_first_not_of_pointer_size(gl_string("fkdrbqltsgmcoiphneaj"), "arosdhcfme", 10, 13);
    _test_string_find_first_not_of_pointer_size(gl_string("crsplifgtqedjohnabmk"), "blkhjeogicatqfnpdmsr", 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("niptglfbosehkamrdqcj"), "", 19, 19);
    _test_string_find_first_not_of_pointer_size(gl_string("copqdhstbingamjfkler"), "djkqc", 19, 19);
    _test_string_find_first_not_of_pointer_size(gl_string("mrtaefilpdsgocnhqbjk"), "lgokshjtpb", 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("kojatdhlcmigpbfrqnes"), "bqjhtkfepimcnsgrlado", 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("eaintpchlqsbdgrkjofm"), "", 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("gjnhidfsepkrtaqbmclo"), "nocfa", 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("spocfaktqdbiejlhngmr"), "bgtajmiedc", 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("rphmlekgfscndtaobiqj"), "lsckfnqgdahejiopbtmr", 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("liatsqdoegkmfcnbhrpj"), "", 21, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("binjagtfldkrspcomqeh"), "gfsrt", 21, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("latkmisecnorjbfhqpdg"), "pfsocbhjtm", 21, gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("lecfratdjkhnsmqpoigb"), "tpflmdnoicjgkberhqsa", 21, gl_string::npos);
}


void test_string_find_first_not_of_pointer_size1()
{
    _test_string_find_first_not_of_pointer_size(gl_string(""), "", gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string(""), "laenf", gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string(""), "pqlnkmbdjo", gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string(""), "qkamfogpnljdcshbreti", gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("nhmko"), "", 0);
    _test_string_find_first_not_of_pointer_size(gl_string("lahfb"), "irkhs", 0);
    _test_string_find_first_not_of_pointer_size(gl_string("gmfhd"), "kantesmpgj", 2);
    _test_string_find_first_not_of_pointer_size(gl_string("odaft"), "oknlrstdpiqmjbaghcfe", gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("eolhfgpjqk"), "", 0);
    _test_string_find_first_not_of_pointer_size(gl_string("nbatdlmekr"), "bnrpe", 2);
    _test_string_find_first_not_of_pointer_size(gl_string("jdmciepkaq"), "jtdaefblso", 2);
    _test_string_find_first_not_of_pointer_size(gl_string("hkbgspoflt"), "oselktgbcapndfjihrmq", gl_string::npos);
    _test_string_find_first_not_of_pointer_size(gl_string("gprdcokbnjhlsfmtieqa"), "", 0);
    _test_string_find_first_not_of_pointer_size(gl_string("qjghlnftcaismkropdeb"), "bjaht", 0);
    _test_string_find_first_not_of_pointer_size(gl_string("pnalfrdtkqcmojiesbhg"), "hjlcmgpket", 1);
    _test_string_find_first_not_of_pointer_size(gl_string("pniotcfrhqsmgdkjbael"), "htaobedqikfplcgjsmrn", gl_string::npos);
}

void
_test_string_find_first_not_of_pointer_size_size(const gl_string& s, const char* str, size_t pos,
     size_t n, size_t x)
{
    TS_ASSERT(s.find_first_not_of(str, pos, n) == x);
    if (x != gl_string::npos)
        TS_ASSERT(pos <= x && x < s.size());
}


void test_string_find_first_not_of_pointer_size_size0()
{
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "", 0, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "irkhs", 0, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "kante", 0, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "oknlr", 0, 2, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "pcdro", 0, 4, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "bnrpe", 0, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "jtdaefblso", 0, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "oselktgbca", 0, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "eqgaplhckj", 0, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "bjahtcmnlp", 0, 9, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "hjlcmgpket", 0, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "htaobedqikfplcgjsmrn", 0, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "hpqiarojkcdlsgnmfetb", 0, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "dfkaprhjloqetcsimnbg", 0, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "ihqrfebgadntlpmjksoc", 0, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "ngtjfcalbseiqrphmkdo", 0, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "", 1, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "lbtqd", 1, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "tboim", 1, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "slcer", 1, 2, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "cbjfs", 1, 4, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "aqibs", 1, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "gtfblmqinc", 1, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "mkqpbtdalg", 1, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "kphatlimcd", 1, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "pblasqogic", 1, 9, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "arosdhcfme", 1, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "blkhjeogicatqfnpdmsr", 1, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "bmhineprjcoadgstflqk", 1, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "djkqcmetslnghpbarfoi", 1, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "lgokshjtpbemarcdqnfi", 1, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string(""), "bqjhtkfepimcnsgrlado", 1, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("eaint"), "", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("binja"), "gfsrt", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("latkm"), "pfsoc", 0, 1, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("lecfr"), "tpflm", 0, 2, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("eqkst"), "sgkec", 0, 4, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("cdafr"), "romds", 0, 5, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("prbhe"), "qhjistlgmr", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("lbisk"), "pedfirsglo", 0, 1, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hrlpd"), "aqcoslgrmk", 0, 5, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ehmja"), "dabckmepqj", 0, 9, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mhqgd"), "pqscrjthli", 0, 10, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("tgklq"), "kfphdcsjqmobliagtren", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bocjs"), "rokpefncljibsdhqtagm", 0, 1, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("grbsd"), "afionmkphlebtcjqsgrd", 0, 10, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ofjqr"), "aenmqplidhkofrjbctsg", 0, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("btlfi"), "osjmbtcadhiklegrpqnf", 0, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("clrgb"), "", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("tjmek"), "osmia", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bgstp"), "ckonl", 1, 1, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hstrk"), "ilcaj", 1, 2, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("kmspj"), "lasiq", 1, 4, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("tjboh"), "kfqmr", 1, 5, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ilbcj"), "klnitfaobg", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jkngf"), "gjhmdlqikp", 1, 1, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("gfcql"), "skbgtahqej", 1, 5, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dqtlg"), "bjsdgtlpkf", 1, 9, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bthpg"), "bjgfmnlkio", 1, 10, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dgsnq"), "lbhepotfsjdqigcnamkr", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rmfhp"), "tebangckmpsrqdlfojhi", 1, 1, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jfdam"), "joflqbdkhtegimscpanr", 1, 10, 3);
    _test_string_find_first_not_of_pointer_size_size(gl_string("edapb"), "adpmcohetfbsrjinlqkg", 1, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("brfsm"), "iacldqjpfnogbsrhmetk", 1, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ndrhl"), "", 2, 0, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mrecp"), "otkgb", 2, 0, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("qlasf"), "cqsjl", 2, 1, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("smaqd"), "dpifl", 2, 2, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hjeni"), "oapht", 2, 4, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ocmfj"), "cifts", 2, 5, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hmftq"), "nmsckbgalo", 2, 0, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fklad"), "tpksqhamle", 2, 1, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dirnm"), "tpdrchmkji", 2, 5, 3);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hrgdc"), "ijagfkblst", 2, 9, 3);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ifakg"), "kpocsignjb", 2, 10, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ebrgd"), "pecqtkjsnbdrialgmohf", 2, 0, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rcjml"), "aiortphfcmkjebgsndql", 2, 1, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("peqmt"), "sdbkeamglhipojqftrcn", 2, 10, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("frehn"), "ljqncehgmfktroapidbs", 2, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("tqolf"), "rtcfodilamkbenjghqps", 2, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("cjgao"), "", 4, 0, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("kjplq"), "mabns", 4, 0, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("herni"), "bdnrp", 4, 1, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("tadrb"), "scidp", 4, 2, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pkfeo"), "agbjl", 4, 4, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hoser"), "jfmpr", 4, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("kgrsp"), "rbpefghsmj", 4, 0, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pgejb"), "apsfntdoqc", 4, 1, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("thlnq"), "ndkjeisgcl", 4, 5, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("nbmit"), "rnfpqatdeo", 4, 9, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jgmib"), "bntjlqrfik", 4, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ncrfj"), "kcrtmpolnaqejghsfdbi", 4, 0, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ncsik"), "lobheanpkmqidsrtcfgj", 4, 1, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("sgbfh"), "athdkljcnreqbgpmisof", 4, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dktbn"), "qkdmjialrscpbhefgont", 4, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fthqm"), "dmasojntqleribkgfchp", 4, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("klopi"), "", 5, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dajhn"), "psthd", 5, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jbgno"), "rpmjd", 5, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hkjae"), "dfsmk", 5, 2, gl_string::npos);
}


void test_string_find_first_not_of_pointer_size_size1()
{
    _test_string_find_first_not_of_pointer_size_size(gl_string("gbhqo"), "skqne", 5, 4, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ktdor"), "kipnf", 5, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ldprn"), "hmrnqdgifl", 5, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("egmjk"), "fsmjcdairn", 5, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("armql"), "pcdgltbrfj", 5, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("cdhjo"), "aekfctpirg", 5, 9, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jcons"), "ledihrsgpf", 5, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("cbrkp"), "mqcklahsbtirgopefndj", 5, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fhgna"), "kmlthaoqgecrnpdbjfis", 5, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ejfcd"), "sfhbamcdptojlkrenqgi", 5, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("kqjhe"), "pbniofmcedrkhlstgaqj", 5, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pbdjl"), "mongjratcskbhqiepfdl", 5, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("gajqn"), "", 6, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("stedk"), "hrnat", 6, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("tjkaf"), "gsqdt", 6, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dthpe"), "bspkd", 6, 2, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("klhde"), "ohcmb", 6, 4, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bhlki"), "heatr", 6, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("lqmoh"), "pmblckedfn", 6, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mtqin"), "aceqmsrbik", 6, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dpqbr"), "lmbtdehjrn", 6, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("kdhmo"), "teqmcrlgib", 6, 9, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jblqp"), "njolbmspac", 6, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("qmjgl"), "pofnhidklamecrbqjgst", 6, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rothp"), "jbhckmtgrqnosafedpli", 6, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ghknq"), "dobntpmqklicsahgjerf", 6, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("eopfi"), "tpdshainjkbfoemlrgcq", 6, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dsnmg"), "oldpfgeakrnitscbjmqh", 6, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jnkrfhotgl"), "", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dltjfngbko"), "rqegt", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bmjlpkiqde"), "dashm", 0, 1, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("skrflobnqm"), "jqirk", 0, 2, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jkpldtshrm"), "rckeg", 0, 4, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ghasdbnjqo"), "jscie", 0, 5, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("igrkhpbqjt"), "efsphndliq", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ikthdgcamf"), "gdicosleja", 0, 1, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pcofgeniam"), "qcpjibosfl", 0, 5, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rlfjgesqhc"), "lrhmefnjcq", 0, 9, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("itphbqsker"), "dtablcrseo", 0, 10, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("skjafcirqm"), "apckjsftedbhgomrnilq", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("tcqomarsfd"), "pcbrgflehjtiadnsokqm", 0, 1, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rocfeldqpk"), "nsiadegjklhobrmtqcpf", 0, 10, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("cfpegndlkt"), "cpmajdqnolikhgsbretf", 0, 19, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fqbtnkeasj"), "jcflkntmgiqrphdosaeb", 0, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("shbcqnmoar"), "", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bdoshlmfin"), "ontrs", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("khfrebnsgq"), "pfkna", 1, 1, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("getcrsaoji"), "ekosa", 1, 2, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fjiknedcpq"), "anqhk", 1, 4, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("tkejgnafrm"), "jekca", 1, 5, 4);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jnakolqrde"), "ikemsjgacf", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("lcjptsmgbe"), "arolgsjkhm", 1, 1, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("itfsmcjorl"), "oftkbldhre", 1, 5, 3);
    _test_string_find_first_not_of_pointer_size_size(gl_string("omchkfrjea"), "gbkqdoeftl", 1, 9, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("cigfqkated"), "sqcflrgtim", 1, 10, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("tscenjikml"), "fmhbkislrjdpanogqcet", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("qcpaemsinf"), "rnioadktqlgpbcjsmhef", 1, 1, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("gltkojeipd"), "oakgtnldpsefihqmjcbr", 1, 10, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("qistfrgnmp"), "gbnaelosidmcjqktfhpr", 1, 19, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bdnpfcqaem"), "akbripjhlosndcmqgfet", 1, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ectnhskflp"), "", 5, 0, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fgtianblpq"), "pijag", 5, 0, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mfeqklirnh"), "jrckd", 5, 1, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("astedncjhk"), "qcloh", 5, 2, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fhlqgcajbr"), "thlmp", 5, 4, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("epfhocmdng"), "qidmo", 5, 5, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("apcnsibger"), "lnegpsjqrd", 5, 0, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("aqkocrbign"), "rjqdablmfs", 5, 1, 6);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ijsmdtqgce"), "enkgpbsjaq", 5, 5, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("clobgsrken"), "kdsgoaijfh", 5, 9, 6);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jbhcfposld"), "trfqgmckbe", 5, 10, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("oqnpblhide"), "igetsracjfkdnpoblhqm", 5, 0, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("lroeasctif"), "nqctfaogirshlekbdjpm", 5, 1, 5);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bpjlgmiedh"), "csehfgomljdqinbartkp", 5, 10, 6);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pamkeoidrj"), "qahoegcmplkfsjbdnitr", 5, 19, 8);
    _test_string_find_first_not_of_pointer_size_size(gl_string("espogqbthk"), "dpteiajrqmsognhlfbkc", 5, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("shoiedtcjb"), "", 9, 0, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ebcinjgads"), "tqbnh", 9, 0, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dqmregkcfl"), "akmle", 9, 1, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ngcrieqajf"), "iqfkm", 9, 2, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("qosmilgnjb"), "tqjsr", 9, 4, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ikabsjtdfl"), "jplqg", 9, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ersmicafdh"), "oilnrbcgtj", 9, 0, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fdnplotmgh"), "morkglpesn", 9, 1, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fdbicojerm"), "dmicerngat", 9, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mbtafndjcq"), "radgeskbtc", 9, 9, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mlenkpfdtc"), "ljikprsmqo", 9, 10, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ahlcifdqgs"), "trqihkcgsjamfdbolnpe", 9, 0, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bgjemaltks"), "lqmthbsrekajgnofcipd", 9, 1, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pdhslbqrfc"), "jtalmedribkgqsopcnfh", 9, 10, 9);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dirhtsnjkc"), "spqfoiclmtagejbndkrh", 9, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dlroktbcja"), "nmotklspigjrdhcfaebq", 9, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ncjpmaekbs"), "", 10, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hlbosgmrak"), "hpmsd", 10, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pqfhsgilen"), "qnpor", 10, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("gqtjsbdckh"), "otdma", 10, 2, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("cfkqpjlegi"), "efhjg", 10, 4, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("beanrfodgj"), "odpte", 10, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("adtkqpbjfi"), "bctdgfmolr", 10, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("iomkfthagj"), "oaklidrbqg", 10, 1, gl_string::npos);
}


void test_string_find_first_not_of_pointer_size_size2()
{
    _test_string_find_first_not_of_pointer_size_size(gl_string("sdpcilonqj"), "dnjfsagktr", 10, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("gtfbdkqeml"), "nejaktmiqg", 10, 9, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bmeqgcdorj"), "pjqonlebsf", 10, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("etqlcanmob"), "dshmnbtolcjepgaikfqr", 10, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("roqmkbdtia"), "iogfhpabtjkqlrnemcds", 10, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("kadsithljf"), "ngridfabjsecpqltkmoh", 10, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("sgtkpbfdmh"), "athmknplcgofrqejsdib", 10, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("qgmetnabkl"), "ldobhmqcafnjtkeisgrp", 10, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("cqjohampgd"), "", 11, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hobitmpsan"), "aocjb", 11, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("tjehkpsalm"), "jbrnk", 11, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ngfbojitcl"), "tqedg", 11, 2, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rcfkdbhgjo"), "nqskp", 11, 4, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("qghptonrea"), "eaqkl", 11, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hnprfgqjdl"), "reaoicljqm", 11, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hlmgabenti"), "lsftgajqpm", 11, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ofcjanmrbs"), "rlpfogmits", 11, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jqedtkornm"), "shkncmiaqj", 11, 9, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rfedlasjmg"), "fpnatrhqgs", 11, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("talpqjsgkm"), "sjclemqhnpdbgikarfot", 11, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("lrkcbtqpie"), "otcmedjikgsfnqbrhpla", 11, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("cipogdskjf"), "bonsaefdqiprkhlgtjcm", 11, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("nqedcojahi"), "egpscmahijlfnkrodqtb", 11, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hefnrkmctj"), "kmqbfepjthgilscrndoa", 11, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("atqirnmekfjolhpdsgcb"), "", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("echfkmlpribjnqsaogtd"), "prboq", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("qnhiftdgcleajbpkrosm"), "fjcqh", 0, 1, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("chamfknorbedjitgslpq"), "fmosa", 0, 2, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("njhqpibfmtlkaecdrgso"), "qdbok", 0, 4, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ebnghfsqkprmdcljoiat"), "amslg", 0, 5, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("letjomsgihfrpqbkancd"), "smpltjneqb", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("nblgoipcrqeaktshjdmf"), "flitskrnge", 0, 1, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("cehkbngtjoiflqapsmrd"), "pgqihmlbef", 0, 5, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mignapfoklbhcqjetdrs"), "cfpdqjtgsb", 0, 9, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ceatbhlsqjgpnokfrmdi"), "htpsiaflom", 0, 10, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ocihkjgrdelpfnmastqb"), "kpjfiaceghsrdtlbnomq", 0, 0, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("noelgschdtbrjfmiqkap"), "qhtbomidljgafneksprc", 0, 1, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dkclqfombepritjnghas"), "nhtjobkcefldimpsaqgr", 0, 10, 0);
    _test_string_find_first_not_of_pointer_size_size(gl_string("miklnresdgbhqcojftap"), "prabcjfqnoeskilmtgdh", 0, 19, 11);
    _test_string_find_first_not_of_pointer_size_size(gl_string("htbcigojaqmdkfrnlsep"), "dtrgmchilkasqoebfpjn", 0, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("febhmqtjanokscdirpgl"), "", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("loakbsqjpcrdhftniegm"), "sqome", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("reagphsqflbitdcjmkno"), "smfte", 1, 1, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jitlfrqemsdhkopncabg"), "ciboh", 1, 2, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mhtaepscdnrjqgbkifol"), "haois", 1, 4, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("tocesrfmnglpbjihqadk"), "abfki", 1, 5, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("lpfmctjrhdagneskbqoi"), "frdkocntmq", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("lsmqaepkdhncirbtjfgo"), "oasbpedlnr", 1, 1, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("epoiqmtldrabnkjhcfsg"), "kltqmhgand", 1, 5, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("emgasrilpknqojhtbdcf"), "gdtfjchpmr", 1, 9, 3);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hnfiagdpcklrjetqbsom"), "ponmcqblet", 1, 10, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("nsdfebgajhmtricpoklq"), "sgphqdnofeiklatbcmjr", 1, 0, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("atjgfsdlpobmeiqhncrk"), "ljqprsmigtfoneadckbh", 1, 1, 1);
    _test_string_find_first_not_of_pointer_size_size(gl_string("sitodfgnrejlahcbmqkp"), "ligeojhafnkmrcsqtbdp", 1, 10, 2);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fraghmbiceknltjpqosd"), "lsimqfnjarbopedkhcgt", 1, 19, 13);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pmafenlhqtdbkirjsogc"), "abedmfjlghniorcqptks", 1, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pihgmoeqtnakrjslcbfd"), "", 10, 0, 10);
    _test_string_find_first_not_of_pointer_size_size(gl_string("gjdkeprctqblnhiafsom"), "hqtoa", 10, 0, 10);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mkpnblfdsahrcqijteog"), "cahif", 10, 1, 10);
    _test_string_find_first_not_of_pointer_size_size(gl_string("gckarqnelodfjhmbptis"), "kehis", 10, 2, 10);
    _test_string_find_first_not_of_pointer_size_size(gl_string("gqpskidtbclomahnrjfe"), "kdlmh", 10, 4, 11);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pkldjsqrfgitbhmaecno"), "paeql", 10, 5, 10);
    _test_string_find_first_not_of_pointer_size_size(gl_string("aftsijrbeklnmcdqhgop"), "aghoqiefnb", 10, 0, 10);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mtlgdrhafjkbiepqnsoc"), "jrbqaikpdo", 10, 1, 10);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pqgirnaefthokdmbsclj"), "smjonaeqcl", 10, 5, 10);
    _test_string_find_first_not_of_pointer_size_size(gl_string("kpdbgjmtherlsfcqoina"), "eqbdrkcfah", 10, 9, 11);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jrlbothiknqmdgcfasep"), "kapmsienhf", 10, 10, 10);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mjogldqferckabinptsh"), "jpqotrlenfcsbhkaimdg", 10, 0, 10);
    _test_string_find_first_not_of_pointer_size_size(gl_string("apoklnefbhmgqcdrisjt"), "jlbmhnfgtcqprikeados", 10, 1, 10);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ifeopcnrjbhkdgatmqls"), "stgbhfmdaljnpqoicker", 10, 10, 11);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ckqhaiesmjdnrgolbtpf"), "oihcetflbjagdsrkmqpn", 10, 19, 11);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bnlgapfimcoterskqdjh"), "adtclebmnpjsrqfkigoh", 10, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("kgdlrobpmjcthqsafeni"), "", 19, 0, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dfkechomjapgnslbtqir"), "beafg", 19, 0, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rloadknfbqtgmhcsipje"), "iclat", 19, 1, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mgjhkolrnadqbpetcifs"), "rkhnf", 19, 2, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("cmlfakiojdrgtbsphqen"), "clshq", 19, 4, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("kghbfipeomsntdalrqjc"), "dtcoj", 19, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("eldiqckrnmtasbghjfpo"), "rqosnjmfth", 19, 0, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("abqjcfedgotihlnspkrm"), "siatdfqglh", 19, 1, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("qfbadrtjsimkolcenhpg"), "mrlshtpgjq", 19, 5, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("abseghclkjqifmtodrnp"), "adlcskgqjt", 19, 9, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ibmsnlrjefhtdokacqpg"), "drshcjknaf", 19, 10, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mrkfciqjebaponsthldg"), "etsaqroinghpkjdlfcbm", 19, 0, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mjkticdeoqshpalrfbgn"), "sgepdnkqliambtrocfhj", 19, 1, 19);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rqnoclbdejgiphtfsakm"), "nlmcjaqgbsortfdihkpe", 19, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("plkqbhmtfaeodjcrsing"), "racfnpmosldibqkghjet", 19, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("oegalhmstjrfickpbndq"), "fjhdsctkqeiolagrnmbp", 19, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rdtgjcaohpblniekmsfq"), "", 20, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ofkqbnjetrmsaidphglc"), "ejanp", 20, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("grkpahljcftesdmonqib"), "odife", 20, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("jimlgbhfqkteospardcn"), "okaqd", 20, 2, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("gftenihpmslrjkqadcob"), "lcdbi", 20, 4, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("bmhldogtckrfsanijepq"), "fsqbj", 20, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("nfqkrpjdesabgtlcmoih"), "bigdomnplq", 20, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("focalnrpiqmdkstehbjg"), "apiblotgcd", 20, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rhqdspkmebiflcotnjga"), "acfhdenops", 20, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rahdtmsckfboqlpniegj"), "jopdeamcrk", 20, 9, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fbkeiopclstmdqranjhg"), "trqncbkgmh", 20, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("lifhpdgmbconstjeqark"), "tomglrkencbsfjqpihda", 20, 0, gl_string::npos);
}


void test_string_find_first_not_of_pointer_size_size3()
{
    _test_string_find_first_not_of_pointer_size_size(gl_string("pboqganrhedjmltsicfk"), "gbkhdnpoietfcmrslajq", 20, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("klchabsimetjnqgorfpd"), "rtfnmbsglkjaichoqedp", 20, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("sirfgmjqhctndbklaepo"), "ohkmdpfqbsacrtjnlgei", 20, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rlbdsiceaonqjtfpghkm"), "dlbrteoisgphmkncajfq", 20, 20, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ecgdanriptblhjfqskom"), "", 21, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("fdmiarlpgcskbhoteqjn"), "sjrlo", 21, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("rlbstjqopignecmfadkh"), "qjpor", 21, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("grjpqmbshektdolcafni"), "odhfn", 21, 2, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("sakfcohtqnibprjmlged"), "qtfin", 21, 4, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("mjtdglasihqpocebrfkn"), "hpqfo", 21, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("okaplfrntghqbmeicsdj"), "fabmertkos", 21, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("sahngemrtcjidqbklfpo"), "brqtgkmaej", 21, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("dlmsipcnekhbgoaftqjr"), "nfrdeihsgl", 21, 5, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("ahegrmqnoiklpfsdbcjt"), "hlfrosekpi", 21, 9, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hdsjbnmlegtkqripacof"), "atgbkrjdsm", 21, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("pcnedrfjihqbalkgtoms"), "blnrptjgqmaifsdkhoec", 21, 0, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("qjidealmtpskrbfhocng"), "ctpmdahebfqjgknloris", 21, 1, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("qeindtagmokpfhsclrbj"), "apnkeqthrmlbfodiscgj", 21, 10, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("kpfegbjhsrnodltqciam"), "jdgictpframeoqlsbknh", 21, 19, gl_string::npos);
    _test_string_find_first_not_of_pointer_size_size(gl_string("hnbrcplsjfgiktoedmaq"), "qprlsfojamgndekthibc", 21, 20, gl_string::npos);
}


void
_test_string_find_first_not_of_string_size(const gl_string& s, const gl_string& str, size_t pos, size_t x)
{
    TS_ASSERT(s.find_first_not_of(str, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(pos <= x && x < s.size());
}


void
_test_string_find_first_not_of_string_size(const gl_string& s, const gl_string& str, size_t x)
{
    TS_ASSERT(s.find_first_not_of(str) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x < s.size());
}


void test_string_find_first_not_of_string_size0()
{
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string(""), 0, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string("laenf"), 0, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string("pqlnkmbdjo"), 0, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string("qkamfogpnljdcshbreti"), 0, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string(""), 1, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string("bjaht"), 1, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string("hjlcmgpket"), 1, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string("htaobedqikfplcgjsmrn"), 1, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("fodgq"), gl_string(""), 0, 0);
    _test_string_find_first_not_of_string_size(gl_string("qanej"), gl_string("dfkap"), 0, 0);
    _test_string_find_first_not_of_string_size(gl_string("clbao"), gl_string("ihqrfebgad"), 0, 0);
    _test_string_find_first_not_of_string_size(gl_string("mekdn"), gl_string("ngtjfcalbseiqrphmkdo"), 0, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("srdfq"), gl_string(""), 1, 1);
    _test_string_find_first_not_of_string_size(gl_string("oemth"), gl_string("ikcrq"), 1, 1);
    _test_string_find_first_not_of_string_size(gl_string("cdaih"), gl_string("dmajblfhsg"), 1, 3);
    _test_string_find_first_not_of_string_size(gl_string("qohtk"), gl_string("oqftjhdmkgsblacenirp"), 1, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("cshmd"), gl_string(""), 2, 2);
    _test_string_find_first_not_of_string_size(gl_string("lhcdo"), gl_string("oebqi"), 2, 2);
    _test_string_find_first_not_of_string_size(gl_string("qnsoh"), gl_string("kojhpmbsfe"), 2, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("pkrof"), gl_string("acbsjqogpltdkhinfrem"), 2, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("fmtsp"), gl_string(""), 4, 4);
    _test_string_find_first_not_of_string_size(gl_string("khbpm"), gl_string("aobjd"), 4, 4);
    _test_string_find_first_not_of_string_size(gl_string("pbsji"), gl_string("pcbahntsje"), 4, 4);
    _test_string_find_first_not_of_string_size(gl_string("mprdj"), gl_string("fhepcrntkoagbmldqijs"), 4, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("eqmpa"), gl_string(""), 5, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("omigs"), gl_string("kocgb"), 5, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("onmje"), gl_string("fbslrjiqkm"), 5, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("oqmrj"), gl_string("jeidpcmalhfnqbgtrsko"), 5, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("schfa"), gl_string(""), 6, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("igdsc"), gl_string("qngpd"), 6, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("brqgo"), gl_string("rodhqklgmb"), 6, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("tnrph"), gl_string("thdjgafrlbkoiqcspmne"), 6, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("hcjitbfapl"), gl_string(""), 0, 0);
    _test_string_find_first_not_of_string_size(gl_string("daiprenocl"), gl_string("ashjd"), 0, 2);
    _test_string_find_first_not_of_string_size(gl_string("litpcfdghe"), gl_string("mgojkldsqh"), 0, 1);
    _test_string_find_first_not_of_string_size(gl_string("aidjksrolc"), gl_string("imqnaghkfrdtlopbjesc"), 0, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("qpghtfbaji"), gl_string(""), 1, 1);
    _test_string_find_first_not_of_string_size(gl_string("gfshlcmdjr"), gl_string("nadkh"), 1, 1);
    _test_string_find_first_not_of_string_size(gl_string("nkodajteqp"), gl_string("ofdrqmkebl"), 1, 4);
    _test_string_find_first_not_of_string_size(gl_string("gbmetiprqd"), gl_string("bdfjqgatlksriohemnpc"), 1, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("crnklpmegd"), gl_string(""), 5, 5);
    _test_string_find_first_not_of_string_size(gl_string("jsbtafedoc"), gl_string("prqgn"), 5, 5);
    _test_string_find_first_not_of_string_size(gl_string("qnmodrtkeb"), gl_string("pejafmnokr"), 5, 6);
    _test_string_find_first_not_of_string_size(gl_string("cpebqsfmnj"), gl_string("odnqkgijrhabfmcestlp"), 5, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("lmofqdhpki"), gl_string(""), 9, 9);
    _test_string_find_first_not_of_string_size(gl_string("hnefkqimca"), gl_string("rtjpa"), 9, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("drtasbgmfp"), gl_string("ktsrmnqagd"), 9, 9);
    _test_string_find_first_not_of_string_size(gl_string("lsaijeqhtr"), gl_string("rtdhgcisbnmoaqkfpjle"), 9, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("elgofjmbrq"), gl_string(""), 10, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("mjqdgalkpc"), gl_string("dplqa"), 10, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("kthqnfcerm"), gl_string("dkacjoptns"), 10, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("dfsjhanorc"), gl_string("hqfimtrgnbekpdcsjalo"), 10, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("eqsgalomhb"), gl_string(""), 11, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("akiteljmoh"), gl_string("lofbc"), 11, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("hlbdfreqjo"), gl_string("astoegbfpn"), 11, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("taqobhlerg"), gl_string("pdgreqomsncafklhtibj"), 11, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("snafbdlghrjkpqtoceim"), gl_string(""), 0, 0);
    _test_string_find_first_not_of_string_size(gl_string("aemtbrgcklhndjisfpoq"), gl_string("lbtqd"), 0, 0);
    _test_string_find_first_not_of_string_size(gl_string("pnracgfkjdiholtbqsem"), gl_string("tboimldpjh"), 0, 1);
    _test_string_find_first_not_of_string_size(gl_string("dicfltehbsgrmojnpkaq"), gl_string("slcerthdaiqjfnobgkpm"), 0, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("jlnkraeodhcspfgbqitm"), gl_string(""), 1, 1);
    _test_string_find_first_not_of_string_size(gl_string("lhosrngtmfjikbqpcade"), gl_string("aqibs"), 1, 1);
    _test_string_find_first_not_of_string_size(gl_string("rbtaqjhgkneisldpmfoc"), gl_string("gtfblmqinc"), 1, 3);
    _test_string_find_first_not_of_string_size(gl_string("gpifsqlrdkbonjtmheca"), gl_string("mkqpbtdalgniorhfescj"), 1, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("hdpkobnsalmcfijregtq"), gl_string(""), 10, 10);
    _test_string_find_first_not_of_string_size(gl_string("jtlshdgqaiprkbcoenfm"), gl_string("pblas"), 10, 11);
    _test_string_find_first_not_of_string_size(gl_string("fkdrbqltsgmcoiphneaj"), gl_string("arosdhcfme"), 10, 13);
    _test_string_find_first_not_of_string_size(gl_string("crsplifgtqedjohnabmk"), gl_string("blkhjeogicatqfnpdmsr"), 10, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("niptglfbosehkamrdqcj"), gl_string(""), 19, 19);
    _test_string_find_first_not_of_string_size(gl_string("copqdhstbingamjfkler"), gl_string("djkqc"), 19, 19);
    _test_string_find_first_not_of_string_size(gl_string("mrtaefilpdsgocnhqbjk"), gl_string("lgokshjtpb"), 19, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("kojatdhlcmigpbfrqnes"), gl_string("bqjhtkfepimcnsgrlado"), 19, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("eaintpchlqsbdgrkjofm"), gl_string(""), 20, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("gjnhidfsepkrtaqbmclo"), gl_string("nocfa"), 20, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("spocfaktqdbiejlhngmr"), gl_string("bgtajmiedc"), 20, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("rphmlekgfscndtaobiqj"), gl_string("lsckfnqgdahejiopbtmr"), 20, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("liatsqdoegkmfcnbhrpj"), gl_string(""), 21, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("binjagtfldkrspcomqeh"), gl_string("gfsrt"), 21, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("latkmisecnorjbfhqpdg"), gl_string("pfsocbhjtm"), 21, gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("lecfratdjkhnsmqpoigb"), gl_string("tpflmdnoicjgkberhqsa"), 21, gl_string::npos);
}


void test_string_find_first_not_of_string_size1()
{
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string(""), gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string("laenf"), gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string("pqlnkmbdjo"), gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string(""), gl_string("qkamfogpnljdcshbreti"), gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("nhmko"), gl_string(""), 0);
    _test_string_find_first_not_of_string_size(gl_string("lahfb"), gl_string("irkhs"), 0);
    _test_string_find_first_not_of_string_size(gl_string("gmfhd"), gl_string("kantesmpgj"), 2);
    _test_string_find_first_not_of_string_size(gl_string("odaft"), gl_string("oknlrstdpiqmjbaghcfe"), gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("eolhfgpjqk"), gl_string(""), 0);
    _test_string_find_first_not_of_string_size(gl_string("nbatdlmekr"), gl_string("bnrpe"), 2);
    _test_string_find_first_not_of_string_size(gl_string("jdmciepkaq"), gl_string("jtdaefblso"), 2);
    _test_string_find_first_not_of_string_size(gl_string("hkbgspoflt"), gl_string("oselktgbcapndfjihrmq"), gl_string::npos);
    _test_string_find_first_not_of_string_size(gl_string("gprdcokbnjhlsfmtieqa"), gl_string(""), 0);
    _test_string_find_first_not_of_string_size(gl_string("qjghlnftcaismkropdeb"), gl_string("bjaht"), 0);
    _test_string_find_first_not_of_string_size(gl_string("pnalfrdtkqcmojiesbhg"), gl_string("hjlcmgpket"), 1);
    _test_string_find_first_not_of_string_size(gl_string("pniotcfrhqsmgdkjbael"), gl_string("htaobedqikfplcgjsmrn"), gl_string::npos);
}


void
_test_string_find_first_of_char_size(const gl_string& s, char c, size_t pos,
     size_t x)
{
    TS_ASSERT(s.find_first_of(c, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(pos <= x && x < s.size());
}


void
_test_string_find_first_of_char_size(const gl_string& s, char c, size_t x)
{
    TS_ASSERT(s.find_first_of(c) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x < s.size());
}

  void test_string_find_first_of_char_size() 
    {
    _test_string_find_first_of_char_size(gl_string(""), 'e', 0, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string(""), 'e', 1, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("kitcj"), 'e', 0, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("qkamf"), 'e', 1, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("nhmko"), 'e', 2, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("tpsaf"), 'e', 4, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("lahfb"), 'e', 5, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("irkhs"), 'e', 6, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("gmfhdaipsr"), 'e', 0, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("kantesmpgj"), 'e', 1, 4);
    _test_string_find_first_of_char_size(gl_string("odaftiegpm"), 'e', 5, 6);
    _test_string_find_first_of_char_size(gl_string("oknlrstdpi"), 'e', 9, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("eolhfgpjqk"), 'e', 10, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("pcdrofikas"), 'e', 11, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("nbatdlmekrgcfqsophij"), 'e', 0, 7);
    _test_string_find_first_of_char_size(gl_string("bnrpehidofmqtcksjgla"), 'e', 1, 4);
    _test_string_find_first_of_char_size(gl_string("jdmciepkaqgotsrfnhlb"), 'e', 10, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("jtdaefblsokrmhpgcnqi"), 'e', 19, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("hkbgspofltajcnedqmri"), 'e', 20, gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("oselktgbcapndfjihrmq"), 'e', 21, gl_string::npos);

    _test_string_find_first_of_char_size(gl_string(""), 'e', gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("csope"), 'e', 4);
    _test_string_find_first_of_char_size(gl_string("gfsmthlkon"), 'e', gl_string::npos);
    _test_string_find_first_of_char_size(gl_string("laenfsbridchgotmkqpj"), 'e', 2);
    }

void
_test_string_find_first_of_pointer_size(const gl_string& s, const char* str, size_t pos,
     size_t x)
{
    TS_ASSERT(s.find_first_of(str, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(pos <= x && x < s.size());
}


void
_test_string_find_first_of_pointer_size(const gl_string& s, const char* str, size_t x)
{
    TS_ASSERT(s.find_first_of(str) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x < s.size());
}


void test_string_find_first_of_pointer_size0()
{
    _test_string_find_first_of_pointer_size(gl_string(""), "", 0, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string(""), "laenf", 0, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string(""), "pqlnkmbdjo", 0, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string(""), "qkamfogpnljdcshbreti", 0, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string(""), "", 1, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string(""), "bjaht", 1, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string(""), "hjlcmgpket", 1, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string(""), "htaobedqikfplcgjsmrn", 1, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("fodgq"), "", 0, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("qanej"), "dfkap", 0, 1);
    _test_string_find_first_of_pointer_size(gl_string("clbao"), "ihqrfebgad", 0, 2);
    _test_string_find_first_of_pointer_size(gl_string("mekdn"), "ngtjfcalbseiqrphmkdo", 0, 0);
    _test_string_find_first_of_pointer_size(gl_string("srdfq"), "", 1, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("oemth"), "ikcrq", 1, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("cdaih"), "dmajblfhsg", 1, 1);
    _test_string_find_first_of_pointer_size(gl_string("qohtk"), "oqftjhdmkgsblacenirp", 1, 1);
    _test_string_find_first_of_pointer_size(gl_string("cshmd"), "", 2, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("lhcdo"), "oebqi", 2, 4);
    _test_string_find_first_of_pointer_size(gl_string("qnsoh"), "kojhpmbsfe", 2, 2);
    _test_string_find_first_of_pointer_size(gl_string("pkrof"), "acbsjqogpltdkhinfrem", 2, 2);
    _test_string_find_first_of_pointer_size(gl_string("fmtsp"), "", 4, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("khbpm"), "aobjd", 4, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("pbsji"), "pcbahntsje", 4, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("mprdj"), "fhepcrntkoagbmldqijs", 4, 4);
    _test_string_find_first_of_pointer_size(gl_string("eqmpa"), "", 5, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("omigs"), "kocgb", 5, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("onmje"), "fbslrjiqkm", 5, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("oqmrj"), "jeidpcmalhfnqbgtrsko", 5, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("schfa"), "", 6, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("igdsc"), "qngpd", 6, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("brqgo"), "rodhqklgmb", 6, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("tnrph"), "thdjgafrlbkoiqcspmne", 6, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("hcjitbfapl"), "", 0, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("daiprenocl"), "ashjd", 0, 0);
    _test_string_find_first_of_pointer_size(gl_string("litpcfdghe"), "mgojkldsqh", 0, 0);
    _test_string_find_first_of_pointer_size(gl_string("aidjksrolc"), "imqnaghkfrdtlopbjesc", 0, 0);
    _test_string_find_first_of_pointer_size(gl_string("qpghtfbaji"), "", 1, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("gfshlcmdjr"), "nadkh", 1, 3);
    _test_string_find_first_of_pointer_size(gl_string("nkodajteqp"), "ofdrqmkebl", 1, 1);
    _test_string_find_first_of_pointer_size(gl_string("gbmetiprqd"), "bdfjqgatlksriohemnpc", 1, 1);
    _test_string_find_first_of_pointer_size(gl_string("crnklpmegd"), "", 5, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("jsbtafedoc"), "prqgn", 5, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("qnmodrtkeb"), "pejafmnokr", 5, 5);
    _test_string_find_first_of_pointer_size(gl_string("cpebqsfmnj"), "odnqkgijrhabfmcestlp", 5, 5);
    _test_string_find_first_of_pointer_size(gl_string("lmofqdhpki"), "", 9, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("hnefkqimca"), "rtjpa", 9, 9);
    _test_string_find_first_of_pointer_size(gl_string("drtasbgmfp"), "ktsrmnqagd", 9, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("lsaijeqhtr"), "rtdhgcisbnmoaqkfpjle", 9, 9);
    _test_string_find_first_of_pointer_size(gl_string("elgofjmbrq"), "", 10, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("mjqdgalkpc"), "dplqa", 10, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("kthqnfcerm"), "dkacjoptns", 10, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("dfsjhanorc"), "hqfimtrgnbekpdcsjalo", 10, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("eqsgalomhb"), "", 11, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("akiteljmoh"), "lofbc", 11, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("hlbdfreqjo"), "astoegbfpn", 11, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("taqobhlerg"), "pdgreqomsncafklhtibj", 11, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("snafbdlghrjkpqtoceim"), "", 0, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("aemtbrgcklhndjisfpoq"), "lbtqd", 0, 3);
    _test_string_find_first_of_pointer_size(gl_string("pnracgfkjdiholtbqsem"), "tboimldpjh", 0, 0);
    _test_string_find_first_of_pointer_size(gl_string("dicfltehbsgrmojnpkaq"), "slcerthdaiqjfnobgkpm", 0, 0);
    _test_string_find_first_of_pointer_size(gl_string("jlnkraeodhcspfgbqitm"), "", 1, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("lhosrngtmfjikbqpcade"), "aqibs", 1, 3);
    _test_string_find_first_of_pointer_size(gl_string("rbtaqjhgkneisldpmfoc"), "gtfblmqinc", 1, 1);
    _test_string_find_first_of_pointer_size(gl_string("gpifsqlrdkbonjtmheca"), "mkqpbtdalgniorhfescj", 1, 1);
    _test_string_find_first_of_pointer_size(gl_string("hdpkobnsalmcfijregtq"), "", 10, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("jtlshdgqaiprkbcoenfm"), "pblas", 10, 10);
    _test_string_find_first_of_pointer_size(gl_string("fkdrbqltsgmcoiphneaj"), "arosdhcfme", 10, 10);
    _test_string_find_first_of_pointer_size(gl_string("crsplifgtqedjohnabmk"), "blkhjeogicatqfnpdmsr", 10, 10);
    _test_string_find_first_of_pointer_size(gl_string("niptglfbosehkamrdqcj"), "", 19, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("copqdhstbingamjfkler"), "djkqc", 19, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("mrtaefilpdsgocnhqbjk"), "lgokshjtpb", 19, 19);
    _test_string_find_first_of_pointer_size(gl_string("kojatdhlcmigpbfrqnes"), "bqjhtkfepimcnsgrlado", 19, 19);
    _test_string_find_first_of_pointer_size(gl_string("eaintpchlqsbdgrkjofm"), "", 20, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("gjnhidfsepkrtaqbmclo"), "nocfa", 20, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("spocfaktqdbiejlhngmr"), "bgtajmiedc", 20, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("rphmlekgfscndtaobiqj"), "lsckfnqgdahejiopbtmr", 20, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("liatsqdoegkmfcnbhrpj"), "", 21, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("binjagtfldkrspcomqeh"), "gfsrt", 21, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("latkmisecnorjbfhqpdg"), "pfsocbhjtm", 21, gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("lecfratdjkhnsmqpoigb"), "tpflmdnoicjgkberhqsa", 21, gl_string::npos);
}


void test_string_find_first_of_pointer_size1()
{
    _test_string_find_first_of_pointer_size(gl_string(""), "", gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string(""), "laenf", gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string(""), "pqlnkmbdjo", gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string(""), "qkamfogpnljdcshbreti", gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("nhmko"), "", gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("lahfb"), "irkhs", 2);
    _test_string_find_first_of_pointer_size(gl_string("gmfhd"), "kantesmpgj", 0);
    _test_string_find_first_of_pointer_size(gl_string("odaft"), "oknlrstdpiqmjbaghcfe", 0);
    _test_string_find_first_of_pointer_size(gl_string("eolhfgpjqk"), "", gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("nbatdlmekr"), "bnrpe", 0);
    _test_string_find_first_of_pointer_size(gl_string("jdmciepkaq"), "jtdaefblso", 0);
    _test_string_find_first_of_pointer_size(gl_string("hkbgspoflt"), "oselktgbcapndfjihrmq", 0);
    _test_string_find_first_of_pointer_size(gl_string("gprdcokbnjhlsfmtieqa"), "", gl_string::npos);
    _test_string_find_first_of_pointer_size(gl_string("qjghlnftcaismkropdeb"), "bjaht", 1);
    _test_string_find_first_of_pointer_size(gl_string("pnalfrdtkqcmojiesbhg"), "hjlcmgpket", 0);
    _test_string_find_first_of_pointer_size(gl_string("pniotcfrhqsmgdkjbael"), "htaobedqikfplcgjsmrn", 0);
}

void
_test_string_find_first_of_pointer_size_size(const gl_string& s, const char* str, size_t pos,
     size_t n, size_t x)
{
    TS_ASSERT(s.find_first_of(str, pos, n) == x);
    if (x != gl_string::npos)
        TS_ASSERT(pos <= x && x < s.size());
}


void test_string_find_first_of_pointer_size_size0()
{
    _test_string_find_first_of_pointer_size_size(gl_string(""), "", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "irkhs", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "kante", 0, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "oknlr", 0, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "pcdro", 0, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "bnrpe", 0, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "jtdaefblso", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "oselktgbca", 0, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "eqgaplhckj", 0, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "bjahtcmnlp", 0, 9, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "hjlcmgpket", 0, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "htaobedqikfplcgjsmrn", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "hpqiarojkcdlsgnmfetb", 0, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "dfkaprhjloqetcsimnbg", 0, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "ihqrfebgadntlpmjksoc", 0, 19, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "ngtjfcalbseiqrphmkdo", 0, 20, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "lbtqd", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "tboim", 1, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "slcer", 1, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "cbjfs", 1, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "aqibs", 1, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "gtfblmqinc", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "mkqpbtdalg", 1, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "kphatlimcd", 1, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "pblasqogic", 1, 9, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "arosdhcfme", 1, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "blkhjeogicatqfnpdmsr", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "bmhineprjcoadgstflqk", 1, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "djkqcmetslnghpbarfoi", 1, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "lgokshjtpbemarcdqnfi", 1, 19, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string(""), "bqjhtkfepimcnsgrlado", 1, 20, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("eaint"), "", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("binja"), "gfsrt", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("latkm"), "pfsoc", 0, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("lecfr"), "tpflm", 0, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("eqkst"), "sgkec", 0, 4, 0);
    _test_string_find_first_of_pointer_size_size(gl_string("cdafr"), "romds", 0, 5, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("prbhe"), "qhjistlgmr", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("lbisk"), "pedfirsglo", 0, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("hrlpd"), "aqcoslgrmk", 0, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ehmja"), "dabckmepqj", 0, 9, 0);
    _test_string_find_first_of_pointer_size_size(gl_string("mhqgd"), "pqscrjthli", 0, 10, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("tgklq"), "kfphdcsjqmobliagtren", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("bocjs"), "rokpefncljibsdhqtagm", 0, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("grbsd"), "afionmkphlebtcjqsgrd", 0, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ofjqr"), "aenmqplidhkofrjbctsg", 0, 19, 0);
    _test_string_find_first_of_pointer_size_size(gl_string("btlfi"), "osjmbtcadhiklegrpqnf", 0, 20, 0);
    _test_string_find_first_of_pointer_size_size(gl_string("clrgb"), "", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("tjmek"), "osmia", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("bgstp"), "ckonl", 1, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("hstrk"), "ilcaj", 1, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("kmspj"), "lasiq", 1, 4, 2);
    _test_string_find_first_of_pointer_size_size(gl_string("tjboh"), "kfqmr", 1, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ilbcj"), "klnitfaobg", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("jkngf"), "gjhmdlqikp", 1, 1, 3);
    _test_string_find_first_of_pointer_size_size(gl_string("gfcql"), "skbgtahqej", 1, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("dqtlg"), "bjsdgtlpkf", 1, 9, 2);
    _test_string_find_first_of_pointer_size_size(gl_string("bthpg"), "bjgfmnlkio", 1, 10, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("dgsnq"), "lbhepotfsjdqigcnamkr", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rmfhp"), "tebangckmpsrqdlfojhi", 1, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("jfdam"), "joflqbdkhtegimscpanr", 1, 10, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("edapb"), "adpmcohetfbsrjinlqkg", 1, 19, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("brfsm"), "iacldqjpfnogbsrhmetk", 1, 20, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("ndrhl"), "", 2, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("mrecp"), "otkgb", 2, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("qlasf"), "cqsjl", 2, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("smaqd"), "dpifl", 2, 2, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("hjeni"), "oapht", 2, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ocmfj"), "cifts", 2, 5, 3);
    _test_string_find_first_of_pointer_size_size(gl_string("hmftq"), "nmsckbgalo", 2, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("fklad"), "tpksqhamle", 2, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("dirnm"), "tpdrchmkji", 2, 5, 2);
    _test_string_find_first_of_pointer_size_size(gl_string("hrgdc"), "ijagfkblst", 2, 9, 2);
    _test_string_find_first_of_pointer_size_size(gl_string("ifakg"), "kpocsignjb", 2, 10, 3);
    _test_string_find_first_of_pointer_size_size(gl_string("ebrgd"), "pecqtkjsnbdrialgmohf", 2, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rcjml"), "aiortphfcmkjebgsndql", 2, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("peqmt"), "sdbkeamglhipojqftrcn", 2, 10, 3);
    _test_string_find_first_of_pointer_size_size(gl_string("frehn"), "ljqncehgmfktroapidbs", 2, 19, 2);
    _test_string_find_first_of_pointer_size_size(gl_string("tqolf"), "rtcfodilamkbenjghqps", 2, 20, 2);
    _test_string_find_first_of_pointer_size_size(gl_string("cjgao"), "", 4, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("kjplq"), "mabns", 4, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("herni"), "bdnrp", 4, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("tadrb"), "scidp", 4, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("pkfeo"), "agbjl", 4, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("hoser"), "jfmpr", 4, 5, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("kgrsp"), "rbpefghsmj", 4, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("pgejb"), "apsfntdoqc", 4, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("thlnq"), "ndkjeisgcl", 4, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("nbmit"), "rnfpqatdeo", 4, 9, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("jgmib"), "bntjlqrfik", 4, 10, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("ncrfj"), "kcrtmpolnaqejghsfdbi", 4, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ncsik"), "lobheanpkmqidsrtcfgj", 4, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("sgbfh"), "athdkljcnreqbgpmisof", 4, 10, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("dktbn"), "qkdmjialrscpbhefgont", 4, 19, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("fthqm"), "dmasojntqleribkgfchp", 4, 20, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("klopi"), "", 5, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("dajhn"), "psthd", 5, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("jbgno"), "rpmjd", 5, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("hkjae"), "dfsmk", 5, 2, gl_string::npos);
}


void test_string_find_first_of_pointer_size_size1()
{
    _test_string_find_first_of_pointer_size_size(gl_string("gbhqo"), "skqne", 5, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ktdor"), "kipnf", 5, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ldprn"), "hmrnqdgifl", 5, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("egmjk"), "fsmjcdairn", 5, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("armql"), "pcdgltbrfj", 5, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("cdhjo"), "aekfctpirg", 5, 9, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("jcons"), "ledihrsgpf", 5, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("cbrkp"), "mqcklahsbtirgopefndj", 5, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("fhgna"), "kmlthaoqgecrnpdbjfis", 5, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ejfcd"), "sfhbamcdptojlkrenqgi", 5, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("kqjhe"), "pbniofmcedrkhlstgaqj", 5, 19, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("pbdjl"), "mongjratcskbhqiepfdl", 5, 20, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("gajqn"), "", 6, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("stedk"), "hrnat", 6, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("tjkaf"), "gsqdt", 6, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("dthpe"), "bspkd", 6, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("klhde"), "ohcmb", 6, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("bhlki"), "heatr", 6, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("lqmoh"), "pmblckedfn", 6, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("mtqin"), "aceqmsrbik", 6, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("dpqbr"), "lmbtdehjrn", 6, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("kdhmo"), "teqmcrlgib", 6, 9, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("jblqp"), "njolbmspac", 6, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("qmjgl"), "pofnhidklamecrbqjgst", 6, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rothp"), "jbhckmtgrqnosafedpli", 6, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ghknq"), "dobntpmqklicsahgjerf", 6, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("eopfi"), "tpdshainjkbfoemlrgcq", 6, 19, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("dsnmg"), "oldpfgeakrnitscbjmqh", 6, 20, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("jnkrfhotgl"), "", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("dltjfngbko"), "rqegt", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("bmjlpkiqde"), "dashm", 0, 1, 8);
    _test_string_find_first_of_pointer_size_size(gl_string("skrflobnqm"), "jqirk", 0, 2, 8);
    _test_string_find_first_of_pointer_size_size(gl_string("jkpldtshrm"), "rckeg", 0, 4, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("ghasdbnjqo"), "jscie", 0, 5, 3);
    _test_string_find_first_of_pointer_size_size(gl_string("igrkhpbqjt"), "efsphndliq", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ikthdgcamf"), "gdicosleja", 0, 1, 5);
    _test_string_find_first_of_pointer_size_size(gl_string("pcofgeniam"), "qcpjibosfl", 0, 5, 0);
    _test_string_find_first_of_pointer_size_size(gl_string("rlfjgesqhc"), "lrhmefnjcq", 0, 9, 0);
    _test_string_find_first_of_pointer_size_size(gl_string("itphbqsker"), "dtablcrseo", 0, 10, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("skjafcirqm"), "apckjsftedbhgomrnilq", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("tcqomarsfd"), "pcbrgflehjtiadnsokqm", 0, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rocfeldqpk"), "nsiadegjklhobrmtqcpf", 0, 10, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("cfpegndlkt"), "cpmajdqnolikhgsbretf", 0, 19, 0);
    _test_string_find_first_of_pointer_size_size(gl_string("fqbtnkeasj"), "jcflkntmgiqrphdosaeb", 0, 20, 0);
    _test_string_find_first_of_pointer_size_size(gl_string("shbcqnmoar"), "", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("bdoshlmfin"), "ontrs", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("khfrebnsgq"), "pfkna", 1, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("getcrsaoji"), "ekosa", 1, 2, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("fjiknedcpq"), "anqhk", 1, 4, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("tkejgnafrm"), "jekca", 1, 5, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("jnakolqrde"), "ikemsjgacf", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("lcjptsmgbe"), "arolgsjkhm", 1, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("itfsmcjorl"), "oftkbldhre", 1, 5, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("omchkfrjea"), "gbkqdoeftl", 1, 9, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("cigfqkated"), "sqcflrgtim", 1, 10, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("tscenjikml"), "fmhbkislrjdpanogqcet", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("qcpaemsinf"), "rnioadktqlgpbcjsmhef", 1, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("gltkojeipd"), "oakgtnldpsefihqmjcbr", 1, 10, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("qistfrgnmp"), "gbnaelosidmcjqktfhpr", 1, 19, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("bdnpfcqaem"), "akbripjhlosndcmqgfet", 1, 20, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("ectnhskflp"), "", 5, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("fgtianblpq"), "pijag", 5, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("mfeqklirnh"), "jrckd", 5, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("astedncjhk"), "qcloh", 5, 2, 6);
    _test_string_find_first_of_pointer_size_size(gl_string("fhlqgcajbr"), "thlmp", 5, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("epfhocmdng"), "qidmo", 5, 5, 6);
    _test_string_find_first_of_pointer_size_size(gl_string("apcnsibger"), "lnegpsjqrd", 5, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("aqkocrbign"), "rjqdablmfs", 5, 1, 5);
    _test_string_find_first_of_pointer_size_size(gl_string("ijsmdtqgce"), "enkgpbsjaq", 5, 5, 7);
    _test_string_find_first_of_pointer_size_size(gl_string("clobgsrken"), "kdsgoaijfh", 5, 9, 5);
    _test_string_find_first_of_pointer_size_size(gl_string("jbhcfposld"), "trfqgmckbe", 5, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("oqnpblhide"), "igetsracjfkdnpoblhqm", 5, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("lroeasctif"), "nqctfaogirshlekbdjpm", 5, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("bpjlgmiedh"), "csehfgomljdqinbartkp", 5, 10, 5);
    _test_string_find_first_of_pointer_size_size(gl_string("pamkeoidrj"), "qahoegcmplkfsjbdnitr", 5, 19, 5);
    _test_string_find_first_of_pointer_size_size(gl_string("espogqbthk"), "dpteiajrqmsognhlfbkc", 5, 20, 5);
    _test_string_find_first_of_pointer_size_size(gl_string("shoiedtcjb"), "", 9, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ebcinjgads"), "tqbnh", 9, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("dqmregkcfl"), "akmle", 9, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ngcrieqajf"), "iqfkm", 9, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("qosmilgnjb"), "tqjsr", 9, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ikabsjtdfl"), "jplqg", 9, 5, 9);
    _test_string_find_first_of_pointer_size_size(gl_string("ersmicafdh"), "oilnrbcgtj", 9, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("fdnplotmgh"), "morkglpesn", 9, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("fdbicojerm"), "dmicerngat", 9, 5, 9);
    _test_string_find_first_of_pointer_size_size(gl_string("mbtafndjcq"), "radgeskbtc", 9, 9, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("mlenkpfdtc"), "ljikprsmqo", 9, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ahlcifdqgs"), "trqihkcgsjamfdbolnpe", 9, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("bgjemaltks"), "lqmthbsrekajgnofcipd", 9, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("pdhslbqrfc"), "jtalmedribkgqsopcnfh", 9, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("dirhtsnjkc"), "spqfoiclmtagejbndkrh", 9, 19, 9);
    _test_string_find_first_of_pointer_size_size(gl_string("dlroktbcja"), "nmotklspigjrdhcfaebq", 9, 20, 9);
    _test_string_find_first_of_pointer_size_size(gl_string("ncjpmaekbs"), "", 10, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("hlbosgmrak"), "hpmsd", 10, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("pqfhsgilen"), "qnpor", 10, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("gqtjsbdckh"), "otdma", 10, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("cfkqpjlegi"), "efhjg", 10, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("beanrfodgj"), "odpte", 10, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("adtkqpbjfi"), "bctdgfmolr", 10, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("iomkfthagj"), "oaklidrbqg", 10, 1, gl_string::npos);
}


void test_string_find_first_of_pointer_size_size2()
{
    _test_string_find_first_of_pointer_size_size(gl_string("sdpcilonqj"), "dnjfsagktr", 10, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("gtfbdkqeml"), "nejaktmiqg", 10, 9, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("bmeqgcdorj"), "pjqonlebsf", 10, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("etqlcanmob"), "dshmnbtolcjepgaikfqr", 10, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("roqmkbdtia"), "iogfhpabtjkqlrnemcds", 10, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("kadsithljf"), "ngridfabjsecpqltkmoh", 10, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("sgtkpbfdmh"), "athmknplcgofrqejsdib", 10, 19, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("qgmetnabkl"), "ldobhmqcafnjtkeisgrp", 10, 20, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("cqjohampgd"), "", 11, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("hobitmpsan"), "aocjb", 11, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("tjehkpsalm"), "jbrnk", 11, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ngfbojitcl"), "tqedg", 11, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rcfkdbhgjo"), "nqskp", 11, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("qghptonrea"), "eaqkl", 11, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("hnprfgqjdl"), "reaoicljqm", 11, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("hlmgabenti"), "lsftgajqpm", 11, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ofcjanmrbs"), "rlpfogmits", 11, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("jqedtkornm"), "shkncmiaqj", 11, 9, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rfedlasjmg"), "fpnatrhqgs", 11, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("talpqjsgkm"), "sjclemqhnpdbgikarfot", 11, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("lrkcbtqpie"), "otcmedjikgsfnqbrhpla", 11, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("cipogdskjf"), "bonsaefdqiprkhlgtjcm", 11, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("nqedcojahi"), "egpscmahijlfnkrodqtb", 11, 19, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("hefnrkmctj"), "kmqbfepjthgilscrndoa", 11, 20, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("atqirnmekfjolhpdsgcb"), "", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("echfkmlpribjnqsaogtd"), "prboq", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("qnhiftdgcleajbpkrosm"), "fjcqh", 0, 1, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("chamfknorbedjitgslpq"), "fmosa", 0, 2, 3);
    _test_string_find_first_of_pointer_size_size(gl_string("njhqpibfmtlkaecdrgso"), "qdbok", 0, 4, 3);
    _test_string_find_first_of_pointer_size_size(gl_string("ebnghfsqkprmdcljoiat"), "amslg", 0, 5, 3);
    _test_string_find_first_of_pointer_size_size(gl_string("letjomsgihfrpqbkancd"), "smpltjneqb", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("nblgoipcrqeaktshjdmf"), "flitskrnge", 0, 1, 19);
    _test_string_find_first_of_pointer_size_size(gl_string("cehkbngtjoiflqapsmrd"), "pgqihmlbef", 0, 5, 2);
    _test_string_find_first_of_pointer_size_size(gl_string("mignapfoklbhcqjetdrs"), "cfpdqjtgsb", 0, 9, 2);
    _test_string_find_first_of_pointer_size_size(gl_string("ceatbhlsqjgpnokfrmdi"), "htpsiaflom", 0, 10, 2);
    _test_string_find_first_of_pointer_size_size(gl_string("ocihkjgrdelpfnmastqb"), "kpjfiaceghsrdtlbnomq", 0, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("noelgschdtbrjfmiqkap"), "qhtbomidljgafneksprc", 0, 1, 16);
    _test_string_find_first_of_pointer_size_size(gl_string("dkclqfombepritjnghas"), "nhtjobkcefldimpsaqgr", 0, 10, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("miklnresdgbhqcojftap"), "prabcjfqnoeskilmtgdh", 0, 19, 0);
    _test_string_find_first_of_pointer_size_size(gl_string("htbcigojaqmdkfrnlsep"), "dtrgmchilkasqoebfpjn", 0, 20, 0);
    _test_string_find_first_of_pointer_size_size(gl_string("febhmqtjanokscdirpgl"), "", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("loakbsqjpcrdhftniegm"), "sqome", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("reagphsqflbitdcjmkno"), "smfte", 1, 1, 6);
    _test_string_find_first_of_pointer_size_size(gl_string("jitlfrqemsdhkopncabg"), "ciboh", 1, 2, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("mhtaepscdnrjqgbkifol"), "haois", 1, 4, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("tocesrfmnglpbjihqadk"), "abfki", 1, 5, 6);
    _test_string_find_first_of_pointer_size_size(gl_string("lpfmctjrhdagneskbqoi"), "frdkocntmq", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("lsmqaepkdhncirbtjfgo"), "oasbpedlnr", 1, 1, 19);
    _test_string_find_first_of_pointer_size_size(gl_string("epoiqmtldrabnkjhcfsg"), "kltqmhgand", 1, 5, 4);
    _test_string_find_first_of_pointer_size_size(gl_string("emgasrilpknqojhtbdcf"), "gdtfjchpmr", 1, 9, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("hnfiagdpcklrjetqbsom"), "ponmcqblet", 1, 10, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("nsdfebgajhmtricpoklq"), "sgphqdnofeiklatbcmjr", 1, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("atjgfsdlpobmeiqhncrk"), "ljqprsmigtfoneadckbh", 1, 1, 7);
    _test_string_find_first_of_pointer_size_size(gl_string("sitodfgnrejlahcbmqkp"), "ligeojhafnkmrcsqtbdp", 1, 10, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("fraghmbiceknltjpqosd"), "lsimqfnjarbopedkhcgt", 1, 19, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("pmafenlhqtdbkirjsogc"), "abedmfjlghniorcqptks", 1, 20, 1);
    _test_string_find_first_of_pointer_size_size(gl_string("pihgmoeqtnakrjslcbfd"), "", 10, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("gjdkeprctqblnhiafsom"), "hqtoa", 10, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("mkpnblfdsahrcqijteog"), "cahif", 10, 1, 12);
    _test_string_find_first_of_pointer_size_size(gl_string("gckarqnelodfjhmbptis"), "kehis", 10, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("gqpskidtbclomahnrjfe"), "kdlmh", 10, 4, 10);
    _test_string_find_first_of_pointer_size_size(gl_string("pkldjsqrfgitbhmaecno"), "paeql", 10, 5, 15);
    _test_string_find_first_of_pointer_size_size(gl_string("aftsijrbeklnmcdqhgop"), "aghoqiefnb", 10, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("mtlgdrhafjkbiepqnsoc"), "jrbqaikpdo", 10, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("pqgirnaefthokdmbsclj"), "smjonaeqcl", 10, 5, 11);
    _test_string_find_first_of_pointer_size_size(gl_string("kpdbgjmtherlsfcqoina"), "eqbdrkcfah", 10, 9, 10);
    _test_string_find_first_of_pointer_size_size(gl_string("jrlbothiknqmdgcfasep"), "kapmsienhf", 10, 10, 11);
    _test_string_find_first_of_pointer_size_size(gl_string("mjogldqferckabinptsh"), "jpqotrlenfcsbhkaimdg", 10, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("apoklnefbhmgqcdrisjt"), "jlbmhnfgtcqprikeados", 10, 1, 18);
    _test_string_find_first_of_pointer_size_size(gl_string("ifeopcnrjbhkdgatmqls"), "stgbhfmdaljnpqoicker", 10, 10, 10);
    _test_string_find_first_of_pointer_size_size(gl_string("ckqhaiesmjdnrgolbtpf"), "oihcetflbjagdsrkmqpn", 10, 19, 10);
    _test_string_find_first_of_pointer_size_size(gl_string("bnlgapfimcoterskqdjh"), "adtclebmnpjsrqfkigoh", 10, 20, 10);
    _test_string_find_first_of_pointer_size_size(gl_string("kgdlrobpmjcthqsafeni"), "", 19, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("dfkechomjapgnslbtqir"), "beafg", 19, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rloadknfbqtgmhcsipje"), "iclat", 19, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("mgjhkolrnadqbpetcifs"), "rkhnf", 19, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("cmlfakiojdrgtbsphqen"), "clshq", 19, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("kghbfipeomsntdalrqjc"), "dtcoj", 19, 5, 19);
    _test_string_find_first_of_pointer_size_size(gl_string("eldiqckrnmtasbghjfpo"), "rqosnjmfth", 19, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("abqjcfedgotihlnspkrm"), "siatdfqglh", 19, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("qfbadrtjsimkolcenhpg"), "mrlshtpgjq", 19, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("abseghclkjqifmtodrnp"), "adlcskgqjt", 19, 9, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ibmsnlrjefhtdokacqpg"), "drshcjknaf", 19, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("mrkfciqjebaponsthldg"), "etsaqroinghpkjdlfcbm", 19, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("mjkticdeoqshpalrfbgn"), "sgepdnkqliambtrocfhj", 19, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rqnoclbdejgiphtfsakm"), "nlmcjaqgbsortfdihkpe", 19, 10, 19);
    _test_string_find_first_of_pointer_size_size(gl_string("plkqbhmtfaeodjcrsing"), "racfnpmosldibqkghjet", 19, 19, 19);
    _test_string_find_first_of_pointer_size_size(gl_string("oegalhmstjrfickpbndq"), "fjhdsctkqeiolagrnmbp", 19, 20, 19);
    _test_string_find_first_of_pointer_size_size(gl_string("rdtgjcaohpblniekmsfq"), "", 20, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ofkqbnjetrmsaidphglc"), "ejanp", 20, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("grkpahljcftesdmonqib"), "odife", 20, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("jimlgbhfqkteospardcn"), "okaqd", 20, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("gftenihpmslrjkqadcob"), "lcdbi", 20, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("bmhldogtckrfsanijepq"), "fsqbj", 20, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("nfqkrpjdesabgtlcmoih"), "bigdomnplq", 20, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("focalnrpiqmdkstehbjg"), "apiblotgcd", 20, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rhqdspkmebiflcotnjga"), "acfhdenops", 20, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rahdtmsckfboqlpniegj"), "jopdeamcrk", 20, 9, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("fbkeiopclstmdqranjhg"), "trqncbkgmh", 20, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("lifhpdgmbconstjeqark"), "tomglrkencbsfjqpihda", 20, 0, gl_string::npos);
}


void test_string_find_first_of_pointer_size_size3()
{
    _test_string_find_first_of_pointer_size_size(gl_string("pboqganrhedjmltsicfk"), "gbkhdnpoietfcmrslajq", 20, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("klchabsimetjnqgorfpd"), "rtfnmbsglkjaichoqedp", 20, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("sirfgmjqhctndbklaepo"), "ohkmdpfqbsacrtjnlgei", 20, 19, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rlbdsiceaonqjtfpghkm"), "dlbrteoisgphmkncajfq", 20, 20, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ecgdanriptblhjfqskom"), "", 21, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("fdmiarlpgcskbhoteqjn"), "sjrlo", 21, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("rlbstjqopignecmfadkh"), "qjpor", 21, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("grjpqmbshektdolcafni"), "odhfn", 21, 2, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("sakfcohtqnibprjmlged"), "qtfin", 21, 4, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("mjtdglasihqpocebrfkn"), "hpqfo", 21, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("okaplfrntghqbmeicsdj"), "fabmertkos", 21, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("sahngemrtcjidqbklfpo"), "brqtgkmaej", 21, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("dlmsipcnekhbgoaftqjr"), "nfrdeihsgl", 21, 5, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("ahegrmqnoiklpfsdbcjt"), "hlfrosekpi", 21, 9, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("hdsjbnmlegtkqripacof"), "atgbkrjdsm", 21, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("pcnedrfjihqbalkgtoms"), "blnrptjgqmaifsdkhoec", 21, 0, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("qjidealmtpskrbfhocng"), "ctpmdahebfqjgknloris", 21, 1, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("qeindtagmokpfhsclrbj"), "apnkeqthrmlbfodiscgj", 21, 10, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("kpfegbjhsrnodltqciam"), "jdgictpframeoqlsbknh", 21, 19, gl_string::npos);
    _test_string_find_first_of_pointer_size_size(gl_string("hnbrcplsjfgiktoedmaq"), "qprlsfojamgndekthibc", 21, 20, gl_string::npos);
}


void
_test_string_find_first_of_string_size(const gl_string& s, const gl_string& str, size_t pos, size_t x)
{
    TS_ASSERT(s.find_first_of(str, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(pos <= x && x < s.size());
}


void
_test_string_find_first_of_string_size(const gl_string& s, const gl_string& str, size_t x)
{
    TS_ASSERT(s.find_first_of(str) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x < s.size());
}


void test_string_find_first_of_string_size0()
{
    _test_string_find_first_of_string_size(gl_string(""), gl_string(""), 0, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string(""), gl_string("laenf"), 0, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string(""), gl_string("pqlnkmbdjo"), 0, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string(""), gl_string("qkamfogpnljdcshbreti"), 0, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string(""), gl_string(""), 1, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string(""), gl_string("bjaht"), 1, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string(""), gl_string("hjlcmgpket"), 1, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string(""), gl_string("htaobedqikfplcgjsmrn"), 1, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("fodgq"), gl_string(""), 0, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("qanej"), gl_string("dfkap"), 0, 1);
    _test_string_find_first_of_string_size(gl_string("clbao"), gl_string("ihqrfebgad"), 0, 2);
    _test_string_find_first_of_string_size(gl_string("mekdn"), gl_string("ngtjfcalbseiqrphmkdo"), 0, 0);
    _test_string_find_first_of_string_size(gl_string("srdfq"), gl_string(""), 1, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("oemth"), gl_string("ikcrq"), 1, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("cdaih"), gl_string("dmajblfhsg"), 1, 1);
    _test_string_find_first_of_string_size(gl_string("qohtk"), gl_string("oqftjhdmkgsblacenirp"), 1, 1);
    _test_string_find_first_of_string_size(gl_string("cshmd"), gl_string(""), 2, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("lhcdo"), gl_string("oebqi"), 2, 4);
    _test_string_find_first_of_string_size(gl_string("qnsoh"), gl_string("kojhpmbsfe"), 2, 2);
    _test_string_find_first_of_string_size(gl_string("pkrof"), gl_string("acbsjqogpltdkhinfrem"), 2, 2);
    _test_string_find_first_of_string_size(gl_string("fmtsp"), gl_string(""), 4, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("khbpm"), gl_string("aobjd"), 4, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("pbsji"), gl_string("pcbahntsje"), 4, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("mprdj"), gl_string("fhepcrntkoagbmldqijs"), 4, 4);
    _test_string_find_first_of_string_size(gl_string("eqmpa"), gl_string(""), 5, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("omigs"), gl_string("kocgb"), 5, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("onmje"), gl_string("fbslrjiqkm"), 5, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("oqmrj"), gl_string("jeidpcmalhfnqbgtrsko"), 5, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("schfa"), gl_string(""), 6, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("igdsc"), gl_string("qngpd"), 6, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("brqgo"), gl_string("rodhqklgmb"), 6, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("tnrph"), gl_string("thdjgafrlbkoiqcspmne"), 6, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("hcjitbfapl"), gl_string(""), 0, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("daiprenocl"), gl_string("ashjd"), 0, 0);
    _test_string_find_first_of_string_size(gl_string("litpcfdghe"), gl_string("mgojkldsqh"), 0, 0);
    _test_string_find_first_of_string_size(gl_string("aidjksrolc"), gl_string("imqnaghkfrdtlopbjesc"), 0, 0);
    _test_string_find_first_of_string_size(gl_string("qpghtfbaji"), gl_string(""), 1, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("gfshlcmdjr"), gl_string("nadkh"), 1, 3);
    _test_string_find_first_of_string_size(gl_string("nkodajteqp"), gl_string("ofdrqmkebl"), 1, 1);
    _test_string_find_first_of_string_size(gl_string("gbmetiprqd"), gl_string("bdfjqgatlksriohemnpc"), 1, 1);
    _test_string_find_first_of_string_size(gl_string("crnklpmegd"), gl_string(""), 5, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("jsbtafedoc"), gl_string("prqgn"), 5, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("qnmodrtkeb"), gl_string("pejafmnokr"), 5, 5);
    _test_string_find_first_of_string_size(gl_string("cpebqsfmnj"), gl_string("odnqkgijrhabfmcestlp"), 5, 5);
    _test_string_find_first_of_string_size(gl_string("lmofqdhpki"), gl_string(""), 9, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("hnefkqimca"), gl_string("rtjpa"), 9, 9);
    _test_string_find_first_of_string_size(gl_string("drtasbgmfp"), gl_string("ktsrmnqagd"), 9, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("lsaijeqhtr"), gl_string("rtdhgcisbnmoaqkfpjle"), 9, 9);
    _test_string_find_first_of_string_size(gl_string("elgofjmbrq"), gl_string(""), 10, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("mjqdgalkpc"), gl_string("dplqa"), 10, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("kthqnfcerm"), gl_string("dkacjoptns"), 10, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("dfsjhanorc"), gl_string("hqfimtrgnbekpdcsjalo"), 10, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("eqsgalomhb"), gl_string(""), 11, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("akiteljmoh"), gl_string("lofbc"), 11, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("hlbdfreqjo"), gl_string("astoegbfpn"), 11, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("taqobhlerg"), gl_string("pdgreqomsncafklhtibj"), 11, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("snafbdlghrjkpqtoceim"), gl_string(""), 0, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("aemtbrgcklhndjisfpoq"), gl_string("lbtqd"), 0, 3);
    _test_string_find_first_of_string_size(gl_string("pnracgfkjdiholtbqsem"), gl_string("tboimldpjh"), 0, 0);
    _test_string_find_first_of_string_size(gl_string("dicfltehbsgrmojnpkaq"), gl_string("slcerthdaiqjfnobgkpm"), 0, 0);
    _test_string_find_first_of_string_size(gl_string("jlnkraeodhcspfgbqitm"), gl_string(""), 1, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("lhosrngtmfjikbqpcade"), gl_string("aqibs"), 1, 3);
    _test_string_find_first_of_string_size(gl_string("rbtaqjhgkneisldpmfoc"), gl_string("gtfblmqinc"), 1, 1);
    _test_string_find_first_of_string_size(gl_string("gpifsqlrdkbonjtmheca"), gl_string("mkqpbtdalgniorhfescj"), 1, 1);
    _test_string_find_first_of_string_size(gl_string("hdpkobnsalmcfijregtq"), gl_string(""), 10, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("jtlshdgqaiprkbcoenfm"), gl_string("pblas"), 10, 10);
    _test_string_find_first_of_string_size(gl_string("fkdrbqltsgmcoiphneaj"), gl_string("arosdhcfme"), 10, 10);
    _test_string_find_first_of_string_size(gl_string("crsplifgtqedjohnabmk"), gl_string("blkhjeogicatqfnpdmsr"), 10, 10);
    _test_string_find_first_of_string_size(gl_string("niptglfbosehkamrdqcj"), gl_string(""), 19, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("copqdhstbingamjfkler"), gl_string("djkqc"), 19, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("mrtaefilpdsgocnhqbjk"), gl_string("lgokshjtpb"), 19, 19);
    _test_string_find_first_of_string_size(gl_string("kojatdhlcmigpbfrqnes"), gl_string("bqjhtkfepimcnsgrlado"), 19, 19);
    _test_string_find_first_of_string_size(gl_string("eaintpchlqsbdgrkjofm"), gl_string(""), 20, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("gjnhidfsepkrtaqbmclo"), gl_string("nocfa"), 20, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("spocfaktqdbiejlhngmr"), gl_string("bgtajmiedc"), 20, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("rphmlekgfscndtaobiqj"), gl_string("lsckfnqgdahejiopbtmr"), 20, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("liatsqdoegkmfcnbhrpj"), gl_string(""), 21, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("binjagtfldkrspcomqeh"), gl_string("gfsrt"), 21, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("latkmisecnorjbfhqpdg"), gl_string("pfsocbhjtm"), 21, gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("lecfratdjkhnsmqpoigb"), gl_string("tpflmdnoicjgkberhqsa"), 21, gl_string::npos);
}


void test_string_find_first_of_string_size1()
{
    _test_string_find_first_of_string_size(gl_string(""), gl_string(""), gl_string::npos);
    _test_string_find_first_of_string_size(gl_string(""), gl_string("laenf"), gl_string::npos);
    _test_string_find_first_of_string_size(gl_string(""), gl_string("pqlnkmbdjo"), gl_string::npos);
    _test_string_find_first_of_string_size(gl_string(""), gl_string("qkamfogpnljdcshbreti"), gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("nhmko"), gl_string(""), gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("lahfb"), gl_string("irkhs"), 2);
    _test_string_find_first_of_string_size(gl_string("gmfhd"), gl_string("kantesmpgj"), 0);
    _test_string_find_first_of_string_size(gl_string("odaft"), gl_string("oknlrstdpiqmjbaghcfe"), 0);
    _test_string_find_first_of_string_size(gl_string("eolhfgpjqk"), gl_string(""), gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("nbatdlmekr"), gl_string("bnrpe"), 0);
    _test_string_find_first_of_string_size(gl_string("jdmciepkaq"), gl_string("jtdaefblso"), 0);
    _test_string_find_first_of_string_size(gl_string("hkbgspoflt"), gl_string("oselktgbcapndfjihrmq"), 0);
    _test_string_find_first_of_string_size(gl_string("gprdcokbnjhlsfmtieqa"), gl_string(""), gl_string::npos);
    _test_string_find_first_of_string_size(gl_string("qjghlnftcaismkropdeb"), gl_string("bjaht"), 1);
    _test_string_find_first_of_string_size(gl_string("pnalfrdtkqcmojiesbhg"), gl_string("hjlcmgpket"), 0);
    _test_string_find_first_of_string_size(gl_string("pniotcfrhqsmgdkjbael"), gl_string("htaobedqikfplcgjsmrn"), 0);
}


void
_test_string_find_last_not_of_char_size(const gl_string& s, char c, size_t pos,
     size_t x)
{
    TS_ASSERT(s.find_last_not_of(c, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x <= pos && x < s.size());
}


void
_test_string_find_last_not_of_char_size(const gl_string& s, char c, size_t x)
{
    TS_ASSERT(s.find_last_not_of(c) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x < s.size());
}

void test_string_find_last_not_of_char_size() 
    {
    _test_string_find_last_not_of_char_size(gl_string(""), 'i', 0, gl_string::npos);
    _test_string_find_last_not_of_char_size(gl_string(""), 'i', 1, gl_string::npos);
    _test_string_find_last_not_of_char_size(gl_string("kitcj"), 'i', 0, 0);
    _test_string_find_last_not_of_char_size(gl_string("qkamf"), 'i', 1, 1);
    _test_string_find_last_not_of_char_size(gl_string("nhmko"), 'i', 2, 2);
    _test_string_find_last_not_of_char_size(gl_string("tpsaf"), 'i', 4, 4);
    _test_string_find_last_not_of_char_size(gl_string("lahfb"), 'i', 5, 4);
    _test_string_find_last_not_of_char_size(gl_string("irkhs"), 'i', 6, 4);
    _test_string_find_last_not_of_char_size(gl_string("gmfhdaipsr"), 'i', 0, 0);
    _test_string_find_last_not_of_char_size(gl_string("kantesmpgj"), 'i', 1, 1);
    _test_string_find_last_not_of_char_size(gl_string("odaftiegpm"), 'i', 5, 4);
    _test_string_find_last_not_of_char_size(gl_string("oknlrstdpi"), 'i', 9, 8);
    _test_string_find_last_not_of_char_size(gl_string("eolhfgpjqk"), 'i', 10, 9);
    _test_string_find_last_not_of_char_size(gl_string("pcdrofikas"), 'i', 11, 9);
    _test_string_find_last_not_of_char_size(gl_string("nbatdlmekrgcfqsophij"), 'i', 0, 0);
    _test_string_find_last_not_of_char_size(gl_string("bnrpehidofmqtcksjgla"), 'i', 1, 1);
    _test_string_find_last_not_of_char_size(gl_string("jdmciepkaqgotsrfnhlb"), 'i', 10, 10);
    _test_string_find_last_not_of_char_size(gl_string("jtdaefblsokrmhpgcnqi"), 'i', 19, 18);
    _test_string_find_last_not_of_char_size(gl_string("hkbgspofltajcnedqmri"), 'i', 20, 18);
    _test_string_find_last_not_of_char_size(gl_string("oselktgbcapndfjihrmq"), 'i', 21, 19);

    _test_string_find_last_not_of_char_size(gl_string(""), 'i', gl_string::npos);
    _test_string_find_last_not_of_char_size(gl_string("csope"), 'i', 4);
    _test_string_find_last_not_of_char_size(gl_string("gfsmthlkon"), 'i', 9);
    _test_string_find_last_not_of_char_size(gl_string("laenfsbridchgotmkqpj"), 'i', 19);
    }


void
_test_string_find_last_not_of_pointer_size(const gl_string& s, const char* str, size_t pos,
     size_t x)
{
    TS_ASSERT(s.find_last_not_of(str, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x <= pos && x < s.size());
}


void
_test_string_find_last_not_of_pointer_size(const gl_string& s, const char* str, size_t x)
{
    TS_ASSERT(s.find_last_not_of(str) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x < s.size());
}


void test_string_find_last_not_of_pointer_size0()
{
    _test_string_find_last_not_of_pointer_size(gl_string(""), "", 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string(""), "laenf", 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string(""), "pqlnkmbdjo", 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string(""), "qkamfogpnljdcshbreti", 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string(""), "", 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string(""), "bjaht", 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string(""), "hjlcmgpket", 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string(""), "htaobedqikfplcgjsmrn", 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("fodgq"), "", 0, 0);
    _test_string_find_last_not_of_pointer_size(gl_string("qanej"), "dfkap", 0, 0);
    _test_string_find_last_not_of_pointer_size(gl_string("clbao"), "ihqrfebgad", 0, 0);
    _test_string_find_last_not_of_pointer_size(gl_string("mekdn"), "ngtjfcalbseiqrphmkdo", 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("srdfq"), "", 1, 1);
    _test_string_find_last_not_of_pointer_size(gl_string("oemth"), "ikcrq", 1, 1);
    _test_string_find_last_not_of_pointer_size(gl_string("cdaih"), "dmajblfhsg", 1, 0);
    _test_string_find_last_not_of_pointer_size(gl_string("qohtk"), "oqftjhdmkgsblacenirp", 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("cshmd"), "", 2, 2);
    _test_string_find_last_not_of_pointer_size(gl_string("lhcdo"), "oebqi", 2, 2);
    _test_string_find_last_not_of_pointer_size(gl_string("qnsoh"), "kojhpmbsfe", 2, 1);
    _test_string_find_last_not_of_pointer_size(gl_string("pkrof"), "acbsjqogpltdkhinfrem", 2, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("fmtsp"), "", 4, 4);
    _test_string_find_last_not_of_pointer_size(gl_string("khbpm"), "aobjd", 4, 4);
    _test_string_find_last_not_of_pointer_size(gl_string("pbsji"), "pcbahntsje", 4, 4);
    _test_string_find_last_not_of_pointer_size(gl_string("mprdj"), "fhepcrntkoagbmldqijs", 4, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("eqmpa"), "", 5, 4);
    _test_string_find_last_not_of_pointer_size(gl_string("omigs"), "kocgb", 5, 4);
    _test_string_find_last_not_of_pointer_size(gl_string("onmje"), "fbslrjiqkm", 5, 4);
    _test_string_find_last_not_of_pointer_size(gl_string("oqmrj"), "jeidpcmalhfnqbgtrsko", 5, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("schfa"), "", 6, 4);
    _test_string_find_last_not_of_pointer_size(gl_string("igdsc"), "qngpd", 6, 4);
    _test_string_find_last_not_of_pointer_size(gl_string("brqgo"), "rodhqklgmb", 6, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("tnrph"), "thdjgafrlbkoiqcspmne", 6, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("hcjitbfapl"), "", 0, 0);
    _test_string_find_last_not_of_pointer_size(gl_string("daiprenocl"), "ashjd", 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("litpcfdghe"), "mgojkldsqh", 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("aidjksrolc"), "imqnaghkfrdtlopbjesc", 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("qpghtfbaji"), "", 1, 1);
    _test_string_find_last_not_of_pointer_size(gl_string("gfshlcmdjr"), "nadkh", 1, 1);
    _test_string_find_last_not_of_pointer_size(gl_string("nkodajteqp"), "ofdrqmkebl", 1, 0);
    _test_string_find_last_not_of_pointer_size(gl_string("gbmetiprqd"), "bdfjqgatlksriohemnpc", 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("crnklpmegd"), "", 5, 5);
    _test_string_find_last_not_of_pointer_size(gl_string("jsbtafedoc"), "prqgn", 5, 5);
    _test_string_find_last_not_of_pointer_size(gl_string("qnmodrtkeb"), "pejafmnokr", 5, 4);
    _test_string_find_last_not_of_pointer_size(gl_string("cpebqsfmnj"), "odnqkgijrhabfmcestlp", 5, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("lmofqdhpki"), "", 9, 9);
    _test_string_find_last_not_of_pointer_size(gl_string("hnefkqimca"), "rtjpa", 9, 8);
    _test_string_find_last_not_of_pointer_size(gl_string("drtasbgmfp"), "ktsrmnqagd", 9, 9);
    _test_string_find_last_not_of_pointer_size(gl_string("lsaijeqhtr"), "rtdhgcisbnmoaqkfpjle", 9, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("elgofjmbrq"), "", 10, 9);
    _test_string_find_last_not_of_pointer_size(gl_string("mjqdgalkpc"), "dplqa", 10, 9);
    _test_string_find_last_not_of_pointer_size(gl_string("kthqnfcerm"), "dkacjoptns", 10, 9);
    _test_string_find_last_not_of_pointer_size(gl_string("dfsjhanorc"), "hqfimtrgnbekpdcsjalo", 10, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("eqsgalomhb"), "", 11, 9);
    _test_string_find_last_not_of_pointer_size(gl_string("akiteljmoh"), "lofbc", 11, 9);
    _test_string_find_last_not_of_pointer_size(gl_string("hlbdfreqjo"), "astoegbfpn", 11, 8);
    _test_string_find_last_not_of_pointer_size(gl_string("taqobhlerg"), "pdgreqomsncafklhtibj", 11, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("snafbdlghrjkpqtoceim"), "", 0, 0);
    _test_string_find_last_not_of_pointer_size(gl_string("aemtbrgcklhndjisfpoq"), "lbtqd", 0, 0);
    _test_string_find_last_not_of_pointer_size(gl_string("pnracgfkjdiholtbqsem"), "tboimldpjh", 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("dicfltehbsgrmojnpkaq"), "slcerthdaiqjfnobgkpm", 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("jlnkraeodhcspfgbqitm"), "", 1, 1);
    _test_string_find_last_not_of_pointer_size(gl_string("lhosrngtmfjikbqpcade"), "aqibs", 1, 1);
    _test_string_find_last_not_of_pointer_size(gl_string("rbtaqjhgkneisldpmfoc"), "gtfblmqinc", 1, 0);
    _test_string_find_last_not_of_pointer_size(gl_string("gpifsqlrdkbonjtmheca"), "mkqpbtdalgniorhfescj", 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("hdpkobnsalmcfijregtq"), "", 10, 10);
    _test_string_find_last_not_of_pointer_size(gl_string("jtlshdgqaiprkbcoenfm"), "pblas", 10, 9);
    _test_string_find_last_not_of_pointer_size(gl_string("fkdrbqltsgmcoiphneaj"), "arosdhcfme", 10, 9);
    _test_string_find_last_not_of_pointer_size(gl_string("crsplifgtqedjohnabmk"), "blkhjeogicatqfnpdmsr", 10, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("niptglfbosehkamrdqcj"), "", 19, 19);
    _test_string_find_last_not_of_pointer_size(gl_string("copqdhstbingamjfkler"), "djkqc", 19, 19);
    _test_string_find_last_not_of_pointer_size(gl_string("mrtaefilpdsgocnhqbjk"), "lgokshjtpb", 19, 16);
    _test_string_find_last_not_of_pointer_size(gl_string("kojatdhlcmigpbfrqnes"), "bqjhtkfepimcnsgrlado", 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("eaintpchlqsbdgrkjofm"), "", 20, 19);
    _test_string_find_last_not_of_pointer_size(gl_string("gjnhidfsepkrtaqbmclo"), "nocfa", 20, 18);
    _test_string_find_last_not_of_pointer_size(gl_string("spocfaktqdbiejlhngmr"), "bgtajmiedc", 20, 19);
    _test_string_find_last_not_of_pointer_size(gl_string("rphmlekgfscndtaobiqj"), "lsckfnqgdahejiopbtmr", 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("liatsqdoegkmfcnbhrpj"), "", 21, 19);
    _test_string_find_last_not_of_pointer_size(gl_string("binjagtfldkrspcomqeh"), "gfsrt", 21, 19);
    _test_string_find_last_not_of_pointer_size(gl_string("latkmisecnorjbfhqpdg"), "pfsocbhjtm", 21, 19);
    _test_string_find_last_not_of_pointer_size(gl_string("lecfratdjkhnsmqpoigb"), "tpflmdnoicjgkberhqsa", 21, gl_string::npos);
}


void test_string_find_last_not_of_pointer_size1()
{
    _test_string_find_last_not_of_pointer_size(gl_string(""), "", gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string(""), "laenf", gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string(""), "pqlnkmbdjo", gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string(""), "qkamfogpnljdcshbreti", gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("nhmko"), "", 4);
    _test_string_find_last_not_of_pointer_size(gl_string("lahfb"), "irkhs", 4);
    _test_string_find_last_not_of_pointer_size(gl_string("gmfhd"), "kantesmpgj", 4);
    _test_string_find_last_not_of_pointer_size(gl_string("odaft"), "oknlrstdpiqmjbaghcfe", gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("eolhfgpjqk"), "", 9);
    _test_string_find_last_not_of_pointer_size(gl_string("nbatdlmekr"), "bnrpe", 8);
    _test_string_find_last_not_of_pointer_size(gl_string("jdmciepkaq"), "jtdaefblso", 9);
    _test_string_find_last_not_of_pointer_size(gl_string("hkbgspoflt"), "oselktgbcapndfjihrmq", gl_string::npos);
    _test_string_find_last_not_of_pointer_size(gl_string("gprdcokbnjhlsfmtieqa"), "", 19);
    _test_string_find_last_not_of_pointer_size(gl_string("qjghlnftcaismkropdeb"), "bjaht", 18);
    _test_string_find_last_not_of_pointer_size(gl_string("pnalfrdtkqcmojiesbhg"), "hjlcmgpket", 17);
    _test_string_find_last_not_of_pointer_size(gl_string("pniotcfrhqsmgdkjbael"), "htaobedqikfplcgjsmrn", gl_string::npos);
}


void
_test_string_find_last_not_of_pointer_size_size(const gl_string& s, const char* str, size_t pos,
     size_t n, size_t x)
{
    TS_ASSERT(s.find_last_not_of(str, pos, n) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x <= pos && x < s.size());
}


void test_string_find_last_not_of_pointer_size_size0()
{
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "", 0, 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "irkhs", 0, 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "kante", 0, 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "oknlr", 0, 2, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "pcdro", 0, 4, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "bnrpe", 0, 5, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "jtdaefblso", 0, 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "oselktgbca", 0, 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "eqgaplhckj", 0, 5, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "bjahtcmnlp", 0, 9, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "hjlcmgpket", 0, 10, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "htaobedqikfplcgjsmrn", 0, 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "hpqiarojkcdlsgnmfetb", 0, 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "dfkaprhjloqetcsimnbg", 0, 10, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "ihqrfebgadntlpmjksoc", 0, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "ngtjfcalbseiqrphmkdo", 0, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "", 1, 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "lbtqd", 1, 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "tboim", 1, 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "slcer", 1, 2, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "cbjfs", 1, 4, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "aqibs", 1, 5, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "gtfblmqinc", 1, 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "mkqpbtdalg", 1, 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "kphatlimcd", 1, 5, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "pblasqogic", 1, 9, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "arosdhcfme", 1, 10, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "blkhjeogicatqfnpdmsr", 1, 0, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "bmhineprjcoadgstflqk", 1, 1, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "djkqcmetslnghpbarfoi", 1, 10, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "lgokshjtpbemarcdqnfi", 1, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string(""), "bqjhtkfepimcnsgrlado", 1, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("eaint"), "", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("binja"), "gfsrt", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("latkm"), "pfsoc", 0, 1, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("lecfr"), "tpflm", 0, 2, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("eqkst"), "sgkec", 0, 4, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("cdafr"), "romds", 0, 5, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("prbhe"), "qhjistlgmr", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("lbisk"), "pedfirsglo", 0, 1, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hrlpd"), "aqcoslgrmk", 0, 5, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ehmja"), "dabckmepqj", 0, 9, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mhqgd"), "pqscrjthli", 0, 10, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("tgklq"), "kfphdcsjqmobliagtren", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bocjs"), "rokpefncljibsdhqtagm", 0, 1, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("grbsd"), "afionmkphlebtcjqsgrd", 0, 10, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ofjqr"), "aenmqplidhkofrjbctsg", 0, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("btlfi"), "osjmbtcadhiklegrpqnf", 0, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("clrgb"), "", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("tjmek"), "osmia", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bgstp"), "ckonl", 1, 1, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hstrk"), "ilcaj", 1, 2, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("kmspj"), "lasiq", 1, 4, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("tjboh"), "kfqmr", 1, 5, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ilbcj"), "klnitfaobg", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jkngf"), "gjhmdlqikp", 1, 1, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("gfcql"), "skbgtahqej", 1, 5, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dqtlg"), "bjsdgtlpkf", 1, 9, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bthpg"), "bjgfmnlkio", 1, 10, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dgsnq"), "lbhepotfsjdqigcnamkr", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rmfhp"), "tebangckmpsrqdlfojhi", 1, 1, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jfdam"), "joflqbdkhtegimscpanr", 1, 10, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("edapb"), "adpmcohetfbsrjinlqkg", 1, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("brfsm"), "iacldqjpfnogbsrhmetk", 1, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ndrhl"), "", 2, 0, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mrecp"), "otkgb", 2, 0, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("qlasf"), "cqsjl", 2, 1, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("smaqd"), "dpifl", 2, 2, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hjeni"), "oapht", 2, 4, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ocmfj"), "cifts", 2, 5, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hmftq"), "nmsckbgalo", 2, 0, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fklad"), "tpksqhamle", 2, 1, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dirnm"), "tpdrchmkji", 2, 5, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hrgdc"), "ijagfkblst", 2, 9, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ifakg"), "kpocsignjb", 2, 10, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ebrgd"), "pecqtkjsnbdrialgmohf", 2, 0, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rcjml"), "aiortphfcmkjebgsndql", 2, 1, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("peqmt"), "sdbkeamglhipojqftrcn", 2, 10, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("frehn"), "ljqncehgmfktroapidbs", 2, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("tqolf"), "rtcfodilamkbenjghqps", 2, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("cjgao"), "", 4, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("kjplq"), "mabns", 4, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("herni"), "bdnrp", 4, 1, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("tadrb"), "scidp", 4, 2, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pkfeo"), "agbjl", 4, 4, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hoser"), "jfmpr", 4, 5, 3);
    _test_string_find_last_not_of_pointer_size_size(gl_string("kgrsp"), "rbpefghsmj", 4, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pgejb"), "apsfntdoqc", 4, 1, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("thlnq"), "ndkjeisgcl", 4, 5, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("nbmit"), "rnfpqatdeo", 4, 9, 3);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jgmib"), "bntjlqrfik", 4, 10, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ncrfj"), "kcrtmpolnaqejghsfdbi", 4, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ncsik"), "lobheanpkmqidsrtcfgj", 4, 1, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("sgbfh"), "athdkljcnreqbgpmisof", 4, 10, 3);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dktbn"), "qkdmjialrscpbhefgont", 4, 19, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fthqm"), "dmasojntqleribkgfchp", 4, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("klopi"), "", 5, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dajhn"), "psthd", 5, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jbgno"), "rpmjd", 5, 1, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hkjae"), "dfsmk", 5, 2, 4);
}


void test_string_find_last_not_of_pointer_size_size1()
{
    _test_string_find_last_not_of_pointer_size_size(gl_string("gbhqo"), "skqne", 5, 4, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ktdor"), "kipnf", 5, 5, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ldprn"), "hmrnqdgifl", 5, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("egmjk"), "fsmjcdairn", 5, 1, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("armql"), "pcdgltbrfj", 5, 5, 3);
    _test_string_find_last_not_of_pointer_size_size(gl_string("cdhjo"), "aekfctpirg", 5, 9, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jcons"), "ledihrsgpf", 5, 10, 3);
    _test_string_find_last_not_of_pointer_size_size(gl_string("cbrkp"), "mqcklahsbtirgopefndj", 5, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fhgna"), "kmlthaoqgecrnpdbjfis", 5, 1, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ejfcd"), "sfhbamcdptojlkrenqgi", 5, 10, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("kqjhe"), "pbniofmcedrkhlstgaqj", 5, 19, 2);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pbdjl"), "mongjratcskbhqiepfdl", 5, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("gajqn"), "", 6, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("stedk"), "hrnat", 6, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("tjkaf"), "gsqdt", 6, 1, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dthpe"), "bspkd", 6, 2, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("klhde"), "ohcmb", 6, 4, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bhlki"), "heatr", 6, 5, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("lqmoh"), "pmblckedfn", 6, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mtqin"), "aceqmsrbik", 6, 1, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dpqbr"), "lmbtdehjrn", 6, 5, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("kdhmo"), "teqmcrlgib", 6, 9, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jblqp"), "njolbmspac", 6, 10, 3);
    _test_string_find_last_not_of_pointer_size_size(gl_string("qmjgl"), "pofnhidklamecrbqjgst", 6, 0, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rothp"), "jbhckmtgrqnosafedpli", 6, 1, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ghknq"), "dobntpmqklicsahgjerf", 6, 10, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("eopfi"), "tpdshainjkbfoemlrgcq", 6, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dsnmg"), "oldpfgeakrnitscbjmqh", 6, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jnkrfhotgl"), "", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dltjfngbko"), "rqegt", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bmjlpkiqde"), "dashm", 0, 1, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("skrflobnqm"), "jqirk", 0, 2, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jkpldtshrm"), "rckeg", 0, 4, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ghasdbnjqo"), "jscie", 0, 5, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("igrkhpbqjt"), "efsphndliq", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ikthdgcamf"), "gdicosleja", 0, 1, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pcofgeniam"), "qcpjibosfl", 0, 5, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rlfjgesqhc"), "lrhmefnjcq", 0, 9, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("itphbqsker"), "dtablcrseo", 0, 10, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("skjafcirqm"), "apckjsftedbhgomrnilq", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("tcqomarsfd"), "pcbrgflehjtiadnsokqm", 0, 1, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rocfeldqpk"), "nsiadegjklhobrmtqcpf", 0, 10, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("cfpegndlkt"), "cpmajdqnolikhgsbretf", 0, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fqbtnkeasj"), "jcflkntmgiqrphdosaeb", 0, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("shbcqnmoar"), "", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bdoshlmfin"), "ontrs", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("khfrebnsgq"), "pfkna", 1, 1, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("getcrsaoji"), "ekosa", 1, 2, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fjiknedcpq"), "anqhk", 1, 4, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("tkejgnafrm"), "jekca", 1, 5, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jnakolqrde"), "ikemsjgacf", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("lcjptsmgbe"), "arolgsjkhm", 1, 1, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("itfsmcjorl"), "oftkbldhre", 1, 5, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("omchkfrjea"), "gbkqdoeftl", 1, 9, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("cigfqkated"), "sqcflrgtim", 1, 10, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("tscenjikml"), "fmhbkislrjdpanogqcet", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("qcpaemsinf"), "rnioadktqlgpbcjsmhef", 1, 1, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("gltkojeipd"), "oakgtnldpsefihqmjcbr", 1, 10, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("qistfrgnmp"), "gbnaelosidmcjqktfhpr", 1, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bdnpfcqaem"), "akbripjhlosndcmqgfet", 1, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ectnhskflp"), "", 5, 0, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fgtianblpq"), "pijag", 5, 0, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mfeqklirnh"), "jrckd", 5, 1, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("astedncjhk"), "qcloh", 5, 2, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fhlqgcajbr"), "thlmp", 5, 4, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("epfhocmdng"), "qidmo", 5, 5, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("apcnsibger"), "lnegpsjqrd", 5, 0, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("aqkocrbign"), "rjqdablmfs", 5, 1, 4);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ijsmdtqgce"), "enkgpbsjaq", 5, 5, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("clobgsrken"), "kdsgoaijfh", 5, 9, 3);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jbhcfposld"), "trfqgmckbe", 5, 10, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("oqnpblhide"), "igetsracjfkdnpoblhqm", 5, 0, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("lroeasctif"), "nqctfaogirshlekbdjpm", 5, 1, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bpjlgmiedh"), "csehfgomljdqinbartkp", 5, 10, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pamkeoidrj"), "qahoegcmplkfsjbdnitr", 5, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("espogqbthk"), "dpteiajrqmsognhlfbkc", 5, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("shoiedtcjb"), "", 9, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ebcinjgads"), "tqbnh", 9, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dqmregkcfl"), "akmle", 9, 1, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ngcrieqajf"), "iqfkm", 9, 2, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("qosmilgnjb"), "tqjsr", 9, 4, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ikabsjtdfl"), "jplqg", 9, 5, 8);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ersmicafdh"), "oilnrbcgtj", 9, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fdnplotmgh"), "morkglpesn", 9, 1, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fdbicojerm"), "dmicerngat", 9, 5, 8);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mbtafndjcq"), "radgeskbtc", 9, 9, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mlenkpfdtc"), "ljikprsmqo", 9, 10, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ahlcifdqgs"), "trqihkcgsjamfdbolnpe", 9, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bgjemaltks"), "lqmthbsrekajgnofcipd", 9, 1, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pdhslbqrfc"), "jtalmedribkgqsopcnfh", 9, 10, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dirhtsnjkc"), "spqfoiclmtagejbndkrh", 9, 19, 3);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dlroktbcja"), "nmotklspigjrdhcfaebq", 9, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ncjpmaekbs"), "", 10, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hlbosgmrak"), "hpmsd", 10, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pqfhsgilen"), "qnpor", 10, 1, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("gqtjsbdckh"), "otdma", 10, 2, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("cfkqpjlegi"), "efhjg", 10, 4, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("beanrfodgj"), "odpte", 10, 5, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("adtkqpbjfi"), "bctdgfmolr", 10, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("iomkfthagj"), "oaklidrbqg", 10, 1, 9);
}


void test_string_find_last_not_of_pointer_size_size2()
{
    _test_string_find_last_not_of_pointer_size_size(gl_string("sdpcilonqj"), "dnjfsagktr", 10, 5, 8);
    _test_string_find_last_not_of_pointer_size_size(gl_string("gtfbdkqeml"), "nejaktmiqg", 10, 9, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bmeqgcdorj"), "pjqonlebsf", 10, 10, 8);
    _test_string_find_last_not_of_pointer_size_size(gl_string("etqlcanmob"), "dshmnbtolcjepgaikfqr", 10, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("roqmkbdtia"), "iogfhpabtjkqlrnemcds", 10, 1, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("kadsithljf"), "ngridfabjsecpqltkmoh", 10, 10, 7);
    _test_string_find_last_not_of_pointer_size_size(gl_string("sgtkpbfdmh"), "athmknplcgofrqejsdib", 10, 19, 5);
    _test_string_find_last_not_of_pointer_size_size(gl_string("qgmetnabkl"), "ldobhmqcafnjtkeisgrp", 10, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("cqjohampgd"), "", 11, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hobitmpsan"), "aocjb", 11, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("tjehkpsalm"), "jbrnk", 11, 1, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ngfbojitcl"), "tqedg", 11, 2, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rcfkdbhgjo"), "nqskp", 11, 4, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("qghptonrea"), "eaqkl", 11, 5, 7);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hnprfgqjdl"), "reaoicljqm", 11, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hlmgabenti"), "lsftgajqpm", 11, 1, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ofcjanmrbs"), "rlpfogmits", 11, 5, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jqedtkornm"), "shkncmiaqj", 11, 9, 7);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rfedlasjmg"), "fpnatrhqgs", 11, 10, 8);
    _test_string_find_last_not_of_pointer_size_size(gl_string("talpqjsgkm"), "sjclemqhnpdbgikarfot", 11, 0, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("lrkcbtqpie"), "otcmedjikgsfnqbrhpla", 11, 1, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("cipogdskjf"), "bonsaefdqiprkhlgtjcm", 11, 10, 8);
    _test_string_find_last_not_of_pointer_size_size(gl_string("nqedcojahi"), "egpscmahijlfnkrodqtb", 11, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hefnrkmctj"), "kmqbfepjthgilscrndoa", 11, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("atqirnmekfjolhpdsgcb"), "", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("echfkmlpribjnqsaogtd"), "prboq", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("qnhiftdgcleajbpkrosm"), "fjcqh", 0, 1, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("chamfknorbedjitgslpq"), "fmosa", 0, 2, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("njhqpibfmtlkaecdrgso"), "qdbok", 0, 4, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ebnghfsqkprmdcljoiat"), "amslg", 0, 5, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("letjomsgihfrpqbkancd"), "smpltjneqb", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("nblgoipcrqeaktshjdmf"), "flitskrnge", 0, 1, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("cehkbngtjoiflqapsmrd"), "pgqihmlbef", 0, 5, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mignapfoklbhcqjetdrs"), "cfpdqjtgsb", 0, 9, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ceatbhlsqjgpnokfrmdi"), "htpsiaflom", 0, 10, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ocihkjgrdelpfnmastqb"), "kpjfiaceghsrdtlbnomq", 0, 0, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("noelgschdtbrjfmiqkap"), "qhtbomidljgafneksprc", 0, 1, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dkclqfombepritjnghas"), "nhtjobkcefldimpsaqgr", 0, 10, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("miklnresdgbhqcojftap"), "prabcjfqnoeskilmtgdh", 0, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("htbcigojaqmdkfrnlsep"), "dtrgmchilkasqoebfpjn", 0, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("febhmqtjanokscdirpgl"), "", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("loakbsqjpcrdhftniegm"), "sqome", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("reagphsqflbitdcjmkno"), "smfte", 1, 1, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jitlfrqemsdhkopncabg"), "ciboh", 1, 2, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mhtaepscdnrjqgbkifol"), "haois", 1, 4, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("tocesrfmnglpbjihqadk"), "abfki", 1, 5, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("lpfmctjrhdagneskbqoi"), "frdkocntmq", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("lsmqaepkdhncirbtjfgo"), "oasbpedlnr", 1, 1, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("epoiqmtldrabnkjhcfsg"), "kltqmhgand", 1, 5, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("emgasrilpknqojhtbdcf"), "gdtfjchpmr", 1, 9, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hnfiagdpcklrjetqbsom"), "ponmcqblet", 1, 10, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("nsdfebgajhmtricpoklq"), "sgphqdnofeiklatbcmjr", 1, 0, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("atjgfsdlpobmeiqhncrk"), "ljqprsmigtfoneadckbh", 1, 1, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("sitodfgnrejlahcbmqkp"), "ligeojhafnkmrcsqtbdp", 1, 10, 0);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fraghmbiceknltjpqosd"), "lsimqfnjarbopedkhcgt", 1, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pmafenlhqtdbkirjsogc"), "abedmfjlghniorcqptks", 1, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pihgmoeqtnakrjslcbfd"), "", 10, 0, 10);
    _test_string_find_last_not_of_pointer_size_size(gl_string("gjdkeprctqblnhiafsom"), "hqtoa", 10, 0, 10);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mkpnblfdsahrcqijteog"), "cahif", 10, 1, 10);
    _test_string_find_last_not_of_pointer_size_size(gl_string("gckarqnelodfjhmbptis"), "kehis", 10, 2, 10);
    _test_string_find_last_not_of_pointer_size_size(gl_string("gqpskidtbclomahnrjfe"), "kdlmh", 10, 4, 9);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pkldjsqrfgitbhmaecno"), "paeql", 10, 5, 10);
    _test_string_find_last_not_of_pointer_size_size(gl_string("aftsijrbeklnmcdqhgop"), "aghoqiefnb", 10, 0, 10);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mtlgdrhafjkbiepqnsoc"), "jrbqaikpdo", 10, 1, 10);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pqgirnaefthokdmbsclj"), "smjonaeqcl", 10, 5, 10);
    _test_string_find_last_not_of_pointer_size_size(gl_string("kpdbgjmtherlsfcqoina"), "eqbdrkcfah", 10, 9, 8);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jrlbothiknqmdgcfasep"), "kapmsienhf", 10, 10, 10);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mjogldqferckabinptsh"), "jpqotrlenfcsbhkaimdg", 10, 0, 10);
    _test_string_find_last_not_of_pointer_size_size(gl_string("apoklnefbhmgqcdrisjt"), "jlbmhnfgtcqprikeados", 10, 1, 10);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ifeopcnrjbhkdgatmqls"), "stgbhfmdaljnpqoicker", 10, 10, 8);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ckqhaiesmjdnrgolbtpf"), "oihcetflbjagdsrkmqpn", 10, 19, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bnlgapfimcoterskqdjh"), "adtclebmnpjsrqfkigoh", 10, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("kgdlrobpmjcthqsafeni"), "", 19, 0, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dfkechomjapgnslbtqir"), "beafg", 19, 0, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rloadknfbqtgmhcsipje"), "iclat", 19, 1, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mgjhkolrnadqbpetcifs"), "rkhnf", 19, 2, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("cmlfakiojdrgtbsphqen"), "clshq", 19, 4, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("kghbfipeomsntdalrqjc"), "dtcoj", 19, 5, 17);
    _test_string_find_last_not_of_pointer_size_size(gl_string("eldiqckrnmtasbghjfpo"), "rqosnjmfth", 19, 0, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("abqjcfedgotihlnspkrm"), "siatdfqglh", 19, 1, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("qfbadrtjsimkolcenhpg"), "mrlshtpgjq", 19, 5, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("abseghclkjqifmtodrnp"), "adlcskgqjt", 19, 9, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ibmsnlrjefhtdokacqpg"), "drshcjknaf", 19, 10, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mrkfciqjebaponsthldg"), "etsaqroinghpkjdlfcbm", 19, 0, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mjkticdeoqshpalrfbgn"), "sgepdnkqliambtrocfhj", 19, 1, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rqnoclbdejgiphtfsakm"), "nlmcjaqgbsortfdihkpe", 19, 10, 18);
    _test_string_find_last_not_of_pointer_size_size(gl_string("plkqbhmtfaeodjcrsing"), "racfnpmosldibqkghjet", 19, 19, 7);
    _test_string_find_last_not_of_pointer_size_size(gl_string("oegalhmstjrfickpbndq"), "fjhdsctkqeiolagrnmbp", 19, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rdtgjcaohpblniekmsfq"), "", 20, 0, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ofkqbnjetrmsaidphglc"), "ejanp", 20, 0, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("grkpahljcftesdmonqib"), "odife", 20, 1, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("jimlgbhfqkteospardcn"), "okaqd", 20, 2, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("gftenihpmslrjkqadcob"), "lcdbi", 20, 4, 18);
    _test_string_find_last_not_of_pointer_size_size(gl_string("bmhldogtckrfsanijepq"), "fsqbj", 20, 5, 18);
    _test_string_find_last_not_of_pointer_size_size(gl_string("nfqkrpjdesabgtlcmoih"), "bigdomnplq", 20, 0, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("focalnrpiqmdkstehbjg"), "apiblotgcd", 20, 1, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rhqdspkmebiflcotnjga"), "acfhdenops", 20, 5, 18);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rahdtmsckfboqlpniegj"), "jopdeamcrk", 20, 9, 18);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fbkeiopclstmdqranjhg"), "trqncbkgmh", 20, 10, 17);
    _test_string_find_last_not_of_pointer_size_size(gl_string("lifhpdgmbconstjeqark"), "tomglrkencbsfjqpihda", 20, 0, 19);
}


void test_string_find_last_not_of_pointer_size_size3()
{
    _test_string_find_last_not_of_pointer_size_size(gl_string("pboqganrhedjmltsicfk"), "gbkhdnpoietfcmrslajq", 20, 1, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("klchabsimetjnqgorfpd"), "rtfnmbsglkjaichoqedp", 20, 10, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("sirfgmjqhctndbklaepo"), "ohkmdpfqbsacrtjnlgei", 20, 19, 1);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rlbdsiceaonqjtfpghkm"), "dlbrteoisgphmkncajfq", 20, 20, gl_string::npos);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ecgdanriptblhjfqskom"), "", 21, 0, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("fdmiarlpgcskbhoteqjn"), "sjrlo", 21, 0, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("rlbstjqopignecmfadkh"), "qjpor", 21, 1, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("grjpqmbshektdolcafni"), "odhfn", 21, 2, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("sakfcohtqnibprjmlged"), "qtfin", 21, 4, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("mjtdglasihqpocebrfkn"), "hpqfo", 21, 5, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("okaplfrntghqbmeicsdj"), "fabmertkos", 21, 0, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("sahngemrtcjidqbklfpo"), "brqtgkmaej", 21, 1, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("dlmsipcnekhbgoaftqjr"), "nfrdeihsgl", 21, 5, 18);
    _test_string_find_last_not_of_pointer_size_size(gl_string("ahegrmqnoiklpfsdbcjt"), "hlfrosekpi", 21, 9, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hdsjbnmlegtkqripacof"), "atgbkrjdsm", 21, 10, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("pcnedrfjihqbalkgtoms"), "blnrptjgqmaifsdkhoec", 21, 0, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("qjidealmtpskrbfhocng"), "ctpmdahebfqjgknloris", 21, 1, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("qeindtagmokpfhsclrbj"), "apnkeqthrmlbfodiscgj", 21, 10, 19);
    _test_string_find_last_not_of_pointer_size_size(gl_string("kpfegbjhsrnodltqciam"), "jdgictpframeoqlsbknh", 21, 19, 7);
    _test_string_find_last_not_of_pointer_size_size(gl_string("hnbrcplsjfgiktoedmaq"), "qprlsfojamgndekthibc", 21, 20, gl_string::npos);
}


void
_test_string_find_last_not_of_string_size(const gl_string& s, const gl_string& str, size_t pos, size_t x)
{
    TS_ASSERT(s.find_last_not_of(str, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x <= pos && x < s.size());
}


void
_test_string_find_last_not_of_string_size(const gl_string& s, const gl_string& str, size_t x)
{
    TS_ASSERT(s.find_last_not_of(str) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x < s.size());
}


void test_string_find_last_not_of_string_size0()
{
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string(""), 0, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string("laenf"), 0, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string("pqlnkmbdjo"), 0, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string("qkamfogpnljdcshbreti"), 0, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string(""), 1, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string("bjaht"), 1, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string("hjlcmgpket"), 1, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string("htaobedqikfplcgjsmrn"), 1, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("fodgq"), gl_string(""), 0, 0);
    _test_string_find_last_not_of_string_size(gl_string("qanej"), gl_string("dfkap"), 0, 0);
    _test_string_find_last_not_of_string_size(gl_string("clbao"), gl_string("ihqrfebgad"), 0, 0);
    _test_string_find_last_not_of_string_size(gl_string("mekdn"), gl_string("ngtjfcalbseiqrphmkdo"), 0, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("srdfq"), gl_string(""), 1, 1);
    _test_string_find_last_not_of_string_size(gl_string("oemth"), gl_string("ikcrq"), 1, 1);
    _test_string_find_last_not_of_string_size(gl_string("cdaih"), gl_string("dmajblfhsg"), 1, 0);
    _test_string_find_last_not_of_string_size(gl_string("qohtk"), gl_string("oqftjhdmkgsblacenirp"), 1, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("cshmd"), gl_string(""), 2, 2);
    _test_string_find_last_not_of_string_size(gl_string("lhcdo"), gl_string("oebqi"), 2, 2);
    _test_string_find_last_not_of_string_size(gl_string("qnsoh"), gl_string("kojhpmbsfe"), 2, 1);
    _test_string_find_last_not_of_string_size(gl_string("pkrof"), gl_string("acbsjqogpltdkhinfrem"), 2, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("fmtsp"), gl_string(""), 4, 4);
    _test_string_find_last_not_of_string_size(gl_string("khbpm"), gl_string("aobjd"), 4, 4);
    _test_string_find_last_not_of_string_size(gl_string("pbsji"), gl_string("pcbahntsje"), 4, 4);
    _test_string_find_last_not_of_string_size(gl_string("mprdj"), gl_string("fhepcrntkoagbmldqijs"), 4, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("eqmpa"), gl_string(""), 5, 4);
    _test_string_find_last_not_of_string_size(gl_string("omigs"), gl_string("kocgb"), 5, 4);
    _test_string_find_last_not_of_string_size(gl_string("onmje"), gl_string("fbslrjiqkm"), 5, 4);
    _test_string_find_last_not_of_string_size(gl_string("oqmrj"), gl_string("jeidpcmalhfnqbgtrsko"), 5, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("schfa"), gl_string(""), 6, 4);
    _test_string_find_last_not_of_string_size(gl_string("igdsc"), gl_string("qngpd"), 6, 4);
    _test_string_find_last_not_of_string_size(gl_string("brqgo"), gl_string("rodhqklgmb"), 6, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("tnrph"), gl_string("thdjgafrlbkoiqcspmne"), 6, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("hcjitbfapl"), gl_string(""), 0, 0);
    _test_string_find_last_not_of_string_size(gl_string("daiprenocl"), gl_string("ashjd"), 0, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("litpcfdghe"), gl_string("mgojkldsqh"), 0, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("aidjksrolc"), gl_string("imqnaghkfrdtlopbjesc"), 0, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("qpghtfbaji"), gl_string(""), 1, 1);
    _test_string_find_last_not_of_string_size(gl_string("gfshlcmdjr"), gl_string("nadkh"), 1, 1);
    _test_string_find_last_not_of_string_size(gl_string("nkodajteqp"), gl_string("ofdrqmkebl"), 1, 0);
    _test_string_find_last_not_of_string_size(gl_string("gbmetiprqd"), gl_string("bdfjqgatlksriohemnpc"), 1, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("crnklpmegd"), gl_string(""), 5, 5);
    _test_string_find_last_not_of_string_size(gl_string("jsbtafedoc"), gl_string("prqgn"), 5, 5);
    _test_string_find_last_not_of_string_size(gl_string("qnmodrtkeb"), gl_string("pejafmnokr"), 5, 4);
    _test_string_find_last_not_of_string_size(gl_string("cpebqsfmnj"), gl_string("odnqkgijrhabfmcestlp"), 5, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("lmofqdhpki"), gl_string(""), 9, 9);
    _test_string_find_last_not_of_string_size(gl_string("hnefkqimca"), gl_string("rtjpa"), 9, 8);
    _test_string_find_last_not_of_string_size(gl_string("drtasbgmfp"), gl_string("ktsrmnqagd"), 9, 9);
    _test_string_find_last_not_of_string_size(gl_string("lsaijeqhtr"), gl_string("rtdhgcisbnmoaqkfpjle"), 9, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("elgofjmbrq"), gl_string(""), 10, 9);
    _test_string_find_last_not_of_string_size(gl_string("mjqdgalkpc"), gl_string("dplqa"), 10, 9);
    _test_string_find_last_not_of_string_size(gl_string("kthqnfcerm"), gl_string("dkacjoptns"), 10, 9);
    _test_string_find_last_not_of_string_size(gl_string("dfsjhanorc"), gl_string("hqfimtrgnbekpdcsjalo"), 10, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("eqsgalomhb"), gl_string(""), 11, 9);
    _test_string_find_last_not_of_string_size(gl_string("akiteljmoh"), gl_string("lofbc"), 11, 9);
    _test_string_find_last_not_of_string_size(gl_string("hlbdfreqjo"), gl_string("astoegbfpn"), 11, 8);
    _test_string_find_last_not_of_string_size(gl_string("taqobhlerg"), gl_string("pdgreqomsncafklhtibj"), 11, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("snafbdlghrjkpqtoceim"), gl_string(""), 0, 0);
    _test_string_find_last_not_of_string_size(gl_string("aemtbrgcklhndjisfpoq"), gl_string("lbtqd"), 0, 0);
    _test_string_find_last_not_of_string_size(gl_string("pnracgfkjdiholtbqsem"), gl_string("tboimldpjh"), 0, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("dicfltehbsgrmojnpkaq"), gl_string("slcerthdaiqjfnobgkpm"), 0, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("jlnkraeodhcspfgbqitm"), gl_string(""), 1, 1);
    _test_string_find_last_not_of_string_size(gl_string("lhosrngtmfjikbqpcade"), gl_string("aqibs"), 1, 1);
    _test_string_find_last_not_of_string_size(gl_string("rbtaqjhgkneisldpmfoc"), gl_string("gtfblmqinc"), 1, 0);
    _test_string_find_last_not_of_string_size(gl_string("gpifsqlrdkbonjtmheca"), gl_string("mkqpbtdalgniorhfescj"), 1, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("hdpkobnsalmcfijregtq"), gl_string(""), 10, 10);
    _test_string_find_last_not_of_string_size(gl_string("jtlshdgqaiprkbcoenfm"), gl_string("pblas"), 10, 9);
    _test_string_find_last_not_of_string_size(gl_string("fkdrbqltsgmcoiphneaj"), gl_string("arosdhcfme"), 10, 9);
    _test_string_find_last_not_of_string_size(gl_string("crsplifgtqedjohnabmk"), gl_string("blkhjeogicatqfnpdmsr"), 10, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("niptglfbosehkamrdqcj"), gl_string(""), 19, 19);
    _test_string_find_last_not_of_string_size(gl_string("copqdhstbingamjfkler"), gl_string("djkqc"), 19, 19);
    _test_string_find_last_not_of_string_size(gl_string("mrtaefilpdsgocnhqbjk"), gl_string("lgokshjtpb"), 19, 16);
    _test_string_find_last_not_of_string_size(gl_string("kojatdhlcmigpbfrqnes"), gl_string("bqjhtkfepimcnsgrlado"), 19, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("eaintpchlqsbdgrkjofm"), gl_string(""), 20, 19);
    _test_string_find_last_not_of_string_size(gl_string("gjnhidfsepkrtaqbmclo"), gl_string("nocfa"), 20, 18);
    _test_string_find_last_not_of_string_size(gl_string("spocfaktqdbiejlhngmr"), gl_string("bgtajmiedc"), 20, 19);
    _test_string_find_last_not_of_string_size(gl_string("rphmlekgfscndtaobiqj"), gl_string("lsckfnqgdahejiopbtmr"), 20, gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("liatsqdoegkmfcnbhrpj"), gl_string(""), 21, 19);
    _test_string_find_last_not_of_string_size(gl_string("binjagtfldkrspcomqeh"), gl_string("gfsrt"), 21, 19);
    _test_string_find_last_not_of_string_size(gl_string("latkmisecnorjbfhqpdg"), gl_string("pfsocbhjtm"), 21, 19);
    _test_string_find_last_not_of_string_size(gl_string("lecfratdjkhnsmqpoigb"), gl_string("tpflmdnoicjgkberhqsa"), 21, gl_string::npos);
}


void test_string_find_last_not_of_string_size1()
{
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string(""), gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string("laenf"), gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string("pqlnkmbdjo"), gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string(""), gl_string("qkamfogpnljdcshbreti"), gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("nhmko"), gl_string(""), 4);
    _test_string_find_last_not_of_string_size(gl_string("lahfb"), gl_string("irkhs"), 4);
    _test_string_find_last_not_of_string_size(gl_string("gmfhd"), gl_string("kantesmpgj"), 4);
    _test_string_find_last_not_of_string_size(gl_string("odaft"), gl_string("oknlrstdpiqmjbaghcfe"), gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("eolhfgpjqk"), gl_string(""), 9);
    _test_string_find_last_not_of_string_size(gl_string("nbatdlmekr"), gl_string("bnrpe"), 8);
    _test_string_find_last_not_of_string_size(gl_string("jdmciepkaq"), gl_string("jtdaefblso"), 9);
    _test_string_find_last_not_of_string_size(gl_string("hkbgspoflt"), gl_string("oselktgbcapndfjihrmq"), gl_string::npos);
    _test_string_find_last_not_of_string_size(gl_string("gprdcokbnjhlsfmtieqa"), gl_string(""), 19);
    _test_string_find_last_not_of_string_size(gl_string("qjghlnftcaismkropdeb"), gl_string("bjaht"), 18);
    _test_string_find_last_not_of_string_size(gl_string("pnalfrdtkqcmojiesbhg"), gl_string("hjlcmgpket"), 17);
    _test_string_find_last_not_of_string_size(gl_string("pniotcfrhqsmgdkjbael"), gl_string("htaobedqikfplcgjsmrn"), gl_string::npos);
}


void
_test_string_find_last_of_char_size(const gl_string& s, char c, size_t pos,
     size_t x)
{
    TS_ASSERT(s.find_last_of(c, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x <= pos && x < s.size());
}


void
_test_string_find_last_of_char_size(const gl_string& s, char c, size_t x)
{
    TS_ASSERT(s.find_last_of(c) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x < s.size());
}

  void test_string_find_last_of_char_size()
    {
    _test_string_find_last_of_char_size(gl_string(""), 'm', 0, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string(""), 'm', 1, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("kitcj"), 'm', 0, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("qkamf"), 'm', 1, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("nhmko"), 'm', 2, 2);
    _test_string_find_last_of_char_size(gl_string("tpsaf"), 'm', 4, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("lahfb"), 'm', 5, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("irkhs"), 'm', 6, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("gmfhdaipsr"), 'm', 0, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("kantesmpgj"), 'm', 1, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("odaftiegpm"), 'm', 5, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("oknlrstdpi"), 'm', 9, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("eolhfgpjqk"), 'm', 10, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("pcdrofikas"), 'm', 11, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("nbatdlmekrgcfqsophij"), 'm', 0, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("bnrpehidofmqtcksjgla"), 'm', 1, gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("jdmciepkaqgotsrfnhlb"), 'm', 10, 2);
    _test_string_find_last_of_char_size(gl_string("jtdaefblsokrmhpgcnqi"), 'm', 19, 12);
    _test_string_find_last_of_char_size(gl_string("hkbgspofltajcnedqmri"), 'm', 20, 17);
    _test_string_find_last_of_char_size(gl_string("oselktgbcapndfjihrmq"), 'm', 21, 18);

    _test_string_find_last_of_char_size(gl_string(""), 'm', gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("csope"), 'm', gl_string::npos);
    _test_string_find_last_of_char_size(gl_string("gfsmthlkon"), 'm', 3);
    _test_string_find_last_of_char_size(gl_string("laenfsbridchgotmkqpj"), 'm', 15);
    }


void
_test_string_find_last_of_pointer_size(const gl_string& s, const char* str, size_t pos,
     size_t x)
{
    TS_ASSERT(s.find_last_of(str, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x <= pos && x < s.size());
}


void
_test_string_find_last_of_pointer_size(const gl_string& s, const char* str, size_t x)
{
    TS_ASSERT(s.find_last_of(str) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x < s.size());
}


void test_string_find_last_of_pointer_size0()
{
    _test_string_find_last_of_pointer_size(gl_string(""), "", 0, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string(""), "laenf", 0, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string(""), "pqlnkmbdjo", 0, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string(""), "qkamfogpnljdcshbreti", 0, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string(""), "", 1, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string(""), "bjaht", 1, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string(""), "hjlcmgpket", 1, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string(""), "htaobedqikfplcgjsmrn", 1, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("fodgq"), "", 0, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("qanej"), "dfkap", 0, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("clbao"), "ihqrfebgad", 0, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("mekdn"), "ngtjfcalbseiqrphmkdo", 0, 0);
    _test_string_find_last_of_pointer_size(gl_string("srdfq"), "", 1, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("oemth"), "ikcrq", 1, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("cdaih"), "dmajblfhsg", 1, 1);
    _test_string_find_last_of_pointer_size(gl_string("qohtk"), "oqftjhdmkgsblacenirp", 1, 1);
    _test_string_find_last_of_pointer_size(gl_string("cshmd"), "", 2, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("lhcdo"), "oebqi", 2, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("qnsoh"), "kojhpmbsfe", 2, 2);
    _test_string_find_last_of_pointer_size(gl_string("pkrof"), "acbsjqogpltdkhinfrem", 2, 2);
    _test_string_find_last_of_pointer_size(gl_string("fmtsp"), "", 4, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("khbpm"), "aobjd", 4, 2);
    _test_string_find_last_of_pointer_size(gl_string("pbsji"), "pcbahntsje", 4, 3);
    _test_string_find_last_of_pointer_size(gl_string("mprdj"), "fhepcrntkoagbmldqijs", 4, 4);
    _test_string_find_last_of_pointer_size(gl_string("eqmpa"), "", 5, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("omigs"), "kocgb", 5, 3);
    _test_string_find_last_of_pointer_size(gl_string("onmje"), "fbslrjiqkm", 5, 3);
    _test_string_find_last_of_pointer_size(gl_string("oqmrj"), "jeidpcmalhfnqbgtrsko", 5, 4);
    _test_string_find_last_of_pointer_size(gl_string("schfa"), "", 6, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("igdsc"), "qngpd", 6, 2);
    _test_string_find_last_of_pointer_size(gl_string("brqgo"), "rodhqklgmb", 6, 4);
    _test_string_find_last_of_pointer_size(gl_string("tnrph"), "thdjgafrlbkoiqcspmne", 6, 4);
    _test_string_find_last_of_pointer_size(gl_string("hcjitbfapl"), "", 0, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("daiprenocl"), "ashjd", 0, 0);
    _test_string_find_last_of_pointer_size(gl_string("litpcfdghe"), "mgojkldsqh", 0, 0);
    _test_string_find_last_of_pointer_size(gl_string("aidjksrolc"), "imqnaghkfrdtlopbjesc", 0, 0);
    _test_string_find_last_of_pointer_size(gl_string("qpghtfbaji"), "", 1, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("gfshlcmdjr"), "nadkh", 1, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("nkodajteqp"), "ofdrqmkebl", 1, 1);
    _test_string_find_last_of_pointer_size(gl_string("gbmetiprqd"), "bdfjqgatlksriohemnpc", 1, 1);
    _test_string_find_last_of_pointer_size(gl_string("crnklpmegd"), "", 5, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("jsbtafedoc"), "prqgn", 5, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("qnmodrtkeb"), "pejafmnokr", 5, 5);
    _test_string_find_last_of_pointer_size(gl_string("cpebqsfmnj"), "odnqkgijrhabfmcestlp", 5, 5);
    _test_string_find_last_of_pointer_size(gl_string("lmofqdhpki"), "", 9, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("hnefkqimca"), "rtjpa", 9, 9);
    _test_string_find_last_of_pointer_size(gl_string("drtasbgmfp"), "ktsrmnqagd", 9, 7);
    _test_string_find_last_of_pointer_size(gl_string("lsaijeqhtr"), "rtdhgcisbnmoaqkfpjle", 9, 9);
    _test_string_find_last_of_pointer_size(gl_string("elgofjmbrq"), "", 10, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("mjqdgalkpc"), "dplqa", 10, 8);
    _test_string_find_last_of_pointer_size(gl_string("kthqnfcerm"), "dkacjoptns", 10, 6);
    _test_string_find_last_of_pointer_size(gl_string("dfsjhanorc"), "hqfimtrgnbekpdcsjalo", 10, 9);
    _test_string_find_last_of_pointer_size(gl_string("eqsgalomhb"), "", 11, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("akiteljmoh"), "lofbc", 11, 8);
    _test_string_find_last_of_pointer_size(gl_string("hlbdfreqjo"), "astoegbfpn", 11, 9);
    _test_string_find_last_of_pointer_size(gl_string("taqobhlerg"), "pdgreqomsncafklhtibj", 11, 9);
    _test_string_find_last_of_pointer_size(gl_string("snafbdlghrjkpqtoceim"), "", 0, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("aemtbrgcklhndjisfpoq"), "lbtqd", 0, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("pnracgfkjdiholtbqsem"), "tboimldpjh", 0, 0);
    _test_string_find_last_of_pointer_size(gl_string("dicfltehbsgrmojnpkaq"), "slcerthdaiqjfnobgkpm", 0, 0);
    _test_string_find_last_of_pointer_size(gl_string("jlnkraeodhcspfgbqitm"), "", 1, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("lhosrngtmfjikbqpcade"), "aqibs", 1, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("rbtaqjhgkneisldpmfoc"), "gtfblmqinc", 1, 1);
    _test_string_find_last_of_pointer_size(gl_string("gpifsqlrdkbonjtmheca"), "mkqpbtdalgniorhfescj", 1, 1);
    _test_string_find_last_of_pointer_size(gl_string("hdpkobnsalmcfijregtq"), "", 10, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("jtlshdgqaiprkbcoenfm"), "pblas", 10, 10);
    _test_string_find_last_of_pointer_size(gl_string("fkdrbqltsgmcoiphneaj"), "arosdhcfme", 10, 10);
    _test_string_find_last_of_pointer_size(gl_string("crsplifgtqedjohnabmk"), "blkhjeogicatqfnpdmsr", 10, 10);
    _test_string_find_last_of_pointer_size(gl_string("niptglfbosehkamrdqcj"), "", 19, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("copqdhstbingamjfkler"), "djkqc", 19, 16);
    _test_string_find_last_of_pointer_size(gl_string("mrtaefilpdsgocnhqbjk"), "lgokshjtpb", 19, 19);
    _test_string_find_last_of_pointer_size(gl_string("kojatdhlcmigpbfrqnes"), "bqjhtkfepimcnsgrlado", 19, 19);
    _test_string_find_last_of_pointer_size(gl_string("eaintpchlqsbdgrkjofm"), "", 20, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("gjnhidfsepkrtaqbmclo"), "nocfa", 20, 19);
    _test_string_find_last_of_pointer_size(gl_string("spocfaktqdbiejlhngmr"), "bgtajmiedc", 20, 18);
    _test_string_find_last_of_pointer_size(gl_string("rphmlekgfscndtaobiqj"), "lsckfnqgdahejiopbtmr", 20, 19);
    _test_string_find_last_of_pointer_size(gl_string("liatsqdoegkmfcnbhrpj"), "", 21, gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("binjagtfldkrspcomqeh"), "gfsrt", 21, 12);
    _test_string_find_last_of_pointer_size(gl_string("latkmisecnorjbfhqpdg"), "pfsocbhjtm", 21, 17);
    _test_string_find_last_of_pointer_size(gl_string("lecfratdjkhnsmqpoigb"), "tpflmdnoicjgkberhqsa", 21, 19);
}


void test_string_find_last_of_pointer_size1()
{
    _test_string_find_last_of_pointer_size(gl_string(""), "", gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string(""), "laenf", gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string(""), "pqlnkmbdjo", gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string(""), "qkamfogpnljdcshbreti", gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("nhmko"), "", gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("lahfb"), "irkhs", 2);
    _test_string_find_last_of_pointer_size(gl_string("gmfhd"), "kantesmpgj", 1);
    _test_string_find_last_of_pointer_size(gl_string("odaft"), "oknlrstdpiqmjbaghcfe", 4);
    _test_string_find_last_of_pointer_size(gl_string("eolhfgpjqk"), "", gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("nbatdlmekr"), "bnrpe", 9);
    _test_string_find_last_of_pointer_size(gl_string("jdmciepkaq"), "jtdaefblso", 8);
    _test_string_find_last_of_pointer_size(gl_string("hkbgspoflt"), "oselktgbcapndfjihrmq", 9);
    _test_string_find_last_of_pointer_size(gl_string("gprdcokbnjhlsfmtieqa"), "", gl_string::npos);
    _test_string_find_last_of_pointer_size(gl_string("qjghlnftcaismkropdeb"), "bjaht", 19);
    _test_string_find_last_of_pointer_size(gl_string("pnalfrdtkqcmojiesbhg"), "hjlcmgpket", 19);
    _test_string_find_last_of_pointer_size(gl_string("pniotcfrhqsmgdkjbael"), "htaobedqikfplcgjsmrn", 19);
}


void
_test_string_find_last_of_pointer_size_size(const gl_string& s, const char* str, size_t pos,
     size_t n, size_t x)
{
    TS_ASSERT(s.find_last_of(str, pos, n) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x <= pos && x < s.size());
}


void test_string_find_last_of_pointer_size_size0()
{
    _test_string_find_last_of_pointer_size_size(gl_string(""), "", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "irkhs", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "kante", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "oknlr", 0, 2, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "pcdro", 0, 4, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "bnrpe", 0, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "jtdaefblso", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "oselktgbca", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "eqgaplhckj", 0, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "bjahtcmnlp", 0, 9, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "hjlcmgpket", 0, 10, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "htaobedqikfplcgjsmrn", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "hpqiarojkcdlsgnmfetb", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "dfkaprhjloqetcsimnbg", 0, 10, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "ihqrfebgadntlpmjksoc", 0, 19, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "ngtjfcalbseiqrphmkdo", 0, 20, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "lbtqd", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "tboim", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "slcer", 1, 2, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "cbjfs", 1, 4, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "aqibs", 1, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "gtfblmqinc", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "mkqpbtdalg", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "kphatlimcd", 1, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "pblasqogic", 1, 9, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "arosdhcfme", 1, 10, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "blkhjeogicatqfnpdmsr", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "bmhineprjcoadgstflqk", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "djkqcmetslnghpbarfoi", 1, 10, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "lgokshjtpbemarcdqnfi", 1, 19, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string(""), "bqjhtkfepimcnsgrlado", 1, 20, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("eaint"), "", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("binja"), "gfsrt", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("latkm"), "pfsoc", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("lecfr"), "tpflm", 0, 2, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("eqkst"), "sgkec", 0, 4, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("cdafr"), "romds", 0, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("prbhe"), "qhjistlgmr", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("lbisk"), "pedfirsglo", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("hrlpd"), "aqcoslgrmk", 0, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ehmja"), "dabckmepqj", 0, 9, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("mhqgd"), "pqscrjthli", 0, 10, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("tgklq"), "kfphdcsjqmobliagtren", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("bocjs"), "rokpefncljibsdhqtagm", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("grbsd"), "afionmkphlebtcjqsgrd", 0, 10, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ofjqr"), "aenmqplidhkofrjbctsg", 0, 19, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("btlfi"), "osjmbtcadhiklegrpqnf", 0, 20, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("clrgb"), "", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("tjmek"), "osmia", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("bgstp"), "ckonl", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("hstrk"), "ilcaj", 1, 2, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("kmspj"), "lasiq", 1, 4, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("tjboh"), "kfqmr", 1, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ilbcj"), "klnitfaobg", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("jkngf"), "gjhmdlqikp", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("gfcql"), "skbgtahqej", 1, 5, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("dqtlg"), "bjsdgtlpkf", 1, 9, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("bthpg"), "bjgfmnlkio", 1, 10, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("dgsnq"), "lbhepotfsjdqigcnamkr", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("rmfhp"), "tebangckmpsrqdlfojhi", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("jfdam"), "joflqbdkhtegimscpanr", 1, 10, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("edapb"), "adpmcohetfbsrjinlqkg", 1, 19, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("brfsm"), "iacldqjpfnogbsrhmetk", 1, 20, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("ndrhl"), "", 2, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("mrecp"), "otkgb", 2, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("qlasf"), "cqsjl", 2, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("smaqd"), "dpifl", 2, 2, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("hjeni"), "oapht", 2, 4, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("ocmfj"), "cifts", 2, 5, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("hmftq"), "nmsckbgalo", 2, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("fklad"), "tpksqhamle", 2, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("dirnm"), "tpdrchmkji", 2, 5, 2);
    _test_string_find_last_of_pointer_size_size(gl_string("hrgdc"), "ijagfkblst", 2, 9, 2);
    _test_string_find_last_of_pointer_size_size(gl_string("ifakg"), "kpocsignjb", 2, 10, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("ebrgd"), "pecqtkjsnbdrialgmohf", 2, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("rcjml"), "aiortphfcmkjebgsndql", 2, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("peqmt"), "sdbkeamglhipojqftrcn", 2, 10, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("frehn"), "ljqncehgmfktroapidbs", 2, 19, 2);
    _test_string_find_last_of_pointer_size_size(gl_string("tqolf"), "rtcfodilamkbenjghqps", 2, 20, 2);
    _test_string_find_last_of_pointer_size_size(gl_string("cjgao"), "", 4, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("kjplq"), "mabns", 4, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("herni"), "bdnrp", 4, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("tadrb"), "scidp", 4, 2, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("pkfeo"), "agbjl", 4, 4, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("hoser"), "jfmpr", 4, 5, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("kgrsp"), "rbpefghsmj", 4, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("pgejb"), "apsfntdoqc", 4, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("thlnq"), "ndkjeisgcl", 4, 5, 3);
    _test_string_find_last_of_pointer_size_size(gl_string("nbmit"), "rnfpqatdeo", 4, 9, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("jgmib"), "bntjlqrfik", 4, 10, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("ncrfj"), "kcrtmpolnaqejghsfdbi", 4, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ncsik"), "lobheanpkmqidsrtcfgj", 4, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("sgbfh"), "athdkljcnreqbgpmisof", 4, 10, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("dktbn"), "qkdmjialrscpbhefgont", 4, 19, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("fthqm"), "dmasojntqleribkgfchp", 4, 20, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("klopi"), "", 5, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("dajhn"), "psthd", 5, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("jbgno"), "rpmjd", 5, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("hkjae"), "dfsmk", 5, 2, gl_string::npos);
}


void test_string_find_last_of_pointer_size_size1()
{
    _test_string_find_last_of_pointer_size_size(gl_string("gbhqo"), "skqne", 5, 4, 3);
    _test_string_find_last_of_pointer_size_size(gl_string("ktdor"), "kipnf", 5, 5, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("ldprn"), "hmrnqdgifl", 5, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("egmjk"), "fsmjcdairn", 5, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("armql"), "pcdgltbrfj", 5, 5, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("cdhjo"), "aekfctpirg", 5, 9, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("jcons"), "ledihrsgpf", 5, 10, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("cbrkp"), "mqcklahsbtirgopefndj", 5, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("fhgna"), "kmlthaoqgecrnpdbjfis", 5, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ejfcd"), "sfhbamcdptojlkrenqgi", 5, 10, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("kqjhe"), "pbniofmcedrkhlstgaqj", 5, 19, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("pbdjl"), "mongjratcskbhqiepfdl", 5, 20, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("gajqn"), "", 6, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("stedk"), "hrnat", 6, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("tjkaf"), "gsqdt", 6, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("dthpe"), "bspkd", 6, 2, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("klhde"), "ohcmb", 6, 4, 2);
    _test_string_find_last_of_pointer_size_size(gl_string("bhlki"), "heatr", 6, 5, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("lqmoh"), "pmblckedfn", 6, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("mtqin"), "aceqmsrbik", 6, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("dpqbr"), "lmbtdehjrn", 6, 5, 3);
    _test_string_find_last_of_pointer_size_size(gl_string("kdhmo"), "teqmcrlgib", 6, 9, 3);
    _test_string_find_last_of_pointer_size_size(gl_string("jblqp"), "njolbmspac", 6, 10, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("qmjgl"), "pofnhidklamecrbqjgst", 6, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("rothp"), "jbhckmtgrqnosafedpli", 6, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ghknq"), "dobntpmqklicsahgjerf", 6, 10, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("eopfi"), "tpdshainjkbfoemlrgcq", 6, 19, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("dsnmg"), "oldpfgeakrnitscbjmqh", 6, 20, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("jnkrfhotgl"), "", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("dltjfngbko"), "rqegt", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("bmjlpkiqde"), "dashm", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("skrflobnqm"), "jqirk", 0, 2, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("jkpldtshrm"), "rckeg", 0, 4, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ghasdbnjqo"), "jscie", 0, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("igrkhpbqjt"), "efsphndliq", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ikthdgcamf"), "gdicosleja", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("pcofgeniam"), "qcpjibosfl", 0, 5, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("rlfjgesqhc"), "lrhmefnjcq", 0, 9, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("itphbqsker"), "dtablcrseo", 0, 10, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("skjafcirqm"), "apckjsftedbhgomrnilq", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("tcqomarsfd"), "pcbrgflehjtiadnsokqm", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("rocfeldqpk"), "nsiadegjklhobrmtqcpf", 0, 10, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("cfpegndlkt"), "cpmajdqnolikhgsbretf", 0, 19, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("fqbtnkeasj"), "jcflkntmgiqrphdosaeb", 0, 20, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("shbcqnmoar"), "", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("bdoshlmfin"), "ontrs", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("khfrebnsgq"), "pfkna", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("getcrsaoji"), "ekosa", 1, 2, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("fjiknedcpq"), "anqhk", 1, 4, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("tkejgnafrm"), "jekca", 1, 5, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("jnakolqrde"), "ikemsjgacf", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("lcjptsmgbe"), "arolgsjkhm", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("itfsmcjorl"), "oftkbldhre", 1, 5, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("omchkfrjea"), "gbkqdoeftl", 1, 9, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("cigfqkated"), "sqcflrgtim", 1, 10, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("tscenjikml"), "fmhbkislrjdpanogqcet", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("qcpaemsinf"), "rnioadktqlgpbcjsmhef", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("gltkojeipd"), "oakgtnldpsefihqmjcbr", 1, 10, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("qistfrgnmp"), "gbnaelosidmcjqktfhpr", 1, 19, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("bdnpfcqaem"), "akbripjhlosndcmqgfet", 1, 20, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("ectnhskflp"), "", 5, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("fgtianblpq"), "pijag", 5, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("mfeqklirnh"), "jrckd", 5, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("astedncjhk"), "qcloh", 5, 2, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("fhlqgcajbr"), "thlmp", 5, 4, 2);
    _test_string_find_last_of_pointer_size_size(gl_string("epfhocmdng"), "qidmo", 5, 5, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("apcnsibger"), "lnegpsjqrd", 5, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("aqkocrbign"), "rjqdablmfs", 5, 1, 5);
    _test_string_find_last_of_pointer_size_size(gl_string("ijsmdtqgce"), "enkgpbsjaq", 5, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("clobgsrken"), "kdsgoaijfh", 5, 9, 5);
    _test_string_find_last_of_pointer_size_size(gl_string("jbhcfposld"), "trfqgmckbe", 5, 10, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("oqnpblhide"), "igetsracjfkdnpoblhqm", 5, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("lroeasctif"), "nqctfaogirshlekbdjpm", 5, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("bpjlgmiedh"), "csehfgomljdqinbartkp", 5, 10, 5);
    _test_string_find_last_of_pointer_size_size(gl_string("pamkeoidrj"), "qahoegcmplkfsjbdnitr", 5, 19, 5);
    _test_string_find_last_of_pointer_size_size(gl_string("espogqbthk"), "dpteiajrqmsognhlfbkc", 5, 20, 5);
    _test_string_find_last_of_pointer_size_size(gl_string("shoiedtcjb"), "", 9, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ebcinjgads"), "tqbnh", 9, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("dqmregkcfl"), "akmle", 9, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ngcrieqajf"), "iqfkm", 9, 2, 6);
    _test_string_find_last_of_pointer_size_size(gl_string("qosmilgnjb"), "tqjsr", 9, 4, 8);
    _test_string_find_last_of_pointer_size_size(gl_string("ikabsjtdfl"), "jplqg", 9, 5, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("ersmicafdh"), "oilnrbcgtj", 9, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("fdnplotmgh"), "morkglpesn", 9, 1, 7);
    _test_string_find_last_of_pointer_size_size(gl_string("fdbicojerm"), "dmicerngat", 9, 5, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("mbtafndjcq"), "radgeskbtc", 9, 9, 6);
    _test_string_find_last_of_pointer_size_size(gl_string("mlenkpfdtc"), "ljikprsmqo", 9, 10, 5);
    _test_string_find_last_of_pointer_size_size(gl_string("ahlcifdqgs"), "trqihkcgsjamfdbolnpe", 9, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("bgjemaltks"), "lqmthbsrekajgnofcipd", 9, 1, 6);
    _test_string_find_last_of_pointer_size_size(gl_string("pdhslbqrfc"), "jtalmedribkgqsopcnfh", 9, 10, 7);
    _test_string_find_last_of_pointer_size_size(gl_string("dirhtsnjkc"), "spqfoiclmtagejbndkrh", 9, 19, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("dlroktbcja"), "nmotklspigjrdhcfaebq", 9, 20, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("ncjpmaekbs"), "", 10, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("hlbosgmrak"), "hpmsd", 10, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("pqfhsgilen"), "qnpor", 10, 1, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("gqtjsbdckh"), "otdma", 10, 2, 2);
    _test_string_find_last_of_pointer_size_size(gl_string("cfkqpjlegi"), "efhjg", 10, 4, 7);
    _test_string_find_last_of_pointer_size_size(gl_string("beanrfodgj"), "odpte", 10, 5, 7);
    _test_string_find_last_of_pointer_size_size(gl_string("adtkqpbjfi"), "bctdgfmolr", 10, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("iomkfthagj"), "oaklidrbqg", 10, 1, 1);
}


void test_string_find_last_of_pointer_size_size2()
{
    _test_string_find_last_of_pointer_size_size(gl_string("sdpcilonqj"), "dnjfsagktr", 10, 5, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("gtfbdkqeml"), "nejaktmiqg", 10, 9, 8);
    _test_string_find_last_of_pointer_size_size(gl_string("bmeqgcdorj"), "pjqonlebsf", 10, 10, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("etqlcanmob"), "dshmnbtolcjepgaikfqr", 10, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("roqmkbdtia"), "iogfhpabtjkqlrnemcds", 10, 1, 8);
    _test_string_find_last_of_pointer_size_size(gl_string("kadsithljf"), "ngridfabjsecpqltkmoh", 10, 10, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("sgtkpbfdmh"), "athmknplcgofrqejsdib", 10, 19, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("qgmetnabkl"), "ldobhmqcafnjtkeisgrp", 10, 20, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("cqjohampgd"), "", 11, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("hobitmpsan"), "aocjb", 11, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("tjehkpsalm"), "jbrnk", 11, 1, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("ngfbojitcl"), "tqedg", 11, 2, 7);
    _test_string_find_last_of_pointer_size_size(gl_string("rcfkdbhgjo"), "nqskp", 11, 4, 3);
    _test_string_find_last_of_pointer_size_size(gl_string("qghptonrea"), "eaqkl", 11, 5, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("hnprfgqjdl"), "reaoicljqm", 11, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("hlmgabenti"), "lsftgajqpm", 11, 1, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("ofcjanmrbs"), "rlpfogmits", 11, 5, 7);
    _test_string_find_last_of_pointer_size_size(gl_string("jqedtkornm"), "shkncmiaqj", 11, 9, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("rfedlasjmg"), "fpnatrhqgs", 11, 10, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("talpqjsgkm"), "sjclemqhnpdbgikarfot", 11, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("lrkcbtqpie"), "otcmedjikgsfnqbrhpla", 11, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("cipogdskjf"), "bonsaefdqiprkhlgtjcm", 11, 10, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("nqedcojahi"), "egpscmahijlfnkrodqtb", 11, 19, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("hefnrkmctj"), "kmqbfepjthgilscrndoa", 11, 20, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("atqirnmekfjolhpdsgcb"), "", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("echfkmlpribjnqsaogtd"), "prboq", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("qnhiftdgcleajbpkrosm"), "fjcqh", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("chamfknorbedjitgslpq"), "fmosa", 0, 2, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("njhqpibfmtlkaecdrgso"), "qdbok", 0, 4, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ebnghfsqkprmdcljoiat"), "amslg", 0, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("letjomsgihfrpqbkancd"), "smpltjneqb", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("nblgoipcrqeaktshjdmf"), "flitskrnge", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("cehkbngtjoiflqapsmrd"), "pgqihmlbef", 0, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("mignapfoklbhcqjetdrs"), "cfpdqjtgsb", 0, 9, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ceatbhlsqjgpnokfrmdi"), "htpsiaflom", 0, 10, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ocihkjgrdelpfnmastqb"), "kpjfiaceghsrdtlbnomq", 0, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("noelgschdtbrjfmiqkap"), "qhtbomidljgafneksprc", 0, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("dkclqfombepritjnghas"), "nhtjobkcefldimpsaqgr", 0, 10, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("miklnresdgbhqcojftap"), "prabcjfqnoeskilmtgdh", 0, 19, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("htbcigojaqmdkfrnlsep"), "dtrgmchilkasqoebfpjn", 0, 20, 0);
    _test_string_find_last_of_pointer_size_size(gl_string("febhmqtjanokscdirpgl"), "", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("loakbsqjpcrdhftniegm"), "sqome", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("reagphsqflbitdcjmkno"), "smfte", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("jitlfrqemsdhkopncabg"), "ciboh", 1, 2, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("mhtaepscdnrjqgbkifol"), "haois", 1, 4, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("tocesrfmnglpbjihqadk"), "abfki", 1, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("lpfmctjrhdagneskbqoi"), "frdkocntmq", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("lsmqaepkdhncirbtjfgo"), "oasbpedlnr", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("epoiqmtldrabnkjhcfsg"), "kltqmhgand", 1, 5, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("emgasrilpknqojhtbdcf"), "gdtfjchpmr", 1, 9, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("hnfiagdpcklrjetqbsom"), "ponmcqblet", 1, 10, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("nsdfebgajhmtricpoklq"), "sgphqdnofeiklatbcmjr", 1, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("atjgfsdlpobmeiqhncrk"), "ljqprsmigtfoneadckbh", 1, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("sitodfgnrejlahcbmqkp"), "ligeojhafnkmrcsqtbdp", 1, 10, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("fraghmbiceknltjpqosd"), "lsimqfnjarbopedkhcgt", 1, 19, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("pmafenlhqtdbkirjsogc"), "abedmfjlghniorcqptks", 1, 20, 1);
    _test_string_find_last_of_pointer_size_size(gl_string("pihgmoeqtnakrjslcbfd"), "", 10, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("gjdkeprctqblnhiafsom"), "hqtoa", 10, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("mkpnblfdsahrcqijteog"), "cahif", 10, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("gckarqnelodfjhmbptis"), "kehis", 10, 2, 7);
    _test_string_find_last_of_pointer_size_size(gl_string("gqpskidtbclomahnrjfe"), "kdlmh", 10, 4, 10);
    _test_string_find_last_of_pointer_size_size(gl_string("pkldjsqrfgitbhmaecno"), "paeql", 10, 5, 6);
    _test_string_find_last_of_pointer_size_size(gl_string("aftsijrbeklnmcdqhgop"), "aghoqiefnb", 10, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("mtlgdrhafjkbiepqnsoc"), "jrbqaikpdo", 10, 1, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("pqgirnaefthokdmbsclj"), "smjonaeqcl", 10, 5, 5);
    _test_string_find_last_of_pointer_size_size(gl_string("kpdbgjmtherlsfcqoina"), "eqbdrkcfah", 10, 9, 10);
    _test_string_find_last_of_pointer_size_size(gl_string("jrlbothiknqmdgcfasep"), "kapmsienhf", 10, 10, 9);
    _test_string_find_last_of_pointer_size_size(gl_string("mjogldqferckabinptsh"), "jpqotrlenfcsbhkaimdg", 10, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("apoklnefbhmgqcdrisjt"), "jlbmhnfgtcqprikeados", 10, 1, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ifeopcnrjbhkdgatmqls"), "stgbhfmdaljnpqoicker", 10, 10, 10);
    _test_string_find_last_of_pointer_size_size(gl_string("ckqhaiesmjdnrgolbtpf"), "oihcetflbjagdsrkmqpn", 10, 19, 10);
    _test_string_find_last_of_pointer_size_size(gl_string("bnlgapfimcoterskqdjh"), "adtclebmnpjsrqfkigoh", 10, 20, 10);
    _test_string_find_last_of_pointer_size_size(gl_string("kgdlrobpmjcthqsafeni"), "", 19, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("dfkechomjapgnslbtqir"), "beafg", 19, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("rloadknfbqtgmhcsipje"), "iclat", 19, 1, 16);
    _test_string_find_last_of_pointer_size_size(gl_string("mgjhkolrnadqbpetcifs"), "rkhnf", 19, 2, 7);
    _test_string_find_last_of_pointer_size_size(gl_string("cmlfakiojdrgtbsphqen"), "clshq", 19, 4, 16);
    _test_string_find_last_of_pointer_size_size(gl_string("kghbfipeomsntdalrqjc"), "dtcoj", 19, 5, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("eldiqckrnmtasbghjfpo"), "rqosnjmfth", 19, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("abqjcfedgotihlnspkrm"), "siatdfqglh", 19, 1, 15);
    _test_string_find_last_of_pointer_size_size(gl_string("qfbadrtjsimkolcenhpg"), "mrlshtpgjq", 19, 5, 17);
    _test_string_find_last_of_pointer_size_size(gl_string("abseghclkjqifmtodrnp"), "adlcskgqjt", 19, 9, 16);
    _test_string_find_last_of_pointer_size_size(gl_string("ibmsnlrjefhtdokacqpg"), "drshcjknaf", 19, 10, 16);
    _test_string_find_last_of_pointer_size_size(gl_string("mrkfciqjebaponsthldg"), "etsaqroinghpkjdlfcbm", 19, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("mjkticdeoqshpalrfbgn"), "sgepdnkqliambtrocfhj", 19, 1, 10);
    _test_string_find_last_of_pointer_size_size(gl_string("rqnoclbdejgiphtfsakm"), "nlmcjaqgbsortfdihkpe", 19, 10, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("plkqbhmtfaeodjcrsing"), "racfnpmosldibqkghjet", 19, 19, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("oegalhmstjrfickpbndq"), "fjhdsctkqeiolagrnmbp", 19, 20, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("rdtgjcaohpblniekmsfq"), "", 20, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("ofkqbnjetrmsaidphglc"), "ejanp", 20, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("grkpahljcftesdmonqib"), "odife", 20, 1, 15);
    _test_string_find_last_of_pointer_size_size(gl_string("jimlgbhfqkteospardcn"), "okaqd", 20, 2, 12);
    _test_string_find_last_of_pointer_size_size(gl_string("gftenihpmslrjkqadcob"), "lcdbi", 20, 4, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("bmhldogtckrfsanijepq"), "fsqbj", 20, 5, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("nfqkrpjdesabgtlcmoih"), "bigdomnplq", 20, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("focalnrpiqmdkstehbjg"), "apiblotgcd", 20, 1, 3);
    _test_string_find_last_of_pointer_size_size(gl_string("rhqdspkmebiflcotnjga"), "acfhdenops", 20, 5, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("rahdtmsckfboqlpniegj"), "jopdeamcrk", 20, 9, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("fbkeiopclstmdqranjhg"), "trqncbkgmh", 20, 10, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("lifhpdgmbconstjeqark"), "tomglrkencbsfjqpihda", 20, 0, gl_string::npos);
}


void test_string_find_last_of_pointer_size_size3()
{
    _test_string_find_last_of_pointer_size_size(gl_string("pboqganrhedjmltsicfk"), "gbkhdnpoietfcmrslajq", 20, 1, 4);
    _test_string_find_last_of_pointer_size_size(gl_string("klchabsimetjnqgorfpd"), "rtfnmbsglkjaichoqedp", 20, 10, 17);
    _test_string_find_last_of_pointer_size_size(gl_string("sirfgmjqhctndbklaepo"), "ohkmdpfqbsacrtjnlgei", 20, 19, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("rlbdsiceaonqjtfpghkm"), "dlbrteoisgphmkncajfq", 20, 20, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("ecgdanriptblhjfqskom"), "", 21, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("fdmiarlpgcskbhoteqjn"), "sjrlo", 21, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("rlbstjqopignecmfadkh"), "qjpor", 21, 1, 6);
    _test_string_find_last_of_pointer_size_size(gl_string("grjpqmbshektdolcafni"), "odhfn", 21, 2, 13);
    _test_string_find_last_of_pointer_size_size(gl_string("sakfcohtqnibprjmlged"), "qtfin", 21, 4, 10);
    _test_string_find_last_of_pointer_size_size(gl_string("mjtdglasihqpocebrfkn"), "hpqfo", 21, 5, 17);
    _test_string_find_last_of_pointer_size_size(gl_string("okaplfrntghqbmeicsdj"), "fabmertkos", 21, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("sahngemrtcjidqbklfpo"), "brqtgkmaej", 21, 1, 14);
    _test_string_find_last_of_pointer_size_size(gl_string("dlmsipcnekhbgoaftqjr"), "nfrdeihsgl", 21, 5, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("ahegrmqnoiklpfsdbcjt"), "hlfrosekpi", 21, 9, 14);
    _test_string_find_last_of_pointer_size_size(gl_string("hdsjbnmlegtkqripacof"), "atgbkrjdsm", 21, 10, 16);
    _test_string_find_last_of_pointer_size_size(gl_string("pcnedrfjihqbalkgtoms"), "blnrptjgqmaifsdkhoec", 21, 0, gl_string::npos);
    _test_string_find_last_of_pointer_size_size(gl_string("qjidealmtpskrbfhocng"), "ctpmdahebfqjgknloris", 21, 1, 17);
    _test_string_find_last_of_pointer_size_size(gl_string("qeindtagmokpfhsclrbj"), "apnkeqthrmlbfodiscgj", 21, 10, 17);
    _test_string_find_last_of_pointer_size_size(gl_string("kpfegbjhsrnodltqciam"), "jdgictpframeoqlsbknh", 21, 19, 19);
    _test_string_find_last_of_pointer_size_size(gl_string("hnbrcplsjfgiktoedmaq"), "qprlsfojamgndekthibc", 21, 20, 19);
}

void
_test_string_find_last_of_string_size(const gl_string& s, const gl_string& str, size_t pos, size_t x)
{
    TS_ASSERT(s.find_last_of(str, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x <= pos && x < s.size());
}


void
_test_string_find_last_of_string_size(const gl_string& s, const gl_string& str, size_t x)
{
    TS_ASSERT(s.find_last_of(str) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x < s.size());
}


void test_string_find_last_of_string_size0()
{
    _test_string_find_last_of_string_size(gl_string(""), gl_string(""), 0, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string(""), gl_string("laenf"), 0, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string(""), gl_string("pqlnkmbdjo"), 0, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string(""), gl_string("qkamfogpnljdcshbreti"), 0, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string(""), gl_string(""), 1, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string(""), gl_string("bjaht"), 1, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string(""), gl_string("hjlcmgpket"), 1, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string(""), gl_string("htaobedqikfplcgjsmrn"), 1, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("fodgq"), gl_string(""), 0, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("qanej"), gl_string("dfkap"), 0, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("clbao"), gl_string("ihqrfebgad"), 0, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("mekdn"), gl_string("ngtjfcalbseiqrphmkdo"), 0, 0);
    _test_string_find_last_of_string_size(gl_string("srdfq"), gl_string(""), 1, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("oemth"), gl_string("ikcrq"), 1, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("cdaih"), gl_string("dmajblfhsg"), 1, 1);
    _test_string_find_last_of_string_size(gl_string("qohtk"), gl_string("oqftjhdmkgsblacenirp"), 1, 1);
    _test_string_find_last_of_string_size(gl_string("cshmd"), gl_string(""), 2, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("lhcdo"), gl_string("oebqi"), 2, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("qnsoh"), gl_string("kojhpmbsfe"), 2, 2);
    _test_string_find_last_of_string_size(gl_string("pkrof"), gl_string("acbsjqogpltdkhinfrem"), 2, 2);
    _test_string_find_last_of_string_size(gl_string("fmtsp"), gl_string(""), 4, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("khbpm"), gl_string("aobjd"), 4, 2);
    _test_string_find_last_of_string_size(gl_string("pbsji"), gl_string("pcbahntsje"), 4, 3);
    _test_string_find_last_of_string_size(gl_string("mprdj"), gl_string("fhepcrntkoagbmldqijs"), 4, 4);
    _test_string_find_last_of_string_size(gl_string("eqmpa"), gl_string(""), 5, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("omigs"), gl_string("kocgb"), 5, 3);
    _test_string_find_last_of_string_size(gl_string("onmje"), gl_string("fbslrjiqkm"), 5, 3);
    _test_string_find_last_of_string_size(gl_string("oqmrj"), gl_string("jeidpcmalhfnqbgtrsko"), 5, 4);
    _test_string_find_last_of_string_size(gl_string("schfa"), gl_string(""), 6, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("igdsc"), gl_string("qngpd"), 6, 2);
    _test_string_find_last_of_string_size(gl_string("brqgo"), gl_string("rodhqklgmb"), 6, 4);
    _test_string_find_last_of_string_size(gl_string("tnrph"), gl_string("thdjgafrlbkoiqcspmne"), 6, 4);
    _test_string_find_last_of_string_size(gl_string("hcjitbfapl"), gl_string(""), 0, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("daiprenocl"), gl_string("ashjd"), 0, 0);
    _test_string_find_last_of_string_size(gl_string("litpcfdghe"), gl_string("mgojkldsqh"), 0, 0);
    _test_string_find_last_of_string_size(gl_string("aidjksrolc"), gl_string("imqnaghkfrdtlopbjesc"), 0, 0);
    _test_string_find_last_of_string_size(gl_string("qpghtfbaji"), gl_string(""), 1, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("gfshlcmdjr"), gl_string("nadkh"), 1, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("nkodajteqp"), gl_string("ofdrqmkebl"), 1, 1);
    _test_string_find_last_of_string_size(gl_string("gbmetiprqd"), gl_string("bdfjqgatlksriohemnpc"), 1, 1);
    _test_string_find_last_of_string_size(gl_string("crnklpmegd"), gl_string(""), 5, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("jsbtafedoc"), gl_string("prqgn"), 5, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("qnmodrtkeb"), gl_string("pejafmnokr"), 5, 5);
    _test_string_find_last_of_string_size(gl_string("cpebqsfmnj"), gl_string("odnqkgijrhabfmcestlp"), 5, 5);
    _test_string_find_last_of_string_size(gl_string("lmofqdhpki"), gl_string(""), 9, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("hnefkqimca"), gl_string("rtjpa"), 9, 9);
    _test_string_find_last_of_string_size(gl_string("drtasbgmfp"), gl_string("ktsrmnqagd"), 9, 7);
    _test_string_find_last_of_string_size(gl_string("lsaijeqhtr"), gl_string("rtdhgcisbnmoaqkfpjle"), 9, 9);
    _test_string_find_last_of_string_size(gl_string("elgofjmbrq"), gl_string(""), 10, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("mjqdgalkpc"), gl_string("dplqa"), 10, 8);
    _test_string_find_last_of_string_size(gl_string("kthqnfcerm"), gl_string("dkacjoptns"), 10, 6);
    _test_string_find_last_of_string_size(gl_string("dfsjhanorc"), gl_string("hqfimtrgnbekpdcsjalo"), 10, 9);
    _test_string_find_last_of_string_size(gl_string("eqsgalomhb"), gl_string(""), 11, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("akiteljmoh"), gl_string("lofbc"), 11, 8);
    _test_string_find_last_of_string_size(gl_string("hlbdfreqjo"), gl_string("astoegbfpn"), 11, 9);
    _test_string_find_last_of_string_size(gl_string("taqobhlerg"), gl_string("pdgreqomsncafklhtibj"), 11, 9);
    _test_string_find_last_of_string_size(gl_string("snafbdlghrjkpqtoceim"), gl_string(""), 0, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("aemtbrgcklhndjisfpoq"), gl_string("lbtqd"), 0, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("pnracgfkjdiholtbqsem"), gl_string("tboimldpjh"), 0, 0);
    _test_string_find_last_of_string_size(gl_string("dicfltehbsgrmojnpkaq"), gl_string("slcerthdaiqjfnobgkpm"), 0, 0);
    _test_string_find_last_of_string_size(gl_string("jlnkraeodhcspfgbqitm"), gl_string(""), 1, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("lhosrngtmfjikbqpcade"), gl_string("aqibs"), 1, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("rbtaqjhgkneisldpmfoc"), gl_string("gtfblmqinc"), 1, 1);
    _test_string_find_last_of_string_size(gl_string("gpifsqlrdkbonjtmheca"), gl_string("mkqpbtdalgniorhfescj"), 1, 1);
    _test_string_find_last_of_string_size(gl_string("hdpkobnsalmcfijregtq"), gl_string(""), 10, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("jtlshdgqaiprkbcoenfm"), gl_string("pblas"), 10, 10);
    _test_string_find_last_of_string_size(gl_string("fkdrbqltsgmcoiphneaj"), gl_string("arosdhcfme"), 10, 10);
    _test_string_find_last_of_string_size(gl_string("crsplifgtqedjohnabmk"), gl_string("blkhjeogicatqfnpdmsr"), 10, 10);
    _test_string_find_last_of_string_size(gl_string("niptglfbosehkamrdqcj"), gl_string(""), 19, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("copqdhstbingamjfkler"), gl_string("djkqc"), 19, 16);
    _test_string_find_last_of_string_size(gl_string("mrtaefilpdsgocnhqbjk"), gl_string("lgokshjtpb"), 19, 19);
    _test_string_find_last_of_string_size(gl_string("kojatdhlcmigpbfrqnes"), gl_string("bqjhtkfepimcnsgrlado"), 19, 19);
    _test_string_find_last_of_string_size(gl_string("eaintpchlqsbdgrkjofm"), gl_string(""), 20, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("gjnhidfsepkrtaqbmclo"), gl_string("nocfa"), 20, 19);
    _test_string_find_last_of_string_size(gl_string("spocfaktqdbiejlhngmr"), gl_string("bgtajmiedc"), 20, 18);
    _test_string_find_last_of_string_size(gl_string("rphmlekgfscndtaobiqj"), gl_string("lsckfnqgdahejiopbtmr"), 20, 19);
    _test_string_find_last_of_string_size(gl_string("liatsqdoegkmfcnbhrpj"), gl_string(""), 21, gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("binjagtfldkrspcomqeh"), gl_string("gfsrt"), 21, 12);
    _test_string_find_last_of_string_size(gl_string("latkmisecnorjbfhqpdg"), gl_string("pfsocbhjtm"), 21, 17);
    _test_string_find_last_of_string_size(gl_string("lecfratdjkhnsmqpoigb"), gl_string("tpflmdnoicjgkberhqsa"), 21, 19);
}


void test_string_find_last_of_string_size1()
{
    _test_string_find_last_of_string_size(gl_string(""), gl_string(""), gl_string::npos);
    _test_string_find_last_of_string_size(gl_string(""), gl_string("laenf"), gl_string::npos);
    _test_string_find_last_of_string_size(gl_string(""), gl_string("pqlnkmbdjo"), gl_string::npos);
    _test_string_find_last_of_string_size(gl_string(""), gl_string("qkamfogpnljdcshbreti"), gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("nhmko"), gl_string(""), gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("lahfb"), gl_string("irkhs"), 2);
    _test_string_find_last_of_string_size(gl_string("gmfhd"), gl_string("kantesmpgj"), 1);
    _test_string_find_last_of_string_size(gl_string("odaft"), gl_string("oknlrstdpiqmjbaghcfe"), 4);
    _test_string_find_last_of_string_size(gl_string("eolhfgpjqk"), gl_string(""), gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("nbatdlmekr"), gl_string("bnrpe"), 9);
    _test_string_find_last_of_string_size(gl_string("jdmciepkaq"), gl_string("jtdaefblso"), 8);
    _test_string_find_last_of_string_size(gl_string("hkbgspoflt"), gl_string("oselktgbcapndfjihrmq"), 9);
    _test_string_find_last_of_string_size(gl_string("gprdcokbnjhlsfmtieqa"), gl_string(""), gl_string::npos);
    _test_string_find_last_of_string_size(gl_string("qjghlnftcaismkropdeb"), gl_string("bjaht"), 19);
    _test_string_find_last_of_string_size(gl_string("pnalfrdtkqcmojiesbhg"), gl_string("hjlcmgpket"), 19);
    _test_string_find_last_of_string_size(gl_string("pniotcfrhqsmgdkjbael"), gl_string("htaobedqikfplcgjsmrn"), 19);
}


void
_test_string_rfind_char_size(const gl_string& s, char c, size_t pos,
     size_t x)
{
    TS_ASSERT(s.rfind(c, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x <= pos && x + 1 <= s.size());
}


void
_test_string_rfind_char_size(const gl_string& s, char c, size_t x)
{
    TS_ASSERT(s.rfind(c) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x + 1 <= s.size());
}

void test_string_rfind_char_size() {
    _test_string_rfind_char_size(gl_string(""), 'b', 0, gl_string::npos);
    _test_string_rfind_char_size(gl_string(""), 'b', 1, gl_string::npos);
    _test_string_rfind_char_size(gl_string("abcde"), 'b', 0, gl_string::npos);
    _test_string_rfind_char_size(gl_string("abcde"), 'b', 1, 1);
    _test_string_rfind_char_size(gl_string("abcde"), 'b', 2, 1);
    _test_string_rfind_char_size(gl_string("abcde"), 'b', 4, 1);
    _test_string_rfind_char_size(gl_string("abcde"), 'b', 5, 1);
    _test_string_rfind_char_size(gl_string("abcde"), 'b', 6, 1);
    _test_string_rfind_char_size(gl_string("abcdeabcde"), 'b', 0, gl_string::npos);
    _test_string_rfind_char_size(gl_string("abcdeabcde"), 'b', 1, 1);
    _test_string_rfind_char_size(gl_string("abcdeabcde"), 'b', 5, 1);
    _test_string_rfind_char_size(gl_string("abcdeabcde"), 'b', 9, 6);
    _test_string_rfind_char_size(gl_string("abcdeabcde"), 'b', 10, 6);
    _test_string_rfind_char_size(gl_string("abcdeabcde"), 'b', 11, 6);
    _test_string_rfind_char_size(gl_string("abcdeabcdeabcdeabcde"), 'b', 0, gl_string::npos);
    _test_string_rfind_char_size(gl_string("abcdeabcdeabcdeabcde"), 'b', 1, 1);
    _test_string_rfind_char_size(gl_string("abcdeabcdeabcdeabcde"), 'b', 10, 6);
    _test_string_rfind_char_size(gl_string("abcdeabcdeabcdeabcde"), 'b', 19, 16);
    _test_string_rfind_char_size(gl_string("abcdeabcdeabcdeabcde"), 'b', 20, 16);
    _test_string_rfind_char_size(gl_string("abcdeabcdeabcdeabcde"), 'b', 21, 16);

    _test_string_rfind_char_size(gl_string(""), 'b', gl_string::npos);
    _test_string_rfind_char_size(gl_string("abcde"), 'b', 1);
    _test_string_rfind_char_size(gl_string("abcdeabcde"), 'b', 6);
    _test_string_rfind_char_size(gl_string("abcdeabcdeabcdeabcde"), 'b', 16);
    }


void
_test_string_rfind_pointer_size(const gl_string& s, const char* str, size_t pos,
     size_t x)
{
    TS_ASSERT(s.rfind(str, pos) == x);
    if (x != gl_string::npos)
    {
        size_t n = std::strlen(str);
        TS_ASSERT(x <= pos && x + n <= s.size());
    }
}


void
_test_string_rfind_pointer_size(const gl_string& s, const char* str, size_t x)
{
    TS_ASSERT(s.rfind(str) == x);
    if (x != gl_string::npos)
    {
        size_t pos = s.size();
        size_t n = std::strlen(str);
        TS_ASSERT(x <= pos && x + n <= s.size());
    }
}


void test_string_rfind_pointer_size0()
{
    _test_string_rfind_pointer_size(gl_string(""), "", 0, 0);
    _test_string_rfind_pointer_size(gl_string(""), "abcde", 0, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string(""), "abcdeabcde", 0, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string(""), "", 1, 0);
    _test_string_rfind_pointer_size(gl_string(""), "abcde", 1, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string(""), "abcdeabcde", 1, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "", 0, 0);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcde", 0, 0);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcde", 0, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "", 1, 1);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcde", 1, 0);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcde", 1, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "", 2, 2);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcde", 2, 0);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcde", 2, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "", 4, 4);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcde", 4, 0);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcde", 4, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "", 5, 5);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcde", 5, 0);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcde", 5, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "", 6, 5);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcde", 6, 0);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcde", 6, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "", 0, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcde", 0, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "", 1, 1);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcde", 1, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 1, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "", 5, 5);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcde", 5, 5);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 5, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "", 9, 9);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcde", 9, 5);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 9, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "", 10, 10);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcde", 10, 5);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 10, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "", 11, 10);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcde", 11, 5);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 11, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 0, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 1, 1);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 10, 10);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 10);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 10);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 19, 19);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, 15);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, 10);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 20, 20);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, 15);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, 10);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 21, 20);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, 15);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, 10);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, 0);
}


void test_string_rfind_pointer_size1()
{
    _test_string_rfind_pointer_size(gl_string(""), "", 0);
    _test_string_rfind_pointer_size(gl_string(""), "abcde", gl_string::npos);
    _test_string_rfind_pointer_size(gl_string(""), "abcdeabcde", gl_string::npos);
    _test_string_rfind_pointer_size(gl_string(""), "abcdeabcdeabcdeabcde", gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "", 5);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcde", 0);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcde", gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "", 10);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcde", 5);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcde", 0);
    _test_string_rfind_pointer_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", gl_string::npos);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "", 20);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 15);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10);
    _test_string_rfind_pointer_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0);
}

void
_test_string_rfind_pointer_size_size(const gl_string& s, const char* str, size_t pos,
      size_t n, size_t x)
{
    TS_ASSERT(s.rfind(str, pos, n) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x <= pos && x + n <= s.size());
}


void test_string_rfind_pointer_size_size0()
{
    _test_string_rfind_pointer_size_size(gl_string(""), "", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcde", 0, 1, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcde", 0, 2, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcde", 0, 4, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcde", 0, 5, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcde", 0, 1, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcde", 0, 5, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcde", 0, 9, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcde", 0, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, 1, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 0, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "", 1, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcde", 1, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcde", 1, 1, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcde", 1, 2, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcde", 1, 4, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcde", 1, 5, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcde", 1, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcde", 1, 1, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcde", 1, 5, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcde", 1, 9, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcde", 1, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, 1, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string(""), "abcdeabcdeabcdeabcde", 1, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 0, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 0, 2, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 0, 4, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 0, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 0, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 0, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 0, 9, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 0, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 0, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 1, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 1, 2, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 1, 4, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 1, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 1, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 1, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 1, 9, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 1, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 1, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "", 2, 0, 2);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 2, 0, 2);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 2, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 2, 2, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 2, 4, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 2, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 2, 0, 2);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 2, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 2, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 2, 9, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 2, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, 0, 2);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 2, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "", 4, 0, 4);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 4, 0, 4);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 4, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 4, 2, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 4, 4, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 4, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 4, 0, 4);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 4, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 4, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 4, 9, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 4, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, 0, 4);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 4, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "", 5, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 5, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 5, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 5, 2, 0);
}


void test_string_rfind_pointer_size_size1()
{
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 5, 4, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 5, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 5, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 5, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 5, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 5, 9, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 5, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 5, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "", 6, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 6, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 6, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 6, 2, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 6, 4, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcde", 6, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 6, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 6, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 6, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 6, 9, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcde", 6, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, 10, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcde"), "abcdeabcdeabcdeabcde", 6, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 0, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 0, 2, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 0, 4, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 0, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 9, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 0, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 0, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 1, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 1, 2, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 1, 4, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 1, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 1, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 1, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 1, 9, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 1, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 1, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "", 5, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 5, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 5, 1, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 5, 2, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 5, 4, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 5, 5, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 5, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 5, 1, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 5, 5, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 5, 9, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 5, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, 0, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, 1, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 5, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "", 9, 0, 9);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 9, 0, 9);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 9, 1, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 9, 2, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 9, 4, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 9, 5, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 9, 0, 9);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 9, 1, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 9, 5, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 9, 9, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 9, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, 0, 9);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, 1, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 9, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "", 10, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 10, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 10, 1, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 10, 2, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 10, 4, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 10, 5, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 10, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 10, 1, 5);
}


void test_string_rfind_pointer_size_size2()
{
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 10, 5, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 10, 9, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 10, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, 1, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 10, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "", 11, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 11, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 11, 1, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 11, 2, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 11, 4, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcde", 11, 5, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 11, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 11, 1, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 11, 5, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 11, 9, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcde", 11, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, 1, 5);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, 19, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcde"), "abcdeabcdeabcdeabcde", 11, 20, gl_string::npos);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 2, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 4, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 0, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 9, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 0, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 0, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 19, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 0, 20, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 2, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 4, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 1, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 5, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 9, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 1, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, 0, 1);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, 1, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, 10, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, 19, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 1, 20, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 10, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 1, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 2, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 4, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 10, 5, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 1, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 5, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 9, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 10, 10, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, 0, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, 1, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, 10, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, 19, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 10, 20, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 19, 0, 19);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, 0, 19);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, 1, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, 2, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, 4, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 19, 5, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, 0, 19);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, 1, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, 5, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, 9, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 19, 10, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, 0, 19);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, 1, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, 10, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, 19, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 19, 20, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 20, 0, 20);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, 0, 20);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, 1, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, 2, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, 4, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 20, 5, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, 0, 20);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, 1, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, 5, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, 9, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 20, 10, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, 0, 20);
}


void test_string_rfind_pointer_size_size3()
{
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, 1, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, 10, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, 19, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 20, 20, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "", 21, 0, 20);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, 0, 20);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, 1, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, 2, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, 4, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcde", 21, 5, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, 0, 20);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, 1, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, 5, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, 9, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcde", 21, 10, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, 0, 20);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, 1, 15);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, 10, 10);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, 19, 0);
    _test_string_rfind_pointer_size_size(gl_string("abcdeabcdeabcdeabcde"), "abcdeabcdeabcdeabcde", 21, 20, 0);
}


void
_test_string_rfind_string_size(const gl_string& s, const gl_string& str, size_t pos, size_t x)
{
    TS_ASSERT(s.rfind(str, pos) == x);
    if (x != gl_string::npos)
        TS_ASSERT(x <= pos && x + str.size() <= s.size());
}


void
_test_string_rfind_string_size(const gl_string& s, const gl_string& str, size_t x)
{
    TS_ASSERT(s.rfind(str) == x);
    if (x != gl_string::npos)
        TS_ASSERT(0 <= x && x + str.size() <= s.size());
}


void test_string_rfind_string_size0()
{
    _test_string_rfind_string_size(gl_string(""), gl_string(""), 0, 0);
    _test_string_rfind_string_size(gl_string(""), gl_string("abcde"), 0, gl_string::npos);
    _test_string_rfind_string_size(gl_string(""), gl_string("abcdeabcde"), 0, gl_string::npos);
    _test_string_rfind_string_size(gl_string(""), gl_string("abcdeabcdeabcdeabcde"), 0, gl_string::npos);
    _test_string_rfind_string_size(gl_string(""), gl_string(""), 1, 0);
    _test_string_rfind_string_size(gl_string(""), gl_string("abcde"), 1, gl_string::npos);
    _test_string_rfind_string_size(gl_string(""), gl_string("abcdeabcde"), 1, gl_string::npos);
    _test_string_rfind_string_size(gl_string(""), gl_string("abcdeabcdeabcdeabcde"), 1, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string(""), 0, 0);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcde"), 0, 0);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 0, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 0, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string(""), 1, 1);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcde"), 1, 0);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 1, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 1, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string(""), 2, 2);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcde"), 2, 0);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 2, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 2, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string(""), 4, 4);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcde"), 4, 0);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 4, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 4, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string(""), 5, 5);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcde"), 5, 0);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 5, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 5, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string(""), 6, 5);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcde"), 6, 0);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcde"), 6, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), 6, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string(""), 0, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 0, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 0, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 0, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string(""), 1, 1);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 1, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 1, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 1, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string(""), 5, 5);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 5, 5);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 5, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 5, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string(""), 9, 9);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 9, 5);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 9, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 9, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string(""), 10, 10);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 10, 5);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 10, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 10, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string(""), 11, 10);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 11, 5);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 11, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 11, gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 0, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 0, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 0, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 0, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 1, 1);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 1, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 1, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 1, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 10, 10);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 10, 10);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 10, 10);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 10, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 19, 19);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 19, 15);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 19, 10);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 19, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 20, 20);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 20, 15);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 20, 10);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 20, 0);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 21, 20);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 21, 15);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 21, 10);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 21, 0);
}


void test_string_rfind_string_size1()
{
    _test_string_rfind_string_size(gl_string(""), gl_string(""), 0);
    _test_string_rfind_string_size(gl_string(""), gl_string("abcde"), gl_string::npos);
    _test_string_rfind_string_size(gl_string(""), gl_string("abcdeabcde"), gl_string::npos);
    _test_string_rfind_string_size(gl_string(""), gl_string("abcdeabcdeabcdeabcde"), gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string(""), 5);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcde"), 0);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcde"), gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcde"), gl_string("abcdeabcdeabcdeabcde"), gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string(""), 10);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcde"), 5);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcde"), 0);
    _test_string_rfind_string_size(gl_string("abcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), gl_string::npos);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string(""), 20);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcde"), 15);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcde"), 10);
    _test_string_rfind_string_size(gl_string("abcdeabcdeabcdeabcde"), gl_string("abcdeabcdeabcdeabcde"), 0);
}

};

BOOST_FIXTURE_TEST_SUITE(_test_string_find, test_string_find)
BOOST_AUTO_TEST_CASE(test_char) {
  test_string_find::test_char();
}
BOOST_AUTO_TEST_CASE(test_string_size0) {
  test_string_find::test_string_size0();
}
BOOST_AUTO_TEST_CASE(test_string_size1) {
  test_string_find::test_string_size1();
}
BOOST_AUTO_TEST_CASE(test_pointer_size0) {
  test_string_find::test_pointer_size0();
}
BOOST_AUTO_TEST_CASE(test_pointer_size1) {
  test_string_find::test_pointer_size1();
}
BOOST_AUTO_TEST_CASE(test_pointer_size_size0) {
  test_string_find::test_pointer_size_size0();
}
BOOST_AUTO_TEST_CASE(test_pointer_size_size1) {
  test_string_find::test_pointer_size_size1();
}
BOOST_AUTO_TEST_CASE(test_pointer_size_size2) {
  test_string_find::test_pointer_size_size2();
}
BOOST_AUTO_TEST_CASE(test_pointer_size_size3) {
  test_string_find::test_pointer_size_size3();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_not_of_char_size) {
  test_string_find::test_string_find_first_not_of_char_size();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_not_of_pointer_size0) {
  test_string_find::test_string_find_first_not_of_pointer_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_not_of_pointer_size1) {
  test_string_find::test_string_find_first_not_of_pointer_size1();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_not_of_pointer_size_size0) {
  test_string_find::test_string_find_first_not_of_pointer_size_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_not_of_pointer_size_size1) {
  test_string_find::test_string_find_first_not_of_pointer_size_size1();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_not_of_pointer_size_size2) {
  test_string_find::test_string_find_first_not_of_pointer_size_size2();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_not_of_pointer_size_size3) {
  test_string_find::test_string_find_first_not_of_pointer_size_size3();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_not_of_string_size0) {
  test_string_find::test_string_find_first_not_of_string_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_not_of_string_size1) {
  test_string_find::test_string_find_first_not_of_string_size1();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_of_char_size) {
  test_string_find::test_string_find_first_of_char_size();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_of_pointer_size0) {
  test_string_find::test_string_find_first_of_pointer_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_of_pointer_size1) {
  test_string_find::test_string_find_first_of_pointer_size1();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_of_pointer_size_size0) {
  test_string_find::test_string_find_first_of_pointer_size_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_of_pointer_size_size1) {
  test_string_find::test_string_find_first_of_pointer_size_size1();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_of_pointer_size_size2) {
  test_string_find::test_string_find_first_of_pointer_size_size2();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_of_pointer_size_size3) {
  test_string_find::test_string_find_first_of_pointer_size_size3();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_of_string_size0) {
  test_string_find::test_string_find_first_of_string_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_first_of_string_size1) {
  test_string_find::test_string_find_first_of_string_size1();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_not_of_char_size) {
  test_string_find::test_string_find_last_not_of_char_size();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_not_of_pointer_size0) {
  test_string_find::test_string_find_last_not_of_pointer_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_not_of_pointer_size1) {
  test_string_find::test_string_find_last_not_of_pointer_size1();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_not_of_pointer_size_size0) {
  test_string_find::test_string_find_last_not_of_pointer_size_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_not_of_pointer_size_size1) {
  test_string_find::test_string_find_last_not_of_pointer_size_size1();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_not_of_pointer_size_size2) {
  test_string_find::test_string_find_last_not_of_pointer_size_size2();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_not_of_pointer_size_size3) {
  test_string_find::test_string_find_last_not_of_pointer_size_size3();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_not_of_string_size0) {
  test_string_find::test_string_find_last_not_of_string_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_not_of_string_size1) {
  test_string_find::test_string_find_last_not_of_string_size1();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_of_char_size) {
  test_string_find::test_string_find_last_of_char_size();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_of_pointer_size0) {
  test_string_find::test_string_find_last_of_pointer_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_of_pointer_size1) {
  test_string_find::test_string_find_last_of_pointer_size1();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_of_pointer_size_size0) {
  test_string_find::test_string_find_last_of_pointer_size_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_of_pointer_size_size1) {
  test_string_find::test_string_find_last_of_pointer_size_size1();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_of_pointer_size_size2) {
  test_string_find::test_string_find_last_of_pointer_size_size2();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_of_pointer_size_size3) {
  test_string_find::test_string_find_last_of_pointer_size_size3();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_of_string_size0) {
  test_string_find::test_string_find_last_of_string_size0();
}
BOOST_AUTO_TEST_CASE(test_string_find_last_of_string_size1) {
  test_string_find::test_string_find_last_of_string_size1();
}
BOOST_AUTO_TEST_CASE(test_string_rfind_char_size) {
  test_string_find::test_string_rfind_char_size();
}
BOOST_AUTO_TEST_CASE(test_string_rfind_pointer_size0) {
  test_string_find::test_string_rfind_pointer_size0();
}
BOOST_AUTO_TEST_CASE(test_string_rfind_pointer_size1) {
  test_string_find::test_string_rfind_pointer_size1();
}
BOOST_AUTO_TEST_CASE(test_string_rfind_pointer_size_size0) {
  test_string_find::test_string_rfind_pointer_size_size0();
}
BOOST_AUTO_TEST_CASE(test_string_rfind_pointer_size_size1) {
  test_string_find::test_string_rfind_pointer_size_size1();
}
BOOST_AUTO_TEST_CASE(test_string_rfind_pointer_size_size2) {
  test_string_find::test_string_rfind_pointer_size_size2();
}
BOOST_AUTO_TEST_CASE(test_string_rfind_pointer_size_size3) {
  test_string_find::test_string_rfind_pointer_size_size3();
}
BOOST_AUTO_TEST_CASE(test_string_rfind_string_size0) {
  test_string_find::test_string_rfind_string_size0();
}
BOOST_AUTO_TEST_CASE(test_string_rfind_string_size1) {
  test_string_find::test_string_rfind_string_size1();
}
BOOST_AUTO_TEST_SUITE_END()
