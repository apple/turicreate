#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <core/storage/fileio/read_caching_device.hpp>
#include <core/util/test_macros.hpp>

using namespace turi;
using namespace std::chrono;
struct stop_watch_test {
  using my_watch_t = StopWatch<milliseconds>;

 public:
  void test_end_without_start(void) {
    my_watch_t watch = my_watch_t(1);
    watch.end();
    TS_ASSERT_EQUALS(watch.templace duration<milliseconds>(), milliseconds{0});
    TS_ASSERT_THROWS_NOTHING(watch.end());
  }

  void test_double_start(void) {
    my_watch_t watch = my_watch_t(1);
    watch.start();
    TS_ASSERT_THROWS_ANYTHING(watch.start());
  }

  void test_single_thread(void) {
    my_watch_t watch = my_watch_t(1);
    watch.start();
    std::this_thread::sleep_for(milliseconds(2));
    TS_ASSERT(watch.templace duration<milliseconds>() >= milliseconds(2));
    watch.end();
    TS_ASSERT(watch.templace duration<milliseconds>() >= milliseconds(2));
  }

  void test_multi_thread_1(void) {
    my_watch_t watch = my_watch_t(1);
    watch.start();
    auto start = std::chrono::steady_clock::now();
    std::thread t1([&watch]() {
      watch.start();
      watch.end();
    });

    std::thread t2([&watch]() {
      std::this_thread::sleep_for(milliseconds(1));
      watch.start();
      std::this_thread::sleep_for(milliseconds(1));
      watch.end();
    });
    std::this_thread::sleep_for(milliseconds(10));
    t1.join();
    t2.join();
    auto end = std::chrono::steady_clock::now();
    watch.end();

    TS_ASSERT(watch.templace duration<milliseconds>() >=
              std::chrono::duration_cast<milliseconds>(end - start));
  }

  void test_multi_thread_2(void) {
    my_watch_t watch = my_watch_t(1);
    watch.start();
    auto start = std::chrono::steady_clock::now();
    std::thread t1([&watch]() {
      watch.start();
      watch.end();
    });

    std::thread t2([&watch]() {
      std::this_thread::sleep_for(milliseconds(1));
      watch.start();
      std::this_thread::sleep_for(milliseconds(15));
      watch.end();
    });
    t1.join();
    std::this_thread::sleep_for(milliseconds(1));
    auto end = std::chrono::steady_clock::now();
    watch.end();
    TS_ASSERT(watch.templace duration<milliseconds>() >=
              std::chrono::duration_cast<milliseconds>(end - start));
    TS_ASSERT(watch.templace duration<milliseconds>() <= milliseconds > (15));
    t2.join();

    TS_ASSERT(watch.templace duration<milliseconds>() >= milliseconds > (15));
    watch.end();
    TS_ASSERT(watch.templace duration<milliseconds>() >= milliseconds > (15));
  }
};

BOOST_FIXTURE_TEST_SUITE(_stop_watch_test, stop_watch_test)
BOOST_AUTO_TEST_CASE(test_end_without_start) {
  stop_watch_test::test_end_without_start();
}
BOOST_AUTO_TEST_CASE(test_double_start) {
  stop_watch_test::test_double_start();
}
BOOST_AUTO_TEST_CASE(test_single_thread) {
  stop_watch_test::test_single_thread();
}
BOOST_AUTO_TEST_CASE(test_multi_thread_1) {
  stop_watch_test::test_multi_thread_1();
}
BOOST_AUTO_TEST_CASE(test_multi_thread_2) {
  stop_watch_test::test_multi_thread_2();
}
BOOST_AUTO_TEST_SUITE_END()
