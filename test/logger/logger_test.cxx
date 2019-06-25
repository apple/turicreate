#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <fstream>
#include <core/logging/logger.hpp>
#include <core/logging/log_rotate.hpp>
#include <core/logging/log_level_setter.hpp>
using namespace turi;

struct logger_test {
 public:
  void test_empty_log() {
    global_logger().set_log_level(LOG_INFO);
    logstream(LOG_INFO) << "\n";
    logstream(LOG_INFO);
    logstream(LOG_INFO);
    logstream(LOG_INFO) << std::endl;
  }

  void test_log_level_setter() {
    logprogress_stream << "This should show up" << std::endl;
    auto x = log_level_setter(LOG_NONE);
    logprogress_stream << "This should not print." << std::endl;
  }

  void test_log_rotation() {
    global_logger().set_log_level(LOG_INFO);
    begin_log_rotation("rotate.log",
                       1 /* log rotates every second */,
                       2 /* we only keep the last 2 logs around*/);
    for (size_t i = 0;i < 5; ++i) {
      logstream(LOG_INFO) << i << std::endl;
      timer::sleep(1);
    }
    TS_ASSERT(std::ifstream("rotate.log").good());
    TS_ASSERT(std::ifstream("rotate.log.0").good() == false);
    TS_ASSERT(std::ifstream("rotate.log.1").good() == false);
    stop_log_rotation();
  }
};

BOOST_FIXTURE_TEST_SUITE(_logger_test, logger_test)
BOOST_AUTO_TEST_CASE(test_empty_log) {
  logger_test::test_empty_log();
}
BOOST_AUTO_TEST_CASE(test_log_level_setter) {
  logger_test::test_log_level_setter();
}
BOOST_AUTO_TEST_CASE(test_log_rotation) {
  logger_test::test_log_rotation();
}
BOOST_AUTO_TEST_SUITE_END()
