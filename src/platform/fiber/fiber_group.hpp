/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_FIBER_GROUP_HPP
#define TURI_FIBER_GROUP_HPP
#include <fiber/fiber_control.hpp>
#include <parallel/atomic.hpp>
#include <parallel/pthread_tools.hpp>
namespace turi {

/**
 * Defines a group of fibers. Analogous to the thread_group, but is meant
 * to run only little user-mode threads. It is important that fibers never 
 * block, since there is no way to context switch out from a blocked fiber.
 * The fiber_group uses the fiber_control singleton instance to manage its
 * fibers.
 * \ingroup threading
 */
class fiber_group {
 public:
  typedef fiber_control::affinity_type affinity_type;

 private:
  size_t stacksize;
  affinity_type affinity;
  atomic<size_t> threads_running;
  mutex join_lock;
  // to be triggered once the threads_running counter becomes 0
  conditional join_cond; 
  // set to true if someone is waiting on a join()
  bool join_waiting;

  bool exception_raised;
  std::string exception_value;
  mutex exception_lock;

  inline void increment_running_counter() {
    threads_running.inc();
  }

  inline void decrement_running_counter() {
    // now, a bit of care is needed here
    size_t r = threads_running.dec();
    if (r == 0) {
      join_lock.lock();
      if (join_waiting) {
        join_cond.signal();
      }
      join_lock.unlock();
    }
  }

  // wraps the call so that we can do the appropriate termination
  static void invoke(const boost::function<void (void)>& spawn_function, 
                     fiber_group* group);

 public:


  fiber_group(size_t stacksize = 8192, 
              affinity_type affinity = fiber_control::all_affinity()) : 
      stacksize(stacksize), 
      affinity(affinity),
      join_waiting(false),
      exception_raised(false) { }


  /**
   * Sets the stacksize of each fiber.
   * Only takes effect for threads launched after this.
   */
  inline void set_stacksize(size_t new_stacksize) {
    stacksize = new_stacksize;
  }


  /**
   * Sets the affinity for each fiber.
   * Only takes effect for threads launched after this.
   */
  inline void set_affinity(affinity_type new_affinity) {
    affinity = new_affinity;
  }

  /**
   * Launch a single thread which calls spawn_function.
   */
  void launch(const boost::function<void (void)> &spawn_function);
              


  /**
   * Launch a single thread which calls spawn_function with worker affinity.
   */
  void launch(const boost::function<void (void)> &spawn_function, 
              affinity_type worker_affinity);


  /**
   * Launch a single thread which calls spawn_function with a single 
   * thread affinity
   */
  void launch(const boost::function<void (void)> &spawn_function,
              size_t worker_affinity);

  /** Waits for all threads to complete execution. const char* exceptions
   *  thrown by threads are forwarded to the join() function.
   */
  void join();

  /// Returns the number of running threads.
  inline size_t running_threads() {
    return threads_running;
  }
  //
  //! Destructor. Waits for all threads to complete execution
  inline ~fiber_group(){ join(); }

};

} // namespace turi
#endif
