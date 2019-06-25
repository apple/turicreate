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

int random_int() {
  return random::fast_uniform<int>(0, std::numeric_limits<int>::max());
}

void verify_serialization(const gl_string& v) {

  // into gl_string
  gl_string v1;
  save_and_load_object(v1, v);
  DASSERT_TRUE(v == v1);

  // Into std
  std::string v2;
  save_and_load_object(v2, v);
  DASSERT_EQ(v2.size(), v.size());
  DASSERT_TRUE(std::equal(v2.begin(), v2.end(), v.begin()));

  // from std into empty
  std::string v3;
  v3 = v;
  gl_string v4;
  save_and_load_object(v4, v3);
  DASSERT_TRUE(v4 == v);

  // from std into non-empty
  save_and_load_object(v1, v3);
  DASSERT_TRUE(v1 == v);
}

void verify_consistency(const gl_string& v) {

  // Constructor 1
  {
    gl_string v2(v);
    DASSERT_TRUE(v == v2);
  }

  // Constructor 2
  {
    gl_string v2(v.begin(), v.end());
    DASSERT_TRUE(v == v2);
  }

  // Assignment
  {
    gl_string v2;
    v2 = v;
    DASSERT_TRUE(v == v2);
  }

  // Assignment
  {
    gl_string v2;
    v2.assign(v.begin(), v.end());
    DASSERT_TRUE(v == v2);
  }

  // Assignment by insert
  {
    gl_string v2;
    v2.insert(v2.end(), v.begin(), v.end());
    DASSERT_TRUE(v == v2);
  }

  // Assignment by insert into cleared vector
  {
    gl_string v2;
    v2.resize(1);
    v2.clear();
    v2.insert(v2.end(), v.begin(), v.end());
    DASSERT_TRUE(v == v2);
  }

  // construction by empty, then resize, then index fill
  {
    gl_string v2;
    v2.resize(v.size());
    for(size_t i = 0; i < v.size(); ++i) {
      v2[i] = v[i];
    }
    DASSERT_TRUE(v == v2);
  }

  // construction by empty, then resize, then
  {
    gl_string v2;
    v2.reserve(v.size());
    for(auto e : v) {
      v2.push_back(e);
    }
    DASSERT_TRUE(v == v2);
  }

  // construction by empty, then resize, then iteration
  {
    gl_string v2;
    v2.resize(v.size());
    auto it = v2.begin();
    for(size_t i = 0; i < v.size(); ++i, ++it) {
      *it = v[i];
    }
    DASSERT_TRUE(v == v2);
  }

  // construction by empty, then resize, then reverse iteration
  {
    gl_string v2;
    v2.resize(v.size());
    auto it = v2.rbegin();
    for(size_t i = v.size(); (i--) != 0; ++it) {
      *it = v[i];
    }
    DASSERT_TRUE(v == v2);
  }

  // Assignment by insert into cleared vector.
  {
    gl_string v2;
    v2.resize(1);
    v2.insert(v2.begin(), v.begin(), v.end());
    v2.resize(v.size());
    DASSERT_TRUE(v == v2);
  }

  // Assignment by insert into cleared vector, then erase
  {
    gl_string v2;
    v2.resize(1);
    v2.insert(v2.end(), v.begin(), v.end());
    v2.erase(v2.begin());
    DASSERT_TRUE(v == v2);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // test casting.
  {
    std::string v_stl;

    v_stl = v;

    TS_ASSERT_EQUALS(v_stl.size(), v.size());
    TS_ASSERT(std::equal(v.begin(), v.end(), v_stl.begin()));

    // Assignment
    gl_string v2(20);

    v2 = v_stl;

    TS_ASSERT_EQUALS(v2, v);

    // Construction
    gl_string v3(v_stl);

    TS_ASSERT_EQUALS(v3, v);
  }
}

////////////////////////////////////////////////////////////////////////////////

void stress_test(size_t n_tests) {

  auto gen_element = [](){ return char(random::fast_uniform<int>(32, 127)); };
  
  gl_string v;
  std::string v_ref;

  std::vector<std::function<void()> > operations;

  // Push back
  operations.push_back([&]() {
      auto e = gen_element();
      v.push_back(e);
      v_ref.push_back(e);
    });

  // Insert, 1 element.
  operations.push_back([&]() {
      auto e = gen_element();
      v.insert(v.begin(), e);
      v_ref.insert(v_ref.begin(), e);
    });

  operations.push_back([&]() {
      auto e = gen_element();
      size_t idx = random::fast_uniform<size_t>(0, v.size());
      v.insert(v.begin() + idx, e);
      v_ref.insert(v_ref.begin() + idx, e);
    });

  operations.push_back([&]() {
      auto e = gen_element();
      v.insert(v.end(), e);
      v_ref.insert(v_ref.end(), e);
    });

  // Insert, multiple elements.
  operations.push_back([&]() {
      auto e = gen_element();
      v.insert(v.begin(), 3, e);
      v_ref.insert(v_ref.begin(), 3, e);
    });

  operations.push_back([&]() {
      auto e = gen_element();
      size_t idx = random::fast_uniform<size_t>(0, v.size());
      v.insert(v.begin() + idx, 3, e);
      v_ref.insert(v_ref.begin() + idx, 3, e);
    });

  operations.push_back([&]() {
      auto e = gen_element();
      v.insert(v.end(), 3, e);
      v_ref.insert(v_ref.end(), 3, e);
    });

  // Insert, move element
  operations.push_back([&]() {
      auto e = gen_element();
      auto e2 = e;
      v.insert(v.begin(), std::move(e));
      v_ref.insert(v_ref.begin(), std::move(e2));
    });

  operations.push_back([&]() {
      auto e = gen_element();
      auto e2 = e;
      size_t idx = random::fast_uniform<size_t>(0, v.size());
      v.insert(v.begin() + idx, std::move(e));
      v_ref.insert(v_ref.begin() + idx, std::move(e2));
    });

  operations.push_back([&]() {
      auto e = gen_element();
      auto e2 = e;
      v.insert(v.end(), std::move(e));
      v_ref.insert(v_ref.end(), std::move(e2));
    });

  // Insert, 3 elements.
  operations.push_back([&]() {
      std::string ev = {gen_element(), gen_element(), gen_element()};
      v.insert(v.begin(), ev.begin(), ev.end());
      v_ref.insert(v_ref.begin(), ev.begin(), ev.end());
    });

  operations.push_back([&]() {
      std::string ev = {gen_element(), gen_element(), gen_element()};
      size_t idx = random::fast_uniform<size_t>(0, v.size());
      v.insert(v.begin() + idx, ev.begin(), ev.end());
      v_ref.insert(v_ref.begin() + idx, ev.begin(), ev.end());
    });

  operations.push_back([&]() {
      std::string ev = {gen_element(), gen_element(), gen_element()};
      v.insert(v.end(), ev.begin(), ev.end());
      v_ref.insert(v_ref.end(), ev.begin(), ev.end());
    });

  // Erase, single element.
  operations.push_back([&]() {
      if(v.empty()) return;
      size_t idx = random::fast_uniform<size_t>(0, v.size() - 1);
      v.erase(v.begin() + idx);
      v_ref.erase(v_ref.begin() + idx);
    });

  // Erase, block
  operations.push_back([&]() {
      if(v.empty()) return;
      size_t idx_1 = random::fast_uniform<size_t>(0, v.size() - 1);
      size_t idx_2 = random::fast_uniform<size_t>(0, v.size() - 1);
      v.erase(v.begin() + std::min(idx_1, idx_2), v.begin() + std::max(idx_1, idx_2));
      v_ref.erase(v_ref.begin() + std::min(idx_1, idx_2), v_ref.begin() + std::max(idx_1, idx_2));
    });

  // Erase, to end
  operations.push_back([&]() {
      if(v.empty()) return;
      size_t idx = random::fast_uniform<size_t>(0, v.size() - 1);
      v.erase(v.begin() + idx, v.end());
      v_ref.erase(v_ref.begin() + idx, v_ref.end());
    });

  // Erase, from beginning
  operations.push_back([&]() {
      if(v.empty()) return;
      size_t idx = random::fast_uniform<size_t>(0, v.size() - 1);
      v.erase(v.begin(), v.begin() + idx);
      v_ref.erase(v_ref.begin(), v_ref.begin() + idx);
    });

  // Clear everything.
  operations.push_back([&]() {
      v.clear();
      v_ref.clear();
    });

  // Total clear.
  operations.push_back([&]() {
      gl_string v_empty;
      std::string vr_empty;
      v.swap(v_empty);
      v_ref.swap(vr_empty);
    });


  // Assignment to init list
  operations.push_back([&]() {
      std::string ev = {gen_element(), gen_element(), gen_element()};
      v = {ev[0], ev[1], ev[2]};
      v_ref = {ev[0], ev[1], ev[2]};
    });

  // Assignment by iterator
  operations.push_back([&]() {
      std::string ev = {gen_element(), gen_element(), gen_element()};
      v.assign(ev.begin(), ev.end());
      v_ref.assign(ev.begin(), ev.end());
    });

  // Assignment by move equality
  operations.push_back([&]() {
      std::string ev = {gen_element(), gen_element(), gen_element()};
      gl_string v_tmp(ev.begin(), ev.end());
      v = std::move(v_tmp);
      std::string v_tmp_ref(ev.begin(), ev.end());
      v_ref = std::move(v_tmp_ref);
    });

  // pop_back
  operations.push_back([&]() {
      if(v.empty()) return;
      v.pop_back();
      v_ref.pop_back();
    });

  // swap front and back
  operations.push_back([&]() {
      if(v.empty()) return;
      std::swap(v.back(), v.front());
      std::swap(v_ref.back(), v_ref.front());
    });

  // shuffle by index
  operations.push_back([&]() {
      for(size_t j = 0; j < v.size(); ++j) {
        size_t idx = random::fast_uniform<size_t>(0, v.size() - 1);
        std::swap(v[j], v[idx]);
        std::swap(v_ref[j], v_ref[idx]);
      }
    });

  // shuffle by iterator
  operations.push_back([&]() {
      for(size_t j = 0; j < v.size(); ++j) {
        size_t idx = random::fast_uniform<size_t>(0, v.size() - 1);
        std::swap(*(v.begin() + j), *(v.begin() + idx));
        std::swap(*(v_ref.begin() + j), *(v_ref.begin() + idx));
      }
    });

  // shuffle by reverse iterator
  operations.push_back([&]() {
      for(size_t j = 0; j < v.size(); ++j) {
        size_t idx = random::fast_uniform<size_t>(0, v.size() - 1);
        std::swap(*(v.rbegin() + j), *(v.rbegin() + idx));
        std::swap(*(v_ref.rbegin() + j), *(v_ref.rbegin() + idx));
      }
    });

  // swap and insert.
  operations.push_back([&]() {
      std::string ev = {gen_element(), gen_element(), gen_element()};
      gl_string v2 = {ev[0], ev[1], ev[2]};
      std::string v2_ref = {ev[0], ev[1], ev[2]};
      v.swap(v2);
      v_ref.swap(v2_ref);

      size_t idx = random::fast_uniform<size_t>(0, v.size());
      v.insert(v.begin() + idx, v2.begin(), v2.end());
      v_ref.insert(v_ref.begin() + idx, v2_ref.begin(), v2_ref.end());
    });


  // setting through an std::string.
  operations.push_back([&]() {

      std::string v2 = v;
      v.assign(v2.begin(), v2.end());

      gl_string v2_ref(v_ref.begin(), v_ref.end());
      v_ref.assign(v2_ref.begin(), v2_ref.end());
    });

  // string deserialization
  operations.push_back([&]() {
      std::string s = serialize_to_string(v);
      v.clear();
      deserialize_from_string(s, v);
    });

  // string deserialization via vector
  operations.push_back([&]() {
      std::string v2 = v;
      std::string s = serialize_to_string(v2);
      deserialize_from_string(s, v);
    });

  // setting through an std::string.
  operations.push_back([&]() {

      std::string v2 = v;
      v.assign(v2.begin(), v2.end());

      gl_string v2_ref(v_ref.begin(), v_ref.end());
      v_ref.assign(v2_ref.begin(), v2_ref.end());
    });

  // substring
  operations.push_back([&]() {

      size_t n1 = random::fast_uniform<size_t>(0, v.size());
      size_t n2 = random::fast_uniform<size_t>(0, v.size());

      if(n1 > n2) std::swap(n1, n2);

      v = v.substr(n1, n2 - n1); 
      v_ref = v_ref.substr(n1, n2 - n1);
    });

  // self-append
  operations.push_back([&]() {
      v+= v;
      v_ref += v_ref;
    });
  
  for(size_t i = 0; i < n_tests; ++i) {
    size_t idx = random::fast_uniform<size_t>(0, operations.size() - 1);
    operations[idx]();

    ASSERT_EQ(v.size(), v_ref.size());

    for(size_t j = 0; j < v.size(); ++j) {
      ASSERT_TRUE(v[j] == v_ref[j]);
    }

    if((i + 1) % 1000 == 0) {
      verify_serialization(v);
      verify_consistency(v);
    }
  }
}

struct gl_string_stress_test   {
 public:

  void test_stress() {
    random::seed(0);
    stress_test(5000000);
  }
};


BOOST_FIXTURE_TEST_SUITE(_gl_string_stress_test, gl_string_stress_test)
BOOST_AUTO_TEST_CASE(test_stress) {
  gl_string_stress_test::test_stress();
}
BOOST_AUTO_TEST_SUITE_END()
