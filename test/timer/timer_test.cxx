#define BOOST_TEST_MODULE
#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>
#include <timer/timer.hpp>
#include <unistd.h>
using namespace turi;

struct timer_test {
public:

  void test_timer() {
    timer ti;
    ti.start();
    double t = ti.current_time();
    TS_ASSERT_DELTA(t, 0, 0.2);
    sleep(3);
    t = ti.current_time();
    TS_ASSERT_DELTA(t, 3, 0.2);
  }


  void test_lowres_timer() {
    timer ti;
    int t = timer::approx_time_seconds();
    sleep(3);
    int t2 = timer::approx_time_seconds();
    TS_ASSERT_DELTA(double(t2 - t), 3.0, 2);
  }


};

BOOST_FIXTURE_TEST_SUITE(_timer_test, timer_test)
BOOST_AUTO_TEST_CASE(test_timer) {
  timer_test::test_timer();
}
BOOST_AUTO_TEST_CASE(test_lowres_timer) {
  timer_test::test_lowres_timer();
}
BOOST_AUTO_TEST_SUITE_END()
