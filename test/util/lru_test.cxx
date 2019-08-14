/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <string>
#include <set>
#include <core/util/lru.hpp>

using namespace turi;


struct lru_test {

 public:

  void test_lru() {
    // basic cache test. whether LRU is performed on just insertions
    lru_cache<std::string, size_t> cache;
    cache.set_size_limit(3);
    cache.insert("a", 1);
    cache.insert("b", 1);
    cache.insert("c", 1);
    cache.insert("d", 1);
    TS_ASSERT_EQUALS(cache.query("a").first, false);
    TS_ASSERT_EQUALS(cache.query("b").first, true);
    TS_ASSERT_EQUALS(cache.query("c").first, true);
    TS_ASSERT_EQUALS(cache.query("d").first, true);
    cache.insert("e", 1);
    cache.insert("f", 1);
    TS_ASSERT_EQUALS(cache.query("b").first, false);
    TS_ASSERT_EQUALS(cache.query("c").first, false);
    TS_ASSERT_EQUALS(cache.query("d").first, true);
    TS_ASSERT_EQUALS(cache.query("e").first, true);
    TS_ASSERT_EQUALS(cache.query("f").first, true);
    TS_ASSERT_EQUALS(cache.size(), 3);

    std::set<std::string> s;
    for (const auto& i : cache) {
      s.insert(i.first);
    }
    TS_ASSERT_EQUALS(s.size(), 3);
    auto iter = s.begin();
    TS_ASSERT_EQUALS((*iter), "d"); ++iter;
    TS_ASSERT_EQUALS((*iter), "e"); ++iter;
    TS_ASSERT_EQUALS((*iter), "f"); 

    auto riter = s.rbegin();
    TS_ASSERT_EQUALS((*riter), "f"); ++riter;
    TS_ASSERT_EQUALS((*riter), "e"); ++riter;
    TS_ASSERT_EQUALS((*riter), "d"); 
  }

  void test_lru_query() {
    // mixed insertions and querying
    lru_cache<std::string, size_t> cache;
    cache.set_size_limit(3);
    cache.insert("a", 1);
    cache.insert("b", 1);
    cache.insert("c", 1);
    cache.insert("d", 1); // bcd in cache
    cache.query("b"); // b is now head so c will be evicted next
    cache.insert("e", 1); // should be bde
    cache.insert("f", 1); // should be bef
    TS_ASSERT_EQUALS(cache.query("d").first, false);
    TS_ASSERT_EQUALS(cache.query("b").first, true);
    TS_ASSERT_EQUALS(cache.query("e").first, true);
    TS_ASSERT_EQUALS(cache.query("f").first, true);
    TS_ASSERT_EQUALS(cache.size(), 3);
  }

  void test_repeated_inserts() {
    lru_cache<std::string, size_t> cache;
    cache.set_size_limit(3);
    cache.insert("a", 1);
    cache.insert("b", 1);
    cache.insert("c", 1);
    cache.insert("d", 1); // bcd in cache
    cache.insert("b", 2); // b is now head so c is tail
    cache.insert("c", 2); // d is tail
    cache.insert("b", 3); // d is stil tail
    cache.insert("e", 1); // d is popped. should be b:3, c:2, e:1
    TS_ASSERT_EQUALS(cache.query("d").first, false);
    TS_ASSERT_EQUALS(cache.query("b").first, true);
    TS_ASSERT_EQUALS(cache.query("b").second, 3);
    TS_ASSERT_EQUALS(cache.query("c").first, true);
    TS_ASSERT_EQUALS(cache.query("c").second, 2);
    TS_ASSERT_EQUALS(cache.query("e").first, true);
    TS_ASSERT_EQUALS(cache.query("e").second, 1);
    TS_ASSERT_EQUALS(cache.size(), 3);
    // deletion
    cache.erase("e");
    TS_ASSERT_EQUALS(cache.size(), 2);
    TS_ASSERT_EQUALS(cache.query("b").first, true);
    TS_ASSERT_EQUALS(cache.query("c").first, true);
    cache.erase("b");
    TS_ASSERT_EQUALS(cache.size(), 1);
    TS_ASSERT_EQUALS(cache.query("c").first, true);
  }
};


BOOST_FIXTURE_TEST_SUITE(_lru_test, lru_test)
BOOST_AUTO_TEST_CASE(test_lru) {
  lru_test::test_lru();
}
BOOST_AUTO_TEST_CASE(test_lru_query) {
  lru_test::test_lru_query();
}
BOOST_AUTO_TEST_CASE(test_repeated_inserts) {
  lru_test::test_repeated_inserts();
}
BOOST_AUTO_TEST_SUITE_END()
