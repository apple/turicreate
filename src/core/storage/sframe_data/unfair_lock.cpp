/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <algorithm>
#include <core/parallel/atomic.hpp>
#include <core/system/platform/timer//timer.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/storage/sframe_data/unfair_lock.hpp>
namespace turi {
namespace {
// Some magic to ensure that keys are created at program startup =========>
constexpr size_t LOCKS_PER_EPOCH = 100;
constexpr size_t DELTA = 5;

/**
 * Ther thread local data required for the unfair lock
 */
struct unfair_lock_priority {
  // the priority for this thread. Lower number is higher priority
  size_t priority;
  // The condition variable this thread will wait on. Required for the
  // unfair lock implementation
  conditional cond;
};
// atomic counter so each thread has a different priority
static atomic<size_t> priority_ctr;

// deleter for the thread local data
void destroy_tls_data(void* ptr) {
  unfair_lock_priority* tsd =
      reinterpret_cast<unfair_lock_priority*>(ptr);
  if(tsd != NULL) {
    delete tsd;
  }
}


// force creation of the thread local datastructures before main starts.
struct thread_keys {
  pthread_key_t UNFAIR_LOCK_PRIORITY;
  thread_keys() {
    pthread_key_create(&UNFAIR_LOCK_PRIORITY, destroy_tls_data);
  }
};
// This function is to be called prior to any thread starting
// execution to ensure that the static member keys is constructed
// prior to any threads launching
static pthread_key_t get_priority_tls_key_id() {
  static thread_keys keys;
  return keys.UNFAIR_LOCK_PRIORITY;
}
static TURI_ATTRIBUTE_UNUSED pthread_key_t __unused_init_keys__(get_priority_tls_key_id());


/**
 * Gets the thread local unfair_lock_priority structure
 */
static unfair_lock_priority* get_unfair_lock_priority() {
  unfair_lock_priority* priority =
      (unfair_lock_priority*)pthread_getspecific(get_priority_tls_key_id());
  if (priority == NULL) {
    // not set. create it
    priority = new unfair_lock_priority();
    priority->priority = priority_ctr.inc();
    pthread_setspecific(get_priority_tls_key_id(), (void*)(priority));
  }
  return priority;
} // end create the thread specific data

} // end anonymous namespace


void unfair_lock::lock() {
  unfair_lock_priority* priority = get_unfair_lock_priority();
  m_internal_lock.lock();
  if (m_lock_acquired ||
      (m_cond.size() > 0 && m_cond.begin()->first < priority->priority)) {
    // slow path:
    //   lock was acquired --> I need to wait.
    //   or lock was not acquired, but I am not the lowest priority.
    //   i.e. some other thread should be waking up --> I need to wait
    m_cond[priority->priority] = &(priority->cond);
    while(m_lock_acquired ||
          (m_cond.size() > 0 && m_cond.begin()->first < priority->priority)) {
      priority->cond.wait(m_internal_lock);
      if (priority->priority > m_previous_owner_priority) {
        // lock stickiness
        m_internal_lock.unlock();
        timer::sleep_ms(m_current_sleep_interval);
        m_internal_lock.lock();
      }
    }
    m_cond.erase(priority->priority);
  } // else ... fast path .. lock was not acquired, and I am
  // the lowest priority, go for it and acquire the lcok
  m_lock.lock();
  m_lock_acquired = true;
  m_internal_lock.unlock();
}

void unfair_lock::unlock() {
  m_internal_lock.lock();
  // some funky magic.
  // Essentially how this works is that we record the amount of time required to
  // acquire LOCKS_PER_EPOCH locks. Then we adapt the sleep interval
  // (the stickiness of the lock)
  ++m_epoch_counter;
  if (m_epoch_counter == LOCKS_PER_EPOCH) {
    m_time_for_epoch = m_ti.current_time();
    if (!m_initial && m_time_for_epoch > m_previous_time_for_epoch + 0.5) {
      //excessive delay. reset counters.
      m_previous_sleep_interval = 0;
      m_previous_time_for_epoch = 0;
      m_current_sleep_interval = 50;
      m_time_for_epoch = 0;
      m_epoch_counter = 0;
      m_initial = true;
    } else {
      m_initial = false;
      int m_new_sleep_interval = m_current_sleep_interval;
      if (m_previous_time_for_epoch > m_time_for_epoch) {
        // current sleep interval is better. keep moving in this direction
        m_new_sleep_interval += (m_current_sleep_interval - m_previous_sleep_interval);
      }
      else if (m_previous_time_for_epoch < m_time_for_epoch) {
        // current sleep interval is worse. move back
        m_new_sleep_interval -= (m_current_sleep_interval - m_previous_sleep_interval);
      }
      // can't go negative.
      if (m_new_sleep_interval < 0) m_new_sleep_interval = 0;
      if (m_new_sleep_interval > 100) m_new_sleep_interval = 100;
      // we can't stay stationary
      if (m_new_sleep_interval == m_current_sleep_interval) {
        // move around a bit.
        m_new_sleep_interval += DELTA;
      }
    /*
     * logstream(LOG_INFO) << m_previous_time_for_epoch << ": " << m_previous_sleep_interval << "\t"
     *                     << m_time_for_epoch << ": " << m_current_sleep_interval << std::endl;
     * logstream(LOG_INFO) << "New adaptive sleep interval: " << m_new_sleep_interval << std::endl;
     */
      // reset counters
      m_previous_sleep_interval = m_current_sleep_interval;
      m_previous_time_for_epoch = m_time_for_epoch;
      m_epoch_counter = 0;
      m_current_sleep_interval = m_new_sleep_interval;

    }
    m_ti.start();
  }
  m_lock.unlock();
  // if there are threads waiting, wake up the lowest priority number
  if (m_cond.size() > 0) {
    m_cond.begin()->second->signal();
  }
  m_previous_owner_priority = get_unfair_lock_priority()->priority;
  m_lock_acquired = false;
  m_internal_lock.unlock();
}
} // namespace turi
