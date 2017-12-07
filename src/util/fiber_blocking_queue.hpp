/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FIBER_BLOCKING_QUEUE_HPP
#define TURI_FIBER_BLOCKING_QUEUE_HPP



#include <list>
#include <deque>
#include <queue>
#include <parallel/pthread_tools.hpp>
#include <fiber/fiber_control.hpp>
#include <random/random.hpp>

namespace turi {

   /**
    * \ingroup util
    * \brief Implements a blocking queue useful for producer/consumer models.
    * Similar to the \ref blocking_queue, but requires all threads waiting on
    * the queue to be fibers. Also only supports the basic wait on dequeue
    * operation, and not the other timed_wait, or wait_until_empty operations
    * supported by the \ref blocking_queue
    */
  template<typename T>
  class fiber_blocking_queue {
  protected:

    typedef typename std::deque<T> queue_type;

    bool m_alive;
    queue_type m_queue;
    mutex m_mutex;
    std::queue<size_t> fiber_queue;

    volatile uint16_t sleeping;


  public:

    //! creates a blocking queue
    fiber_blocking_queue() : m_alive(true),sleeping(0) { }

    void wake_a_fiber() {
      if (!fiber_queue.empty()) {
        size_t fiber_id = fiber_queue.front();
        fiber_queue.pop();
        fiber_control::schedule_tid(fiber_id);
      }
    }

    void wake_all_fibers() {
      while(!fiber_queue.empty()) {
        size_t fiber_id = fiber_queue.front();
        fiber_queue.pop();
        fiber_control::schedule_tid(fiber_id);
      }
    }

    void fiber_sleep() {
      fiber_queue.push(fiber_control::get_tid());
      fiber_control::deschedule_self(&m_mutex.m_mut);
      m_mutex.lock();
    }

    //! Add an element to the blocking queue
    inline void enqueue(const T& elem, bool wake_consumer = true) {
      m_mutex.lock();
      m_queue.push_back(elem);
      // Signal threads waiting on the queue
      if (wake_consumer && sleeping) wake_a_fiber();
      m_mutex.unlock();
    }

    //! Add an element to the blocking queue
    inline void enqueue_to_head(const T& elem) {
      m_mutex.lock();
      m_queue.push_front(elem);
      // Signal threads waiting on the queue
      if (sleeping) wake_a_fiber();
      m_mutex.unlock();
    }


    bool empty_unsafe() {
      return m_queue.empty();
    }

    bool is_alive() {
      return m_alive;
    }

    void swap(queue_type &q) {
      m_mutex.lock();
      q.swap(m_queue);
      m_mutex.unlock();
    }


    inline bool wait_for_data() {
      m_mutex.lock();
      bool success = false;
      // Wait while the queue is empty and this queue is alive
      while(m_queue.empty() && m_alive) {
        sleeping++;
        fiber_sleep();
        sleeping--;
      }
      // An element has been added or a signal was raised
      if(!m_queue.empty()) {
        success = true;
      }
      m_mutex.unlock();

      return success;
    }


    /**
     * Blocks until an element is available in the queue
     * or until stop_blocking() is called.
     * The return value is a pair of <T value, bool success>
     * If "success" if set, then "value" is valid and
     * is an element popped from the queue.
     * If "success" is false, stop_blocking() was called
     * and the queue has been destroyed.
     */
    inline std::pair<T, bool> dequeue() {

      m_mutex.lock();
      T elem = T();
      bool success = false;
      // Wait while the queue is empty and this queue is alive
      while(m_queue.empty() && m_alive) {
        sleeping++;
        fiber_sleep();
        sleeping--;
      }
      // An element has been added or a signal was raised
      if(!m_queue.empty()) {
        success = true;
        elem = m_queue.front();
        m_queue.pop_front();
      }
      m_mutex.unlock();

      return std::make_pair(elem, success);
    }

    /**
    * Returns an element if the queue has an entry.
    * returns [item, false] otherwise.
    */
    inline std::pair<T, bool> try_dequeue() {
      if (m_queue.empty() || m_alive == false) return std::make_pair(T(), false);
      m_mutex.lock();
      T elem = T();
      // Wait while the queue is empty and this queue is alive
      if (m_queue.empty() || m_alive == false) {
        m_mutex.unlock();
        return std::make_pair(elem, false);
      }
      else {
        elem = m_queue.front();
        m_queue.pop_front();
      }
      m_mutex.unlock();

      return std::make_pair(elem, true);
    }

    //! Returns true if the queue is empty
    inline bool empty() {
      m_mutex.lock();
      bool res = m_queue.empty();
      m_mutex.unlock();
      return res;
    }

    /** Wakes up all threads waiting on the queue whether
        or not an element is available. Once this function is called,
        all existing and future dequeue operations will return with failure.
        Note that there could be elements remaining in the queue after
        stop_blocking() is called.
    */
    inline void stop_blocking() {
      m_mutex.lock();
      m_alive = false;
      wake_all_fibers();
      m_mutex.unlock();
    }

    /**
      Resumes operation of the fiber_blocking_queue. Future calls to
      dequeue will proceed as normal.
    */
    inline void start_blocking() {
      m_mutex.lock();
      m_alive = true;
      m_mutex.unlock();
    }

    //! get the current size of the queue
    inline size_t size() {
      return m_queue.size();
    }


    /**
     * Causes any threads currently blocking on a dequeue to wake up
     * and evaluate the state of the queue. If the queue is empty,
     * the threads will return back to sleep immediately. If the queue
     * is destroyed through stop_blocking, all threads will return.
     */
    void broadcast() {
      m_mutex.lock();
      wake_all_fibers();
      m_mutex.unlock();
    }


    ~fiber_blocking_queue() {
      m_alive = false;
      broadcast();
    }
  }; // end of fiber_blocking_queue class


} // end of namespace turi

#endif
