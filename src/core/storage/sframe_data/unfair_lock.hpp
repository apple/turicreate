/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_UNFAIR_IO_SCHEDULER_HPP
#define TURI_SFRAME_UNFAIR_IO_SCHEDULER_HPP
#include <cstdint>
#include <cstddef>
#include <map>
#include <core/parallel/pthread_tools.hpp>
namespace turi {


/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_internal SFrame Internal
 * \{
 */

/**
 * This class implements a completely unfair lock.
 *
 * The basic mechanic of operation is that every thread is assigned a
 * priority ID (this is via a thread local variable) (rant if apple compiler
 * has proper C++11 support this would just be using the thread_local keyword.
 * But we don't. So it is this annoying boilerplate around pthread_getspecific.).
 *
 * Then if many threads are contending for the lock, the lock will always go to
 * the thread with the lowest priority ID.
 *
 * Furthermore, the lock has a parameterized "stickiness". In other words, when
 * a thread releases the lock, it is granted a certain time window in which
 * if it (or a lower ID thread) returns to acquire the lock, it will be able to
 * get it immediately. This "stickness" essentially parameterizes the
 * CPU-utilization Disk-utilization balance. The more IO bound a task is, the
 * better it is for it to be executed on just one CPU. This threshold attempts
 * to self-tune by trying to maximize the total throughput of the lock.
 * (i.e.. maximising lock acquisitions per second). This is done by gradually
 * adapting the sleep interval: i.e. if increasing it increases throughput,
 * it gets increases, and vice versa.
 */
class unfair_lock {
 public:
  void lock();
  void unlock();
 private:
  turi::mutex m_lock;
  turi::mutex m_internal_lock;
  volatile bool m_lock_acquired = false;
  std::map<size_t, turi::conditional*> m_cond;
  // autotuning parameters for the lock stickness
  size_t m_previous_owner_priority = 0;
  int m_previous_sleep_interval = 0;
  double m_previous_time_for_epoch = 0;
  int m_current_sleep_interval = 50;
  double m_time_for_epoch = 0;
  size_t m_epoch_counter = 0;
  bool m_initial = true;
  timer m_ti;
};

/// \}
//
} // turicreate
#endif
