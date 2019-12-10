/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.

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

#ifndef NN_MUTEX_INCLUDED
#define NN_MUTEX_INCLUDED

#ifdef NN_HAVE_WINDOWS
#include "win.h"
#else
#include <pthread.h>
#endif

struct nn_mutex {
    /*  NB: The fields of this structure are private to the mutex
        implementation. */
#ifdef NN_HAVE_WINDOWS
    CRITICAL_SECTION cs;
    DWORD owner;
    int debug;
#else
    pthread_mutex_t mutex;
#endif
};

typedef struct nn_mutex nn_mutex_t;

/*  Initialise the mutex. */
void nn_mutex_init (nn_mutex_t *self);

/*  Terminate the mutex. */
void nn_mutex_term (nn_mutex_t *self);

/*  Lock the mutex. Behaviour of multiple locks from the same thread is
    undefined. */
void nn_mutex_lock (nn_mutex_t *self);

/*  Unlock the mutex. Behaviour of unlocking an unlocked mutex is undefined */
void nn_mutex_unlock (nn_mutex_t *self);

#endif
