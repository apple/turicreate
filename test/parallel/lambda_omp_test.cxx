#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <core/parallel/lambda_omp.hpp>
#include <core/parallel/mutex.hpp>



using namespace turi;

struct lambda_omp_test {
public:
  void test_parallel_for(void) {
    std::vector<int> ctr(100000);
    // parallel for over integers
    parallel_for(0, ctr.size(), [&](size_t idx) {
                      ctr[idx]++;
                    });
    for (size_t i = 0; i < ctr.size(); ++i) {
      TS_ASSERT_EQUALS(ctr[i], 1);
    }

    // fold
    int sum = fold_reduce(0, ctr.size(), 
                          [&](size_t idx, int& sum) {
                            sum += ctr[idx];
                          }, 0);
    TS_ASSERT_EQUALS(sum, 100000);

    // parallel for over iterators 
    parallel_for(ctr.begin(), ctr.end(), [&](int& c) {
                      c++;
                    });
    for (size_t i = 0; i < ctr.size(); ++i) {
      TS_ASSERT_EQUALS(ctr[i], 2);
    }

    // just do stuff in parallel
    in_parallel([&](size_t thrid, size_t num_threads) {
                     ctr[thrid]++;
                   });

    size_t nthreads = thread_pool::get_instance().size();
    for (size_t i = 0; i < nthreads; ++i) {
      TS_ASSERT_EQUALS(ctr[i], 3);
    }
    for (size_t i = nthreads; i < ctr.size(); ++i) {
      TS_ASSERT_EQUALS(ctr[i], 2);
    }

  }

  long fib (long n) {
    if (n <= 2) {
      return 1;
    } else {
      return fib(n-1) + fib(n-2);
    }
  }

  void test_parallel_for_fib(void) {

    std::vector<long> ls {40,40,40,40,40,40};
    std::cout << "----------------------------------------------------------" << std::endl;
    parallel_for(0, ls.size(), [&](size_t idx) {
                    std::cout << ls[idx] << ": " << fib(ls[idx]) << "\n";
                 });
  }

  void test_exception_forward() {
    std::vector<int> ctr(100000);
    // parallel for over integers
    TS_ASSERT_THROWS_ANYTHING(parallel_for((size_t)0, (size_t)100, 
                                           [&](size_t idx) {
                                             throw("hello world");
                                           }));

    TS_ASSERT_THROWS_ANYTHING(fold_reduce((size_t)0, (size_t)100, 
                                          [&](size_t idx, double& sum) {
                                            throw("hello world");
                                          }, 0.0));

    TS_ASSERT_THROWS_ANYTHING(parallel_for(ctr.begin(), ctr.end(), 
                                           [&](int& c) {
                                             throw("hello world");
                                           }));

    TS_ASSERT_THROWS_ANYTHING(in_parallel([&](size_t thrid, size_t num_threads) {
                                             throw("hello world");
                                          }));
  }

  void test_mutex(void) {
    turi::mutex lock;
    size_t i = 0;
    std::cout << "----------------------------------------------------------" << std::endl;
    parallel_for((size_t)0, (size_t)10000, [&](size_t idx) {
      std::lock_guard<turi::mutex> guard(lock);
      TS_ASSERT_EQUALS(lock.try_lock(), false);
      ++i;
    });
    TS_ASSERT_EQUALS(i, 10000);
  }

  void test_recursive_mutex(void) {
    turi::recursive_mutex lock;
    size_t i = 0;
    std::cout << "----------------------------------------------------------" << std::endl;
    parallel_for((size_t)0, (size_t)10000, [&](size_t idx) {
      std::lock_guard<turi::recursive_mutex> guard(lock);
      {
        std::lock_guard<turi::recursive_mutex> guard2(lock);
        ++i;
      }
    });
    TS_ASSERT_EQUALS(i, 10000);
  }
};

BOOST_FIXTURE_TEST_SUITE(_lambda_omp_test, lambda_omp_test)
BOOST_AUTO_TEST_CASE(test_parallel_for) {
  lambda_omp_test::test_parallel_for();
}
BOOST_AUTO_TEST_CASE(test_parallel_for_fib) {
  lambda_omp_test::test_parallel_for_fib();
}
BOOST_AUTO_TEST_CASE(test_exception_forward) {
  lambda_omp_test::test_exception_forward();
}
BOOST_AUTO_TEST_CASE(test_mutex) {
  lambda_omp_test::test_mutex();
}
BOOST_AUTO_TEST_CASE(test_recursive_mutex) {
  lambda_omp_test::test_recursive_mutex();
}
BOOST_AUTO_TEST_SUITE_END()
