/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <iostream>
#include <signal.h>
#include <sys/time.h>
#include <boost/bind.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <timer/timer.hpp>

std::ostream&  operator<<(std::ostream& out, const turi::timer& t) {
  return out << t.current_time();
}

namespace turi {


class hundredms_timer {
  thread timer_thread;
 public:
  hundredms_timer() {
    stop = false;
    ti.start();
    timer_thread.launch(boost::bind(&hundredms_timer::alarm_thread, this));
  }
  size_t ctr;
  timer ti;
  mutex lock;
  conditional cond;
  bool stop;

  void alarm_thread() {
    lock.lock();
    while(!stop) {
      cond.timedwait_ms(lock, 50);
      double realtime = ti.current_time() ;
      ctr = (size_t)(realtime * 10);
    }
    lock.unlock();
  }

  void stop_timer() {
    if (stop == false) {
      lock.lock();
      stop = true;
      cond.signal();
      lock.unlock();
      timer_thread.join();
    }
  }

  ~hundredms_timer() {
    stop_timer();
  }
};

static hundredms_timer& get_hms_timer() {
  static hundredms_timer hmstimer;
  return hmstimer;
}



  /**
   * Precision of deciseconds
   */
  float timer::approx_time_seconds() {
    return float(get_hms_timer().ctr) / 10;
  }

  /**
   * Precision of deciseconds
   */
  size_t timer::approx_time_millis() {
    return get_hms_timer().ctr * 100;
  }

  void timer::stop_approx_timer() {
    get_hms_timer().stop_timer();
  }


  void timer::sleep(size_t sleeplen) {
    struct timespec timeout;
    timeout.tv_sec = sleeplen;
    timeout.tv_nsec = 0;
    while (nanosleep(&timeout, &timeout) == -1);
  }


  /**
  Sleeps for sleeplen milliseconds.
  */
  void timer::sleep_ms(size_t sleeplen) {
    struct timespec timeout;
    timeout.tv_sec = sleeplen / 1000;
    timeout.tv_nsec = (sleeplen % 1000) * 1000000;
    while (nanosleep(&timeout, &timeout) == -1);
  }




static unsigned long long rtdsc_ticks_per_sec = 0;
static mutex rtdsc_ticks_per_sec_mutex;

unsigned long long estimate_ticks_per_second() {
  if (rtdsc_ticks_per_sec == 0) {
    rtdsc_ticks_per_sec_mutex.lock();
      if (rtdsc_ticks_per_sec == 0) {
      unsigned long long tstart = rdtsc();
      turi::timer::sleep(1);
      unsigned long long tend = rdtsc();
      rtdsc_ticks_per_sec = tend - tstart;
      }
    rtdsc_ticks_per_sec_mutex.unlock();
  }
  return rtdsc_ticks_per_sec;
}

}
