/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <iostream>
#include <boost/bind.hpp>
#include <fiber/fiber_control.hpp>
using namespace turi;

struct fibonacci_compute_promise {
  mutex* lock;
  size_t argument;
  size_t result;
  size_t parent_tid;
  bool result_set;
};

void fibonacci(fibonacci_compute_promise* promise) {
  //std::cout << promise->argument << "\n";
  if (promise->argument == 1 ||  promise->argument == 2) {
    promise->result = 1;
  } else {
    // recursive case
    mutex lock;
    fibonacci_compute_promise left, right;
    left.lock = &lock;
    left.argument = promise->argument - 1;
    left.result_set = false;
    left.parent_tid = fiber_control::get_tid();

    right.lock = &lock;
    right.argument = promise->argument - 2;
    right.result_set = false;
    right.parent_tid = fiber_control::get_tid();

    fiber_control::get_instance().launch(boost::bind(fibonacci, &left));
    fiber_control::get_instance().launch(boost::bind(fibonacci, &right));

    // wait on the left and right promise
    lock.lock();
    while (left.result_set == false || right.result_set == false) {
      fiber_control::deschedule_self(&lock.m_mut);
      lock.lock();
    }
    lock.unlock();

    assert(left.result_set);
    assert(right.result_set);
    promise->result = left.result + right.result;
  }
  promise->lock->lock();
  promise->result_set = true;
  if (promise->parent_tid) fiber_control::schedule_tid(promise->parent_tid);
  promise->lock->unlock();
}


int main(int argc, char** argv) {

  timer ti; ti.start();

  fibonacci_compute_promise promise;
  mutex lock;
  promise.lock = &lock;
  promise.result_set = false;
  promise.argument = 24;
  promise.parent_tid = 0;
  fiber_control::get_instance().launch(boost::bind(fibonacci, &promise));
  fiber_control::get_instance().join();
  assert(promise.result_set);
  std::cout << "Fib(" << promise.argument << ") = " << promise.result << "\n";

  std::cout << "Completion in " << ti.current_time() << "s\n";
  std::cout << fiber_control::get_instance().total_threads_created() << " threads created\n";
}
