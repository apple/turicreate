#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/parallel/atomic_ops.hpp>
#include <core/parallel/atomic.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <core/util/cityhash_tc.hpp>

using namespace turi;

struct atomic_ops_test {
public:

  template <typename T>
  void _run_max_simple_test() {

    T v = 9;
    volatile T vv = 9;

    // Single threaded, so this should just work.
    for(T i = 10; i < 100; ++i) {
      {
        // Change.
        T old_v = atomic_set_max(v, i);
        T old_v_2 = atomic_set_max(vv, i);
        TS_ASSERT_EQUALS(old_v, i - 1);
        TS_ASSERT_EQUALS(v, i);
        TS_ASSERT_EQUALS(old_v_2, i - 1);
        TS_ASSERT_EQUALS(vv, i);
      }

      {
        // No change.
        T old_v = atomic_set_max(v, i - 2);
        T old_v_2 = atomic_set_max(vv, i - 2);
        TS_ASSERT_EQUALS(old_v, i);
        TS_ASSERT_EQUALS(v, i);
        TS_ASSERT_EQUALS(old_v_2, i);
        TS_ASSERT_EQUALS(T(vv), i);
      }
    }
  }

  template <typename T>
  void _run_max_parallel_test() {

    volatile T v = 0;

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        while(v < 10000000) {

          T base_value = v;
          T new_v = base_value + thread_idx + 1;
          T old_v = atomic_set_max(v, new_v);
          // we do a double check here rather than TS_ASSERT_LESS_THAN_EQUAL
          // because both boost test and TS_ASSERT macros have
          // a somewhat large overhead...
          if (!(base_value <= old_v)) {
            TS_ASSERT_LESS_THAN_EQUALS(base_value, old_v);
          }
          // we just ASSERT(false) here because v is accessed in parallel.
          // If this comparison fails for whatever reason, a second compare
          // may give a different result
          if (!(new_v <= v)) {
            TS_ASSERT(false);
          }
        }
      });
  }

  void test_max_simple_int32() {
    _run_max_simple_test<int32_t>();
  }

  void test_max_simple_int64() {
    _run_max_simple_test<int64_t>();
  }

  void test_max_simple_uint32() {
    _run_max_simple_test<uint32_t>();
  }

  void test_max_simple_uint64() {
    _run_max_simple_test<uint64_t>();
  }

  void test_max_parallel_int32() {
    _run_max_parallel_test<int32_t>();
  }

  void test_max_parallel_int64() {
    _run_max_parallel_test<int64_t>();
  }

  void test_max_parallel_uint32() {
    _run_max_parallel_test<uint32_t>();
  }

  void test_max_parallel_uint64() {
    _run_max_parallel_test<uint64_t>();
  }



  template <typename T>
  void _run_min_simple_test() {

    T v = 101;
    volatile T vv = 101;

    // Single threaded, so this should just work.
    for(T i = 100; i > 10; --i) {
      {
        // Change.
        T old_v = atomic_set_min(v, i);
        T old_v_2 = atomic_set_min(vv, i);
        TS_ASSERT_EQUALS(old_v, i + 1);
        TS_ASSERT_EQUALS(v, i);
        TS_ASSERT_EQUALS(old_v_2, i + 1);
        TS_ASSERT_EQUALS(vv, i);
      }

      {
        // No change.
        T old_v = atomic_set_min(v, i + 2);
        T old_v_2 = atomic_set_min(vv, i + 2);
        TS_ASSERT_EQUALS(old_v, i);
        TS_ASSERT_EQUALS(v, i);
        TS_ASSERT_EQUALS(old_v_2, i);
        TS_ASSERT_EQUALS(vv, i);
      }
    }
  }

  template <typename T>
  void _run_min_parallel_test() {

    volatile T v = 20000000;

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        while(v > 10000000) {

          T base_value = v;
          T new_v = base_value - thread_idx - 1;
          T old_v = atomic_set_min(v, new_v);
          if (! (old_v <= base_value)) {
            TS_ASSERT_LESS_THAN_EQUALS(old_v, base_value);
          }
          // we just ASSERT(false) here because v is accessed in parallel.
          // If this comparison fails for whatever reason, a second compare
          // may give a different result
          if (!(v <= new_v)) {
            TS_ASSERT(false);
          }
        }
      });
  }


  void test_min_simple_int32() {
    _run_min_simple_test<int32_t>();
  }

  void test_min_simple_int64() {
    _run_min_simple_test<int64_t>();
  }

  void test_min_simple_uint32() {
    _run_min_simple_test<uint32_t>();
  }

  void test_min_simple_uint64() {
    _run_min_simple_test<uint64_t>();
  }

  void test_min_parallel_int32() {
    _run_min_parallel_test<int32_t>();
  }

  void test_min_parallel_int64() {
    _run_min_parallel_test<int64_t>();
  }

  void test_min_parallel_uint32() {
    _run_min_parallel_test<uint32_t>();
  }

  void test_min_parallel_uint64() {
    _run_min_parallel_test<uint64_t>();
  }

  template <typename T>
  void _run_atomic_increment_test_1() {

    volatile T v = 16 * 10000 * thread::cpu_count();

    atomic<T> true_v = v;

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        for(size_t i = 0; i < 10000; ++i) {

          int inc = hash64(thread_idx, i) % 16 - 8;
          atomic_increment(v, inc);
          true_v += inc;
        }
      });

    TS_ASSERT_EQUALS(T(v), T(true_v));
  }

  template <typename T>
  void _run_atomic_increment_test_2() {

    volatile T v = 0;

    atomic<T> true_v = v;

    in_parallel([&](size_t thread_idx, size_t num_threads) {

        for(size_t i = 0; i < 10000; ++i) {

          int inc = hash64(thread_idx, i) % 16;
          T old_v = atomic_increment(v, inc);
          // we just ASSERT(false) here because v is accessed in parallel.
          // If this comparison fails for whatever reason, a second compare
          // may give a different result
          if (!(old_v <= T(v))) {
            TS_ASSERT(false);
          }

          true_v += inc;
        }
      });

    TS_ASSERT_EQUALS(T(v), T(true_v));
  }


};

BOOST_FIXTURE_TEST_SUITE(_atomic_ops_test, atomic_ops_test)
BOOST_AUTO_TEST_CASE(test_max_simple_int32) {
  atomic_ops_test::test_max_simple_int32();
}
BOOST_AUTO_TEST_CASE(test_max_simple_int64) {
  atomic_ops_test::test_max_simple_int64();
}
BOOST_AUTO_TEST_CASE(test_max_simple_uint32) {
  atomic_ops_test::test_max_simple_uint32();
}
BOOST_AUTO_TEST_CASE(test_max_simple_uint64) {
  atomic_ops_test::test_max_simple_uint64();
}
BOOST_AUTO_TEST_CASE(test_max_parallel_int32) {
  atomic_ops_test::test_max_parallel_int32();
}
BOOST_AUTO_TEST_CASE(test_max_parallel_int64) {
  atomic_ops_test::test_max_parallel_int64();
}
BOOST_AUTO_TEST_CASE(test_max_parallel_uint32) {
  atomic_ops_test::test_max_parallel_uint32();
}
BOOST_AUTO_TEST_CASE(test_max_parallel_uint64) {
  atomic_ops_test::test_max_parallel_uint64();
}
BOOST_AUTO_TEST_CASE(test_min_simple_int32) {
  atomic_ops_test::test_min_simple_int32();
}
BOOST_AUTO_TEST_CASE(test_min_simple_int64) {
  atomic_ops_test::test_min_simple_int64();
}
BOOST_AUTO_TEST_CASE(test_min_simple_uint32) {
  atomic_ops_test::test_min_simple_uint32();
}
BOOST_AUTO_TEST_CASE(test_min_simple_uint64) {
  atomic_ops_test::test_min_simple_uint64();
}
BOOST_AUTO_TEST_CASE(test_min_parallel_int32) {
  atomic_ops_test::test_min_parallel_int32();
}
BOOST_AUTO_TEST_CASE(test_min_parallel_int64) {
  atomic_ops_test::test_min_parallel_int64();
}
BOOST_AUTO_TEST_CASE(test_min_parallel_uint32) {
  atomic_ops_test::test_min_parallel_uint32();
}
BOOST_AUTO_TEST_CASE(test_min_parallel_uint64) {
  atomic_ops_test::test_min_parallel_uint64();
}
BOOST_AUTO_TEST_SUITE_END()
