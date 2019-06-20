/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_MUTEX_HPP
#define TURI_MUTEX_HPP

#include <core/parallel/pthread_h.h>
#include <core/logging/assertions.hpp>
#include <mutex>

namespace turi {

  /**
   * \ingroup threading
   *
   * Simple wrapper around pthread's mutex.
   * Before you use, see \ref parallel_object_intricacies.
   *
   * Windows recursive mutex are annoyingly recursive.
   * We need to prevent recursive locks. We do this by associating an
   * addition boolean "locked" to the mutex.
   * Hence in the event of a double lock, on Windows the behavior is slightly
   * different. On Linux/Mac this will trigger a deadlock. On Windows,
   * this will trigger an assertion failure.
   */
  class mutex {
  public:
    // mutable not actually needed
    mutable pthread_mutex_t m_mut;
#ifdef _WIN32
    mutable volatile bool locked = false;
#endif
    /// constructs a mutex
    mutex() {
      int error = pthread_mutex_init(&m_mut, NULL);
      ASSERT_MSG(!error, "Mutex create error %d", error);
    }
    /** Copy constructor which does not copy. Do not use!
        Required for compatibility with some STL implementations (LLVM).
        which use the copy constructor for vector resize,
        rather than the standard constructor.    */
    mutex(const mutex&) {
      int error = pthread_mutex_init(&m_mut, NULL);
      ASSERT_MSG(!error, "Mutex create error %d", error);
    }

    ~mutex() noexcept {
      int error = pthread_mutex_destroy( &m_mut );
      if(error) {
        try {
        std::cerr << "Mutex destroy error " << error << std::endl;
        } catch (...) {
        }
        abort();
      }
    }

    // not copyable
    void operator=(const mutex& m) { }

    /// Acquires a lock on the mutex
    inline void lock() const {
      TURI_ATTRIBUTE_UNUSED_NDEBUG int error = pthread_mutex_lock( &m_mut  );
      DASSERT_MSG(!error, "Mutex lock error %d", error);
#ifdef _WIN32
      DASSERT_TRUE(!locked);
      locked = true;
#endif
    }
    /// Releases a lock on the mutex
    inline void unlock() const {
#ifdef _WIN32
      locked = false;
#endif

      TURI_ATTRIBUTE_UNUSED_NDEBUG int error = pthread_mutex_unlock( &m_mut );
      DASSERT_MSG(!error, "Mutex unlock error %d", error);
    }
    /// Non-blocking attempt to acquire a lock on the mutex
    inline bool try_lock() const {
#ifdef _WIN32
      if (locked) return false;
#endif
      return pthread_mutex_trylock( &m_mut ) == 0;
    }
    friend class conditional;
  }; // End of Mutex



  /**
   * \ingroup threading
   *
   * Simple wrapper around pthread's recursive mutex.
   * Before you use, see \ref parallel_object_intricacies.
   */
  class recursive_mutex {
  public:
    // mutable not actually needed
    mutable pthread_mutex_t m_mut;
    /// constructs a mutex
    recursive_mutex() {
      pthread_mutexattr_t attr;
      int error = pthread_mutexattr_init(&attr);
      ASSERT_TRUE(!error);
      error = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
      ASSERT_TRUE(!error);
      error = pthread_mutex_init(&m_mut, &attr);
      ASSERT_TRUE(!error);
      pthread_mutexattr_destroy(&attr);
    }
    /** Copy constructor which does not copy. Do not use!
        Required for compatibility with some STL implementations (LLVM).
        which use the copy constructor for vector resize,
        rather than the standard constructor.    */
    recursive_mutex(const recursive_mutex&) {
      pthread_mutexattr_t attr;
      int error = pthread_mutexattr_init(&attr);
      ASSERT_TRUE(!error);
      error = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
      ASSERT_TRUE(!error);
      error = pthread_mutex_init(&m_mut, &attr);
      ASSERT_TRUE(!error);
      pthread_mutexattr_destroy(&attr);
    }

    ~recursive_mutex(){
      TURI_ATTRIBUTE_UNUSED_NDEBUG int error = pthread_mutex_destroy( &m_mut );
      DASSERT_TRUE(!error);
    }

    // not copyable
    void operator=(const recursive_mutex& m) { }

    /// Acquires a lock on the mutex
    inline void lock() const {
      TURI_ATTRIBUTE_UNUSED_NDEBUG int error = pthread_mutex_lock( &m_mut  );
      // if (error) std::cerr << "mutex.lock() error: " << error << std::endl;
      DASSERT_TRUE(!error);
    }
    /// Releases a lock on the mutex
    inline void unlock() const {
      TURI_ATTRIBUTE_UNUSED_NDEBUG int error = pthread_mutex_unlock( &m_mut );
      DASSERT_TRUE(!error);
    }
    /// Non-blocking attempt to acquire a lock on the mutex
    inline bool try_lock() const {
      return pthread_mutex_trylock( &m_mut ) == 0;
    }
    friend class conditional;
  }; // End of Mutex




} // end of turi namespace

#endif
