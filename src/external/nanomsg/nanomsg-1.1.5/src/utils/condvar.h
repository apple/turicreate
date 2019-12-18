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

#ifndef NN_CONDVAR_INCLUDED
#define NN_CONDVAR_INCLUDED

#include "mutex.h"

#ifdef NN_HAVE_WINDOWS
#include "win.h"
struct nn_condvar {
    CONDITION_VARIABLE cv;
};

#else /* !NN_HAVE_WINDOWS */

#include <pthread.h>

struct nn_condvar {
    pthread_cond_t cv;
};

#endif /* NN_HAVE_WINDOWS */

typedef struct nn_condvar nn_condvar_t;

/*  Initialise the condvar. */
int nn_condvar_init (nn_condvar_t *cond);

/*  Terminate the condvar. */
void nn_condvar_term (nn_condvar_t *cond);

/*  Sleep on a condition variable, with a possible timeout.  The mutex should
    be held when calling, and will be dropped on entry and reacquired
    atomically on return.  The caller will wake when signaled, or when the
    timeout expires, but may be woken spuriously as well.  If the timeout
    expires without a signal, -ETIMEDOUT will be returned, otherwise 0
    will be returned.  If expire is < 0, then no timeout will be used,
    representing a potentially infinite wait. */
int nn_condvar_wait (nn_condvar_t *cond, nn_mutex_t *lock, int timeout);

/*  Signal (wake) one condition variable waiter. */
void nn_condvar_signal (nn_condvar_t *cond);

/* Signal all condition variable waiters, waking all of them. */
void nn_condvar_broadcast (nn_condvar_t *cond);

#endif
