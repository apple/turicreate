/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef QUEUED_RWLOCK_HPP
#define QUEUED_RWLOCK_HPP


namespace turi {


#define QUEUED_RW_LOCK_REQUEST_READ 0
#define QUEUED_RW_LOCK_REQUEST_WRITE 1
#define QUEUED_RW_LOCK_REQUEST_NONE 2

/**
 * Fair rw-lock with local-only spinning implemented and
 * modified from
 * Scalable Reader-Writer Synchronization for Shared-Memory Multiprocessors.
 * John M. Mellor-Crummey and Michael L. Scott
 *
 * \ingroup threading
 */
class queued_rw_lock{
 public:

  union state_union {
    volatile uint32_t stateu;
    struct {
      volatile uint16_t successor_class;
      volatile bool blocked;
    } state;
  };

  struct request{
    void* id;
    volatile request* volatile next;
    volatile state_union s;
    volatile char lockclass;
  };
 private:
  request* volatile tail;
  atomic<size_t> reader_count;
  request* volatile next_writer;
 public:
  queued_rw_lock(): tail(NULL), reader_count(0), next_writer(NULL) { }

  inline void writelock(request *I) {
    I->lockclass = QUEUED_RW_LOCK_REQUEST_WRITE;
    I->next = NULL;
    I->s.stateu = 0;
    I->s.state.blocked = true;
    I->s.state.successor_class = QUEUED_RW_LOCK_REQUEST_NONE;
    __sync_synchronize();
    request* predecessor = __sync_lock_test_and_set(&tail, I);

    if (predecessor == NULL) {
      next_writer = I;
      __sync_synchronize();
      if (reader_count.value == 0) {
        if (__sync_lock_test_and_set(&next_writer, (request*)NULL) == I) {
          I->s.state.blocked = false;
        }
      }
    }
    else {
      predecessor->s.state.successor_class = QUEUED_RW_LOCK_REQUEST_WRITE;
    __sync_synchronize();
      predecessor->next = I;
    }
    // while I->blocked. continue
    volatile state_union& is = I->s;
    while (is.state.blocked) sched_yield();
    assert(reader_count.value == 0);
  }

  inline void wrunlock(request *I) {
    __sync_synchronize();
    if (I->next != NULL || !__sync_bool_compare_and_swap(&tail, I, (request*)NULL)) {
      // wait
      while(I->next == NULL) sched_yield();
     __sync_synchronize();

      if (I->next->lockclass == QUEUED_RW_LOCK_REQUEST_READ) {
        reader_count.inc();
      }
      I->next->s.state.blocked = false;
    }
  }

  inline void readlock(request *I)  {
    I->lockclass =QUEUED_RW_LOCK_REQUEST_READ;
    I->next = NULL;
    I->s.stateu = 0;
    I->s.state.successor_class = QUEUED_RW_LOCK_REQUEST_NONE;
    I->s.state.blocked = true;
    __sync_synchronize();
    request* predecessor = __sync_lock_test_and_set(&tail, I);
    if (predecessor == NULL) {
      reader_count.inc();
      I->s.state.blocked = false;
    }
    else {

      state_union tempold, tempnew;
      tempold.state.blocked = true;
      tempold.state.successor_class = QUEUED_RW_LOCK_REQUEST_NONE;
      tempnew.state.blocked = true;
      tempnew.state.successor_class = QUEUED_RW_LOCK_REQUEST_READ;
      __sync_synchronize();
      if (predecessor->lockclass == QUEUED_RW_LOCK_REQUEST_WRITE ||
          atomic_compare_and_swap(predecessor->s.stateu,
                                  tempold.stateu,
                                  tempnew.stateu)) {

        predecessor->next = I;
        // wait
        __sync_synchronize();
        volatile state_union& is = I->s;
        while(is.state.blocked) sched_yield();
      }
      else {
        reader_count.inc();
        predecessor->next = I;
        __sync_synchronize();
        I->s.state.blocked = false;
      }
    }
    __sync_synchronize();
    if (I->s.state.successor_class == QUEUED_RW_LOCK_REQUEST_READ) {

      // wait
      while(I->next == NULL) sched_yield();
      reader_count.inc();
      I->next->s.state.blocked = false;
    }
  }

  inline void rdunlock(request *I)  {
    __sync_synchronize();
    if (I->next != NULL || !__sync_bool_compare_and_swap(&tail, I, (request*)NULL)) {
      while(I->next == NULL) sched_yield();
      if (I->s.state.successor_class == QUEUED_RW_LOCK_REQUEST_WRITE) {
        next_writer = (request*)(I->next);
        __sync_synchronize();
      }
    }
    if (reader_count.dec() == 0) {
      __sync_synchronize();
      request * w = __sync_lock_test_and_set(&next_writer, (request*)NULL);
      if (w != NULL) {
        w->s.state.blocked = false;
        __sync_synchronize();
      }
    }
  }
};

}
#endif
