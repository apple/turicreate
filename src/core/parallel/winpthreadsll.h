/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
/*
 * Posix Threads library for Microsoft Windows
 *
 * Use at own risk, there is no implied warranty to this code.
 * It uses undocumented features of Microsoft Windows that can change
 * at any time in the future.
 *
 * (C) 2010 Lockless Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of Lockless Inc. nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AN
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * You may want to use the MingW64 winpthreads library instead.
 * It is based on this, but adds error checking.
 */

/*
 * Version 1.0.1 Released 2 Feb 2012
 * Fixes pthread_barrier_destroy() to wait for threads to exit the barrier.
 */

#ifndef GRAHPLAB_WIN_PTHREADS
#define GRAHPLAB_WIN_PTHREADS

#define WIN32_LEAN_AND_MEAN
#include <cross_platform/windows_wrapper.hpp>
#undef WIN32_LEAN_AND_MEAN
#include <errno.h>
#include <ctime>
#include <cstring>
#include <cstddef>
#define ETIMEDOUT 110

#define PTHREAD_MUTEX_INITIALIZER {(void*)-1,-1,0,0,0,0}
#define PTHREAD_RWLOCK_INITIALIZER {0}
#define PTHREAD_COND_INITIALIZER {0}
#define PTHREAD_BARRIER_INITIALIZER \
  {0,0,PTHREAD_MUTEX_INITIALIZER,PTHREAD_COND_INITIALIZER}
#define PTHREAD_SPINLOCK_INITIALIZER 0

#define PTHREAD_DESTRUCTOR_ITERATIONS 256
#define PTHREAD_KEYS_MAX (1<<20)

#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_ERRORCHECK 1
#define PTHREAD_MUTEX_RECURSIVE 2
#define PTHREAD_MUTEX_DEFAULT 3
#define PTHREAD_MUTEX_SHARED 4
#define PTHREAD_MUTEX_PRIVATE 0
#define PTHREAD_PRIO_NONE 0
#define PTHREAD_PRIO_INHERIT 8
#define PTHREAD_PRIO_PROTECT 16
#define PTHREAD_PRIO_MULT 32
#define PTHREAD_PROCESS_SHARED 0
#define PTHREAD_PROCESS_PRIVATE 1

#define PTHREAD_BARRIER_SERIAL_THREAD 1
namespace turi {
struct pthread_barrier_t
{
  int count;
  int total;
  CRITICAL_SECTION m;
  CONDITION_VARIABLE cv;
};


typedef unsigned pthread_mutexattr_t;
typedef SRWLOCK pthread_rwlock_t;
typedef CRITICAL_SECTION pthread_mutex_t;
typedef void *pthread_barrierattr_t;
typedef long pthread_spinlock_t;
typedef int pthread_condattr_t;
typedef CONDITION_VARIABLE pthread_cond_t;
typedef int pthread_rwlockattr_t;


static unsigned long long _my_pthread_time_in_ms(void)
{
  struct __timeb64 tb;

  _ftime64(&tb);

  return tb.time * 1000 + tb.millitm;
}

static unsigned long long _my_pthread_time_in_ms_from_timespec(const struct timespec *ts)
{
  unsigned long long t = ts->tv_sec * 1000;
  t += ts->tv_nsec / 1000000;

  return t;
}

static unsigned long long _my_pthread_rel_time_in_ms(const struct timespec *ts)
{
  unsigned long long t1 = _my_pthread_time_in_ms_from_timespec(ts);
  unsigned long long t2 = _my_pthread_time_in_ms();

  /* Prevent underflow */
  if (t1 < t2) return 0;
  return t1 - t2;
}

static int pthread_mutex_lock(pthread_mutex_t *m)
{
  EnterCriticalSection(m);
  return 0;
}

static int pthread_mutex_unlock(pthread_mutex_t *m)
{
  LeaveCriticalSection(m);
  return 0;
}

static int pthread_mutex_trylock(pthread_mutex_t *m)
{
  return TryEnterCriticalSection(m) ? 0 : EBUSY;
}

static int pthread_mutex_init(pthread_mutex_t *m, pthread_mutexattr_t *a)
{
  (void) a;
  InitializeCriticalSection(m);

  return 0;
}

static int pthread_mutex_destroy(pthread_mutex_t *m)
{
  DeleteCriticalSection(m);
  return 0;
}

static int pthread_rwlock_destroy(pthread_rwlock_t *l)
{
  (void) *l;
  return 0;
}

static int pthread_rwlock_rdlock(pthread_rwlock_t *l)
{
  AcquireSRWLockShared(l);
  return 0;
}

static int pthread_rwlock_wrlock(pthread_rwlock_t *l)
{
  AcquireSRWLockExclusive(l);

  return 0;
}
static int pthread_rwlock_unlock(pthread_rwlock_t *l)
{
  void *state = *(void **)l;

  if (state == (void *) 1)
  {
    /* Known to be an exclusive lock */
    ReleaseSRWLockExclusive(l);
  }
  else
  {
    /* A shared unlock will work */
    ReleaseSRWLockShared(l);
  }

  return 0;
}


static int pthread_rwlock_tryrdlock(pthread_rwlock_t *l)
{
  /* Get the current state of the lock */
  void *state = *(void **) l;

  if (!state)
  {
    /* Unlocked to locked */
    if (!_InterlockedCompareExchangePointer((void * volatile*) l, (void *)0x11, NULL)) return 0;
    return EBUSY;
  }

  /* A single writer exists */
  if (state == (void *) 1) return EBUSY;

  /* Multiple writers exist? */
  if ((uintptr_t) state & 14) return EBUSY;

  if (_InterlockedCompareExchangePointer((void * volatile*) l, (void *) ((uintptr_t)state + 16), state) == state) return 0;

  return EBUSY;
}

static int pthread_rwlock_trywrlock(pthread_rwlock_t *l)
{
  /* Try to grab lock if it has no users */
  if (!_InterlockedCompareExchangePointer((void * volatile*) l, (void *)1, NULL)) return 0;

  return EBUSY;
}
static int pthread_rwlock_timedrdlock(pthread_rwlock_t *l, const struct timespec *ts)
{
  unsigned long long ct = _my_pthread_time_in_ms();
  unsigned long long t = _my_pthread_time_in_ms_from_timespec(ts);


  /* Use a busy-loop */
  while (1)
  {
    /* Try to grab lock */
    if (!pthread_rwlock_tryrdlock(l)) return 0;

    /* Get current time */
    ct = _my_pthread_time_in_ms();

    /* Have we waited long enough? */
    if (ct > t) return ETIMEDOUT;
  }
}

static int pthread_rwlock_timedwrlock(pthread_rwlock_t *l, const struct timespec *ts)
{
  unsigned long long ct = _my_pthread_time_in_ms();
  unsigned long long t = _my_pthread_time_in_ms_from_timespec(ts);


  /* Use a busy-loop */
  while (1)
  {
    /* Try to grab lock */
    if (!pthread_rwlock_trywrlock(l)) return 0;

    /* Get current time */
    ct = _my_pthread_time_in_ms();

    /* Have we waited long enough? */
    if (ct > t) return ETIMEDOUT;
  }
}
static int pthread_mutexattr_init(pthread_mutexattr_t *a)
{
  *a = 0;
  return 0;
}

static int pthread_mutexattr_destroy(pthread_mutexattr_t *a)
{
  (void) a;
  return 0;
}

static int pthread_mutexattr_gettype(pthread_mutexattr_t *a, int *type)
{
  *type = *a & 3;

  return 0;
}

static int pthread_mutexattr_settype(pthread_mutexattr_t *a, int type)
{
  if ((unsigned) type > 3) return EINVAL;
  *a &= ~3;
  *a |= type;

  return 0;
}

static int pthread_mutexattr_getpshared(pthread_mutexattr_t *a, int *type)
{
  *type = *a & 4;

  return 0;
}

static int pthread_mutexattr_setpshared(pthread_mutexattr_t * a, int type)
{
  if ((type & 4) != type) return EINVAL;

  *a &= ~4;
  *a |= type;

  return 0;
}

static int pthread_mutexattr_getprotocol(pthread_mutexattr_t *a, int *type)
{
  *type = *a & (8 + 16);

  return 0;
}

static int pthread_mutexattr_setprotocol(pthread_mutexattr_t *a, int type)
{
  if ((type & (8 + 16)) != 8 + 16) return EINVAL;

  *a &= ~(8 + 16);
  *a |= type;

  return 0;
}

static int pthread_mutexattr_getprioceiling(pthread_mutexattr_t *a, int * prio)
{
  *prio = *a / PTHREAD_PRIO_MULT;
  return 0;
}

static int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *a, int prio)
{
  *a &= (PTHREAD_PRIO_MULT - 1);
  *a += prio * PTHREAD_PRIO_MULT;

  return 0;
}

static int pthread_mutex_timedlock(pthread_mutex_t *m, struct timespec *ts)
{
  unsigned long long t, ct;

  struct _pthread_crit_t
  {
    void *debug;
    LONG count;
    LONG r_count;
    HANDLE owner;
    HANDLE sem;
    ULONG_PTR spin;
  };

  /* Try to lock it without waiting */
  if (!pthread_mutex_trylock(m)) return 0;

  ct = _my_pthread_time_in_ms();
  t = _my_pthread_time_in_ms_from_timespec(ts);

  while (1)
  {
    /* Have we waited long enough? */
    if (ct > t) return ETIMEDOUT;

    /* Wait on semaphore within critical section */
    WaitForSingleObject(((struct _pthread_crit_t *)m)->sem, t - ct);

    /* Try to grab lock */
    if (!pthread_mutex_trylock(m)) return 0;

    /* Get current time */
    ct = _my_pthread_time_in_ms();
  }
}

#define _PTHREAD_BARRIER_FLAG (1<<30)

static int pthread_barrier_destroy(pthread_barrier_t *b)
{
  EnterCriticalSection(&b->m);

  while (b->total > _PTHREAD_BARRIER_FLAG)
  {
    /* Wait until everyone exits the barrier */
    SleepConditionVariableCS(&b->cv, &b->m, INFINITE);
  }

  LeaveCriticalSection(&b->m);

  DeleteCriticalSection(&b->m);

  return 0;
}

static int pthread_barrier_init(pthread_barrier_t *b, void *attr, int count)
{
  /* Ignore attr */
  (void) attr;

  b->count = count;
  b->total = 0;

  InitializeCriticalSection(&b->m);
  InitializeConditionVariable(&b->cv);

  return 0;
}

static int pthread_barrier_wait(pthread_barrier_t *b)
{
  EnterCriticalSection(&b->m);

  while (b->total > _PTHREAD_BARRIER_FLAG)
  {
    /* Wait until everyone exits the barrier */
    SleepConditionVariableCS(&b->cv, &b->m, INFINITE);
  }

  /* Are we the first to enter? */
  if (b->total == _PTHREAD_BARRIER_FLAG) b->total = 0;

  b->total++;

  if (b->total == b->count)
  {
    b->total += _PTHREAD_BARRIER_FLAG - 1;
    WakeAllConditionVariable(&b->cv);

    LeaveCriticalSection(&b->m);

    return 1;
  }
  else
  {
    while (b->total < _PTHREAD_BARRIER_FLAG)
    {
      /* Wait until enough threads enter the barrier */
      SleepConditionVariableCS(&b->cv, &b->m, INFINITE);
    }

    b->total--;

    /* Get entering threads to wake up */
    if (b->total == _PTHREAD_BARRIER_FLAG) WakeAllConditionVariable(&b->cv);

    LeaveCriticalSection(&b->m);

    return 0;
  }
}

static int pthread_barrierattr_init(void **attr)
{
  *attr = NULL;
  return 0;
}

static int pthread_barrierattr_destroy(void **attr)
{
  /* Ignore attr */
  (void) attr;

  return 0;
}

static int pthread_barrierattr_setpshared(void **attr, int s)
{
  *attr = (void *) s;
  return 0;
}

static int pthread_barrierattr_getpshared(void **attr, int *s)
{
  *s = (int) (size_t) *attr;

  return 0;
}

static int pthread_spin_init(pthread_spinlock_t *l, int pshared)
{
  (void) pshared;

  *l = 0;
  return 0;
}

static int pthread_spin_destroy(pthread_spinlock_t *l)
{
  (void) l;
  return 0;
}

/* No-fair spinlock due to lack of knowledge of thread number */
static int pthread_spin_lock(pthread_spinlock_t *l)
{
  while (_InterlockedExchange(l, EBUSY))
  {
    /* Don't lock the bus whilst waiting */
    while (*l)
    {
      YieldProcessor();

      /* Compiler barrier.  Prevent caching of *l */
      _ReadWriteBarrier();
    }
  }

  return 0;
}

static int pthread_spin_trylock(pthread_spinlock_t *l)
{
  return _InterlockedExchange(l, EBUSY);
}

static int pthread_spin_unlock(pthread_spinlock_t *l)
{
  /* Compiler barrier.  The store below acts with release symmantics */
  _ReadWriteBarrier();

  *l = 0;

  return 0;
}

static int pthread_cond_init(pthread_cond_t *c, pthread_condattr_t *a)
{
  (void) a;

  InitializeConditionVariable(c);
  return 0;
}

static int pthread_cond_signal(pthread_cond_t *c)
{
  WakeConditionVariable(c);
  return 0;
}

static int pthread_cond_broadcast(pthread_cond_t *c)
{
  WakeAllConditionVariable(c);
  return 0;
}

static int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
  SleepConditionVariableCS(c, m, INFINITE);
  return 0;
}

static int pthread_cond_destroy(pthread_cond_t *c)
{
  (void) c;
  return 0;
}

static int pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m, const struct timespec *t)
{
  unsigned long long tm = _my_pthread_rel_time_in_ms(t);


  if (!SleepConditionVariableCS(c, m, tm)) return ETIMEDOUT;

  /* We can have a spurious wakeup after the timeout */
  if (!_my_pthread_rel_time_in_ms(t)) return ETIMEDOUT;

  return 0;
}

static int pthread_condattr_destroy(pthread_condattr_t *a)
{
  (void) a;
  return 0;
}


static int pthread_condattr_init(pthread_condattr_t *a)
{
  *a = 0;
  return 0;
}

static int pthread_condattr_getpshared(pthread_condattr_t *a, int *s)
{
  *s = *a;
  return 0;
}

static int pthread_condattr_setpshared(pthread_condattr_t *a, int s)
{
  *a = s;
  return 0;
}

static int pthread_rwlock_init(pthread_rwlock_t *l, pthread_rwlockattr_t *a)
{
  (void) a;
  InitializeSRWLock(l);
  return 0;
}


static int pthread_rwlockattr_destroy(pthread_rwlockattr_t *a)
{
  (void) a;
  return 0;
}

static int pthread_rwlockattr_init(pthread_rwlockattr_t *a)
{
  *a = 0;
}

static int pthread_rwlockattr_getpshared(pthread_rwlockattr_t *a, int *s)
{
  *s = *a;
  return 0;
}

static int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *a, int s)
{
  *a = s;
  return 0;
}
}
#endif /* WIN_PTHREADS */
