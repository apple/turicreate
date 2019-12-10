/*
    Copyright 2016 Garrett D'Amore <garrett@damore.org>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom
    the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
*/

#include "mutex.h"
#include "condvar.h"
#include "err.h"

#if NN_HAVE_WINDOWS

int nn_condvar_init (nn_condvar_t *cond)
{
    InitializeConditionVariable (&cond->cv);
    return (0);
}

void nn_condvar_term (nn_condvar_t *cond)
{
}

int nn_condvar_wait (nn_condvar_t *cond, nn_mutex_t *lock, int timeout)
{
    BOOL brc;
    DWORD expire;

    /*  Likely this is redundant, but for API correctness be explicit. */
    expire = (timeout < 0) ? INFINITE : (DWORD) timeout;

    /*  We must own the lock if we are going to call this. */
    nn_assert (lock->owner == GetCurrentThreadId());
    /*  Clear ownership as SleepConditionVariableCS will drop it. */
    lock->owner = 0;

    brc = SleepConditionVariableCS (&cond->cv, &lock->cs, expire);

    /*  We have reacquired the lock, so nobody should own it right now. */
    nn_assert (lock->owner == 0);
    /*  Note we own it now. */
    lock->owner = GetCurrentThreadId();

    if (!brc && GetLastError () == ERROR_TIMEOUT) {
        return (-ETIMEDOUT);
    }
    return (0);
}

void nn_condvar_signal (nn_condvar_t *cond)
{
    WakeConditionVariable (&cond->cv);
}

void nn_condvar_broadcast (nn_condvar_t *cond)
{
    WakeAllConditionVariable (&cond->cv);
}

#else /* !NN_HAVE_WINDOWS */

#include <sys/time.h>

int nn_condvar_init (nn_condvar_t *cond)
{
    int rc;

    /* This should really never fail, but the system may do so for
       ENOMEM or EAGAIN due to resource exhaustion, or EBUSY if reusing
       a condition variable with no intervening destroy call. */
    rc = pthread_cond_init (&cond->cv, NULL);
    return (-rc);
}

void nn_condvar_term (nn_condvar_t *cond)
{
    int rc;

    /* This should never fail, the system is allowed to return EBUSY if
       there are outstanding waiters (a serious bug!), or EINVAL if the
       cv is somehow invalid or illegal.  Either of these cases would
       represent a serious programming defect in our caller. */
    rc = pthread_cond_destroy (&cond->cv);
    errnum_assert (rc == 0, rc);
}

int nn_condvar_wait (nn_condvar_t *cond, nn_mutex_t *lock, int timeout)
{
    int rc;
    struct timeval tv;
    struct timespec ts;

    if (timeout < 0) {
        /*  This is an infinite sleep.  We don't care about return values,
            as any error we can treat as just a premature wakeup. */
        (void) pthread_cond_wait (&cond->cv, &lock->mutex);
        return (0);
    }

    rc = gettimeofday(&tv, NULL);
    errnum_assert (rc == 0, rc);

    /* There are extra operations performed here, but they are done to avoid
       wrap of the tv_usec and ts_nsec members on 32-bit systems. */
    tv.tv_sec += timeout / 1000;
    tv.tv_usec += (timeout % 1000) * 1000;

    ts.tv_sec = tv.tv_sec + (tv.tv_usec / 1000000);
    ts.tv_nsec = (tv.tv_usec % 1000000) * 1000;

    rc = pthread_cond_timedwait (&cond->cv, &lock->mutex, &ts);
    if (rc == ETIMEDOUT)
        return (-ETIMEDOUT);
    /* Treat all other cases (including errors) as normal wakeup. */
    return (0);
}

void nn_condvar_signal (nn_condvar_t *cond)
{
    /* The only legal failure mode here is EINVAL if we passed a bad
       condition variable.  We don't check that. */
    (void) pthread_cond_signal (&cond->cv);
}

void nn_condvar_broadcast (nn_condvar_t *cond)
{
    /* The only legal failure mode here is EINVAL if we passed a bad
       condition variable.  We don't check that. */
    (void) pthread_cond_broadcast (&cond->cv);
}

#endif
