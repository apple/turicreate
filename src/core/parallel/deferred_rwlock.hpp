/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef DEFERRED_RWLOCK_HPP
#define DEFERRED_RWLOCK_HPP
#include <core/parallel/pthread_tools.hpp>
#include <core/parallel/queued_rwlock.hpp>
#include <core/logging/assertions.hpp>
namespace turi {
/**
 * \ingroup threading
 * The deferred rwlock is a variant of a regular rwlock which is completely
 * non-blocking.
 *
 * The user creates request objects:
 * \code
 * // we use new here, but it is preferable that you have a big statically
 * // allocated pool of request objects
 * deferred_rwlock::request* request = new deferred_rwlock::request;
 * // request->id can be used here to associate additional information with the
 * // request. note that request->id is a 62 bit integer
 * \endcode
 *
 * After which, all lock acquisition / release operations return a released
 * argument. For instance, to acquire a readlock
 * \code
 * deferred_rwlock::request* released;
 * rwlock.readlock(request, &released);
 * \endcode
 *
 * or to complete a readlock:
 * \code
 * deferred_rwlock::request* released;
 * rwlock.complete_rdlock(&released);
 * \endcode
 *
 * All operations are \b non-blocking, meaning that the lock request you
 * issued/completed, need not be the set of requests that are satisfied.
 * The set of released locks returned is a linked list of locks
 * that are satisfied at completion of the operation.
 *
 * For instance:
 * \code
 * deferred_rwlock::request writelock;
 * deferred_rwlock::request readlocks[4];
 * deferred_rwlock::request* released;
 *
 * // we are going to acquire a write lock
 * // this will be successful if rwlock is a fresh lock
 * bool success = rwlock.writelock(writelock);
 *
 * // acquire 4 read locks consecutively. Note that this does not block.
 * // but since a write lock has already been acquired, all of these will
 * // return num_released = 0
 * int num_released;
 * num_released = rwlock.readlock(readlocks[0], &released);
 * num_released = rwlock.readlock(readlocks[1], &released);
 * num_released = rwlock.readlock(readlocks[2], &released);
 * num_released = rwlock.readlock(readlocks[3], &released);
 *
 * // release the write lock we acquired earlier.
 * num_released = rwlock.wrunlock(writelock, &released);
 *
 * // since the write lock is released, all readers can proceed and
 * // num_released will be 4 here. released now is a linked list of read requests.
 * // For instance, the following may be true:
 * released == &(readlocks[0]);
 * released->next == &(readlocks[1]);
 * released->next->next == &(readlocks[2]);
 * released->next->next->next == &(readlocks[3]);
 * released->next->next->next->next == nullptr;
 * // strictly the implementation need not have this particular linked list
 * // ordering, though that is indeed currently the case since it maintains
 * // a strict queue.
 *
 * // At some point in the future you do need to call rdunlock() as many times
 * // as there are read requests completed.
 * rwlock.rdunlock();
 * rwlock.rdunlock();
 * rwlock.rdunlock();
 * rwlock.rdunlock();
 * \endcode
 *
 * This deferred_rwlock is tricky and not easy to use. You need to manage
 * the request objects carefully and it is easy to get into inconsistent
 * scenarios.
 */
class deferred_rwlock{
 public:

  struct request{
    char lockclass : 2;
    __attribute__((may_alias)) uint64_t id : 62;
    request* next;
  };
 private:
  request* head;
  request* tail;
  uint16_t reader_count;
  bool writer;
  simple_spinlock lock;
 public:

  deferred_rwlock(): head(NULL),
                      tail(NULL), reader_count(0),writer(false) { }

  // debugging purposes only
  inline size_t get_reader_count() {
    __sync_synchronize();
    return reader_count;
  }

  // debugging purposes only
  inline bool has_waiters() {
    return head != NULL || tail != NULL;
  }

  inline void insert_queue(request *I) {
    if (head == NULL) {
      head = I;
      tail = I;
    }
    else {
      tail->next = I;
      tail = I;
    }
  }
  inline void insert_queue_head(request *I) {
    if (head == NULL) {
      head = I;
      tail = I;
    }
    else {
      I->next = head;
      head = I;
    }
  }

  /**
   * Tries to acquire a high priority writelock.
   * Returns true if the write lock is available immediately.
   * False otherwise, in which case the request object may be returned in a
   * released linked list via another complete lock operation.
   */
  inline bool writelock_priority(request *I) {
    I->next = NULL;
    I->lockclass = QUEUED_RW_LOCK_REQUEST_WRITE;
    lock.lock();
    if (reader_count == 0 && writer == false) {
      // fastpath
      writer = true;
      lock.unlock();
      return true;
    }
    else {
      insert_queue_head(I);
      lock.unlock();
      return false;
    }
  }

  /**
   * Tries to acquire a writelock.
   * Returns true if the write lock is available immediately.
   * False otherwise, in which case the request object may be returned in a
   * released linked list via another complete lock operation.
   */
  inline bool writelock(request *I) {
    I->next = NULL;
    I->lockclass = QUEUED_RW_LOCK_REQUEST_WRITE;
    lock.lock();
    if (reader_count == 0 && writer == false) {
      // fastpath
      writer = true;
      lock.unlock();
      return true;
    }
    else {
      insert_queue(I);
      lock.unlock();
      return false;
    }
  }

  /**
   * \internal
   * completes the write lock on the head. lock must be acquired
   * head must be a write lock
   */
  inline void complete_wrlock() {
  //  ASSERT_EQ(reader_count.value, 0);
    head = head->next;
    if (head == NULL) tail = NULL;
    writer = true;
  }

  /**
   * \internal
   * completes the read lock on the head. lock must be acquired
   * head must be a read lock
   */
  inline size_t complete_rdlock(request* &released) {
    released = head;
    size_t numcompleted = 1;
    head = head->next;
    request* readertail = released;
    while (head != NULL && head->lockclass == QUEUED_RW_LOCK_REQUEST_READ) {
      readertail = head;
      head = head->next;
      numcompleted++;
    }
    reader_count += numcompleted;
    if (head == NULL) tail = NULL;

    // now released is the head to a reader list
    // and head is the head of a writer list
    // I want to go through the writer list and extract all the readers
    // this essentially
    // splits the list into two sections, one containing only readers, and
    // one containing only writers.
    // (reader biased locking)
    if (head != NULL) {
      request* latestwriter = head;
      request* cur = head->next;
      while (1) {
        if (cur->lockclass == QUEUED_RW_LOCK_REQUEST_WRITE) {
          latestwriter = cur;
        }
        else {
          readertail->next = cur;
          readertail = cur;
          reader_count++;
          numcompleted++;
          latestwriter->next = cur->next;
        }
        if (cur == tail) break;
        cur=cur->next;
      }
    }
    return numcompleted;
  }

  /**
   * Released a currently acquired write lock.
   * Returns the number of new locks acquired, and the output argument
   * 'released' contains a linked list of locks next acquired,
   */
  inline size_t wrunlock(request* &released) {
    released = NULL;
    lock.lock();
    writer = false;
    size_t ret = 0;
    if (head != NULL) {
      if (head->lockclass == QUEUED_RW_LOCK_REQUEST_READ) {
        ret = complete_rdlock(released);
        if (ret == 2) assert(released->next != NULL);
      }
      else {
        writer = true;
        released = head;
        complete_wrlock();
        ret = 1;
      }
    }
    lock.unlock();
    return ret;
  }

  /**
   * Tries to acquire a readlock.
   * Returns the number of locks now released.
   * 'released' contains a linked list of locks next acquired,
   */
  inline size_t readlock(request *I, request* &released)  {
    released = NULL;
    size_t ret = 0;
    I->next = NULL;
    I->lockclass = QUEUED_RW_LOCK_REQUEST_READ;
    lock.lock();
    // there are readers and no one is writing
    if (head == NULL && writer == false) {
      // fast path
      ++reader_count;
      lock.unlock();
      released = I;
      return 1;
    }
    else {
      // slow path. Insert into queue
      insert_queue(I);
      if (head->lockclass == QUEUED_RW_LOCK_REQUEST_READ && writer == false) {
        ret = complete_rdlock(released);
      }
      lock.unlock();
      return ret;
    }
  }

  /**
   * Tries to acquire a high priority readlock.
   * Returns the number of locks now released.
   * 'released' contains a linked list of locks next acquired,
   */
  inline size_t readlock_priority(request *I, request* &released)  {
    released = NULL;
    size_t ret = 0;
    I->next = NULL;
    I->lockclass = QUEUED_RW_LOCK_REQUEST_READ;
    lock.lock();
    // there are readers and no one is writing
    if (head == NULL && writer == false) {
      // fast path
      ++reader_count;
      lock.unlock();
      released = I;
      return 1;
    }
    else {
      // slow path. Insert into queue
      insert_queue_head(I);
      if (head->lockclass == QUEUED_RW_LOCK_REQUEST_READ && writer == false) {
        ret = complete_rdlock(released);
      }
      lock.unlock();
      return ret;
    }
  }

  /**
   * Released a currently acquired read lock.
   * Returns the number of new locks acquired, and the output argument
   * 'released' contains a linked list of locks next acquired,
   */
  inline size_t rdunlock(request* &released)  {
    released = NULL;
    lock.lock();
    --reader_count;
    if (reader_count == 0) {
      size_t ret = 0;
      if (head != NULL) {
        if (head->lockclass == QUEUED_RW_LOCK_REQUEST_READ) {
          ret = complete_rdlock(released);
        }
        else {
          writer = true;
          released = head;
          complete_wrlock();
          ret = 1;
        }
      }
      lock.unlock();
      return ret;
    }
    else {
      lock.unlock();
      return 0;
    }
  }
};

}
#endif
