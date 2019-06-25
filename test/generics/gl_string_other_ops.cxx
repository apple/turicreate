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

struct test_string_other_operators  {
 public:

  void test_string_io_get_line_delim()
  {
    std::istringstream in(" abc*  def**   ghij");
    gl_string s("initial text");
    std::getline(in, s, '*');
    TS_ASSERT(in.good());
    TS_ASSERT(s == " abc");
    std::getline(in, s, '*');
    TS_ASSERT(in.good());
    TS_ASSERT(s == "  def");
    std::getline(in, s, '*');
    TS_ASSERT(in.good());
    TS_ASSERT(s == "");
    std::getline(in, s, '*');
    TS_ASSERT(in.eof());
    TS_ASSERT(s == "   ghij");
  }

  void test_string_io_get_line_delim_rv()
  {
    gl_string s("initial text");
    std::getline(std::istringstream(" abc*  def*   ghij"), s, '*');
    TS_ASSERT(s == " abc");
  }

  void test_string_io_get_line()
  {
    std::istringstream in(" abc\n  def\n   ghij");
    gl_string s("initial text");
    std::getline(in, s);
    TS_ASSERT(in.good());
    TS_ASSERT(s == " abc");
    std::getline(in, s);
    TS_ASSERT(in.good());
    TS_ASSERT(s == "  def");
    std::getline(in, s);
    TS_ASSERT(in.eof());
    TS_ASSERT(s == "   ghij");
  }

  void test_string_io_get_line_rv()
  {
    gl_string s("initial text");
    std::getline(std::istringstream(" abc\n  def\n   ghij"), s);
    TS_ASSERT(s == " abc");
  }

  void test_string_io_stream_extract()
  {
    std::istringstream in("a bc defghij");
    gl_string s("initial text");
    in >> s;
    TS_ASSERT(in.good());
    TS_ASSERT(s == "a");
    TS_ASSERT(in.peek() == ' ');
    in >> s;
    TS_ASSERT(in.good());
    TS_ASSERT(s == "bc");
    TS_ASSERT(in.peek() == ' ');
    in.width(3);
    in >> s;
    TS_ASSERT(in.good());
    TS_ASSERT(s == "def");
    TS_ASSERT(in.peek() == 'g');
    in >> s;
    TS_ASSERT(in.eof());
    TS_ASSERT(s == "ghij");
    in >> s;
    TS_ASSERT(in.fail());
  }

  void test_string_io_stream_insert()
  {
    std::ostringstream out;
    gl_string s("some text");
    out << s;
    TS_ASSERT(out.good());
    TS_ASSERT(s == out.str());
  }
  
  void _test_string_operator_eq__pointer_string(const char* lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs == rhs) == x);
  }

  void test_string_operator_eq__pointer_string()
  {
    _test_string_operator_eq__pointer_string("", gl_string(""), true);
    _test_string_operator_eq__pointer_string("", gl_string("abcde"), false);
    _test_string_operator_eq__pointer_string("", gl_string("abcdefghij"), false);
    _test_string_operator_eq__pointer_string("", gl_string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__pointer_string("abcde", gl_string(""), false);
    _test_string_operator_eq__pointer_string("abcde", gl_string("abcde"), true);
    _test_string_operator_eq__pointer_string("abcde", gl_string("abcdefghij"), false);
    _test_string_operator_eq__pointer_string("abcde", gl_string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__pointer_string("abcdefghij", gl_string(""), false);
    _test_string_operator_eq__pointer_string("abcdefghij", gl_string("abcde"), false);
    _test_string_operator_eq__pointer_string("abcdefghij", gl_string("abcdefghij"), true);
    _test_string_operator_eq__pointer_string("abcdefghij", gl_string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__pointer_string("abcdefghijklmnopqrst", gl_string(""), false);
    _test_string_operator_eq__pointer_string("abcdefghijklmnopqrst", gl_string("abcde"), false);
    _test_string_operator_eq__pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghij"), false);
    _test_string_operator_eq__pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghijklmnopqrst"), true);
  }

  void _test_string_operator_eq__string_string(const gl_string& lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs == rhs) == x);
  }

  void test_string_operator_eq__string_string()
  {
    
    _test_string_operator_eq__string_string(gl_string(""), gl_string(""), true);
    _test_string_operator_eq__string_string(gl_string(""), gl_string("abcde"), false);
    _test_string_operator_eq__string_string(gl_string(""), gl_string("abcdefghij"), false);
    _test_string_operator_eq__string_string(gl_string(""), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__string_string(gl_string("abcde"), gl_string(""), false);
    _test_string_operator_eq__string_string(gl_string("abcde"), gl_string("abcde"), true);
    _test_string_operator_eq__string_string(gl_string("abcde"), gl_string("abcdefghij"), false);
    _test_string_operator_eq__string_string(gl_string("abcde"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__string_string(gl_string("abcdefghij"), gl_string(""), false);
    _test_string_operator_eq__string_string(gl_string("abcdefghij"), gl_string("abcde"), false);
    _test_string_operator_eq__string_string(gl_string("abcdefghij"), gl_string("abcdefghij"), true);
    _test_string_operator_eq__string_string(gl_string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__string_string(gl_string("abcdefghijklmnopqrst"), gl_string(""), false);
    _test_string_operator_eq__string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcde"), false);
    _test_string_operator_eq__string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), false);
    _test_string_operator_eq__string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), true);
  }

  void _test_string_operator_eq__string_stdstring(const gl_string& lhs, const std::string& rhs, bool x)
  {
    TS_ASSERT((lhs == rhs) == x);
  }

  void test_string_operator_eq__string_stdstring()
  {
    
    _test_string_operator_eq__string_stdstring(gl_string(""), std::string(""), true);
    _test_string_operator_eq__string_stdstring(gl_string(""), std::string("abcde"), false);
    _test_string_operator_eq__string_stdstring(gl_string(""), std::string("abcdefghij"), false);
    _test_string_operator_eq__string_stdstring(gl_string(""), std::string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__string_stdstring(gl_string("abcde"), std::string(""), false);
    _test_string_operator_eq__string_stdstring(gl_string("abcde"), std::string("abcde"), true);
    _test_string_operator_eq__string_stdstring(gl_string("abcde"), std::string("abcdefghij"), false);
    _test_string_operator_eq__string_stdstring(gl_string("abcde"), std::string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__string_stdstring(gl_string("abcdefghij"), std::string(""), false);
    _test_string_operator_eq__string_stdstring(gl_string("abcdefghij"), std::string("abcde"), false);
    _test_string_operator_eq__string_stdstring(gl_string("abcdefghij"), std::string("abcdefghij"), true);
    _test_string_operator_eq__string_stdstring(gl_string("abcdefghij"), std::string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__string_stdstring(gl_string("abcdefghijklmnopqrst"), std::string(""), false);
    _test_string_operator_eq__string_stdstring(gl_string("abcdefghijklmnopqrst"), std::string("abcde"), false);
    _test_string_operator_eq__string_stdstring(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghij"), false);
    _test_string_operator_eq__string_stdstring(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghijklmnopqrst"), true);
  }


  void _test_string_operator_eq__string_string(const std::string& lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs == rhs) == x);
  }

  void test_string_operator_eq__stdstring_string()
  {
    _test_string_operator_eq__string_string(std::string(""), gl_string(""), true);
    _test_string_operator_eq__string_string(std::string(""), gl_string("abcde"), false);
    _test_string_operator_eq__string_string(std::string(""), gl_string("abcdefghij"), false);
    _test_string_operator_eq__string_string(std::string(""), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__string_string(std::string("abcde"), gl_string(""), false);
    _test_string_operator_eq__string_string(std::string("abcde"), gl_string("abcde"), true);
    _test_string_operator_eq__string_string(std::string("abcde"), gl_string("abcdefghij"), false);
    _test_string_operator_eq__string_string(std::string("abcde"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__string_string(std::string("abcdefghij"), gl_string(""), false);
    _test_string_operator_eq__string_string(std::string("abcdefghij"), gl_string("abcde"), false);
    _test_string_operator_eq__string_string(std::string("abcdefghij"), gl_string("abcdefghij"), true);
    _test_string_operator_eq__string_string(std::string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_operator_eq__string_string(std::string("abcdefghijklmnopqrst"), gl_string(""), false);
    _test_string_operator_eq__string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcde"), false);
    _test_string_operator_eq__string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), false);
    _test_string_operator_eq__string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), true);
  }

  
  void _test_string_opgteq_pointer_string(const char* lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs >= rhs) == x);
  }

  void test_string_opgteq_pointer_string()
  {
    _test_string_opgteq_pointer_string("", gl_string(""), true);
    _test_string_opgteq_pointer_string("", gl_string("abcde"), false);
    _test_string_opgteq_pointer_string("", gl_string("abcdefghij"), false);
    _test_string_opgteq_pointer_string("", gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_pointer_string("abcde", gl_string(""), true);
    _test_string_opgteq_pointer_string("abcde", gl_string("abcde"), true);
    _test_string_opgteq_pointer_string("abcde", gl_string("abcdefghij"), false);
    _test_string_opgteq_pointer_string("abcde", gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_pointer_string("abcdefghij", gl_string(""), true);
    _test_string_opgteq_pointer_string("abcdefghij", gl_string("abcde"), true);
    _test_string_opgteq_pointer_string("abcdefghij", gl_string("abcdefghij"), true);
    _test_string_opgteq_pointer_string("abcdefghij", gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_pointer_string("abcdefghijklmnopqrst", gl_string(""), true);
    _test_string_opgteq_pointer_string("abcdefghijklmnopqrst", gl_string("abcde"), true);
    _test_string_opgteq_pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghij"), true);
    _test_string_opgteq_pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghijklmnopqrst"), true);
  }

  
  void _test_string_opgteq_string_pointer(const gl_string& lhs, const char* rhs, bool x)
  {
    TS_ASSERT((lhs >= rhs) == x);
  }

  void test_string_opgteq_string_pointer()
  {
    _test_string_opgteq_string_pointer(gl_string(""), "", true);
    _test_string_opgteq_string_pointer(gl_string(""), "abcde", false);
    _test_string_opgteq_string_pointer(gl_string(""), "abcdefghij", false);
    _test_string_opgteq_string_pointer(gl_string(""), "abcdefghijklmnopqrst", false);
    _test_string_opgteq_string_pointer(gl_string("abcde"), "", true);
    _test_string_opgteq_string_pointer(gl_string("abcde"), "abcde", true);
    _test_string_opgteq_string_pointer(gl_string("abcde"), "abcdefghij", false);
    _test_string_opgteq_string_pointer(gl_string("abcde"), "abcdefghijklmnopqrst", false);
    _test_string_opgteq_string_pointer(gl_string("abcdefghij"), "", true);
    _test_string_opgteq_string_pointer(gl_string("abcdefghij"), "abcde", true);
    _test_string_opgteq_string_pointer(gl_string("abcdefghij"), "abcdefghij", true);
    _test_string_opgteq_string_pointer(gl_string("abcdefghij"), "abcdefghijklmnopqrst", false);
    _test_string_opgteq_string_pointer(gl_string("abcdefghijklmnopqrst"), "", true);
    _test_string_opgteq_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcde", true);
    _test_string_opgteq_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcdefghij", true);
    _test_string_opgteq_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcdefghijklmnopqrst", true);
  }
  
  void _test_string_opgteq_string_string(const gl_string& lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs >= rhs) == x);
  }

  void _test_string_opgteq_string_string(const gl_string& lhs, const std::string& rhs, bool x)
  {
    TS_ASSERT((lhs >= rhs) == x);
  }
  
  void _test_string_opgteq_string_string(const std::string& lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs >= rhs) == x);
  }

  void test_string_opgteq_string_string0()
  {
    _test_string_opgteq_string_string(gl_string(""), gl_string(""), true);
    _test_string_opgteq_string_string(gl_string(""), gl_string("abcde"), false);
    _test_string_opgteq_string_string(gl_string(""), gl_string("abcdefghij"), false);
    _test_string_opgteq_string_string(gl_string(""), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_string_string(gl_string("abcde"), gl_string(""), true);
    _test_string_opgteq_string_string(gl_string("abcde"), gl_string("abcde"), true);
    _test_string_opgteq_string_string(gl_string("abcde"), gl_string("abcdefghij"), false);
    _test_string_opgteq_string_string(gl_string("abcde"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_string_string(gl_string("abcdefghij"), gl_string(""), true);
    _test_string_opgteq_string_string(gl_string("abcdefghij"), gl_string("abcde"), true);
    _test_string_opgteq_string_string(gl_string("abcdefghij"), gl_string("abcdefghij"), true);
    _test_string_opgteq_string_string(gl_string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string(""), true);
    _test_string_opgteq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcde"), true);
    _test_string_opgteq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), true);
    _test_string_opgteq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), true);
  }
  
  void test_string_opgteq_string_string1() {
    _test_string_opgteq_string_string(gl_string(""), std::string(""), true);
    _test_string_opgteq_string_string(gl_string(""), std::string("abcde"), false);
    _test_string_opgteq_string_string(gl_string(""), std::string("abcdefghij"), false);
    _test_string_opgteq_string_string(gl_string(""), std::string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_string_string(gl_string("abcde"), std::string(""), true);
    _test_string_opgteq_string_string(gl_string("abcde"), std::string("abcde"), true);
    _test_string_opgteq_string_string(gl_string("abcde"), std::string("abcdefghij"), false);
    _test_string_opgteq_string_string(gl_string("abcde"), std::string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_string_string(gl_string("abcdefghij"), std::string(""), true);
    _test_string_opgteq_string_string(gl_string("abcdefghij"), std::string("abcde"), true);
    _test_string_opgteq_string_string(gl_string("abcdefghij"), std::string("abcdefghij"), true);
    _test_string_opgteq_string_string(gl_string("abcdefghij"), std::string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_string_string(gl_string("abcdefghijklmnopqrst"), std::string(""), true);
    _test_string_opgteq_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcde"), true);
    _test_string_opgteq_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghij"), true);
    _test_string_opgteq_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghijklmnopqrst"), true);
  }


  void test_string_opgteq_string_string2() {
    _test_string_opgteq_string_string(std::string(""), gl_string(""), true);
    _test_string_opgteq_string_string(std::string(""), gl_string("abcde"), false);
    _test_string_opgteq_string_string(std::string(""), gl_string("abcdefghij"), false);
    _test_string_opgteq_string_string(std::string(""), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_string_string(std::string("abcde"), gl_string(""), true);
    _test_string_opgteq_string_string(std::string("abcde"), gl_string("abcde"), true);
    _test_string_opgteq_string_string(std::string("abcde"), gl_string("abcdefghij"), false);
    _test_string_opgteq_string_string(std::string("abcde"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_string_string(std::string("abcdefghij"), gl_string(""), true);
    _test_string_opgteq_string_string(std::string("abcdefghij"), gl_string("abcde"), true);
    _test_string_opgteq_string_string(std::string("abcdefghij"), gl_string("abcdefghij"), true);
    _test_string_opgteq_string_string(std::string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgteq_string_string(std::string("abcdefghijklmnopqrst"), gl_string(""), true);
    _test_string_opgteq_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcde"), true);
    _test_string_opgteq_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), true);
    _test_string_opgteq_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), true);
  }

  void _test_string_opgt_pointer_string(const char* lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs > rhs) == x);
  }

  void test_string_opgt_pointer_string()
  {
    _test_string_opgt_pointer_string("", gl_string(""), false);
    _test_string_opgt_pointer_string("", gl_string("abcde"), false);
    _test_string_opgt_pointer_string("", gl_string("abcdefghij"), false);
    _test_string_opgt_pointer_string("", gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_pointer_string("abcde", gl_string(""), true);
    _test_string_opgt_pointer_string("abcde", gl_string("abcde"), false);
    _test_string_opgt_pointer_string("abcde", gl_string("abcdefghij"), false);
    _test_string_opgt_pointer_string("abcde", gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_pointer_string("abcdefghij", gl_string(""), true);
    _test_string_opgt_pointer_string("abcdefghij", gl_string("abcde"), true);
    _test_string_opgt_pointer_string("abcdefghij", gl_string("abcdefghij"), false);
    _test_string_opgt_pointer_string("abcdefghij", gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_pointer_string("abcdefghijklmnopqrst", gl_string(""), true);
    _test_string_opgt_pointer_string("abcdefghijklmnopqrst", gl_string("abcde"), true);
    _test_string_opgt_pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghij"), true);
    _test_string_opgt_pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghijklmnopqrst"), false);
  }
  
  void _test_string_opgt_string_pointer(const gl_string& lhs, const char* rhs, bool x)
  {
    TS_ASSERT((lhs > rhs) == x);
  }

  void test_string_opgt_string_pointer()
  {
    _test_string_opgt_string_pointer(gl_string(""), "", false);
    _test_string_opgt_string_pointer(gl_string(""), "abcde", false);
    _test_string_opgt_string_pointer(gl_string(""), "abcdefghij", false);
    _test_string_opgt_string_pointer(gl_string(""), "abcdefghijklmnopqrst", false);
    _test_string_opgt_string_pointer(gl_string("abcde"), "", true);
    _test_string_opgt_string_pointer(gl_string("abcde"), "abcde", false);
    _test_string_opgt_string_pointer(gl_string("abcde"), "abcdefghij", false);
    _test_string_opgt_string_pointer(gl_string("abcde"), "abcdefghijklmnopqrst", false);
    _test_string_opgt_string_pointer(gl_string("abcdefghij"), "", true);
    _test_string_opgt_string_pointer(gl_string("abcdefghij"), "abcde", true);
    _test_string_opgt_string_pointer(gl_string("abcdefghij"), "abcdefghij", false);
    _test_string_opgt_string_pointer(gl_string("abcdefghij"), "abcdefghijklmnopqrst", false);
    _test_string_opgt_string_pointer(gl_string("abcdefghijklmnopqrst"), "", true);
    _test_string_opgt_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcde", true);
    _test_string_opgt_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcdefghij", true);
    _test_string_opgt_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcdefghijklmnopqrst", false);
  }
  
  void _test_string_opgt_string_string(const gl_string& lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs > rhs) == x);
  }

  void _test_string_opgt_string_string(const gl_string& lhs, const std::string& rhs, bool x)
  {
    TS_ASSERT((lhs > rhs) == x);
  }

  void _test_string_opgt_string_string(const std::string& lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs > rhs) == x);
  }

  void test_string_opgt_string_string0()
  {
    _test_string_opgt_string_string(gl_string(""), gl_string(""), false);
    _test_string_opgt_string_string(gl_string(""), gl_string("abcde"), false);
    _test_string_opgt_string_string(gl_string(""), gl_string("abcdefghij"), false);
    _test_string_opgt_string_string(gl_string(""), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_string_string(gl_string("abcde"), gl_string(""), true);
    _test_string_opgt_string_string(gl_string("abcde"), gl_string("abcde"), false);
    _test_string_opgt_string_string(gl_string("abcde"), gl_string("abcdefghij"), false);
    _test_string_opgt_string_string(gl_string("abcde"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_string_string(gl_string("abcdefghij"), gl_string(""), true);
    _test_string_opgt_string_string(gl_string("abcdefghij"), gl_string("abcde"), true);
    _test_string_opgt_string_string(gl_string("abcdefghij"), gl_string("abcdefghij"), false);
    _test_string_opgt_string_string(gl_string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_string_string(gl_string("abcdefghijklmnopqrst"), gl_string(""), true);
    _test_string_opgt_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcde"), true);
    _test_string_opgt_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), true);
    _test_string_opgt_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), false);
  }
  
  void test_string_opgt_string_string1()
  {
    _test_string_opgt_string_string(gl_string(""), std::string(""), false);
    _test_string_opgt_string_string(gl_string(""), std::string("abcde"), false);
    _test_string_opgt_string_string(gl_string(""), std::string("abcdefghij"), false);
    _test_string_opgt_string_string(gl_string(""), std::string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_string_string(gl_string("abcde"), std::string(""), true);
    _test_string_opgt_string_string(gl_string("abcde"), std::string("abcde"), false);
    _test_string_opgt_string_string(gl_string("abcde"), std::string("abcdefghij"), false);
    _test_string_opgt_string_string(gl_string("abcde"), std::string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_string_string(gl_string("abcdefghij"), std::string(""), true);
    _test_string_opgt_string_string(gl_string("abcdefghij"), std::string("abcde"), true);
    _test_string_opgt_string_string(gl_string("abcdefghij"), std::string("abcdefghij"), false);
    _test_string_opgt_string_string(gl_string("abcdefghij"), std::string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_string_string(gl_string("abcdefghijklmnopqrst"), std::string(""), true);
    _test_string_opgt_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcde"), true);
    _test_string_opgt_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghij"), true);
    _test_string_opgt_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghijklmnopqrst"), false);
  }

  void test_string_opgt_string_string2()
  {
    _test_string_opgt_string_string(std::string(""), gl_string(""), false);
    _test_string_opgt_string_string(std::string(""), gl_string("abcde"), false);
    _test_string_opgt_string_string(std::string(""), gl_string("abcdefghij"), false);
    _test_string_opgt_string_string(std::string(""), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_string_string(std::string("abcde"), gl_string(""), true);
    _test_string_opgt_string_string(std::string("abcde"), gl_string("abcde"), false);
    _test_string_opgt_string_string(std::string("abcde"), gl_string("abcdefghij"), false);
    _test_string_opgt_string_string(std::string("abcde"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_string_string(std::string("abcdefghij"), gl_string(""), true);
    _test_string_opgt_string_string(std::string("abcdefghij"), gl_string("abcde"), true);
    _test_string_opgt_string_string(std::string("abcdefghij"), gl_string("abcdefghij"), false);
    _test_string_opgt_string_string(std::string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), false);
    _test_string_opgt_string_string(std::string("abcdefghijklmnopqrst"), gl_string(""), true);
    _test_string_opgt_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcde"), true);
    _test_string_opgt_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), true);
    _test_string_opgt_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), false);
  }
  
  

  void _test_string_oplteq_pointer_string(const char* lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs <= rhs) == x);
  }

  void test_string_oplteq_pointer_string()
  {
    _test_string_oplteq_pointer_string("", gl_string(""), true);
    _test_string_oplteq_pointer_string("", gl_string("abcde"), true);
    _test_string_oplteq_pointer_string("", gl_string("abcdefghij"), true);
    _test_string_oplteq_pointer_string("", gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_pointer_string("abcde", gl_string(""), false);
    _test_string_oplteq_pointer_string("abcde", gl_string("abcde"), true);
    _test_string_oplteq_pointer_string("abcde", gl_string("abcdefghij"), true);
    _test_string_oplteq_pointer_string("abcde", gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_pointer_string("abcdefghij", gl_string(""), false);
    _test_string_oplteq_pointer_string("abcdefghij", gl_string("abcde"), false);
    _test_string_oplteq_pointer_string("abcdefghij", gl_string("abcdefghij"), true);
    _test_string_oplteq_pointer_string("abcdefghij", gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_pointer_string("abcdefghijklmnopqrst", gl_string(""), false);
    _test_string_oplteq_pointer_string("abcdefghijklmnopqrst", gl_string("abcde"), false);
    _test_string_oplteq_pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghij"), false);
    _test_string_oplteq_pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghijklmnopqrst"), true);
  }


  void _test_string_oplteq_string_pointer(const gl_string& lhs, const char* rhs, bool x)
  {
    TS_ASSERT((lhs <= rhs) == x);
  }

  void test_string_oplteq_string_pointer()
  {
    _test_string_oplteq_string_pointer(gl_string(""), "", true);
    _test_string_oplteq_string_pointer(gl_string(""), "abcde", true);
    _test_string_oplteq_string_pointer(gl_string(""), "abcdefghij", true);
    _test_string_oplteq_string_pointer(gl_string(""), "abcdefghijklmnopqrst", true);
    _test_string_oplteq_string_pointer(gl_string("abcde"), "", false);
    _test_string_oplteq_string_pointer(gl_string("abcde"), "abcde", true);
    _test_string_oplteq_string_pointer(gl_string("abcde"), "abcdefghij", true);
    _test_string_oplteq_string_pointer(gl_string("abcde"), "abcdefghijklmnopqrst", true);
    _test_string_oplteq_string_pointer(gl_string("abcdefghij"), "", false);
    _test_string_oplteq_string_pointer(gl_string("abcdefghij"), "abcde", false);
    _test_string_oplteq_string_pointer(gl_string("abcdefghij"), "abcdefghij", true);
    _test_string_oplteq_string_pointer(gl_string("abcdefghij"), "abcdefghijklmnopqrst", true);
    _test_string_oplteq_string_pointer(gl_string("abcdefghijklmnopqrst"), "", false);
    _test_string_oplteq_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcde", false);
    _test_string_oplteq_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcdefghij", false);
    _test_string_oplteq_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcdefghijklmnopqrst", true);
  }


  void _test_string_oplteq_string_string(const gl_string& lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs <= rhs) == x);
  }

  void _test_string_oplteq_string_string(const gl_string& lhs, const std::string& rhs, bool x)
  {
    TS_ASSERT((lhs <= rhs) == x);
  }
  
  void _test_string_oplteq_string_string(const std::string& lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs <= rhs) == x);
  }

  void test_string_oplteq_string_string0()
  {
    _test_string_oplteq_string_string(gl_string(""), gl_string(""), true);
    _test_string_oplteq_string_string(gl_string(""), gl_string("abcde"), true);
    _test_string_oplteq_string_string(gl_string(""), gl_string("abcdefghij"), true);
    _test_string_oplteq_string_string(gl_string(""), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_string_string(gl_string("abcde"), gl_string(""), false);
    _test_string_oplteq_string_string(gl_string("abcde"), gl_string("abcde"), true);
    _test_string_oplteq_string_string(gl_string("abcde"), gl_string("abcdefghij"), true);
    _test_string_oplteq_string_string(gl_string("abcde"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_string_string(gl_string("abcdefghij"), gl_string(""), false);
    _test_string_oplteq_string_string(gl_string("abcdefghij"), gl_string("abcde"), false);
    _test_string_oplteq_string_string(gl_string("abcdefghij"), gl_string("abcdefghij"), true);
    _test_string_oplteq_string_string(gl_string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string(""), false);
    _test_string_oplteq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcde"), false);
    _test_string_oplteq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), false);
    _test_string_oplteq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), true);
  }

  void test_string_oplteq_string_string1()
  {
    _test_string_oplteq_string_string(gl_string(""), std::string(""), true);
    _test_string_oplteq_string_string(gl_string(""), std::string("abcde"), true);
    _test_string_oplteq_string_string(gl_string(""), std::string("abcdefghij"), true);
    _test_string_oplteq_string_string(gl_string(""), std::string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_string_string(gl_string("abcde"), std::string(""), false);
    _test_string_oplteq_string_string(gl_string("abcde"), std::string("abcde"), true);
    _test_string_oplteq_string_string(gl_string("abcde"), std::string("abcdefghij"), true);
    _test_string_oplteq_string_string(gl_string("abcde"), std::string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_string_string(gl_string("abcdefghij"), std::string(""), false);
    _test_string_oplteq_string_string(gl_string("abcdefghij"), std::string("abcde"), false);
    _test_string_oplteq_string_string(gl_string("abcdefghij"), std::string("abcdefghij"), true);
    _test_string_oplteq_string_string(gl_string("abcdefghij"), std::string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_string_string(gl_string("abcdefghijklmnopqrst"), std::string(""), false);
    _test_string_oplteq_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcde"), false);
    _test_string_oplteq_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghij"), false);
    _test_string_oplteq_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghijklmnopqrst"), true);
  }

  void test_string_oplteq_string_string2()
  {
    _test_string_oplteq_string_string(std::string(""), gl_string(""), true);
    _test_string_oplteq_string_string(std::string(""), gl_string("abcde"), true);
    _test_string_oplteq_string_string(std::string(""), gl_string("abcdefghij"), true);
    _test_string_oplteq_string_string(std::string(""), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_string_string(std::string("abcde"), gl_string(""), false);
    _test_string_oplteq_string_string(std::string("abcde"), gl_string("abcde"), true);
    _test_string_oplteq_string_string(std::string("abcde"), gl_string("abcdefghij"), true);
    _test_string_oplteq_string_string(std::string("abcde"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_string_string(std::string("abcdefghij"), gl_string(""), false);
    _test_string_oplteq_string_string(std::string("abcdefghij"), gl_string("abcde"), false);
    _test_string_oplteq_string_string(std::string("abcdefghij"), gl_string("abcdefghij"), true);
    _test_string_oplteq_string_string(std::string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplteq_string_string(std::string("abcdefghijklmnopqrst"), gl_string(""), false);
    _test_string_oplteq_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcde"), false);
    _test_string_oplteq_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), false);
    _test_string_oplteq_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), true);
  }
  

  void _test_string_oplt_pointer_string(const char* lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs < rhs) == x);
  }

  void test_string_oplt_pointer_string()
  {
    _test_string_oplt_pointer_string("", gl_string(""), false);
    _test_string_oplt_pointer_string("", gl_string("abcde"), true);
    _test_string_oplt_pointer_string("", gl_string("abcdefghij"), true);
    _test_string_oplt_pointer_string("", gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_pointer_string("abcde", gl_string(""), false);
    _test_string_oplt_pointer_string("abcde", gl_string("abcde"), false);
    _test_string_oplt_pointer_string("abcde", gl_string("abcdefghij"), true);
    _test_string_oplt_pointer_string("abcde", gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_pointer_string("abcdefghij", gl_string(""), false);
    _test_string_oplt_pointer_string("abcdefghij", gl_string("abcde"), false);
    _test_string_oplt_pointer_string("abcdefghij", gl_string("abcdefghij"), false);
    _test_string_oplt_pointer_string("abcdefghij", gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_pointer_string("abcdefghijklmnopqrst", gl_string(""), false);
    _test_string_oplt_pointer_string("abcdefghijklmnopqrst", gl_string("abcde"), false);
    _test_string_oplt_pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghij"), false);
    _test_string_oplt_pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghijklmnopqrst"), false);
  }

  void _test_string_oplt_string_pointer(const gl_string& lhs, const char* rhs, bool x)
  {
    TS_ASSERT((lhs < rhs) == x);
  }

  void test_string_oplt_string_pointer() {
    _test_string_oplt_string_pointer(gl_string(""), "", false);
    _test_string_oplt_string_pointer(gl_string(""), "abcde", true);
    _test_string_oplt_string_pointer(gl_string(""), "abcdefghij", true);
    _test_string_oplt_string_pointer(gl_string(""), "abcdefghijklmnopqrst", true);
    _test_string_oplt_string_pointer(gl_string("abcde"), "", false);
    _test_string_oplt_string_pointer(gl_string("abcde"), "abcde", false);
    _test_string_oplt_string_pointer(gl_string("abcde"), "abcdefghij", true);
    _test_string_oplt_string_pointer(gl_string("abcde"), "abcdefghijklmnopqrst", true);
    _test_string_oplt_string_pointer(gl_string("abcdefghij"), "", false);
    _test_string_oplt_string_pointer(gl_string("abcdefghij"), "abcde", false);
    _test_string_oplt_string_pointer(gl_string("abcdefghij"), "abcdefghij", false);
    _test_string_oplt_string_pointer(gl_string("abcdefghij"), "abcdefghijklmnopqrst", true);
    _test_string_oplt_string_pointer(gl_string("abcdefghijklmnopqrst"), "", false);
    _test_string_oplt_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcde", false);
    _test_string_oplt_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcdefghij", false);
    _test_string_oplt_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcdefghijklmnopqrst", false);
  }


  void _test_string_oplt_string_string(const gl_string& lhs, const gl_string& rhs, bool x) {
    TS_ASSERT((lhs < rhs) == x);
  }


  void _test_string_oplt_string_string(const gl_string& lhs, const std::string& rhs, bool x) {
    TS_ASSERT((lhs < rhs) == x);
  }


  void _test_string_oplt_string_string(const std::string& lhs, const gl_string& rhs, bool x) {
    TS_ASSERT((lhs < rhs) == x);
  }
  
  void test_string_oplt_string_string0()
  {
    _test_string_oplt_string_string(gl_string(""), gl_string(""), false);
    _test_string_oplt_string_string(gl_string(""), gl_string("abcde"), true);
    _test_string_oplt_string_string(gl_string(""), gl_string("abcdefghij"), true);
    _test_string_oplt_string_string(gl_string(""), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_string_string(gl_string("abcde"), gl_string(""), false);
    _test_string_oplt_string_string(gl_string("abcde"), gl_string("abcde"), false);
    _test_string_oplt_string_string(gl_string("abcde"), gl_string("abcdefghij"), true);
    _test_string_oplt_string_string(gl_string("abcde"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_string_string(gl_string("abcdefghij"), gl_string(""), false);
    _test_string_oplt_string_string(gl_string("abcdefghij"), gl_string("abcde"), false);
    _test_string_oplt_string_string(gl_string("abcdefghij"), gl_string("abcdefghij"), false);
    _test_string_oplt_string_string(gl_string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_string_string(gl_string("abcdefghijklmnopqrst"), gl_string(""), false);
    _test_string_oplt_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcde"), false);
    _test_string_oplt_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), false);
    _test_string_oplt_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), false);
  }

  void test_string_oplt_string_string1()
  {
    _test_string_oplt_string_string(gl_string(""), std::string(""), false);
    _test_string_oplt_string_string(gl_string(""), std::string("abcde"), true);
    _test_string_oplt_string_string(gl_string(""), std::string("abcdefghij"), true);
    _test_string_oplt_string_string(gl_string(""), std::string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_string_string(gl_string("abcde"), std::string(""), false);
    _test_string_oplt_string_string(gl_string("abcde"), std::string("abcde"), false);
    _test_string_oplt_string_string(gl_string("abcde"), std::string("abcdefghij"), true);
    _test_string_oplt_string_string(gl_string("abcde"), std::string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_string_string(gl_string("abcdefghij"), std::string(""), false);
    _test_string_oplt_string_string(gl_string("abcdefghij"), std::string("abcde"), false);
    _test_string_oplt_string_string(gl_string("abcdefghij"), std::string("abcdefghij"), false);
    _test_string_oplt_string_string(gl_string("abcdefghij"), std::string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_string_string(gl_string("abcdefghijklmnopqrst"), std::string(""), false);
    _test_string_oplt_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcde"), false);
    _test_string_oplt_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghij"), false);
    _test_string_oplt_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghijklmnopqrst"), false);
  }

  void test_string_oplt_string_string2()
  {
    _test_string_oplt_string_string(std::string(""), gl_string(""), false);
    _test_string_oplt_string_string(std::string(""), gl_string("abcde"), true);
    _test_string_oplt_string_string(std::string(""), gl_string("abcdefghij"), true);
    _test_string_oplt_string_string(std::string(""), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_string_string(std::string("abcde"), gl_string(""), false);
    _test_string_oplt_string_string(std::string("abcde"), gl_string("abcde"), false);
    _test_string_oplt_string_string(std::string("abcde"), gl_string("abcdefghij"), true);
    _test_string_oplt_string_string(std::string("abcde"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_string_string(std::string("abcdefghij"), gl_string(""), false);
    _test_string_oplt_string_string(std::string("abcdefghij"), gl_string("abcde"), false);
    _test_string_oplt_string_string(std::string("abcdefghij"), gl_string("abcdefghij"), false);
    _test_string_oplt_string_string(std::string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_oplt_string_string(std::string("abcdefghijklmnopqrst"), gl_string(""), false);
    _test_string_oplt_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcde"), false);
    _test_string_oplt_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), false);
    _test_string_oplt_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), false);
  }
  

  void _test_string_op_not_eq_pointer_string(const char* lhs, const gl_string& rhs, bool x) {
    TS_ASSERT((lhs != rhs) == x);
  }

  void test_string_op_not_eq_pointer_string() {
    _test_string_op_not_eq_pointer_string("", gl_string(""), false);
    _test_string_op_not_eq_pointer_string("", gl_string("abcde"), true);
    _test_string_op_not_eq_pointer_string("", gl_string("abcdefghij"), true);
    _test_string_op_not_eq_pointer_string("", gl_string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_pointer_string("abcde", gl_string(""), true);
    _test_string_op_not_eq_pointer_string("abcde", gl_string("abcde"), false);
    _test_string_op_not_eq_pointer_string("abcde", gl_string("abcdefghij"), true);
    _test_string_op_not_eq_pointer_string("abcde", gl_string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_pointer_string("abcdefghij", gl_string(""), true);
    _test_string_op_not_eq_pointer_string("abcdefghij", gl_string("abcde"), true);
    _test_string_op_not_eq_pointer_string("abcdefghij", gl_string("abcdefghij"), false);
    _test_string_op_not_eq_pointer_string("abcdefghij", gl_string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_pointer_string("abcdefghijklmnopqrst", gl_string(""), true);
    _test_string_op_not_eq_pointer_string("abcdefghijklmnopqrst", gl_string("abcde"), true);
    _test_string_op_not_eq_pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghij"), true);
    _test_string_op_not_eq_pointer_string("abcdefghijklmnopqrst", gl_string("abcdefghijklmnopqrst"), false);
  }


  void _test_string_op_not_eq_string_pointer(const gl_string& lhs, const char* rhs, bool x) {
    TS_ASSERT((lhs != rhs) == x);
  }

  void test_string_op_not_eq_string_pointer()
  {
    _test_string_op_not_eq_string_pointer(gl_string(""), "", false);
    _test_string_op_not_eq_string_pointer(gl_string(""), "abcde", true);
    _test_string_op_not_eq_string_pointer(gl_string(""), "abcdefghij", true);
    _test_string_op_not_eq_string_pointer(gl_string(""), "abcdefghijklmnopqrst", true);
    _test_string_op_not_eq_string_pointer(gl_string("abcde"), "", true);
    _test_string_op_not_eq_string_pointer(gl_string("abcde"), "abcde", false);
    _test_string_op_not_eq_string_pointer(gl_string("abcde"), "abcdefghij", true);
    _test_string_op_not_eq_string_pointer(gl_string("abcde"), "abcdefghijklmnopqrst", true);
    _test_string_op_not_eq_string_pointer(gl_string("abcdefghij"), "", true);
    _test_string_op_not_eq_string_pointer(gl_string("abcdefghij"), "abcde", true);
    _test_string_op_not_eq_string_pointer(gl_string("abcdefghij"), "abcdefghij", false);
    _test_string_op_not_eq_string_pointer(gl_string("abcdefghij"), "abcdefghijklmnopqrst", true);
    _test_string_op_not_eq_string_pointer(gl_string("abcdefghijklmnopqrst"), "", true);
    _test_string_op_not_eq_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcde", true);
    _test_string_op_not_eq_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcdefghij", true);
    _test_string_op_not_eq_string_pointer(gl_string("abcdefghijklmnopqrst"), "abcdefghijklmnopqrst", false);
  }

  void _test_string_op_not_eq_string_string(const gl_string& lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs != rhs) == x);
  }

  void _test_string_op_not_eq_string_string(const gl_string& lhs, const std::string& rhs, bool x)
  {
    TS_ASSERT((lhs != rhs) == x);
  }

  void _test_string_op_not_eq_string_string(const std::string& lhs, const gl_string& rhs, bool x)
  {
    TS_ASSERT((lhs != rhs) == x);
  }
  
  void test_string_op_not_eq_string_string0()
  {
    _test_string_op_not_eq_string_string(gl_string(""), gl_string(""), false);
    _test_string_op_not_eq_string_string(gl_string(""), gl_string("abcde"), true);
    _test_string_op_not_eq_string_string(gl_string(""), gl_string("abcdefghij"), true);
    _test_string_op_not_eq_string_string(gl_string(""), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_string_string(gl_string("abcde"), gl_string(""), true);
    _test_string_op_not_eq_string_string(gl_string("abcde"), gl_string("abcde"), false);
    _test_string_op_not_eq_string_string(gl_string("abcde"), gl_string("abcdefghij"), true);
    _test_string_op_not_eq_string_string(gl_string("abcde"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghij"), gl_string(""), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghij"), gl_string("abcde"), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghij"), gl_string("abcdefghij"), false);
    _test_string_op_not_eq_string_string(gl_string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string(""), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcde"), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), false);
  }

  void test_string_op_not_eq_string_string1()
  {
    _test_string_op_not_eq_string_string(gl_string(""), std::string(""), false);
    _test_string_op_not_eq_string_string(gl_string(""), std::string("abcde"), true);
    _test_string_op_not_eq_string_string(gl_string(""), std::string("abcdefghij"), true);
    _test_string_op_not_eq_string_string(gl_string(""), std::string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_string_string(gl_string("abcde"), std::string(""), true);
    _test_string_op_not_eq_string_string(gl_string("abcde"), std::string("abcde"), false);
    _test_string_op_not_eq_string_string(gl_string("abcde"), std::string("abcdefghij"), true);
    _test_string_op_not_eq_string_string(gl_string("abcde"), std::string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghij"), std::string(""), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghij"), std::string("abcde"), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghij"), std::string("abcdefghij"), false);
    _test_string_op_not_eq_string_string(gl_string("abcdefghij"), std::string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghijklmnopqrst"), std::string(""), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcde"), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghij"), true);
    _test_string_op_not_eq_string_string(gl_string("abcdefghijklmnopqrst"), std::string("abcdefghijklmnopqrst"), false);
  }

  void test_string_op_not_eq_string_string2()
  {
    _test_string_op_not_eq_string_string(std::string(""), gl_string(""), false);
    _test_string_op_not_eq_string_string(std::string(""), gl_string("abcde"), true);
    _test_string_op_not_eq_string_string(std::string(""), gl_string("abcdefghij"), true);
    _test_string_op_not_eq_string_string(std::string(""), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_string_string(std::string("abcde"), gl_string(""), true);
    _test_string_op_not_eq_string_string(std::string("abcde"), gl_string("abcde"), false);
    _test_string_op_not_eq_string_string(std::string("abcde"), gl_string("abcdefghij"), true);
    _test_string_op_not_eq_string_string(std::string("abcde"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_string_string(std::string("abcdefghij"), gl_string(""), true);
    _test_string_op_not_eq_string_string(std::string("abcdefghij"), gl_string("abcde"), true);
    _test_string_op_not_eq_string_string(std::string("abcdefghij"), gl_string("abcdefghij"), false);
    _test_string_op_not_eq_string_string(std::string("abcdefghij"), gl_string("abcdefghijklmnopqrst"), true);
    _test_string_op_not_eq_string_string(std::string("abcdefghijklmnopqrst"), gl_string(""), true);
    _test_string_op_not_eq_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcde"), true);
    _test_string_op_not_eq_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghij"), true);
    _test_string_op_not_eq_string_string(std::string("abcdefghijklmnopqrst"), gl_string("abcdefghijklmnopqrst"), false);
  }
  
  void _test_string_op_plus__char_string0(char lhs, const gl_string& rhs, const gl_string& x) {
    TS_ASSERT(lhs + rhs == x);
  }

  void _test_string_op_plus__char_string1(char lhs, gl_string&& rhs, const gl_string& x) {
    TS_ASSERT(lhs + std::move(rhs) == x);
  }

  void test_string_op_plus__char_string()
  {
    _test_string_op_plus__char_string0('a', gl_string(""), gl_string("a"));
    _test_string_op_plus__char_string0('a', gl_string("12345"), gl_string("a12345"));
    _test_string_op_plus__char_string0('a', gl_string("1234567890"), gl_string("a1234567890"));
    _test_string_op_plus__char_string0('a', gl_string("12345678901234567890"), gl_string("a12345678901234567890"));

    _test_string_op_plus__char_string1('a', gl_string(""), gl_string("a"));
    _test_string_op_plus__char_string1('a', gl_string("12345"), gl_string("a12345"));
    _test_string_op_plus__char_string1('a', gl_string("1234567890"), gl_string("a1234567890"));
    _test_string_op_plus__char_string1('a', gl_string("12345678901234567890"), gl_string("a12345678901234567890"));
  }
  
  void _test_string_op_plus__pointer_string0(const char* lhs, const gl_string& rhs, const gl_string& x)
  {
    TS_ASSERT(lhs + rhs == x);
  }

  void _test_string_op_plus__pointer_string1(const char* lhs, gl_string&& rhs, const gl_string& x)
  {
    TS_ASSERT(lhs + std::move(rhs) == x);
  }

  void test_string_op_plus__pointer_string0() {
    
    _test_string_op_plus__pointer_string0("", gl_string(""), gl_string(""));
    _test_string_op_plus__pointer_string0("", gl_string("12345"), gl_string("12345"));
    _test_string_op_plus__pointer_string0("", gl_string("1234567890"), gl_string("1234567890"));
    _test_string_op_plus__pointer_string0("", gl_string("12345678901234567890"), gl_string("12345678901234567890"));
    _test_string_op_plus__pointer_string0("abcde", gl_string(""), gl_string("abcde"));
    _test_string_op_plus__pointer_string0("abcde", gl_string("12345"), gl_string("abcde12345"));
    _test_string_op_plus__pointer_string0("abcde", gl_string("1234567890"), gl_string("abcde1234567890"));
    _test_string_op_plus__pointer_string0("abcde", gl_string("12345678901234567890"), gl_string("abcde12345678901234567890"));
    _test_string_op_plus__pointer_string0("abcdefghij", gl_string(""), gl_string("abcdefghij"));
    _test_string_op_plus__pointer_string0("abcdefghij", gl_string("12345"), gl_string("abcdefghij12345"));
    _test_string_op_plus__pointer_string0("abcdefghij", gl_string("1234567890"), gl_string("abcdefghij1234567890"));
    _test_string_op_plus__pointer_string0("abcdefghij", gl_string("12345678901234567890"), gl_string("abcdefghij12345678901234567890"));
    _test_string_op_plus__pointer_string0("abcdefghijklmnopqrst", gl_string(""), gl_string("abcdefghijklmnopqrst"));
    _test_string_op_plus__pointer_string0("abcdefghijklmnopqrst", gl_string("12345"), gl_string("abcdefghijklmnopqrst12345"));
    _test_string_op_plus__pointer_string0("abcdefghijklmnopqrst", gl_string("1234567890"), gl_string("abcdefghijklmnopqrst1234567890"));
    _test_string_op_plus__pointer_string0("abcdefghijklmnopqrst", gl_string("12345678901234567890"), gl_string("abcdefghijklmnopqrst12345678901234567890"));
  }

  void test_string_op_plus__pointer_string1() {

    _test_string_op_plus__pointer_string1("", gl_string(""), gl_string(""));
    _test_string_op_plus__pointer_string1("", gl_string("12345"), gl_string("12345"));
    _test_string_op_plus__pointer_string1("", gl_string("1234567890"), gl_string("1234567890"));
    _test_string_op_plus__pointer_string1("", gl_string("12345678901234567890"), gl_string("12345678901234567890"));
    _test_string_op_plus__pointer_string1("abcde", gl_string(""), gl_string("abcde"));
    _test_string_op_plus__pointer_string1("abcde", gl_string("12345"), gl_string("abcde12345"));
    _test_string_op_plus__pointer_string1("abcde", gl_string("1234567890"), gl_string("abcde1234567890"));
    _test_string_op_plus__pointer_string1("abcde", gl_string("12345678901234567890"), gl_string("abcde12345678901234567890"));
    _test_string_op_plus__pointer_string1("abcdefghij", gl_string(""), gl_string("abcdefghij"));
    _test_string_op_plus__pointer_string1("abcdefghij", gl_string("12345"), gl_string("abcdefghij12345"));
    _test_string_op_plus__pointer_string1("abcdefghij", gl_string("1234567890"), gl_string("abcdefghij1234567890"));
    _test_string_op_plus__pointer_string1("abcdefghij", gl_string("12345678901234567890"), gl_string("abcdefghij12345678901234567890"));
    _test_string_op_plus__pointer_string1("abcdefghijklmnopqrst", gl_string(""), gl_string("abcdefghijklmnopqrst"));
    _test_string_op_plus__pointer_string1("abcdefghijklmnopqrst", gl_string("12345"), gl_string("abcdefghijklmnopqrst12345"));
    _test_string_op_plus__pointer_string1("abcdefghijklmnopqrst", gl_string("1234567890"), gl_string("abcdefghijklmnopqrst1234567890"));
    _test_string_op_plus__pointer_string1("abcdefghijklmnopqrst", gl_string("12345678901234567890"), gl_string("abcdefghijklmnopqrst12345678901234567890"));
  }
    
  void _test_string_op_plus__string_char0(const gl_string& lhs, char rhs, const gl_string& x) {
    TS_ASSERT(lhs + rhs == x);
  }

  void test_string_op_plus__string_char() {
    _test_string_op_plus__string_char0(gl_string(""), '1', gl_string("1"));
    _test_string_op_plus__string_char0(gl_string("abcde"), '1', gl_string("abcde1"));
    _test_string_op_plus__string_char0(gl_string("abcdefghij"), '1', gl_string("abcdefghij1"));
    _test_string_op_plus__string_char0(gl_string("abcdefghijklmnopqrst"), '1', gl_string("abcdefghijklmnopqrst1"));
  }
  void _test_string_op_plus__string_pointer0(const gl_string& lhs, const char* rhs, const gl_string& x)
  {
    TS_ASSERT(lhs + rhs == x);
  }

  void _test_string_op_plus__string_pointer1(gl_string&& lhs, const char* rhs, const gl_string& x)
  {
    TS_ASSERT(std::move(lhs) + rhs == x);
  }

  void test_string_op_plus__string_pointer0() {
    _test_string_op_plus__string_pointer0(gl_string(""), "", gl_string(""));
    _test_string_op_plus__string_pointer0(gl_string(""), "12345", gl_string("12345"));
    _test_string_op_plus__string_pointer0(gl_string(""), "1234567890", gl_string("1234567890"));
    _test_string_op_plus__string_pointer0(gl_string(""), "12345678901234567890", gl_string("12345678901234567890"));
    _test_string_op_plus__string_pointer0(gl_string("abcde"), "", gl_string("abcde"));
    _test_string_op_plus__string_pointer0(gl_string("abcde"), "12345", gl_string("abcde12345"));
    _test_string_op_plus__string_pointer0(gl_string("abcde"), "1234567890", gl_string("abcde1234567890"));
    _test_string_op_plus__string_pointer0(gl_string("abcde"), "12345678901234567890", gl_string("abcde12345678901234567890"));
    _test_string_op_plus__string_pointer0(gl_string("abcdefghij"), "", gl_string("abcdefghij"));
    _test_string_op_plus__string_pointer0(gl_string("abcdefghij"), "12345", gl_string("abcdefghij12345"));
    _test_string_op_plus__string_pointer0(gl_string("abcdefghij"), "1234567890", gl_string("abcdefghij1234567890"));
    _test_string_op_plus__string_pointer0(gl_string("abcdefghij"), "12345678901234567890", gl_string("abcdefghij12345678901234567890"));
    _test_string_op_plus__string_pointer0(gl_string("abcdefghijklmnopqrst"), "", gl_string("abcdefghijklmnopqrst"));
    _test_string_op_plus__string_pointer0(gl_string("abcdefghijklmnopqrst"), "12345", gl_string("abcdefghijklmnopqrst12345"));
    _test_string_op_plus__string_pointer0(gl_string("abcdefghijklmnopqrst"), "1234567890", gl_string("abcdefghijklmnopqrst1234567890"));
    _test_string_op_plus__string_pointer0(gl_string("abcdefghijklmnopqrst"), "12345678901234567890", gl_string("abcdefghijklmnopqrst12345678901234567890"));
  }

  void test_string_op_plus__string_pointer1() {
    _test_string_op_plus__string_pointer1(gl_string(""), "", gl_string(""));
    _test_string_op_plus__string_pointer1(gl_string(""), "12345", gl_string("12345"));
    _test_string_op_plus__string_pointer1(gl_string(""), "1234567890", gl_string("1234567890"));
    _test_string_op_plus__string_pointer1(gl_string(""), "12345678901234567890", gl_string("12345678901234567890"));
    _test_string_op_plus__string_pointer1(gl_string("abcde"), "", gl_string("abcde"));
    _test_string_op_plus__string_pointer1(gl_string("abcde"), "12345", gl_string("abcde12345"));
    _test_string_op_plus__string_pointer1(gl_string("abcde"), "1234567890", gl_string("abcde1234567890"));
    _test_string_op_plus__string_pointer1(gl_string("abcde"), "12345678901234567890", gl_string("abcde12345678901234567890"));
    _test_string_op_plus__string_pointer1(gl_string("abcdefghij"), "", gl_string("abcdefghij"));
    _test_string_op_plus__string_pointer1(gl_string("abcdefghij"), "12345", gl_string("abcdefghij12345"));
    _test_string_op_plus__string_pointer1(gl_string("abcdefghij"), "1234567890", gl_string("abcdefghij1234567890"));
    _test_string_op_plus__string_pointer1(gl_string("abcdefghij"), "12345678901234567890", gl_string("abcdefghij12345678901234567890"));
    _test_string_op_plus__string_pointer1(gl_string("abcdefghijklmnopqrst"), "", gl_string("abcdefghijklmnopqrst"));
    _test_string_op_plus__string_pointer1(gl_string("abcdefghijklmnopqrst"), "12345", gl_string("abcdefghijklmnopqrst12345"));
    _test_string_op_plus__string_pointer1(gl_string("abcdefghijklmnopqrst"), "1234567890", gl_string("abcdefghijklmnopqrst1234567890"));
    _test_string_op_plus__string_pointer1(gl_string("abcdefghijklmnopqrst"), "12345678901234567890", gl_string("abcdefghijklmnopqrst12345678901234567890"));
  }

  void _test_string_op_plus__string_string0(const gl_string& lhs, const gl_string& rhs, const gl_string& x)
  {
    TS_ASSERT(lhs + rhs == x);
  }

  void _test_string_op_plus__string_string1(gl_string&& lhs, const gl_string& rhs, const gl_string& x)
  {
    TS_ASSERT(std::move(lhs) + rhs == x);
  }


  void _test_string_op_plus__string_string2(const gl_string& lhs, gl_string&& rhs, const gl_string& x) {
    TS_ASSERT(lhs + std::move(rhs) == x);
  }


  void _test_string_op_plus__string_string3(gl_string&& lhs, gl_string&& rhs, const gl_string& x) {
    TS_ASSERT(std::move(lhs) + std::move(rhs) == x);
  }

  void _test_string_op_plus__string_string4(const gl_string& lhs, const std::string& rhs, const gl_string& x)
  {
    TS_ASSERT(lhs + rhs == x);
  }

  void _test_string_op_plus__string_string5(const std::string& lhs, const gl_string& rhs, const gl_string& x)
  {
    TS_ASSERT(lhs + rhs == x);
  }
  
  void test_string_op_plus__string_string0() {
    _test_string_op_plus__string_string0(gl_string(""), gl_string(""), gl_string(""));
    _test_string_op_plus__string_string0(gl_string(""), gl_string("12345"), gl_string("12345"));
    _test_string_op_plus__string_string0(gl_string(""), gl_string("1234567890"), gl_string("1234567890"));
    _test_string_op_plus__string_string0(gl_string(""), gl_string("12345678901234567890"), gl_string("12345678901234567890"));
    _test_string_op_plus__string_string0(gl_string("abcde"), gl_string(""), gl_string("abcde"));
    _test_string_op_plus__string_string0(gl_string("abcde"), gl_string("12345"), gl_string("abcde12345"));
    _test_string_op_plus__string_string0(gl_string("abcde"), gl_string("1234567890"), gl_string("abcde1234567890"));
    _test_string_op_plus__string_string0(gl_string("abcde"), gl_string("12345678901234567890"), gl_string("abcde12345678901234567890"));
    _test_string_op_plus__string_string0(gl_string("abcdefghij"), gl_string(""), gl_string("abcdefghij"));
    _test_string_op_plus__string_string0(gl_string("abcdefghij"), gl_string("12345"), gl_string("abcdefghij12345"));
    _test_string_op_plus__string_string0(gl_string("abcdefghij"), gl_string("1234567890"), gl_string("abcdefghij1234567890"));
    _test_string_op_plus__string_string0(gl_string("abcdefghij"), gl_string("12345678901234567890"), gl_string("abcdefghij12345678901234567890"));
    _test_string_op_plus__string_string0(gl_string("abcdefghijklmnopqrst"), gl_string(""), gl_string("abcdefghijklmnopqrst"));
    _test_string_op_plus__string_string0(gl_string("abcdefghijklmnopqrst"), gl_string("12345"), gl_string("abcdefghijklmnopqrst12345"));
    _test_string_op_plus__string_string0(gl_string("abcdefghijklmnopqrst"), gl_string("1234567890"), gl_string("abcdefghijklmnopqrst1234567890"));
    _test_string_op_plus__string_string0(gl_string("abcdefghijklmnopqrst"), gl_string("12345678901234567890"), gl_string("abcdefghijklmnopqrst12345678901234567890"));
  }
  void test_string_op_plus__string_string1() {
    _test_string_op_plus__string_string1(gl_string(""), gl_string(""), gl_string(""));
    _test_string_op_plus__string_string1(gl_string(""), gl_string("12345"), gl_string("12345"));
    _test_string_op_plus__string_string1(gl_string(""), gl_string("1234567890"), gl_string("1234567890"));
    _test_string_op_plus__string_string1(gl_string(""), gl_string("12345678901234567890"), gl_string("12345678901234567890"));
    _test_string_op_plus__string_string1(gl_string("abcde"), gl_string(""), gl_string("abcde"));
    _test_string_op_plus__string_string1(gl_string("abcde"), gl_string("12345"), gl_string("abcde12345"));
    _test_string_op_plus__string_string1(gl_string("abcde"), gl_string("1234567890"), gl_string("abcde1234567890"));
    _test_string_op_plus__string_string1(gl_string("abcde"), gl_string("12345678901234567890"), gl_string("abcde12345678901234567890"));
    _test_string_op_plus__string_string1(gl_string("abcdefghij"), gl_string(""), gl_string("abcdefghij"));
    _test_string_op_plus__string_string1(gl_string("abcdefghij"), gl_string("12345"), gl_string("abcdefghij12345"));
    _test_string_op_plus__string_string1(gl_string("abcdefghij"), gl_string("1234567890"), gl_string("abcdefghij1234567890"));
    _test_string_op_plus__string_string1(gl_string("abcdefghij"), gl_string("12345678901234567890"), gl_string("abcdefghij12345678901234567890"));
    _test_string_op_plus__string_string1(gl_string("abcdefghijklmnopqrst"), gl_string(""), gl_string("abcdefghijklmnopqrst"));
    _test_string_op_plus__string_string1(gl_string("abcdefghijklmnopqrst"), gl_string("12345"), gl_string("abcdefghijklmnopqrst12345"));
    _test_string_op_plus__string_string1(gl_string("abcdefghijklmnopqrst"), gl_string("1234567890"), gl_string("abcdefghijklmnopqrst1234567890"));
    _test_string_op_plus__string_string1(gl_string("abcdefghijklmnopqrst"), gl_string("12345678901234567890"), gl_string("abcdefghijklmnopqrst12345678901234567890"));
  }

  void test_string_op_plus__string_string2() {
    _test_string_op_plus__string_string2(gl_string(""), gl_string(""), gl_string(""));
    _test_string_op_plus__string_string2(gl_string(""), gl_string("12345"), gl_string("12345"));
    _test_string_op_plus__string_string2(gl_string(""), gl_string("1234567890"), gl_string("1234567890"));
    _test_string_op_plus__string_string2(gl_string(""), gl_string("12345678901234567890"), gl_string("12345678901234567890"));
    _test_string_op_plus__string_string2(gl_string("abcde"), gl_string(""), gl_string("abcde"));
    _test_string_op_plus__string_string2(gl_string("abcde"), gl_string("12345"), gl_string("abcde12345"));
    _test_string_op_plus__string_string2(gl_string("abcde"), gl_string("1234567890"), gl_string("abcde1234567890"));
    _test_string_op_plus__string_string2(gl_string("abcde"), gl_string("12345678901234567890"), gl_string("abcde12345678901234567890"));
    _test_string_op_plus__string_string2(gl_string("abcdefghij"), gl_string(""), gl_string("abcdefghij"));
    _test_string_op_plus__string_string2(gl_string("abcdefghij"), gl_string("12345"), gl_string("abcdefghij12345"));
    _test_string_op_plus__string_string2(gl_string("abcdefghij"), gl_string("1234567890"), gl_string("abcdefghij1234567890"));
    _test_string_op_plus__string_string2(gl_string("abcdefghij"), gl_string("12345678901234567890"), gl_string("abcdefghij12345678901234567890"));
    _test_string_op_plus__string_string2(gl_string("abcdefghijklmnopqrst"), gl_string(""), gl_string("abcdefghijklmnopqrst"));
    _test_string_op_plus__string_string2(gl_string("abcdefghijklmnopqrst"), gl_string("12345"), gl_string("abcdefghijklmnopqrst12345"));
    _test_string_op_plus__string_string2(gl_string("abcdefghijklmnopqrst"), gl_string("1234567890"), gl_string("abcdefghijklmnopqrst1234567890"));
    _test_string_op_plus__string_string2(gl_string("abcdefghijklmnopqrst"), gl_string("12345678901234567890"), gl_string("abcdefghijklmnopqrst12345678901234567890"));
  }

  void test_string_op_plus__string_string3() {
    _test_string_op_plus__string_string3(gl_string(""), gl_string(""), gl_string(""));
    _test_string_op_plus__string_string3(gl_string(""), gl_string("12345"), gl_string("12345"));
    _test_string_op_plus__string_string3(gl_string(""), gl_string("1234567890"), gl_string("1234567890"));
    _test_string_op_plus__string_string3(gl_string(""), gl_string("12345678901234567890"), gl_string("12345678901234567890"));
    _test_string_op_plus__string_string3(gl_string("abcde"), gl_string(""), gl_string("abcde"));
    _test_string_op_plus__string_string3(gl_string("abcde"), gl_string("12345"), gl_string("abcde12345"));
    _test_string_op_plus__string_string3(gl_string("abcde"), gl_string("1234567890"), gl_string("abcde1234567890"));
    _test_string_op_plus__string_string3(gl_string("abcde"), gl_string("12345678901234567890"), gl_string("abcde12345678901234567890"));
    _test_string_op_plus__string_string3(gl_string("abcdefghij"), gl_string(""), gl_string("abcdefghij"));
    _test_string_op_plus__string_string3(gl_string("abcdefghij"), gl_string("12345"), gl_string("abcdefghij12345"));
    _test_string_op_plus__string_string3(gl_string("abcdefghij"), gl_string("1234567890"), gl_string("abcdefghij1234567890"));
    _test_string_op_plus__string_string3(gl_string("abcdefghij"), gl_string("12345678901234567890"), gl_string("abcdefghij12345678901234567890"));
    _test_string_op_plus__string_string3(gl_string("abcdefghijklmnopqrst"), gl_string(""), gl_string("abcdefghijklmnopqrst"));
    _test_string_op_plus__string_string3(gl_string("abcdefghijklmnopqrst"), gl_string("12345"), gl_string("abcdefghijklmnopqrst12345"));
    _test_string_op_plus__string_string3(gl_string("abcdefghijklmnopqrst"), gl_string("1234567890"), gl_string("abcdefghijklmnopqrst1234567890"));
    _test_string_op_plus__string_string3(gl_string("abcdefghijklmnopqrst"), gl_string("12345678901234567890"), gl_string("abcdefghijklmnopqrst12345678901234567890"));
  }

  void test_string_op_plus__string_string4() {
    _test_string_op_plus__string_string4(gl_string(""), gl_string(""), gl_string(""));
    _test_string_op_plus__string_string4(gl_string(""), gl_string("12345"), gl_string("12345"));
    _test_string_op_plus__string_string4(gl_string(""), gl_string("1234567890"), gl_string("1234567890"));
    _test_string_op_plus__string_string4(gl_string(""), gl_string("12345678901234567890"), gl_string("12345678901234567890"));
    _test_string_op_plus__string_string4(gl_string("abcde"), gl_string(""), gl_string("abcde"));
    _test_string_op_plus__string_string4(gl_string("abcde"), gl_string("12345"), gl_string("abcde12345"));
    _test_string_op_plus__string_string4(gl_string("abcde"), gl_string("1234567890"), gl_string("abcde1234567890"));
    _test_string_op_plus__string_string4(gl_string("abcde"), gl_string("12345678901234567890"), gl_string("abcde12345678901234567890"));
    _test_string_op_plus__string_string4(gl_string("abcdefghij"), std::string(""), gl_string("abcdefghij"));
    _test_string_op_plus__string_string4(gl_string("abcdefghij"), std::string("12345"), gl_string("abcdefghij12345"));
    _test_string_op_plus__string_string4(gl_string("abcdefghij"), std::string("1234567890"), gl_string("abcdefghij1234567890"));
    _test_string_op_plus__string_string4(gl_string("abcdefghij"), std::string("12345678901234567890"), gl_string("abcdefghij12345678901234567890"));
    _test_string_op_plus__string_string4(gl_string("abcdefghijklmnopqrst"), std::string(""), gl_string("abcdefghijklmnopqrst"));
    _test_string_op_plus__string_string4(gl_string("abcdefghijklmnopqrst"), std::string("12345"), gl_string("abcdefghijklmnopqrst12345"));
    _test_string_op_plus__string_string4(gl_string("abcdefghijklmnopqrst"), std::string("1234567890"), gl_string("abcdefghijklmnopqrst1234567890"));
    _test_string_op_plus__string_string4(gl_string("abcdefghijklmnopqrst"), std::string("12345678901234567890"), gl_string("abcdefghijklmnopqrst12345678901234567890"));
  }

  void test_string_op_plus__string_string5() {
    _test_string_op_plus__string_string5(std::string(""), gl_string(""), gl_string(""));
    _test_string_op_plus__string_string5(std::string(""), gl_string("12345"), gl_string("12345"));
    _test_string_op_plus__string_string5(std::string(""), gl_string("1234567890"), gl_string("1234567890"));
    _test_string_op_plus__string_string5(std::string(""), gl_string("12345678901234567890"), gl_string("12345678901234567890"));
    _test_string_op_plus__string_string5(std::string("abcde"), gl_string(""), gl_string("abcde"));
    _test_string_op_plus__string_string5(std::string("abcde"), gl_string("12345"), gl_string("abcde12345"));
    _test_string_op_plus__string_string5(std::string("abcde"), gl_string("1234567890"), gl_string("abcde1234567890"));
    _test_string_op_plus__string_string5(std::string("abcde"), gl_string("12345678901234567890"), gl_string("abcde12345678901234567890"));
    _test_string_op_plus__string_string5(std::string("abcdefghij"), gl_string(""), gl_string("abcdefghij"));
    _test_string_op_plus__string_string5(std::string("abcdefghij"), gl_string("12345"), gl_string("abcdefghij12345"));
    _test_string_op_plus__string_string5(std::string("abcdefghij"), gl_string("1234567890"), gl_string("abcdefghij1234567890"));
    _test_string_op_plus__string_string5(std::string("abcdefghij"), gl_string("12345678901234567890"), gl_string("abcdefghij12345678901234567890"));
    _test_string_op_plus__string_string5(std::string("abcdefghijklmnopqrst"), gl_string(""), gl_string("abcdefghijklmnopqrst"));
    _test_string_op_plus__string_string5(std::string("abcdefghijklmnopqrst"), gl_string("12345"), gl_string("abcdefghijklmnopqrst12345"));
    _test_string_op_plus__string_string5(std::string("abcdefghijklmnopqrst"), gl_string("1234567890"), gl_string("abcdefghijklmnopqrst1234567890"));
    _test_string_op_plus__string_string5(std::string("abcdefghijklmnopqrst"), gl_string("12345678901234567890"), gl_string("abcdefghijklmnopqrst12345678901234567890"));
  }
  
  void _test_string_special_swap(gl_string s1, gl_string s2) {
    gl_string s1_ = s1;
    gl_string s2_ = s2;
    std::swap(s1, s2);
    TS_ASSERT(s1 == s2_);
    TS_ASSERT(s2 == s1_);
  }

  void test_string_special_swap()
  {
    _test_string_special_swap(gl_string(""), gl_string(""));
    _test_string_special_swap(gl_string(""), gl_string("12345"));
    _test_string_special_swap(gl_string(""), gl_string("1234567890"));
    _test_string_special_swap(gl_string(""), gl_string("12345678901234567890"));
    _test_string_special_swap(gl_string("abcde"), gl_string(""));
    _test_string_special_swap(gl_string("abcde"), gl_string("12345"));
    _test_string_special_swap(gl_string("abcde"), gl_string("1234567890"));
    _test_string_special_swap(gl_string("abcde"), gl_string("12345678901234567890"));
    _test_string_special_swap(gl_string("abcdefghij"), gl_string(""));
    _test_string_special_swap(gl_string("abcdefghij"), gl_string("12345"));
    _test_string_special_swap(gl_string("abcdefghij"), gl_string("1234567890"));
    _test_string_special_swap(gl_string("abcdefghij"), gl_string("12345678901234567890"));
    _test_string_special_swap(gl_string("abcdefghijklmnopqrst"), gl_string(""));
    _test_string_special_swap(gl_string("abcdefghijklmnopqrst"), gl_string("12345"));
    _test_string_special_swap(gl_string("abcdefghijklmnopqrst"), gl_string("1234567890"));
    _test_string_special_swap(gl_string("abcdefghijklmnopqrst"), gl_string("12345678901234567890"));
  }
};

BOOST_FIXTURE_TEST_SUITE(_test_string_other_operators, test_string_other_operators)
BOOST_AUTO_TEST_CASE(test_string_io_get_line_delim) {
  test_string_other_operators::test_string_io_get_line_delim();
}
BOOST_AUTO_TEST_CASE(test_string_io_get_line_delim_rv) {
  test_string_other_operators::test_string_io_get_line_delim_rv();
}
BOOST_AUTO_TEST_CASE(test_string_io_get_line) {
  test_string_other_operators::test_string_io_get_line();
}
BOOST_AUTO_TEST_CASE(test_string_io_get_line_rv) {
  test_string_other_operators::test_string_io_get_line_rv();
}
BOOST_AUTO_TEST_CASE(test_string_io_stream_extract) {
  test_string_other_operators::test_string_io_stream_extract();
}
BOOST_AUTO_TEST_CASE(test_string_io_stream_insert) {
  test_string_other_operators::test_string_io_stream_insert();
}
BOOST_AUTO_TEST_CASE(test_string_operator_eq__pointer_string) {
  test_string_other_operators::test_string_operator_eq__pointer_string();
}
BOOST_AUTO_TEST_CASE(test_string_operator_eq__string_string) {
  test_string_other_operators::test_string_operator_eq__string_string();
}
BOOST_AUTO_TEST_CASE(test_string_operator_eq__string_stdstring) {
  test_string_other_operators::test_string_operator_eq__string_stdstring();
}
BOOST_AUTO_TEST_CASE(test_string_operator_eq__stdstring_string) {
  test_string_other_operators::test_string_operator_eq__stdstring_string();
}
BOOST_AUTO_TEST_CASE(test_string_opgteq_pointer_string) {
  test_string_other_operators::test_string_opgteq_pointer_string();
}
BOOST_AUTO_TEST_CASE(test_string_opgteq_string_pointer) {
  test_string_other_operators::test_string_opgteq_string_pointer();
}
BOOST_AUTO_TEST_CASE(test_string_opgteq_string_string0) {
  test_string_other_operators::test_string_opgteq_string_string0();
}
BOOST_AUTO_TEST_CASE(test_string_opgteq_string_string1) {
  test_string_other_operators::test_string_opgteq_string_string1();
}
BOOST_AUTO_TEST_CASE(test_string_opgteq_string_string2) {
  test_string_other_operators::test_string_opgteq_string_string2();
}
BOOST_AUTO_TEST_CASE(test_string_opgt_pointer_string) {
  test_string_other_operators::test_string_opgt_pointer_string();
}
BOOST_AUTO_TEST_CASE(test_string_opgt_string_pointer) {
  test_string_other_operators::test_string_opgt_string_pointer();
}
BOOST_AUTO_TEST_CASE(test_string_opgt_string_string0) {
  test_string_other_operators::test_string_opgt_string_string0();
}
BOOST_AUTO_TEST_CASE(test_string_opgt_string_string1) {
  test_string_other_operators::test_string_opgt_string_string1();
}
BOOST_AUTO_TEST_CASE(test_string_opgt_string_string2) {
  test_string_other_operators::test_string_opgt_string_string2();
}
BOOST_AUTO_TEST_CASE(test_string_oplteq_pointer_string) {
  test_string_other_operators::test_string_oplteq_pointer_string();
}
BOOST_AUTO_TEST_CASE(test_string_oplteq_string_pointer) {
  test_string_other_operators::test_string_oplteq_string_pointer();
}
BOOST_AUTO_TEST_CASE(test_string_oplteq_string_string0) {
  test_string_other_operators::test_string_oplteq_string_string0();
}
BOOST_AUTO_TEST_CASE(test_string_oplteq_string_string1) {
  test_string_other_operators::test_string_oplteq_string_string1();
}
BOOST_AUTO_TEST_CASE(test_string_oplteq_string_string2) {
  test_string_other_operators::test_string_oplteq_string_string2();
}
BOOST_AUTO_TEST_CASE(test_string_oplt_pointer_string) {
  test_string_other_operators::test_string_oplt_pointer_string();
}
BOOST_AUTO_TEST_CASE(test_string_oplt_string_pointer) {
  test_string_other_operators::test_string_oplt_string_pointer();
}
BOOST_AUTO_TEST_CASE(test_string_oplt_string_string0) {
  test_string_other_operators::test_string_oplt_string_string0();
}
BOOST_AUTO_TEST_CASE(test_string_oplt_string_string1) {
  test_string_other_operators::test_string_oplt_string_string1();
}
BOOST_AUTO_TEST_CASE(test_string_oplt_string_string2) {
  test_string_other_operators::test_string_oplt_string_string2();
}
BOOST_AUTO_TEST_CASE(test_string_op_not_eq_pointer_string) {
  test_string_other_operators::test_string_op_not_eq_pointer_string();
}
BOOST_AUTO_TEST_CASE(test_string_op_not_eq_string_pointer) {
  test_string_other_operators::test_string_op_not_eq_string_pointer();
}
BOOST_AUTO_TEST_CASE(test_string_op_not_eq_string_string0) {
  test_string_other_operators::test_string_op_not_eq_string_string0();
}
BOOST_AUTO_TEST_CASE(test_string_op_not_eq_string_string1) {
  test_string_other_operators::test_string_op_not_eq_string_string1();
}
BOOST_AUTO_TEST_CASE(test_string_op_not_eq_string_string2) {
  test_string_other_operators::test_string_op_not_eq_string_string2();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__char_string) {
  test_string_other_operators::test_string_op_plus__char_string();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__pointer_string0) {
  test_string_other_operators::test_string_op_plus__pointer_string0();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__pointer_string1) {
  test_string_other_operators::test_string_op_plus__pointer_string1();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__string_char) {
  test_string_other_operators::test_string_op_plus__string_char();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__string_pointer0) {
  test_string_other_operators::test_string_op_plus__string_pointer0();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__string_pointer1) {
  test_string_other_operators::test_string_op_plus__string_pointer1();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__string_string0) {
  test_string_other_operators::test_string_op_plus__string_string0();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__string_string1) {
  test_string_other_operators::test_string_op_plus__string_string1();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__string_string2) {
  test_string_other_operators::test_string_op_plus__string_string2();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__string_string3) {
  test_string_other_operators::test_string_op_plus__string_string3();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__string_string4) {
  test_string_other_operators::test_string_op_plus__string_string4();
}
BOOST_AUTO_TEST_CASE(test_string_op_plus__string_string5) {
  test_string_other_operators::test_string_op_plus__string_string5();
}
BOOST_AUTO_TEST_CASE(test_string_special_swap) {
  test_string_other_operators::test_string_special_swap();
}
BOOST_AUTO_TEST_SUITE_END()
