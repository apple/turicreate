/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FIBER_CONDITIONAL_HPP
#define TURI_FIBER_CONDITIONAL_HPP
#include <vector>
#include <queue>
#include <fiber/fiber_control.hpp>
namespace turi {

/**
 * \ingroup util
 * Wrapper around pthread's condition variable, that can work with both
 * fibers and threads simultaneously, but at a cost of much greater memory 
 * requirements.
 *
 * Limitations.
 *  - Does not support timed wait
 *  - threads and fibers are not queued perfectly. fibers are preferentially 
 *    signaled.
 *
 * \ingroup threading
 */
class fiber_conditional {
 private:
   mutable pthread_cond_t  m_cond;
   mutex lock;
   mutable std::queue<size_t> fibers; // used to hold the fibers that are waiting here

   // not copyable
   void operator=(const fiber_conditional& m) { }

 public:
   fiber_conditional() {
     int error = pthread_cond_init(&m_cond, NULL);
     ASSERT_TRUE(!error);
   }

   /** Copy constructor which does not copy. Do not use!
     Required for compatibility with some STL implementations (LLVM).
     which use the copy constructor for vector resize, 
     rather than the standard constructor.    */
   fiber_conditional(const fiber_conditional &) {
     int error = pthread_cond_init(&m_cond, NULL);
     ASSERT_TRUE(!error);
   }



   /// Waits on condition. The mutex must already be acquired. Caller
   /// must be careful about spurious wakes.
   inline void wait(const mutex& mut) const {
     size_t tid = fiber_control::get_tid();
     if (tid > 0) {
       lock.lock();
       fibers.push(tid);
       lock.unlock();
       fiber_control::deschedule_self(&mut.m_mut);
       mut.lock();
     } else {
       int error = pthread_cond_wait(&m_cond, &mut.m_mut);
       ASSERT_TRUE(!error);
      }
   }
   /// Signals one waiting thread to wake up
   inline void signal() const {
     if (!fibers.empty()) {
       lock.lock();
       if (!fibers.empty()) {
         size_t tid = fibers.front();
         fibers.pop();
         lock.unlock();
         fiber_control::schedule_tid(tid);
         return;
       }
       lock.unlock();
     }
     int error = pthread_cond_signal(&m_cond);
     ASSERT_TRUE(!error);
   }
   /// Wakes up all waiting threads
   inline void broadcast() const {
     lock.lock();
     while (!fibers.empty()) {
       size_t tid = fibers.front();
       fibers.pop();
       fiber_control::schedule_tid(tid);
     }
     lock.unlock();
     int error = pthread_cond_broadcast(&m_cond);
     ASSERT_TRUE(!error);
   }

   ~fiber_conditional() {
     ASSERT_EQ(fibers.size(), 0);
     int error = pthread_cond_destroy(&m_cond);
     ASSERT_TRUE(!error);
   }
}; 
}
#endif
