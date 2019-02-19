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


#include <generics/sparse_parallel_2d_array.hpp>

using namespace turi;

////////////////////////////////////////////////////////////////////////////////

struct test_sparse_2d_parallel_array  {
 public:

  void test_basic_access() {
    sparse_parallel_2d_array<size_t> X(193, 43);

    TS_ASSERT_EQUALS(X.rows(), 193);
    TS_ASSERT_EQUALS(X.cols(), 43);

    for(size_t i = 0; i < 193; ++i) {
      for(size_t j = 0; j < 43; ++j) {
        X(i, j) = i + j;
      }
    }

    for(size_t i = 0; i < 193; ++i) {
      for(size_t j = 0; j < 43; ++j) {
        DASSERT_EQ(i + j, X(i, j));
      }
    }

    X.apply_all([](size_t i, size_t j, size_t value) {
        DASSERT_EQ(value, i + j);
      });
  }

  void test_basic_access_2d_aligned() {
    sparse_parallel_2d_array<size_t> X(32, 32);

    for(size_t i = 0; i < 32; ++i) {
      for(size_t j = 0; j < 32; ++j) {
        X(i, j) = (i + 33) * j;
      }
    }

    X.apply_all([](size_t i, size_t j, size_t value) {
        DASSERT_EQ(value, (i+33) * j);
      });
  }

  void test_parallel_access() {
    sparse_parallel_2d_array<size_t> X(193, 43);

    parallel_for(size_t(0), X.rows(), [&](size_t i) {
        for(size_t j = 0; j < 43; ++j) {
          X.apply(i, j, [&](size_t& value) { value = i + j; });
        }
      });

    X.apply_all([](size_t i, size_t j, size_t value) {
        DASSERT_EQ(value, i + j);
      });
  }

  void test_parallel_access_default_value() {
    sparse_parallel_2d_array<size_t> X(193, 43);

    parallel_for(0UL, X.rows(), [&](size_t i) {
        for(size_t j = 0; j < 43; ++j) {
          X.apply(i, j, [&](size_t& value) { value += i + j; });
        }
      });

    X.apply_all([](size_t i, size_t j, size_t value) {
        DASSERT_EQ(value, i + j);
      });
  }

  void test_thread_isolation() {
    sparse_parallel_2d_array<size_t> X(1935, 128);

    parallel_for(0UL, X.rows(), [&](size_t i) {
        for(size_t _j = 0; _j < 5; ++_j) {
          size_t j = hash64(_j) % 128;
          X.apply(i, j, [&](size_t& value) { value += i + j; });
        }
      });

    std::vector<size_t> accessing_thread(1935, size_t(-1));

    X.apply_all([&](size_t i, size_t j, size_t value) {
        size_t thread_idx = thread::thread_id();

        if(accessing_thread[i] == size_t(-1)) {
          accessing_thread[i] = thread_idx;
        } else {
          DASSERT_EQ(accessing_thread[i], thread_idx);
        }

        DASSERT_EQ(value, i + j);
      });
  }

  void test_modifying_apply_all() {

    // Negligent chances of a colision; plus test is deterministic so
    // this will always work.
    size_t n = 512*1024;
    size_t m = 1024*1024*1024;
    size_t n_values = 10000;

    sparse_parallel_2d_array<size_t> X(n, m);

    struct info {
      size_t i=0, j=0, value=-1;
    };

    std::vector<info> vf(n_values);
    std::vector<info> new_vf(n_values);

    for(size_t i = 0; i < n_values; ++i) {
      size_t rng = hash64(i);
      vf[i].i = rng % n;
      vf[i].j = hash64(rng) % m;
      vf[i].value = rng;
    }

    parallel_for(0, n_values, [&](size_t i) {
        X.apply(vf[i].i, vf[i].j, [&](size_t& value) { value = vf[i].value; });
      });

    X.apply_all([&](size_t i, size_t j, size_t& value) {
        value ^= hash64(i, j);
      });

    const auto& X_const = X;

    atomic<size_t> write_pos = 0;
    X_const.apply_all([&](size_t i, size_t j, size_t value) {

        size_t idx = (++write_pos) - 1;

        DASSERT_LT(idx, n_values);

        new_vf[idx].i = i;
        new_vf[idx].j = j;
        new_vf[idx].value = value;
      });

    auto cmp_info = [](const info& i1, const info& i2) {
      return (std::make_pair(i1.i, i1.j) < std::make_pair(i2.i, i2.j));
    };

    std::sort(vf.begin(), vf.end(), cmp_info);
    std::sort(new_vf.begin(), new_vf.end(), cmp_info);

    DASSERT_EQ(vf.size(), new_vf.size());

    for(size_t i = 0; i < vf.size(); ++i) {
      TS_ASSERT_EQUALS(vf[i].i, new_vf[i].i);
      TS_ASSERT_EQUALS(vf[i].j, new_vf[i].j);

      size_t new_value = vf[i].value ^ hash64(vf[i].i, vf[i].j);

      TS_ASSERT_EQUALS(new_value, new_vf[i].value);
    }
  }

  void test_large_stress_test() {
    size_t n = 512*1024;
    size_t m = 1024*1024*1024;
    size_t n_values = 100000;

    sparse_parallel_2d_array<size_t> X(n, m);

    struct info {
      size_t i=0, j=0, value=-1;
    };

    std::vector<info> vf(n_values);
    std::vector<info> new_vf(n_values);

    for(size_t i = 0; i < n_values; ++i) {
      size_t rng = hash64(i);
      vf[i].i = rng % n;
      vf[i].j = hash64(rng) % m;
      vf[i].value = rng;
    }

    parallel_for(0, n_values, [&](size_t i) {
        X.apply(vf[i].i, vf[i].j, [&](size_t& value) { value = vf[i].value; });
      });

    atomic<size_t> write_pos = 0;
    X.apply_all([&](size_t i, size_t j, size_t value) {

        size_t idx = (++write_pos) - 1;

        DASSERT_LT(idx, n_values);

        new_vf[idx].i = i;
        new_vf[idx].j = j;
        new_vf[idx].value = value;
      });

    auto cmp_info = [](const info& i1, const info& i2) {
      return (std::make_pair(std::make_pair(i1.i, i1.j), i1.value)
              < std::make_pair(std::make_pair(i2.i, i2.j), i2.value));
    };

    std::sort(vf.begin(), vf.end(), cmp_info);
    std::sort(new_vf.begin(), new_vf.end(), cmp_info);

    DASSERT_EQ(vf.size(), new_vf.size());

    for(size_t i = 0; i < vf.size(); ++i) {
      TS_ASSERT_EQUALS(vf[i].i, new_vf[i].i);
      TS_ASSERT_EQUALS(vf[i].j, new_vf[i].j);
      TS_ASSERT_EQUALS(vf[i].value, new_vf[i].value);
    }
  }

  void test_with_vector() {

    // Negligent chances of a colision; plus test is deterministic so
    // this will always work.
    size_t n = 55100377;
    size_t m = 10243223;
    size_t n_values = 1000;

    sparse_parallel_2d_array<std::vector<size_t> > X(n, m);

    struct info {
      size_t i=0, j=0, value=-1;
    };

    std::vector<info> vf(n_values);
    std::vector<info> new_vf(n_values);

    for(size_t i = 0; i < n_values; ++i) {
      size_t rng = hash64(i);
      vf[i].i = rng % n;
      vf[i].j = hash64(rng) % m;
      vf[i].value = rng;
    }

    parallel_for(0, n_values, [&](size_t i) {
        X.apply(vf[i].i, vf[i].j, [&](std::vector<size_t>& value) { value = {vf[i].value}; });
      });

    const auto& X_const = X;

    atomic<size_t> write_pos = 0;
    X_const.apply_all([&](size_t i, size_t j, const std::vector<size_t>& value) {

        size_t idx = (++write_pos) - 1;

        DASSERT_LT(idx, n_values);

        new_vf[idx].i = i;
        new_vf[idx].j = j;
        new_vf[idx].value = value[0];
      });

    auto cmp_info = [](const info& i1, const info& i2) {
      return (std::make_pair(i1.i, i1.j) < std::make_pair(i2.i, i2.j));
    };

    std::sort(vf.begin(), vf.end(), cmp_info);
    std::sort(new_vf.begin(), new_vf.end(), cmp_info);

    DASSERT_EQ(vf.size(), new_vf.size());

    for(size_t i = 0; i < vf.size(); ++i) {
      TS_ASSERT_EQUALS(vf[i].i, new_vf[i].i);
      TS_ASSERT_EQUALS(vf[i].j, new_vf[i].j);

      TS_ASSERT_EQUALS(vf[i].value, new_vf[i].value);
    }
  }



};

BOOST_FIXTURE_TEST_SUITE(_test_sparse_2d_parallel_array, test_sparse_2d_parallel_array)
BOOST_AUTO_TEST_CASE(test_basic_access) {
  test_sparse_2d_parallel_array::test_basic_access();
}
BOOST_AUTO_TEST_CASE(test_basic_access_2d_aligned) {
  test_sparse_2d_parallel_array::test_basic_access_2d_aligned();
}
BOOST_AUTO_TEST_CASE(test_parallel_access) {
  test_sparse_2d_parallel_array::test_parallel_access();
}
BOOST_AUTO_TEST_CASE(test_parallel_access_default_value) {
  test_sparse_2d_parallel_array::test_parallel_access_default_value();
}
BOOST_AUTO_TEST_CASE(test_thread_isolation) {
  test_sparse_2d_parallel_array::test_thread_isolation();
}
BOOST_AUTO_TEST_CASE(test_modifying_apply_all) {
  test_sparse_2d_parallel_array::test_modifying_apply_all();
}
BOOST_AUTO_TEST_CASE(test_large_stress_test) {
  test_sparse_2d_parallel_array::test_large_stress_test();
}
BOOST_AUTO_TEST_CASE(test_with_vector) {
  test_sparse_2d_parallel_array::test_with_vector();
}
BOOST_AUTO_TEST_SUITE_END()
