/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <mutex>
#include <core/parallel/thread_pool.hpp>
#include <core/logging/assertions.hpp>
#include <core/parallel/pthread_tools.hpp>
#include <core/system/platform/config/apple_config.hpp>

namespace turi {

parallel_task_queue::parallel_task_queue(thread_pool& pool):pool(pool) { }

void parallel_task_queue::launch(const boost::function<void (void)> &spawn_function,
                                 int thread_id) {
  std::lock_guard<mutex> ulock(mut);
  tasks_inserted++;
  pool.launch(
      [&,spawn_function]() {
        try {
          spawn_function();
        } catch(...) {
          // if an exception was raised, put it in the exception queue
          std::lock_guard<mutex> exlock(mut);
          exception_queue.push(std::current_exception());
        }
        std::lock_guard<mutex> finishlock(mut);
        tasks_completed++;
        if (waiting_on_join &&
            tasks_completed == tasks_inserted) event_condition.signal();
      }, thread_id);
}

void parallel_task_queue::join() {
  std::unique_lock<mutex> join_lock(mut);
  waiting_on_join = true;
  while(1) {
    // nothing to throw, check if all tasks were completed
    if (tasks_completed == tasks_inserted) {
      // yup
      break;
    }
    event_condition.wait(join_lock);
  }
  waiting_on_join = false;

  if (exception_queue.size() > 0) {
    // check the exception queue.
    auto first_exception = exception_queue.front();
    exception_queue = std::queue<std::exception_ptr>();
    std::rethrow_exception(first_exception);
  }
}

parallel_task_queue::~parallel_task_queue() {
  // keep joining, and throwing away exceptions
  try {
    join();
  } catch (...) { }
}



thread_pool::thread_pool(size_t nthreads, bool affinity) {
  cpu_affinity = affinity;
  pool_size = nthreads;
  spawn_thread_group();
} // end of thread_pool


void thread_pool::resize(size_t nthreads) {
  // if the current pool size does not equal the requested number of
  // threads shut the pool down and startup with correct number of
  // threads.  \todo: If the pool size is too small just add
  // additional threads rather than destroying the pool
  if(nthreads != pool_size) {
    pool_size = nthreads;

    // stop the queue from blocking
    spawn_queue.stop_blocking();

    // join the threads in the thread group
    while(true) {
      try {
        threads.join(); break;
      } catch (const char* error_str) {
        // this should not be possible!
        logstream(LOG_FATAL)
            << "Unexpected exception caught in thread pool destructor: "
            << error_str << std::endl;
      }
    }
    spawn_queue.start_blocking();
    spawn_thread_group();
  }
} // end of set_nthreads


size_t thread_pool::size() const { return pool_size; }


/**
  Creates the thread group
  */
void thread_pool::spawn_thread_group() {

#ifdef __APPLE__
  // Ensure the cocoa runtime is set up for multithreaded execution
  config::init_cocoa_multithreaded_runtime();
#endif

  size_t ncpus = thread::cpu_count();
  // start all the threads if CPU affinity is set
  for (size_t i = 0;i < pool_size; ++i) {
    if (cpu_affinity) {
      threads.launch(boost::bind(&thread_pool::wait_for_task, this), i % ncpus);
    }
    else {
      threads.launch(boost::bind(&thread_pool::wait_for_task, this));
    }
  }
} // end of spawn_thread_group


void thread_pool::destroy_all_threads() {
  // wait for all execution to complete
  spawn_queue.wait_until_empty();
  // kill the queue
  spawn_queue.stop_blocking();

  // join the threads in the thread group
  while(1) {
    try {
      threads.join();
      break;
    }
    catch (const char* c) {
      // this should not be possible!
      logstream(LOG_FATAL)
          << "Unexpected exception caught in thread pool destructor: "
          << c << std::endl;
      ASSERT_TRUE(false);
    }
  }
} // end of destroy_all_threads

void thread_pool::set_cpu_affinity(bool affinity) {
  if (affinity != cpu_affinity) {
    cpu_affinity = affinity;
    // stop the queue from blocking
    spawn_queue.stop_blocking();

    // join the threads in the thread group
    while(1) {
      try {
        threads.join(); break;
      } catch (const char* c) {
        // this should not be possible!
        logstream(LOG_FATAL)
            << "Unexpected exception caught in thread pool destructor: "
            << c << std::endl;
        // ASSERT_TRUE(false); // unnecessary
      }
    }
    spawn_queue.start_blocking();
    spawn_thread_group();
  }
} // end of set_cpu_affinity


void thread_pool::launch(const boost::function<void (void)> &spawn_function,
                         int virtual_threadid) {
  std::lock_guard<mutex> lock(mut);
  ++tasks_inserted;
  spawn_queue.enqueue(std::make_pair(spawn_function, virtual_threadid));
}

void thread_pool::wait_for_task() {
  thread::get_tls_data().set_in_thread_flag(true);
  while(1) {
    std::pair<std::pair<boost::function<void (void)>, int>, bool> queue_entry;
    // pop from the queue
    queue_entry = spawn_queue.dequeue();
    if (queue_entry.second) {
      // try to run the function. remember to put it in a try catch
      int virtual_thread_id = queue_entry.first.second;
      size_t cur_thread_id = thread::thread_id();
      if (virtual_thread_id != -1) {
        thread::set_thread_id(virtual_thread_id);
      }
      queue_entry.first.first();
      thread::set_thread_id(cur_thread_id);
      std::lock_guard<mutex> lock(mut);
      ++tasks_completed;
      if (waiting_on_join &&
          tasks_completed == tasks_inserted) event_condition.signal();

    }
    else {
      // quit if the queue is dead
      break;
    }
  }
} // end of wait_for_task

void thread_pool::join() {
  spawn_queue.wait_until_empty();

  std::unique_lock<mutex> lock(mut);
  waiting_on_join = true;
  while(1) {
    // nothing to throw, check if all tasks were completed
    if (tasks_completed == tasks_inserted) {
      // yup
      break;
    }
    event_condition.wait(lock);
  }
  waiting_on_join = false;
}

thread_pool::~thread_pool() {
  destroy_all_threads();
}

static mutex& pool_creation_lock() {
  static mutex lock;
  return lock;
}
/**  In some odd situations, multiple threads can call this at the
 *   same time (such as with the local inproc cluster).  This prevents
 *   uses like this from causing problems.
 */
static std::shared_ptr<thread_pool>& get_pool_ptr_instance() {
  std::lock_guard<mutex> lg(pool_creation_lock());

  static std::shared_ptr<thread_pool> pool;
  if (pool == nullptr) {
    pool = std::make_shared<thread_pool>(thread::cpu_count(), true);
  }
  return pool;
}

thread_pool& thread_pool::get_instance() {
  return *get_pool_ptr_instance();
}

void thread_pool::release_instance() {
  get_pool_ptr_instance().reset();
}
}
