/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_THREAD_POOL_HPP
#define TURI_THREAD_POOL_HPP

#include <boost/bind.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/util/blocking_queue.hpp>

namespace turi {

class thread_pool;

/**
 * This class adds a task queueing structure on top of a thread_pool, while
 * providing the same interface as the thread_pool.
 * Multiple parallel_task_queue objects can be associated with the same
 * thread_pool and each parallel_task_queue instance has its own indepedendent
 * task joining capability.
 *
 * The parallel_task_queue also provides exception forwarding.
 * exception throws within a thread of type const char* will be caught
 * and forwarded to the join() function.
 * If the call to join() is wrapped by a try-catch block, the exception
 * will be caught safely and thread cleanup will be completed properly.
 *
 * Usage:
 * \code
 * parallel_task_queue queue(thread_pool::get_instance());
 *
 * queue.launch(...)
 * ...
 * queue.join()
 * \endcode
 */
class parallel_task_queue {
 public:
  /**
   * Create a parallel_task_queue which is associated with a particular pool.
   * All tasks used by the parallel task_queue will use the pool for its pool
   * of threads.
   */
  parallel_task_queue(thread_pool& pool);

  /**
   * Launch a single task into the thread pool which calls spawn_function.
   *
   * If virtual_threadid is set, the target thread will appear to have
   * thread ID equal to the requested thread ID.
   */
  void launch(const boost::function<void (void)> &spawn_function,
              int virtual_threadid = -1);


  /** Waits for all tasks to complete. const char* exceptions
    thrown by threads are forwarded to the join() function.
    Once this function returns normally, the queue is empty.

    Note that this function may not return if producers continually insert
    tasks through launch.
    */
  void join();

  /**
   * Destructor. Waits for all tasks to complete.
   */
  ~parallel_task_queue();

 private:
  thread_pool& pool;
  // protects the exception queue, and the task counters
  mutex mut;
  conditional event_condition;  // to wake up the joining thread
  std::queue<std::exception_ptr> exception_queue;
  size_t tasks_inserted = 0;
  size_t tasks_completed = 0;
  bool waiting_on_join = false; // true if a thread is waiting in join
};


  /**
   * \ingroup util
   * Manages a pool of threads.
   *
   * The interface is nearly identical to the \ref thread_group.
   * The key difference is internal behavior. The thread pool preallocates a
   * collection of threads which it keeps asleep. When tasks are issued
   * through the "launch" function, threads are woken up to perform the
   * tasks.
   *
   * The thread_pool object does not perform exception forwarding, use
   * parallel_task_queue for that.
   *
   * If multiple threads are running in the thread-group, the master should
   * test if running_threads() is > 0, and retry the join().
   *
   */
  class thread_pool {
  private:
    thread_group threads;
    blocking_queue<std::pair<boost::function<void (void)>, int> > spawn_queue;
    size_t pool_size;

    mutex mut;
    conditional event_condition;
    size_t tasks_inserted = 0;
    size_t tasks_completed = 0;
    bool waiting_on_join = false;

    bool cpu_affinity;
    // not implemented
    thread_pool& operator=(const thread_pool &thrgrp);
    thread_pool(const thread_pool&);

    /**
       Called by each thread. Loops around a queue of tasks.
    */
    void wait_for_task();

    /**
       Creates all the threads in the thread pool.
       Resets the task and exception queue
    */
    void spawn_thread_group();

    /**
       Destroys the thread pool.
       Also destroys the task queue
    */
    void destroy_all_threads();
  public:

    /** Initializes a thread pool with nthreads.
     * If affinity is set, the nthreads will by default stripe across
     * the available cores on the system.
     */
    thread_pool(size_t nthreads = 2, bool affinity = false);

    /**
     * Set the number of threads in the queue
     */
    void resize(size_t nthreads);

    /**
     * Get the number of threads
     */
    size_t size() const;


    /**
     * Queues a single task into the thread pool which calls spawn_function.
     *
     * If virtual_threadid is set, the target thread will appear to have
     * thread ID equal to the requested thread ID.
     */
    void launch(const boost::function<void (void)> &spawn_function,
                 int virtual_threadid = -1);

    void join();

    /**
     * Changes the CPU affinity. Note that pthread does not provide
     * a way to change CPU affinity on a currently started thread.
     * This function therefore waits for all threads in the pool
     * to finish their current task, and destroy all the threads. Then
     * new threads are created with the new affinity setting.
     */
    void set_cpu_affinity(bool affinity);

    /**
       Gets the CPU affinity.
    */
    bool get_cpu_affinity() { return cpu_affinity; };


    /**
     * Returns a singleton instance of the thread pool
     */
    static thread_pool& get_instance();

    /**
     * Frees the singleton instance of the thread pool
     */
    static void release_instance();

    //! Destructor. Cleans up all threads
    ~thread_pool();
  };

}
#endif
