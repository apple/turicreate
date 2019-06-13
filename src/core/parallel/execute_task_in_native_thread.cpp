/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <core/parallel/mutex.hpp>
#include <core/parallel/thread_pool.hpp>
#include <core/parallel/pthread_tools.hpp>

namespace turi {
/*
 * LIBHDFS is rather annoying in that it does not handle coroutines
 * correctly since libJVM's AttachCurrentThread() does not handle
 * coroutines correctly.
 *
 * The solution is to simply have a task queue model which allows the
 * readers from HDFS to redirect the read to some other thread for processing.
 */

static parallel_task_queue& get_task_queue() {
  static mutex lock;
  static bool inited = false;

  // Intentional leak. Native theads are required to cleanup hdfs temp space.
  static thread_pool* pool;
  static parallel_task_queue* task_queue;
  // ensure that initialization is thread safe
  if (inited) {
    return *task_queue;
  } else {
    std::lock_guard<mutex> GUARD(lock);
    if (inited) return *task_queue;
    pool = new thread_pool(thread::cpu_count());
    task_queue = new parallel_task_queue(*pool);
    inited = true;
    return *task_queue;
  }
}

std::exception_ptr execute_task_in_native_thread(const std::function<void(void)>& fn) {
  auto& queue = get_task_queue();
  // the threading structures required to wait for a result
  mutex lock;
  conditional cond;
  bool done = false;
  std::exception_ptr ret;

  queue.launch([&](void)->void {
    // run the function performing my own exception forwarding
    try {
      fn();
    } catch (...) {
      ret = std::current_exception();
    }
    std::lock_guard<mutex> GUARD(lock);
    done = true;
    cond.signal();
  });

  lock.lock();
  while (!done) {
    cond.wait(lock);
  }
  lock.unlock();
  return ret;
}

} // turicreate
