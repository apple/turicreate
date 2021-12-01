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
  void test_stop_without_start(void) {
    my_watch_t watch(1);
    TS_ASSERT_THROWS_ANYTHING(watch.stop());
  }

  void test_double_start(void) {
    my_watch_t watch(1);
    watch.start();
    // no double start
    TS_ASSERT_THROWS_NOTHING(watch.start());
  }

  void test_single_thread(void) {
    my_watch_t watch(1);
    watch.start();
    std::this_thread::sleep_for(milliseconds(2));
    TS_ASSERT(watch.duration<milliseconds>() >= milliseconds(2));
    watch.stop();
    TS_ASSERT(watch.duration<milliseconds>() >= milliseconds(2));
  }

  // main thread's stop watch stops as the last
  void test_multi_thread_1(void) {
    my_watch_t watch(1);
    watch.start();
    auto start = std::chrono::steady_clock::now();
    std::thread t1([&watch]() {
      watch.start();
      watch.stop();
    });

    std::thread t2([&watch]() {
      std::this_thread::sleep_for(milliseconds(1));
      watch.start();
      std::this_thread::sleep_for(milliseconds(1));
      watch.stop();
    });
    std::this_thread::sleep_for(milliseconds(10));
    t1.join();
    t2.join();
    auto stop = std::chrono::steady_clock::now();
    watch.stop();

    TS_ASSERT(watch.duration<milliseconds>() >=
              std::chrono::duration_cast<milliseconds>(stop - start));
  }

  // t2 stops as the last one
  void test_multi_thread_2(void) {
    my_watch_t watch(1);
    watch.start();
    auto start = std::chrono::steady_clock::now();

    std::thread t1([&watch]() {
      watch.start();
      TS_ASSERT(watch.stop() > 0);
    });

    std::thread t2([&watch]() {
      std::this_thread::sleep_for(milliseconds(1));
      watch.start();
      std::this_thread::sleep_for(milliseconds(25));
      // t2 is the last one to stop the watch
      TS_ASSERT(watch.stop() == 0);
    });

    t1.join();
    std::this_thread::sleep_for(milliseconds(5));
    // main also stops
    auto stop = std::chrono::steady_clock::now();
    TS_ASSERT(watch.stop() > 0);

    // the clock is still on becuase t2 is still running
    TS_ASSERT(watch.duration<milliseconds>() >=
              std::chrono::duration_cast<milliseconds>(stop - start));
    TS_ASSERT(watch.duration<milliseconds>() >= milliseconds(5));

    t2.join();
    TS_ASSERT(watch.duration<milliseconds>() >= milliseconds(25));
  }

  void test_stop_and_continue(void) {
    my_watch_t watch(1);
    watch.start();
    watch.stop();
    // interval is less than 150ms
    std::this_thread::sleep_for(milliseconds(3));
    watch.start();
    watch.stop();
    std::this_thread::sleep_for(milliseconds(3));
    watch.start();
    watch.stop();

    TS_ASSERT(watch.duration<milliseconds>() >= milliseconds(6));
  }

  void test_time_to_record(void) {
    my_watch_t watch(5);
    watch.start();
    TS_ASSERT(watch.is_time_to_record() == true);
    TS_ASSERT(watch.is_time_to_record() == false);
    std::this_thread::sleep_for(milliseconds(5));
    TS_ASSERT(watch.is_time_to_record() == true);
    TS_ASSERT(watch.is_time_to_record() == false);
  }
};

BOOST_FIXTURE_TEST_SUITE(_stop_watch_test, stop_watch_test)
BOOST_AUTO_TEST_CASE(test_stop_without_start) {
  stop_watch_test::test_stop_without_start();
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
BOOST_AUTO_TEST_CASE(test_stop_and_continue) {
  stop_watch_test::test_stop_and_continue();
}
BOOST_AUTO_TEST_CASE(test_time_to_record) {
  stop_watch_test::test_time_to_record();
}
BOOST_AUTO_TEST_SUITE_END()
