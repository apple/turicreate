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

struct test_string_constructors  {
 public:

  void test_alloc() {
    gl_string s;
    
    TS_ASSERT(s.size() == 0);
    TS_ASSERT(s.capacity() >= s.size());
  }

  void _test_char_assignment(gl_string s1, char s2) {
    s1 = s2;
    TS_ASSERT(s1.size() == 1);
    TS_ASSERT(s1[0] == s2);
    TS_ASSERT(s1.capacity() >= s1.size());
  }

  void test_char_assignment() {
    _test_char_assignment(gl_string(), 'a');
    _test_char_assignment(gl_string("1"), 'a');
    _test_char_assignment(gl_string("123456789"), 'a');
    _test_char_assignment(gl_string("1234567890123456789012345678901234567890123456789012345678901234567890"), 'a');
  }

  void test_copy(gl_string s1) {
    gl_string s2(s1);
    TS_ASSERT(s2 == s1);
    TS_ASSERT(s2.capacity() >= s2.size());
  }
  
  void _test_copy_assignment(gl_string s1, const gl_string& s2) {
    s1 = s2;
    TS_ASSERT(s1 == s2);
    TS_ASSERT(s1.capacity() >= s1.size());
  }

  void test_copy_assignment() {
    _test_copy_assignment(gl_string(), gl_string());
    _test_copy_assignment(gl_string("1"), gl_string());
    _test_copy_assignment(gl_string(), gl_string("1"));
    _test_copy_assignment(gl_string("1"), gl_string("2"));
    _test_copy_assignment(gl_string("1"), gl_string("2"));

    _test_copy_assignment(gl_string(),
                          gl_string("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
    _test_copy_assignment(gl_string("123456789"),
                          gl_string("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
    _test_copy_assignment(gl_string("1234567890123456789012345678901234567890123456789012345678901234567890"),
                          gl_string("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
    _test_copy_assignment(gl_string("1234567890123456789012345678901234567890123456789012345678901234567890"
                                    "1234567890123456789012345678901234567890123456789012345678901234567890"),
                          gl_string("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
  }
  void test_initializer_list_assignment() {
    gl_string s;
    s = {'a', 'b', 'c'};
    TS_ASSERT(s == "abc");
  }

  
  void _test_move_assignment(gl_string s1, gl_string s2) {
    gl_string s0 = s2;
    s1 = std::move(s2);
    TS_ASSERT(s1 == s0);
    TS_ASSERT(s1.capacity() >= s1.size());
  }

  void test_move_assignment() {
    _test_move_assignment(gl_string(), gl_string());
    _test_move_assignment(gl_string("1"), gl_string());
    _test_move_assignment(gl_string(), gl_string("1"));
    _test_move_assignment(gl_string("1"), gl_string("2"));
    _test_move_assignment(gl_string("1"), gl_string("2"));

    _test_move_assignment(gl_string(),
                          gl_string("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
    _test_move_assignment(gl_string("123456789"),
                          gl_string("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
    _test_move_assignment(gl_string("1234567890123456789012345678901234567890123456789012345678901234567890"),
                          gl_string("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
    _test_move_assignment(gl_string("1234567890123456789012345678901234567890123456789012345678901234567890"
                                    "1234567890123456789012345678901234567890123456789012345678901234567890"),
                          gl_string("abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz"));
  }
  
  void _test_pointer_assignment(gl_string s1, const char* s2) {
    s1 = s2;
    TS_ASSERT(s1.size() == std::strlen(s2));
    TS_ASSERT(std::strncmp(s1.data(), s2, s1.size()) == 0);
    TS_ASSERT(s1.capacity() >= s1.size());
  }

  void test_pointer_assignment() {
    _test_pointer_assignment(gl_string(), "");
    _test_pointer_assignment(gl_string("1"), "");
    _test_pointer_assignment(gl_string(), "1");
    _test_pointer_assignment(gl_string("1"), "2");
    _test_pointer_assignment(gl_string("1"), "2");

    _test_pointer_assignment(gl_string(),
                             "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
    _test_pointer_assignment(gl_string("123456789"),
                             "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
    _test_pointer_assignment(gl_string("1234567890123456789012345678901234567890123456789012345678901234567890"),
                             "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
    _test_pointer_assignment(gl_string("1234567890123456789012345678901234567890123456789012345678901234567890"
                                       "1234567890123456789012345678901234567890123456789012345678901234567890"),
                             "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
  }

  void _test_string_substr_substr(const gl_string& s, size_t pos, size_t n) {
#ifndef NDEBUG    
    try {
      gl_string str = s.substr(pos, n);
      TS_ASSERT(pos <= s.size());
      size_t rlen = std::min(n, s.size() - pos);
      TS_ASSERT(str.size() == rlen);
      TS_ASSERT(std::strncmp(s.data()+pos, str.data(), rlen) == 0);
    } catch (...) {
      TS_ASSERT(pos > s.size());
    }
#endif    
  }

  void test_substr() {
    _test_string_substr_substr(gl_string(""), 0, 0);
    _test_string_substr_substr(gl_string(""), 1, 0);
    _test_string_substr_substr(gl_string("pniot"), 0, 0);
    _test_string_substr_substr(gl_string("htaob"), 0, 1);
    _test_string_substr_substr(gl_string("fodgq"), 0, 2);
    _test_string_substr_substr(gl_string("hpqia"), 0, 4);
    _test_string_substr_substr(gl_string("qanej"), 0, 5);
    _test_string_substr_substr(gl_string("dfkap"), 1, 0);
    _test_string_substr_substr(gl_string("clbao"), 1, 1);
    _test_string_substr_substr(gl_string("ihqrf"), 1, 2);
    _test_string_substr_substr(gl_string("mekdn"), 1, 3);
    _test_string_substr_substr(gl_string("ngtjf"), 1, 4);
    _test_string_substr_substr(gl_string("srdfq"), 2, 0);
    _test_string_substr_substr(gl_string("qkdrs"), 2, 1);
    _test_string_substr_substr(gl_string("ikcrq"), 2, 2);
    _test_string_substr_substr(gl_string("cdaih"), 2, 3);
    _test_string_substr_substr(gl_string("dmajb"), 4, 0);
    _test_string_substr_substr(gl_string("karth"), 4, 1);
    _test_string_substr_substr(gl_string("lhcdo"), 5, 0);
    _test_string_substr_substr(gl_string("acbsj"), 6, 0);
    _test_string_substr_substr(gl_string("pbsjikaole"), 0, 0);
    _test_string_substr_substr(gl_string("pcbahntsje"), 0, 1);
    _test_string_substr_substr(gl_string("mprdjbeiak"), 0, 5);
    _test_string_substr_substr(gl_string("fhepcrntko"), 0, 9);
    _test_string_substr_substr(gl_string("eqmpaidtls"), 0, 10);
    _test_string_substr_substr(gl_string("joidhalcmq"), 1, 0);
    _test_string_substr_substr(gl_string("omigsphflj"), 1, 1);
    _test_string_substr_substr(gl_string("kocgbphfji"), 1, 4);
    _test_string_substr_substr(gl_string("onmjekafbi"), 1, 8);
    _test_string_substr_substr(gl_string("fbslrjiqkm"), 1, 9);
    _test_string_substr_substr(gl_string("oqmrjahnkg"), 5, 0);
    _test_string_substr_substr(gl_string("jeidpcmalh"), 5, 1);
    _test_string_substr_substr(gl_string("schfalibje"), 5, 2);
    _test_string_substr_substr(gl_string("crliponbqe"), 5, 4);
    _test_string_substr_substr(gl_string("igdscopqtm"), 5, 5);
    _test_string_substr_substr(gl_string("qngpdkimlc"), 9, 0);
    _test_string_substr_substr(gl_string("thdjgafrlb"), 9, 1);
    _test_string_substr_substr(gl_string("hcjitbfapl"), 10, 0);
    _test_string_substr_substr(gl_string("mgojkldsqh"), 11, 0);
    _test_string_substr_substr(gl_string("gfshlcmdjreqipbontak"), 0, 0);
    _test_string_substr_substr(gl_string("nadkhpfemgclosibtjrq"), 0, 1);
    _test_string_substr_substr(gl_string("nkodajteqplrbifhmcgs"), 0, 10);
    _test_string_substr_substr(gl_string("ofdrqmkeblthacpgijsn"), 0, 19);
    _test_string_substr_substr(gl_string("gbmetiprqdoasckjfhln"), 0, 20);
    _test_string_substr_substr(gl_string("bdfjqgatlksriohemnpc"), 1, 0);
    _test_string_substr_substr(gl_string("crnklpmegdqfiashtojb"), 1, 1);
    _test_string_substr_substr(gl_string("ejqcnahdrkfsmptilgbo"), 1, 9);
    _test_string_substr_substr(gl_string("jsbtafedocnirgpmkhql"), 1, 18);
    _test_string_substr_substr(gl_string("prqgnlbaejsmkhdctoif"), 1, 19);
    _test_string_substr_substr(gl_string("qnmodrtkebhpasifgcjl"), 10, 0);
    _test_string_substr_substr(gl_string("pejafmnokrqhtisbcdgl"), 10, 1);
    _test_string_substr_substr(gl_string("cpebqsfmnjdolhkratgi"), 10, 5);
    _test_string_substr_substr(gl_string("odnqkgijrhabfmcestlp"), 10, 9);
    _test_string_substr_substr(gl_string("lmofqdhpkibagnrcjste"), 10, 10);
    _test_string_substr_substr(gl_string("lgjqketopbfahrmnsicd"), 19, 0);
    _test_string_substr_substr(gl_string("ktsrmnqagdecfhijpobl"), 19, 1);
    _test_string_substr_substr(gl_string("lsaijeqhtrbgcdmpfkno"), 20, 0);
    _test_string_substr_substr(gl_string("dplqartnfgejichmoskb"), 21, 0);
  }
  
  
};

BOOST_FIXTURE_TEST_SUITE(_test_string_constructors, test_string_constructors)
BOOST_AUTO_TEST_CASE(test_alloc) {
  test_string_constructors::test_alloc();
}
BOOST_AUTO_TEST_CASE(test_char_assignment) {
  test_string_constructors::test_char_assignment();
}
BOOST_AUTO_TEST_CASE(test_copy_assignment) {
  test_string_constructors::test_copy_assignment();
}
BOOST_AUTO_TEST_CASE(test_initializer_list_assignment) {
  test_string_constructors::test_initializer_list_assignment();
}
BOOST_AUTO_TEST_CASE(test_move_assignment) {
  test_string_constructors::test_move_assignment();
}
BOOST_AUTO_TEST_CASE(test_pointer_assignment) {
  test_string_constructors::test_pointer_assignment();
}
BOOST_AUTO_TEST_CASE(test_substr) {
  test_string_constructors::test_substr();
}
BOOST_AUTO_TEST_SUITE_END()
