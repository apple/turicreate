/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_BLOCKING_QUEUE_HPP
#define TURI_BLOCKING_QUEUE_HPP



#include <list>
#include <deque>
#include <core/parallel/pthread_tools.hpp>
#include <core/random/random.hpp>

namespace turi {

   /**
    * \ingroup util
    * \brief Implements a blocking queue useful for producer/consumer models
    */
  template<typename T>
  class blocking_queue {
  protected:

    typedef typename std::deque<T> queue_type;

    bool m_alive;
    queue_type m_queue;
    mutex m_mutex;
    conditional m_conditional;
    conditional m_empty_conditional;

    volatile uint16_t sleeping;
    volatile uint16_t sleeping_on_empty;


  public:

    //! creates a blocking queue
    blocking_queue() : m_alive(true),sleeping(0),sleeping_on_empty(0) { }

    //! Add an element to the blocking queue
    inline void enqueue(const T& elem) {
      m_mutex.lock();
      m_queue.push_back(elem);
      // Signal threads waiting on the queue
      if (sleeping) m_conditional.signal();
      m_mutex.unlock();
    }

    //! Add an element to the blocking queue
    inline void enqueue_to_head(const T& elem) {
      m_mutex.lock();
      m_queue.push_front(elem);
      // Signal threads waiting on the queue
      if (sleeping) m_conditional.signal();
      m_mutex.unlock();
    }



    inline void enqueue_conditional_signal(const T& elem, size_t signal_at_size) {
      m_mutex.lock();
      m_queue.push_back(elem);
      // Signal threads waiting on the queue
      if (sleeping && m_queue.size() >= signal_at_size) m_conditional.signal();
      m_mutex.unlock();
    }


    bool empty_unsafe() {
      return m_queue.empty();
    }

    void begin_critical_section() {
      m_mutex.lock();
    }


    bool is_alive() {
      return m_alive;
    }

    void swap(queue_type &q) {
      m_mutex.lock();
      q.swap(m_queue);
      if (m_queue.empty() && sleeping_on_empty) {
        m_empty_conditional.signal();
      }
      m_mutex.unlock();
    }

    inline std::pair<T, bool> try_dequeue_in_critical_section() {
      T elem = T();
      // Wait while the queue is empty and this queue is alive
      if (m_queue.empty() || m_alive == false) {
        return std::make_pair(elem, false);
      }
      else {
        elem = m_queue.front();
        m_queue.pop_front();
        if (m_queue.empty() && sleeping_on_empty) {
          m_empty_conditional.signal();
        }
        return std::make_pair(elem, true);
      }
    }

    void end_critical_section() {
     m_mutex.unlock();
    }


    inline std::pair<T, bool> dequeue_and_begin_critical_section_on_success() {
      m_mutex.lock();
      T elem = T();
      bool success = false;
      // Wait while the queue is empty and this queue is alive
      while(m_queue.empty() && m_alive) {
        sleeping++;
        m_conditional.wait(m_mutex);
        sleeping--;
      }
      // An element has been added or a signal was raised
      if(!m_queue.empty()) {
        success = true;
        elem = m_queue.front();
        m_queue.pop_front();
        if (m_queue.empty() && sleeping_on_empty) {
          m_empty_conditional.signal();
        }
      }
      if (!success) m_mutex.unlock();
      return std::make_pair(elem, success);
    }

    /// Returns immediately of queue size is >= immedeiate_size
    /// Otherwise, it will poll over 'ns' nanoseconds or on a signal
    /// until queue is not empty.
    inline bool timed_wait_for_data(size_t ns, size_t immediate_size) {
      m_mutex.lock();
      bool success = false;
      // Wait while the queue is empty and this queue is alive
      if (m_queue.size() < immediate_size) {
        do {
          sleeping++;
          m_conditional.timedwait_ns(m_mutex, ns);
          sleeping--;
        }while(m_queue.empty() && m_alive);
      }
      // An element has been added or a signal was raised
      if(!m_queue.empty()) {
        success = true;
      }
      m_mutex.unlock();

      return success;
    }


    /// Returns immediately of queue size is >= immedeiate_size
    /// Otherwise, it will poll over 'ns' nanoseconds or on a signal
    /// until queue is not empty.
    inline bool try_timed_wait_for_data(size_t ns, size_t immediate_size) {
      m_mutex.lock();
      bool success = false;
      // Wait while the queue is empty and this queue is alive
      if (m_queue.size() < immediate_size) {
        if (m_queue.empty() && m_alive) {
          sleeping++;
          m_conditional.timedwait_ns(m_mutex, ns);
          sleeping--;
        }
      }
      // An element has been added or a signal was raised
      if(!m_queue.empty()) {
        success = true;
      }
      m_mutex.unlock();

      return success;
    }



    inline bool wait_for_data() {

      m_mutex.lock();
      bool success = false;
      // Wait while the queue is empty and this queue is alive
      while(m_queue.empty() && m_alive) {
        sleeping++;
        m_conditional.wait(m_mutex);
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
        m_conditional.wait(m_mutex);
        sleeping--;
      }
      // An element has been added or a signal was raised
      if(!m_queue.empty()) {
        success = true;
        elem = m_queue.front();
        m_queue.pop_front();
        if (m_queue.empty() && sleeping_on_empty) {
          m_empty_conditional.signal();
        }
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
        if (m_queue.empty() && sleeping_on_empty) {
          m_empty_conditional.signal();
        }
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
      m_conditional.broadcast();
      m_empty_conditional.broadcast();
      m_mutex.unlock();
    }

    /**
      Resumes operation of the blocking_queue. Future calls to
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
     * The conceptual "reverse" of dequeue().
     * This function will block until the queue becomes empty, or
     * until stop_blocking() is called.
     * Returns true on success.
     * Returns false if the queue is no longer alive
    */
    bool wait_until_empty() {
      m_mutex.lock();
      // if the queue still has elements in it while I am still alive, wait
      while (m_queue.empty() == false && m_alive == true) {
        sleeping_on_empty++;
        m_empty_conditional.wait(m_mutex);
        sleeping_on_empty--;
      }
      m_mutex.unlock();
      // if I am alive, the queue must be empty. i.e. success
      // otherwise I am dead
      return m_alive;
    }

    /**
     * Causes any threads currently blocking on a dequeue to wake up
     * and evaluate the state of the queue. If the queue is empty,
     * the threads will return back to sleep immediately. If the queue
     * is destroyed through stop_blocking, all threads will return.
     */
    void broadcast() {
      m_mutex.lock();
      m_conditional.broadcast();
      m_mutex.unlock();
    }



    /**
     * Causes any threads blocking on "wait_until_empty()" to wake
     * up and evaluate the state of the queue. If the queue is not empty,
     * the threads will return back to sleep immediately. If the queue
     * is empty, all threads will return.
    */
    void broadcast_blocking_empty() {
      m_mutex.lock();
      m_empty_conditional.broadcast();
      m_mutex.unlock();
    }


    ~blocking_queue() {
      m_alive = false;
      broadcast();
      broadcast_blocking_empty();
    }
  }; // end of blocking_queue class


} // end of namespace turi


#endif
