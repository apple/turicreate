/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <boost/bind.hpp>
#include <fiber/fiber_group.hpp>
#include <logger/assertions.hpp>
namespace turi {

void fiber_group::invoke(const boost::function<void (void)>& spawn_function, 
                         fiber_group* group) {
  try {
    spawn_function();
  } catch (std::string& s) {
    group->exception_lock.lock();
    group->exception_value = s;
    group->exception_raised = true;
    group->exception_lock.unlock();
  } catch (const char* c)  {
    group->exception_lock.lock();
    group->exception_raised = true;
    group->exception_value = c;
    group->exception_lock.unlock();
  } catch (std::exception& except)  {
    group->exception_lock.lock();
    group->exception_raised = true;
    group->exception_value = except.what();
    group->exception_lock.unlock();
  } catch (...)  {
    group->exception_lock.lock();
    group->exception_raised = true;
    group->exception_value = "Unknown exception";
    group->exception_lock.unlock();
  }
  group->decrement_running_counter();
}


void fiber_group::launch(const boost::function<void (void)> &spawn_function) {
  launch(spawn_function, affinity);
}


void fiber_group::launch(const boost::function<void (void)> &spawn_function,
                         affinity_type worker_affinity) {
  increment_running_counter();
  fiber_control::get_instance().launch(boost::bind(invoke, spawn_function, this), 
                                       stacksize,
                                       worker_affinity);  
}

void fiber_group::launch(const boost::function<void (void)> &spawn_function,
                         size_t worker_affinity) {
  increment_running_counter();
  fiber_group::affinity_type affinity;
  affinity.set_bit(worker_affinity);
  fiber_control::get_instance().launch(boost::bind(invoke, spawn_function, this), 
                                       stacksize,
                                       affinity);  
}


void fiber_group::join() {
  join_lock.lock();
  // no one else is waiting
  ASSERT_EQ(join_waiting, false);
  // otherwise, we need to wait
  join_waiting = true;
  while(threads_running.value != 0) {
    join_cond.wait(join_lock);
  }
  join_waiting = false;
  join_lock.unlock();

  if (exception_raised) {
    exception_raised = false;
    throw(exception_value);
  }
}

} // namespace turi

