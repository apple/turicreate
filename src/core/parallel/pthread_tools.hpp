/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_PTHREAD_TOOLS_HPP
#define TURI_PTHREAD_TOOLS_HPP


#include <cstdlib>
#include <core/parallel/pthread_h.h>
#include <semaphore.h>
#include <sched.h>
#include <signal.h>
#include <sys/time.h>
#include <vector>
#include <list>
#include <queue>
#include <iostream>
#include <boost/function.hpp>
#include <core/logging/assertions.hpp>
#include <core/parallel/atomic_ops.hpp>
#include <core/util/branch_hints.hpp>
#include <boost/unordered_map.hpp>
#undef _POSIX_SPIN_LOCKS
#define _POSIX_SPIN_LOCKS -1

#ifdef _WIN32
typedef long suseconds_t;
#endif

#include <core/parallel/mutex.hpp>
#include <core/util/any.hpp>


namespace turi {



#if _POSIX_SPIN_LOCKS >= 0
  /**
   * \ingroup threading
   *
   * Wrapper around pthread's spinlock.
   *
   * Before you use, see \ref parallel_object_intricacies.
   */
  class spinlock {
  private:
    // mutable not actually needed
    mutable pthread_spinlock_t m_spin;
  public:
    /// constructs a spinlock
    spinlock () {
      int error = pthread_spin_init(&m_spin, PTHREAD_PROCESS_PRIVATE);
      ASSERT_TRUE(!error);
    }

    /** Copy constructor which does not copy. Do not use!
        Required for compatibility with some STL implementations (LLVM).
        which use the copy constructor for vector resize,
        rather than the standard constructor.    */
    spinlock(const spinlock&) {
      int error = pthread_spin_init(&m_spin, PTHREAD_PROCESS_PRIVATE);
      ASSERT_TRUE(!error);
    }

    // not copyable
    void operator=(const spinlock& m) { }


    /// Acquires a lock on the spinlock
    inline void lock() const {
      int error = pthread_spin_lock( &m_spin  );
      ASSERT_TRUE(!error);
    }
    /// Releases a lock on the spinlock
    inline void unlock() const {
      int error = pthread_spin_unlock( &m_spin );
      ASSERT_TRUE(!error);
    }
    /// Non-blocking attempt to acquire a lock on the spinlock
    inline bool try_lock() const {
      return pthread_spin_trylock( &m_spin ) == 0;
    }
    ~spinlock(){
      int error = pthread_spin_destroy( &m_spin );
      ASSERT_TRUE(!error);
    }
    friend class conditional;
  }; // End of spinlock
#define SPINLOCK_SUPPORTED 1
#else
  //! if spinlock not supported, it is typedef it to a mutex.
  typedef mutex spinlock;
#define SPINLOCK_SUPPORTED 0
#endif


  /**
   * \ingroup threading
   *If pthread spinlock is not implemented,
   * this provides a simple alternate spin lock implementation.
   *
   * Before you use, see \ref parallel_object_intricacies.
   */
  class simple_spinlock {
  private:
    // mutable not actually needed
    mutable volatile char spinner;
  public:
    /// constructs a spinlock
    simple_spinlock () {
      spinner = 0;
    }

    /** Copy constructor which does not copy. Do not use!
    Required for compatibility with some STL implementations (LLVM).
    which use the copy constructor for vector resize,
    rather than the standard constructor.    */
    simple_spinlock(const simple_spinlock&) {
      spinner = 0;
    }

    // not copyable
    void operator=(const simple_spinlock& m) { }


    /// Acquires a lock on the spinlock
    inline void lock() const {
      while(spinner == 1 || __sync_lock_test_and_set(&spinner, 1));
    }
    /// Releases a lock on the spinlock
    inline void unlock() const {
      __sync_synchronize();
      spinner = 0;
    }
    /// Non-blocking attempt to acquire a lock on the spinlock
    inline bool try_lock() const {
      return (__sync_lock_test_and_set(&spinner, 1) == 0);
    }
    ~simple_spinlock(){
      ASSERT_TRUE(spinner == 0);
    }
  };


  /**
   * \ingroup threading
   *If pthread spinlock is not implemented,
   * this provides a simple alternate spin lock implementation.
   *
   * Before you use, see \ref parallel_object_intricacies.
   */
  class padded_simple_spinlock {
  private:
    // mutable not actually needed
    mutable volatile char spinner;
    // char padding[63];
  public:
    /// constructs a spinlock
    padded_simple_spinlock () {
      spinner = 0;
    }

    /** Copy constructor which does not copy. Do not use!
    Required for compatibility with some STL implementations (LLVM).
    which use the copy constructor for vector resize,
    rather than the standard constructor.    */
    padded_simple_spinlock(const padded_simple_spinlock&) {
      spinner = 0;
    }

    // not copyable
    void operator=(const padded_simple_spinlock& m) { }


    /// Acquires a lock on the spinlock
    inline void lock() const {
      while(spinner == 1 || __sync_lock_test_and_set(&spinner, 1));
    }
    /// Releases a lock on the spinlock
    inline void unlock() const {
      __sync_synchronize();
      spinner = 0;
    }
    /// Non-blocking attempt to acquire a lock on the spinlock
    inline bool try_lock() const {
      return (__sync_lock_test_and_set(&spinner, 1) == 0);
    }
    ~padded_simple_spinlock(){
      ASSERT_TRUE(spinner == 0);
    }
  };




  /**
   * \ingroup threading
   * Wrapper around pthread's condition variable
   *
   * Before you use, see \ref parallel_object_intricacies.
   */
  class conditional {
  private:
    mutable pthread_cond_t  m_cond;

  public:
    conditional() {
      int error = pthread_cond_init(&m_cond, NULL);
      ASSERT_TRUE(!error);
    }

    /** Copy constructor which does not copy. Do not use!
        Required for compatibility with some STL implementations (LLVM).
        which use the copy constructor for vector resize,
        rather than the standard constructor.    */
    conditional(const conditional &) {
      int error = pthread_cond_init(&m_cond, NULL);
      ASSERT_TRUE(!error);
    }

    // not copyable
    void operator=(const conditional& m) { }


    /// Waits on condition. The mutex must already be acquired. Caller
    /// must be careful about spurious wakes.
    inline void wait(const mutex& mut) const {
#ifdef _WIN32
      mut.locked = false;
      int error = pthread_cond_wait(&m_cond, &mut.m_mut);
      mut.locked = true;
#else
      int error = pthread_cond_wait(&m_cond, &mut.m_mut);
#endif
      ASSERT_MSG(!error, "Condition variable wait error %d", error);
    }
    /// Waits on condition. The mutex must already be acquired. Caller
    /// must be careful about spurious wakes.
    inline void wait(std::unique_lock<mutex>& mut) const {
      // take over the pointer
      auto lock_ptr = mut.mutex();
      mut.release();

      wait(*lock_ptr);
      // put it back into the unique lock
      std::unique_lock<mutex> retlock(*lock_ptr, std::adopt_lock);
      mut.swap(retlock);
    }
    /// Waits on condition. The mutex must already be acquired.
    /// Returns only when predicate evaulates to true
    template <typename Predicate>
    inline void wait(const mutex& mut, Predicate pred) const {
      while (!pred()) wait(mut);
    }
    /// Waits on condition. The mutex must already be acquired.
    /// Returns only when predicate evaulates to true
    template <typename Predicate>
    inline void wait(std::unique_lock<mutex>& mut, Predicate pred) const {
      while (!pred()) wait(mut);
    }

    /// Wait till a timeout time
    inline int timedwait(const mutex& mut, const struct timespec& timeout) const {
#ifdef _WIN32
      mut.locked = false;
      int ret = pthread_cond_timedwait(&m_cond, &mut.m_mut, &timeout);
      mut.locked = true;
#else
      int ret = pthread_cond_timedwait(&m_cond, &mut.m_mut, &timeout);
#endif
      return ret;
    }

    /// Like wait() but with a time limit of "sec" seconds
    inline int timedwait(const mutex& mut, size_t sec) const {
      struct timespec timeout;
      struct timeval tv;
      struct timezone tz;
      gettimeofday(&tv, &tz);
      timeout.tv_nsec = tv.tv_usec * 1000;
      timeout.tv_sec = tv.tv_sec + (time_t)sec;
      return timedwait(mut, timeout);
    }

    /// Like wait() but with a time limit of "ms" milliseconds
    inline int timedwait_ms(const mutex& mut, size_t ms) const {
      struct timespec timeout;
      struct timeval tv;
      gettimeofday(&tv, NULL);
      // convert ms to s and ns
      size_t s = ms / 1000;
      ms = ms % 1000;
      size_t ns = ms * 1000000;
      // convert timeval to timespec
      timeout.tv_nsec = tv.tv_usec * 1000;
      timeout.tv_sec = tv.tv_sec;

      // add the time
      timeout.tv_nsec += (suseconds_t)ns;
      timeout.tv_sec += (time_t)s;
      // shift the nsec to sec if overflow
      if (timeout.tv_nsec > 1000000000) {
        timeout.tv_sec ++;
        timeout.tv_nsec -= 1000000000;
      }
      return timedwait(mut, timeout);
    }
    /// Like wait() but with a time limit of "ns" nanoseconds
    inline int timedwait_ns(const mutex& mut, size_t ns) const {
      struct timespec timeout;
      struct timeval tv;
      gettimeofday(&tv, NULL);
      assert(ns > 0);
      // convert ns to s and ns
      size_t s = ns / 1000000;
      ns = ns % 1000000;

      // convert timeval to timespec
      timeout.tv_nsec = tv.tv_usec * 1000;
      timeout.tv_sec = tv.tv_sec;

      // add the time
      timeout.tv_nsec += (suseconds_t)ns;
      timeout.tv_sec += (time_t)s;
      // shift the nsec to sec if overflow
      if (timeout.tv_nsec > 1000000000) {
        timeout.tv_sec ++;
        timeout.tv_nsec -= 1000000000;
      }
      return timedwait(mut, timeout);
    }

    /// Like wait() but with a time limit of "sec" seconds
    inline int timedwait(std::unique_lock<mutex>& mut, size_t sec) const {
      // take over the pointer
      auto lock_ptr = mut.mutex();
      mut.release();

      int ret = timedwait(*lock_ptr, sec);

      // put it back into the unique lock
      std::unique_lock<mutex> retlock(*lock_ptr, std::adopt_lock);
      mut.swap(retlock);
      return ret;
    }
    /// Like wait() but with a time limit of "ms" milliseconds
    inline int timedwait_ms(std::unique_lock<mutex>& mut, size_t ms) const {
      // take over the pointer
      auto lock_ptr = mut.mutex();
      mut.release();

      int ret = timedwait_ms(*mut.mutex(), ms);

      // put it back into the unique lock
      std::unique_lock<mutex> retlock(*lock_ptr, std::adopt_lock);
      mut.swap(retlock);
      return ret;
    }
    /// Like wait() but with a time limit of "ns" nanoseconds
    inline int timedwait_ns(std::unique_lock<mutex>& mut, size_t ns) const {
      // take over the pointer
      auto lock_ptr = mut.mutex();
      mut.release();

      int ret = timedwait_ns(*lock_ptr, ns);

      // put it back into the unique lock
      std::unique_lock<mutex> retlock(*lock_ptr, std::adopt_lock);
      mut.swap(retlock);
      return ret;
    }

    /// Signals one waiting thread to wake up
    inline void signal() const {
      int error = pthread_cond_signal(&m_cond);
      ASSERT_MSG(!error, "Condition variable signal error %d", error);
    }
    /// Signals one waiting thread to wake up. Synonym for signal()
    inline void notify_one() const {
      signal();
    }
    /// Wakes up all waiting threads
    inline void broadcast() const {
      int error = pthread_cond_broadcast(&m_cond);
      ASSERT_MSG(!error, "Condition variable broadcast error %d", error);
    }
    /// Synonym for broadcast
    inline void notify_all() const {
      broadcast();
    }
    ~conditional() noexcept {
      int error = pthread_cond_destroy(&m_cond);
      if (error) {
        try {
          std::cerr << "Condition variable destroy error " << error
                     << std::endl;
        } catch (...) {
        }
        abort();
      }
    }
  }; // End conditional


#ifdef __APPLE__
  /**
   * Custom implementation of a semaphore.
   *
   * Before you use, see \ref parallel_object_intricacies.
   */
  class semaphore {
  private:
    conditional cond;
    mutex mut;
    mutable volatile size_t semvalue;
    mutable volatile size_t waitercount;

  public:
    semaphore() {
      semvalue = 0;
      waitercount = 0;
    }
    /** Copy constructor which does not copy. Do not use!
        Required for compatibility with some STL implementations (LLVM).
        which use the copy constructor for vector resize,
        rather than the standard constructor.    */
    semaphore(const semaphore&) {
      semvalue = 0;
      waitercount = 0;
    }

    // not copyable
    void operator=(const semaphore& m) { }

    inline void post() const {
      mut.lock();
      if (waitercount > 0) {
        cond.signal();
      }
      semvalue++;
      mut.unlock();
    }
    inline void wait() const {
      mut.lock();
      waitercount++;
      while (semvalue == 0) {
        cond.wait(mut);
      }
      waitercount--;
      semvalue--;
      mut.unlock();
    }
    ~semaphore() {
      ASSERT_TRUE(waitercount == 0);
      ASSERT_TRUE(semvalue == 0);
    }
  }; // End semaphore
#else
  /**
   * Wrapper around pthread's semaphore
   *
   * Before you use, see \ref parallel_object_intricacies.
   */
  class semaphore {
  private:
    mutable sem_t  m_sem;

  public:
    semaphore() {
      int error = sem_init(&m_sem, 0,0);
      ASSERT_TRUE(!error);
    }

    /** Copy constructor with does not copy. Do not use!
        Required for compatibility with some STL implementations (LLVM).
        which use the copy constructor for vector resize,
        rather than the standard constructor.    */
    semaphore(const semaphore&) {
      int error = sem_init(&m_sem, 0,0);
      ASSERT_TRUE(!error);
    }

    // not copyable
    void operator=(const semaphore& m) { }

    inline void post() const {
      int error = sem_post(&m_sem);
      ASSERT_TRUE(!error);
    }
    inline void wait() const {
      int error = sem_wait(&m_sem);
      ASSERT_TRUE(!error);
    }
    ~semaphore() {
      int error = sem_destroy(&m_sem);
      ASSERT_TRUE(!error);
    }
  }; // End semaphore
#endif


#define atomic_xadd(P, V) __sync_fetch_and_add((P), (V))
#define cmpxchg(P, O, N) __sync_val_compare_and_swap((P), (O), (N))
#define atomic_inc(P) __sync_add_and_fetch((P), 1)
#define atomic_add(P, V) __sync_add_and_fetch((P), (V))
#define atomic_set_bit(P, V) __sync_or_and_fetch((P), 1<<(V))
#ifdef __arm64
#define cpu_relax() asm volatile("yield\n": : :"memory")
#else
#define cpu_relax() asm volatile("pause\n": : :"memory")
#endif

  /**
   * \class spinrwlock
   * rwlock built around "spinning"
   * source adapted from http://locklessinc.com/articles/locks/
   * "Scalable Reader-Writer Synchronization for Shared-Memory Multiprocessors"
   * John Mellor-Crummey and Michael Scott
   */
  class spinrwlock {

    union rwticket {
      unsigned u;
      unsigned short us;
      __extension__ struct {
        unsigned char write;
        unsigned char read;
        unsigned char users;
      } s;
    };
    mutable bool writing;
    mutable volatile rwticket l;
  public:
    spinrwlock() {
      memset(const_cast<rwticket*>(&l), 0, sizeof(rwticket));
    }
    inline void writelock() const {
      unsigned me = atomic_xadd(&l.u, (1<<16));
      unsigned char val = (unsigned char)(me >> 16);

      while (val != l.s.write) asm volatile("pause\n": : :"memory");
      writing = true;
    }

    inline void wrunlock() const{
      rwticket t = *const_cast<rwticket*>(&l);

      t.s.write++;
      t.s.read++;

      *(volatile unsigned short *) (&l) = t.us;
      writing = false;
      __asm("mfence");
    }

    inline void readlock() const {
      unsigned me = atomic_xadd(&l.u, (1<<16));
      unsigned char val = (unsigned char)(me >> 16);

      while (val != l.s.read) asm volatile("pause\n": : :"memory");
      l.s.read++;
    }

    inline void rdunlock() const {
      atomic_inc(&l.s.write);
    }

    inline void unlock() const {
      if (!writing) rdunlock();
      else wrunlock();
    }
  };



#define RW_WAIT_BIT 0
#define RW_WRITE_BIT 1
#define RW_READ_BIT 2

#define RW_WAIT 1
#define RW_WRITE 2
#define RW_READ 4

  struct spinrwlock2 {
    mutable unsigned int l;

    spinrwlock2():l(0) {}
    void writelock() const {
      while (1) {
        unsigned state = l;

        /* No readers or writers? */
        if (state < RW_WRITE)
        {
          /* Turn off RW_WAIT, and turn on RW_WRITE */
          if (cmpxchg(&l, state, RW_WRITE) == state) return;

          /* Someone else got there... time to wait */
          state = l;
        }

        /* Turn on writer wait bit */
        if (!(state & RW_WAIT)) atomic_set_bit(&l, RW_WAIT_BIT);

        /* Wait until can try to take the lock */
        while (l > RW_WAIT) cpu_relax();
      }
    }

    void wrunlock() const {
      atomic_add(&l, -RW_WRITE);
    }

    void readlock() const {
      while (1) {
        /* A writer exists? */
        while (l & (RW_WAIT | RW_WRITE)) cpu_relax();

        /* Try to get read lock */
        if (!(atomic_xadd(&l, RW_READ) & (RW_WAIT | RW_WRITE))) return;

        /* Undo */
        atomic_add(&l, -RW_READ);
      }
    }

    void rdunlock() const {
      atomic_add(&l, -RW_READ);
    }
  };

#undef atomic_xadd
#undef cmpxchg
#undef atomic_inc
#undef atomic_set_bit
#undef atomic_add
#undef RW_WAIT_BIT
#undef RW_WRITE_BIT
#undef RW_READ_BIT
#undef RW_WAIT
#undef RW_WRITE
#undef RW_READ


  /**
   * \class rwlock
   * Wrapper around pthread's rwlock
   *
   * Before you use, see \ref parallel_object_intricacies.
   */
  class rwlock {
  private:
    mutable pthread_rwlock_t m_rwlock;
   public:
    rwlock() {
      int error = pthread_rwlock_init(&m_rwlock, NULL);
      ASSERT_TRUE(!error);
    }
    ~rwlock() {
      int error = pthread_rwlock_destroy(&m_rwlock);
      ASSERT_TRUE(!error);
    }

    // not copyable
    void operator=(const rwlock& m) { }

    /**
     * \todo: Remove!
     *
     * Copy constructor which does not copy. Do not use!  Required for
     * compatibility with some STL implementations (LLVM).  which use
     * the copy constructor for vector resize, rather than the
     * standard constructor.  */
    rwlock(const rwlock &) {
      int error = pthread_rwlock_init(&m_rwlock, NULL);
      ASSERT_TRUE(!error);
    }

    inline void readlock() const {
      pthread_rwlock_rdlock(&m_rwlock);
      //ASSERT_TRUE(!error);
    }
    inline void writelock() const {
      pthread_rwlock_wrlock(&m_rwlock);
      //ASSERT_TRUE(!error);
    }
    inline bool try_readlock() const {
      return pthread_rwlock_tryrdlock(&m_rwlock) == 0;
    }
    inline bool try_writelock() const {
      return pthread_rwlock_trywrlock(&m_rwlock) == 0;
    }
    inline void unlock() const {
      pthread_rwlock_unlock(&m_rwlock);
      //ASSERT_TRUE(!error);
    }
    inline void rdunlock() const {
      unlock();
    }
    inline void wrunlock() const {
      unlock();
    }
  }; // End rwlock





  /**
   * \ingroup threading
   * This is a simple sense-reversing barrier implementation.
   * In addition to standard barrier functionality, this also
   * provides a "cancel" function which can be used to destroy
   * the barrier, releasing all threads stuck in the barrier.
   *
   * Before you use, see \ref parallel_object_intricacies.
   */
  class cancellable_barrier {
  private:
    turi::mutex mutex;
    turi::conditional conditional;
    mutable int needed;
    mutable int called;

    mutable bool barrier_sense;
    mutable bool barrier_release;
    bool alive;

    // not copyconstructible
    cancellable_barrier(const cancellable_barrier&) { }


  public:
    /// Construct a barrier which will only fall when numthreads enter
    cancellable_barrier(size_t numthreads) {
      needed = numthreads;
      called = 0;
      barrier_sense = false;
      barrier_release = true;
      alive = true;
    }

    // not copyable
    void operator=(const cancellable_barrier& m) { }

    void resize_unsafe(size_t numthreads) {
      needed = numthreads;
    }

    /**
     * \warning: This barrier is safely NOT reusable with this cancel
     * definition
     */
    inline void cancel() {
      alive = false;
      conditional.broadcast();
    }
    /// Wait on the barrier until numthreads has called wait
    inline void wait() const {
      if (!alive) return;
      mutex.lock();
      // set waiting;
      called++;
      bool listening_on = barrier_sense;
      if (called == needed) {
        // if I have reached the required limit, wait up. Set waiting
        // to 0 to make sure everyone wakes up
        called = 0;
        barrier_release = barrier_sense;
        barrier_sense = !barrier_sense;
        // clear all waiting
        conditional.broadcast();
      } else {
        // while no one has broadcasted, sleep
        while(barrier_release != listening_on && alive) conditional.wait(mutex);
      }
      mutex.unlock();
    }
  }; // end of conditional



  /**
   * \class barrier
   * Wrapper around pthread's barrier
   *
   * Before you use, see \ref parallel_object_intricacies.
   */
#ifdef __linux__
  /**
   * \ingroup threading
   * Wrapper around pthread's barrier
   */
  class barrier {
  private:
    mutable pthread_barrier_t m_barrier;
    // not copyconstructable
    barrier(const barrier&) { }
  public:
    /// Construct a barrier which will only fall when numthreads enter
    barrier(size_t numthreads) {
      pthread_barrier_init(&m_barrier, NULL, (unsigned)numthreads); }
    // not copyable
    void operator=(const barrier& m) { }
    void resize_unsafe(size_t numthreads) {
      pthread_barrier_destroy(&m_barrier);
      pthread_barrier_init(&m_barrier, NULL, (unsigned)numthreads);
    }
    ~barrier() { pthread_barrier_destroy(&m_barrier); }
    /// Wait on the barrier until numthreads has called wait
    inline void wait() const { pthread_barrier_wait(&m_barrier); }
  };

#else
   /* In some systems, pthread_barrier is not available.
   */
  typedef cancellable_barrier barrier;
#endif



  inline void prefetch_range(void *addr, size_t len) {
    char *cp;
    char *end = (char*)(addr) + len;

    for (cp = (char*)(addr); cp < end; cp += 64) __builtin_prefetch(cp, 0);
  }
  inline void prefetch_range_write(void *addr, size_t len) {
    char *cp;
    char *end = (char*)(addr) + len;

    for (cp = (char*)(addr); cp < end; cp += 64) __builtin_prefetch(cp, 1);
  }









  /**
   * \ingroup threading
   * A collection of routines for creating and managing threads.
   *
   * The thread object performs limited exception forwarding.
   * exception throws within a thread of type const char* will be caught
   * and forwarded to the join() function.
   * If the call to join() is wrapped by a try-catch block, the exception
   * will be caught safely and thread cleanup will be completed properly.
   */
  class thread {
  public:

    /**
     * This class contains the data unique to each thread. All threads
     * are gauranteed to have an associated turicreate thread_specific
     * data. The thread object is copyable.
     */
    class tls_data {
    public:
      inline tls_data(size_t thread_id);
      inline size_t thread_id() { return thread_id_; }
      inline void set_thread_id(size_t t) { thread_id_ = t; }
      any& operator[](const size_t& id);
      bool contains(const size_t& id) const;
      size_t erase(const size_t& id);
      inline void set_in_thread_flag(bool val) { in_thread = val; }
      inline bool is_in_thread() { return in_thread; }
    private:
      size_t thread_id_;
      bool in_thread = false;
      std::unique_ptr<boost::unordered_map<size_t, any> > local_data;
    }; // end of thread specific data



    /// Static helper routines
    // ===============================================================

    /**
     * Get the thread specific data associated with this thread
     */
    static tls_data& get_tls_data();

    /** Get the id of the calling thread.  This will typically be the
        index in the thread group. Between 0 to ncpus. */
    static inline size_t thread_id() { return get_tls_data().thread_id(); }

    /** Set the id of the calling thread.  This will typically be the
        index in the thread group. Between 0 to ncpus. */
    static inline void set_thread_id(size_t t) { get_tls_data().set_thread_id(t); }

    /**
     * Get a reference to an any object
     */
    static inline any& get_local(const size_t& id) {
      return get_tls_data()[id];
    }

    /**
     * Check to see if there is an entry in the local map
     */
    static inline bool contains(const size_t& id) {
      return get_tls_data().contains(id);
    }

    /**
     * Removes the entry from the local map.
     * @return number of elements erased.
     */
    static inline size_t erase(const size_t& id){
      return get_tls_data().erase(id);
    }

    /**
     * This static method joins the invoking thread with the other
     * thread object.  This thread will not return from the join
     * routine until the other thread complets it run.
     */
    static void join(thread& other);

    // Called just before thread exits. Can be used
    // to do special cleanup... (need for Java JNI)
    static void thread_destroy_callback();
    static void set_thread_destroy_callback(void (*callback)());


    /**
     * Return the number processing units (individual cores) on this
     * system
     */
    static size_t cpu_count();


  private:

    struct invoke_args{
      size_t m_thread_id;
      boost::function<void(void)> spawn_routine;
      invoke_args(size_t m_thread_id, const boost::function<void(void)> &spawn_routine)
          : m_thread_id(m_thread_id), spawn_routine(spawn_routine) { };
    };

    //! Little helper function used to launch threads
    static void* invoke(void *_args);

  public:

    /**
     * Creates a thread with a user-defined associated thread ID
     */
    inline thread(size_t thread_id = 0) :
      m_stack_size(0),
      m_p_thread(0),
      m_thread_id(thread_id),
      thread_started(false){
      // Calculate the stack size in in bytes;
      const int BYTES_PER_MB = 1048576;
      const int DEFAULT_SIZE_IN_MB = 8;
      m_stack_size = DEFAULT_SIZE_IN_MB * BYTES_PER_MB;
    }

    /**
     * execute this function to spawn a new thread running spawn_function
     * routine
     */
    void launch(const boost::function<void (void)> &spawn_routine);

    /**
     * Same as launch() except that you can specify a CPU on which to
     * run the thread.  This only currently supported in Linux and if
     * invoked on a non Linux based system this will be equivalent to
     * start().
     */
     void launch(const boost::function<void (void)> &spawn_routine, size_t cpu_id);


    /**
     * Join the calling thread with this thread.
     * const char* exceptions
     * thrown by the thread is forwarded to the join() function.
     */
    inline void join() {
      join(*this);
    }

    /// Returns true if the thread is still running
    inline bool active() const {
      return thread_started;
    }

    inline ~thread() {  }

    /// Returns the pthread thread id
    inline pthread_t pthreadid() {
      return m_p_thread;
    }
  private:


    //! The size of the internal stack for this thread
    size_t m_stack_size;

    //! The internal pthread object
    pthread_t m_p_thread;

    //! the threads id
    size_t m_thread_id;

    bool thread_started;
  }; // End of class thread





  /**
   * \ingroup threading
   * Manages a collection of threads.
   *
   * The thread_group object performs limited exception forwarding.
   * exception throws within a thread of type const char* will be caught
   * and forwarded to the join() function.
   * If the call to join() is wrapped by a try-catch block, the exception
   * will be caught safely and thread cleanup will be completed properly.
   *
   * If multiple threads are running in the thread-group, the master should
   * test if running_threads() is > 0, and retry the join().
   */
  class thread_group {
   private:
    size_t m_thread_counter;
    size_t threads_running;
    mutex mut;
    conditional cond;
    std::queue<std::pair<pthread_t, const char*> > joinqueue;
    // not implemented
    thread_group& operator=(const thread_group &thrgrp);
    thread_group(const thread_group&);
    static void invoke(boost::function<void (void)> spawn_function, thread_group *group);
   public:
    /**
     * Initializes a thread group.
     */
    thread_group() : m_thread_counter(0), threads_running(0) { }

    /**
     * Launch a single thread which calls spawn_function No CPU affinity is
     * set so which core it runs on is up to the OS Scheduler
     */
    void launch(const boost::function<void (void)> &spawn_function);

    /**
     * Launch a single thread which calls spawn_function Also sets CPU
     *  Affinity
     */
    void launch(const boost::function<void (void)> &spawn_function, size_t cpu_id);

    /** Waits for all threads to complete execution. const char* exceptions
    thrown by threads are forwarded to the join() function.
    */
    void join();

    /// Returns the number of running threads.
    inline size_t running_threads() {
      return threads_running;
    }
    //! Destructor. Waits for all threads to complete execution
    inline ~thread_group(){ join(); }

  }; // End of thread group


  /// Runs f in a new thread. convenience function for creating a new thread quickly.
  inline thread launch_in_new_thread(const boost::function<void (void)> &f,
                               size_t cpuid = size_t(-1)) {
    thread thr;
    if (cpuid != size_t(-1)) thr.launch(f, cpuid);
    else thr.launch(f);
    return thr;
  }

  /// an integer value padded to 64 bytes
  struct padded_integer {
    size_t val;
    char __pad__[64 - sizeof(size_t)];
  };


  /**
   * Convenience typedef to be equivalent to the std::condition_variable
   *
   */
  typedef conditional condition_variable;
}; // End Namespace

#endif
