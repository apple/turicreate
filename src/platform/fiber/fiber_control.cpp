/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <cstdlib>
#include <boost/bind.hpp>
#include <fiber/fiber_control.hpp>
#include <logger/assertions.hpp>
#include <random/random.hpp>

//#include <valgrind/valgrind.h>
namespace turi {

bool fiber_control::tls_created = false;
bool fiber_control::instance_created = false;
size_t fiber_control::instance_construct_params_nworkers = 0;
size_t fiber_control::instance_construct_params_affinity_base = 0;
pthread_key_t fiber_control::tlskey;

fiber_control::affinity_type fiber_control::all_affinity() {
  affinity_type ret;
  ret.fill();
  return ret;
}

fiber_control::fiber_control(size_t nworkers, 
                             size_t affinity_base)
    :nworkers(nworkers),
    affinity_base(affinity_base),
    stop_workers(false),
    flsdeleter(NULL) {
  // initialize the thread local storage keys
  if (!tls_created) {
    pthread_key_create(&tlskey, fiber_control::tls_deleter);
    tls_created = true;
  }

  // set up the queues.
  schedule.resize(nworkers);
  for (size_t i = 0;i < nworkers; ++i) {
    schedule[i].waiting = false;
    schedule[i].nwaiting = 0;
    schedule[i].affinity_queue = new inplace_lf_queue2<fiber>;
    schedule[i].priority_queue = new inplace_lf_queue2<fiber>;
    schedule[i].popped_affinity_queue = NULL;
    schedule[i].popped_priority_queue = NULL;
  }
  // launch the workers
  for (size_t i = 0;i < nworkers; ++i) {
    workers.launch(boost::bind(&fiber_control::worker_init, this, i), 
                   affinity_base + i);
  }
}

fiber_control::~fiber_control() {
  join();
  stop_workers = true;
  for (size_t i = 0;i < nworkers; ++i) {
    schedule[i].active_lock.lock();
    schedule[i].active_cond.broadcast();
    schedule[i].active_lock.unlock();
    delete schedule[i].affinity_queue;
    delete schedule[i].priority_queue;
  }
  workers.join();



  pthread_key_delete(tlskey);
}


void fiber_control::tls_deleter(void* f) {
  fiber_control::tls* t = (fiber_control::tls*)(f);
  delete t;
}

void fiber_control::create_tls_ptr() {
  pthread_setspecific(tlskey, (void*)(new fiber_control::tls));
}


fiber_control::tls* fiber_control::get_tls_ptr() {
  if (tls_created == false) return NULL;
  else return (fiber_control::tls*) pthread_getspecific(tlskey);
}

fiber_control::fiber* fiber_control::get_active_fiber() {
  tls* t = get_tls_ptr();
  if (t != NULL) return t->cur_fiber;
  else return NULL;
}




void fiber_control::active_queue_insert_tail(size_t workerid, fiber_control::fiber* value) {
  if (value->scheduleable) {
     //printf("%ld: Scheduling Tail %ld on %ld\n", get_worker_id(), value->id, workerid);
    schedule[workerid].affinity_queue->enqueue(value);
    ++schedule[workerid].nwaiting;
    if (schedule[workerid].waiting) {
      //printf("%ld: Waking %ld\n", value->id, workerid);
      schedule[workerid].active_lock.lock();
      schedule[workerid].active_cond.signal();
      schedule[workerid].active_lock.unlock();
    }
  }
}


void fiber_control::active_queue_insert_head(size_t workerid, fiber_control::fiber* value) {
  if (value->scheduleable) {
     //printf("%ld: Scheduling Head %ld on %ld\n", get_worker_id(), value->id, workerid);
    schedule[workerid].priority_queue->enqueue(value);
    ++schedule[workerid].nwaiting;
    if (schedule[workerid].waiting) {
      //printf("%ld: Waking %ld\n", value->id, workerid);
      schedule[workerid].active_lock.lock();
      schedule[workerid].active_cond.signal();
      schedule[workerid].active_lock.unlock();
    }
  }
}

fiber_control::fiber* fiber_control::try_pop_queue(inplace_lf_queue2<fiber>& lfqueue,
                                                   fiber*& popped_queue) {
  fiber_control::fiber* ret = NULL;
  // if there is stuff in the popped queue, pop it.
  if (popped_queue == NULL) {
    popped_queue = lfqueue.dequeue_all();
  }

  if (popped_queue != NULL) {
    ret = popped_queue;
    do {
      popped_queue = ret->next;
      asm volatile("pause\n": : :"memory");
    } while(popped_queue == NULL);
    // we have reached the end of the queue. clear the popped queue
    // and return
    if (popped_queue == lfqueue.end_of_dequeue_list()) {
      popped_queue = NULL;
    }
  }
  return ret;
}

fiber_control::fiber* fiber_control::active_queue_remove(size_t workerid) {
  fiber_control::fiber* ret = NULL;
  thread_schedule& curts = schedule[workerid];
  ret = try_pop_queue(*curts.priority_queue, curts.popped_priority_queue);
  if (ret == NULL) {
    ret = try_pop_queue(*curts.affinity_queue , curts.popped_affinity_queue);
  }
  if (ret) {
    --curts.nwaiting;
    //printf("%ld: Running %ld\n", get_worker_id(), ret->id);
  } else {
    //printf("%ld: Queue Empty\n", get_worker_id());
  }
  return ret;
}

void fiber_control::exit() {
  fiber* fib = get_active_fiber();
  if (fib->parent->fiber_exit_callback) {
    fib->parent->fiber_exit_callback(get_worker_id());
  }
  if (fib != NULL) {
    // add to garbage.
    fib->terminate = true;
    yield(); // never returns
    ASSERT_MSG(false, "Impossible Condition. Dead Fiber woke up");
  } else {
    ASSERT_MSG(false, "Calling fiber exit not from a fiber");
  }
}

static timer flush_timer;
mutex flush_lock;

void fiber_control::worker_init(size_t workerid) {
  /*
   * This is the "root" stack for each worker.
   * When there are active user threads associated with this worker, 
   * it will switch directly between the fibers.
   * But, when the worker has no other fiber to run, it will return to this
   * stack and and wait in a condition variable
   */
  // create a root context
  create_tls_ptr();
  // set up the tls structure
  tls* t = get_tls_ptr();
  t->prev_fiber = NULL;
  t->cur_fiber = NULL;
  t->garbage = NULL;
  t->workerid = workerid;
  t->parent = this;

  schedule[workerid].waiting = true;
  schedule[workerid].active_lock.lock();
  while(!stop_workers) {
    // get a fiber to run
    fiber* next_fib = t->parent->active_queue_remove(workerid);
    if (next_fib != NULL) {
      //printf("%ld: Switching to %ld\n", get_worker_id(), next_fib->id);
      // if there is a fiber. yield to it
      schedule[workerid].active_lock.unlock();
      schedule[workerid].waiting = false;
      active_workers.inc();
      yield_to(next_fib);
      if (context_switch_periodic_callback && 
          flush_timer.current_time() > 0.0001 && flush_lock.try_lock()) {
        context_switch_periodic_callback(get_worker_id());
        flush_timer.start();
        flush_lock.unlock();
      }
      active_workers.dec();
      schedule[workerid].waiting = true;
      schedule[workerid].active_lock.lock();
    } else {
      //printf("%ld: Nothing to do. Sleep\n", get_worker_id());
      // if there is no fiber. wait.
      schedule[workerid].active_cond.wait(schedule[workerid].active_lock);
    }
  }
  schedule[workerid].active_lock.unlock();
}

struct trampoline_args {
  boost::function<void(void)> fn;
};

// the trampoline to call the user function. This function never returns
#if BOOST_VERSION >= 106100
void fiber_control::trampoline(boost::context::transfer_t _args) {
#else
void fiber_control::trampoline(intptr_t _args) {
#endif
  // we may have launched to here by switching in from another fiber.
  // we will need to clean up the previous fiber
  tls* t = get_tls_ptr();

#if BOOST_VERSION >= 106100
  if (t->prev_fiber != NULL) {
    t->prev_fiber->context = _args.fctx;
  } else {
    t->base_context = _args.fctx;
  }
#endif

  if (t->prev_fiber) t->parent->reschedule_fiber(t->workerid, t->prev_fiber);
  t->prev_fiber = NULL;
  //printf("%ld: Running fiber: %ld\n", t->workerid, t->cur_fiber->id);

#if BOOST_VERSION >= 106100
  trampoline_args* args = reinterpret_cast<trampoline_args*>(_args.data);
#else
  trampoline_args* args = reinterpret_cast<trampoline_args*>(_args);
#endif

  try {
    args->fn();
  } catch (...) {
  }
  delete args;
  fiber_control::exit();
}

size_t fiber_control::launch(boost::function<void(void)> fn, 
                             size_t stacksize, 
                             affinity_type affinity) {
  ASSERT_GT(affinity.popcount(), 0);
  size_t b = 0;
  ASSERT_TRUE(affinity.first_bit(b));
  // make sure there is always a worker I can work on
  ASSERT_LT(b, nworkers);

  // allocate a stack
  fiber* fib = new fiber;
  fib->parent = this;
  fib->stack = malloc(stacksize);
  fib->id = fiber_id_counter.inc();
  for(size_t b: affinity) {
    if (b < nworkers) fib->affinity_array.push_back((unsigned char)b);
    else break;
  }
  ASSERT_GT(fib->affinity_array.size(), 0);
  fib->affinity = affinity;
  //VALGRIND_STACK_REGISTER(fib->stack, (char*)fib->stack + stacksize);
  fib->fls = NULL;
  fib->next = NULL;
  fib->deschedule_lock = NULL;
  fib->terminate = false;
  fib->descheduled = false;
  fib->scheduleable = true;
  // construct the initial context
  trampoline_args* args = new trampoline_args;
  args->fn = fn;

#if BOOST_VERSION >= 106100
   fib->initial_trampoline_args = (void*)args;
#else
   fib->initial_trampoline_args = (intptr_t)(args);
#endif

  // stack grows downwards.
  fib->context = boost::context::make_fcontext((char*)fib->stack + stacksize,
                                               stacksize,
                                               trampoline);
  fibers_active.inc();

  // find a place to put the thread
  size_t choice = pick_fiber_worker(fib);
  active_queue_insert_tail(choice, fib);
  return reinterpret_cast<size_t>(fib);
}

size_t fiber_control::pick_fiber_worker(fiber* fib) {
  // first try to use the original worker if possible
  size_t choice = get_worker_id();
  if (choice == (size_t)(-1) || fib->affinity.get(choice) == 0) {
    //choice rejected, pick randomly from the available choices
    // if there is only one affinity option, return it
    if (fib->affinity_array.size() == 1) {
      choice = fib->affinity_array[0];
    } else {
      size_t ra = turi::random::fast_uniform<size_t>(0,fib->affinity_array.size() - 1);
      std::swap(fib->affinity_array[ra], fib->affinity_array[0]);
      choice = fib->affinity_array[0];
    }
  }
  return choice;
}

void fiber_control::yield_to(fiber* next_fib) {
  // the core scheduling logic
  tls* t = get_tls_ptr();
  
  // if (next_fib) {
  //   if (t->cur_fiber) {
  //     printf("%ld: yield to: %ld from %ld\n", get_worker_id(), next_fib->id, t->cur_fiber->id);
  //   }  else {
  //     printf("%ld: yield to: %ld\n", get_worker_id(), next_fib->id);
  //   }
  // }
#if BOOST_VERSION >= 106100
  boost::context::detail::transfer_t res;
#endif

  if (next_fib != NULL) {
    // reset the priority flag
    next_fib->priority = false;
    // current fiber moves to previous
    // next fiber move to current
    t->prev_fiber = t->cur_fiber;
    t->cur_fiber = next_fib;
    if (t->prev_fiber != NULL) {

      // context switch to fib outside the lock
#if BOOST_VERSION >= 106100
      res = boost::context::jump_fcontext(//&t->prev_fiber->context,
                                    t->cur_fiber->context,
                                    t->cur_fiber->initial_trampoline_args);
#else
      boost::context::jump_fcontext(&t->prev_fiber->context,
                                    t->cur_fiber->context,
                                    t->cur_fiber->initial_trampoline_args);
#endif

    } else {

#if BOOST_VERSION >= 106100
      res = boost::context::jump_fcontext(t->cur_fiber->context,
                                          t->cur_fiber->initial_trampoline_args);
#else

      boost::context::jump_fcontext(&t->base_context,
                                    t->cur_fiber->context,
                                    t->cur_fiber->initial_trampoline_args);
#endif

    }
  } else {
    // ok. there isn't anything to schedule to
    // am I meant to be terminated? or descheduled?
    if (t->cur_fiber &&
        (t->cur_fiber->terminate || t->cur_fiber->descheduled) ) {
      // yup. killing current fiber
      // context switch back to basecontext which will
      // do the cleanup
      //
      // current fiber moves to previous
      // next fiber (base context) move to current
      // (as identifibed by cur_fiber = NULL)
      t->prev_fiber = t->cur_fiber;
      t->cur_fiber = NULL;

#if BOOST_VERSION >= 106100
      res = boost::context::jump_fcontext(t->base_context, nullptr);
#else
      boost::context::jump_fcontext(&t->prev_fiber->context,
                                    t->base_context,
                                    0);
#endif
    } else {
      // nothing to do, and not terminating...
      // then don't yield!
      return;
    }
  }
  // reread the tls pointer because we may have woken up in a different thread
  t = get_tls_ptr();

#if BOOST_VERSION >= 106100
  if (t->prev_fiber != NULL) {
    t->prev_fiber->context = res.fctx;
  } else {
    t->base_context = res.fctx;
  }
#endif

  // reschedule the previous fiber
  if (t->prev_fiber) reschedule_fiber(t->workerid, t->prev_fiber);
  t->prev_fiber = NULL;

  if (context_switch_callback) {
    context_switch_callback(t->workerid);
  }
}

void fiber_control::reschedule_fiber(size_t workerid, fiber* fib) {
  fib->lock.lock();
  if (!fib->terminate && !fib->descheduled) {
    fib->lock.unlock();
    // we reschedule it
    // Re-lock the queue
    //printf("%ld: Reinserting %ld\n", get_worker_id(), fib->id);
    if (!fib->priority) active_queue_insert_tail(workerid, fib);
    else active_queue_insert_head(workerid, fib);
  } else if (fib->descheduled) {
    // unflag descheduled and unset scheduleable
    fib->descheduled = false;
    fib->scheduleable = false;
    if (fib->deschedule_lock) pthread_mutex_unlock(fib->deschedule_lock);
    fib->deschedule_lock = NULL;
    //printf("%ld: Descheduling complete %ld\n", get_worker_id(), fib->id);
    fib->lock.unlock();
  } else if (fib->terminate) {
    fib->lock.unlock();
    // previous fiber is dead. destroy it
    free(fib->stack);
    //VALGRIND_STACK_DEREGISTER(fib->stack);
    // delete the fiber local storage if any
    if (fib->fls && flsdeleter) flsdeleter(fib->fls);
    delete fib;
    // if we are out of threads, signal the join
    if (fibers_active.dec() == 0) {
      join_lock.lock();
      join_cond.signal();
      join_lock.unlock();
    }
  } else {
    // impossible condition
    assert(false);
  }
}

void fiber_control::yield() {
  // the core scheduling logic
  tls* t = get_tls_ptr();
  if (t == NULL) return;
  // remove some other work to do.
  fiber_control* parentgroup = t->parent;
  size_t workerid = t->workerid;
  fiber* next_fib = parentgroup->active_queue_remove(workerid);
  t->parent->yield_to(next_fib);
}


void fiber_control::fast_yield() {
  yield();
}


void fiber_control::join() {
  join_lock.lock();
  while(fibers_active.value > 0) {
    join_cond.wait(join_lock);
  }
  join_lock.unlock();
}

size_t fiber_control::get_tid() {
  fiber_control::tls* tls = get_tls_ptr();
  if (tls != NULL) return reinterpret_cast<size_t>(tls->cur_fiber);
  else return (size_t)(0);
}


bool fiber_control::in_fiber() {
  return get_tls_ptr() != NULL;
}

void fiber_control::deschedule_self(pthread_mutex_t* lock) {
  fiber* fib = get_tls_ptr()->cur_fiber;
  fib->lock.lock();
  assert(fib->descheduled == false);
  assert(fib->scheduleable == true);
  fib->deschedule_lock = lock;
  fib->descheduled = true;
  //printf("%ld: Descheduling requested %ld\n", get_worker_id(), fib->id);
  fib->lock.unlock();
  yield();
}

bool fiber_control::worker_has_priority_fibers_on_queue() {
  tls* t = get_tls_ptr();
  if (t == NULL) return false;
  fiber_control* parentgroup = t->parent;
  size_t workerid = t->workerid;
  return !parentgroup->schedule[workerid].priority_queue->empty();
}

bool fiber_control::worker_has_fibers_on_queue() {
  tls* t = get_tls_ptr();
  if (t == NULL) return false;
  fiber_control* parentgroup = t->parent;
  size_t workerid = t->workerid;
  return !parentgroup->schedule[workerid].priority_queue->empty() ||
          !parentgroup->schedule[workerid].affinity_queue->empty();
}

size_t fiber_control::get_worker_id() {
  fiber_control::tls* tls = get_tls_ptr();
  if (tls != NULL) return tls->workerid;
  else return (size_t)(-1);
}

void fiber_control::schedule_tid(size_t tid, bool priority) {
  fiber* fib = reinterpret_cast<fiber*>(tid);
  fib->lock.lock();
  // we MUST get here only after the thread was completely descheduled
  // or no deschedule operation has happened yet.
  assert(fib->descheduled == false);
  fib->descheduled = false;
  if (fib->scheduleable == false) {
    // if this thread was descheduled completely. Reschedule it.
    //printf("%ld: Scheduling requested %ld\n", get_worker_id(), fib->id);
    fib->scheduleable = true;
    fib->priority = priority;
    fib->lock.unlock();
    size_t choice = fib->parent->pick_fiber_worker(fib);
    fib->parent->reschedule_fiber(choice, fib);
  } else {
    //printf("%ld: Scheduling requested of running thread %ld\n", get_worker_id(), fib->id);
    fib->lock.unlock();
  }
}


void fiber_control::set_tls_deleter(void (*deleter)(void*)) {
  flsdeleter = deleter;
}

void* fiber_control::get_tls() {
  fiber_control::tls* f = get_tls_ptr();
  if (f != NULL) {
    return f->cur_fiber->fls;
  } else {
    // cannot get TLS of a non-fiber
    ASSERT_MSG(false, "Trying to get a fiber TLS from a non-fiber");
    return NULL;
  }
}

void fiber_control::set_tls(void* tls) {
  fiber_control::tls* f = get_tls_ptr();
  if (f != NULL) {
    f->cur_fiber->fls = tls;
  } else {
    // cannot get TLS of a non-fiber
    ASSERT_MSG(false, "Trying to get a fiber TLS from a non-fiber");
  }
}


void fiber_control::instance_set_parameters(size_t nworkers = 0,
                                            size_t affinity_base = 0) {
  instance_construct_params_nworkers = nworkers;
  instance_construct_params_affinity_base = affinity_base;
}


fiber_control& fiber_control::get_instance() {
  fiber_control::instance_created = true;
  if (instance_construct_params_nworkers == 0) {
    instance_construct_params_nworkers = thread::cpu_count();
  }
  static fiber_control* singleton = new fiber_control
      (instance_construct_params_nworkers, instance_construct_params_affinity_base);
  return *singleton;
}

void fiber_control::delete_instance() {
  if (instance_created) {
    delete &get_instance();
  }
}

}
