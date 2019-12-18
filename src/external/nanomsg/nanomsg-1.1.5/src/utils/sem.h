/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.

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

#ifndef NN_SEM_INCLUDED
#define NN_SEM_INCLUDED

/*  Simple semaphore. It can have only two values (0/1 i.e. locked/unlocked). */

struct nn_sem;

/*  Initialise the sem object. It is created in locked state. */
void nn_sem_init (struct nn_sem *self);

/*  Uninitialise the sem object. */
void nn_sem_term (struct nn_sem *self);

/*  Unlock the semaphore. */
void nn_sem_post (struct nn_sem *self);

/*  Waits till sem object becomes unlocked and locks it. */
int nn_sem_wait (struct nn_sem *self);

#if defined NN_HAVE_WINDOWS

#include "win.h"

struct nn_sem {
    HANDLE h;
};

#elif defined NN_HAVE_SEMAPHORE

#include <semaphore.h>

struct nn_sem {
    sem_t sem;
};

#else /*  Simulate semaphore with condition variable. */

#include <pthread.h>

struct nn_sem {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int signaled;
};

#endif

#endif

