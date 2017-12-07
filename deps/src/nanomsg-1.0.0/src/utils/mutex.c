/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
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
#include "err.h"

#include <stdlib.h>

#ifdef NN_HAVE_WINDOWS

void nn_mutex_init (nn_mutex_t *self)
{
    InitializeCriticalSection (&self->cs);
    self->owner = 0;
}

void nn_mutex_term (nn_mutex_t *self)
{
    /*  Make sure we don't free a locked mutex. */
    nn_assert(self->owner == 0);
    DeleteCriticalSection (&self->cs);
}

void nn_mutex_lock (nn_mutex_t *self)
{
    EnterCriticalSection (&self->cs);

    /*  Make sure we don't recursively enter mutexes. */
    nn_assert(self->owner == 0);
    self->owner = GetCurrentThreadId();
}

void nn_mutex_unlock (nn_mutex_t *self)
{
    /*  Make sure that we own the mutex we are releasing. */
    nn_assert(self->owner == GetCurrentThreadId());
    self->owner = 0;
    LeaveCriticalSection (&self->cs);
}

#else

void nn_mutex_init (nn_mutex_t *self)
{
    int rc;
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    errnum_assert (rc == 0, rc);
    rc = pthread_mutex_init (&self->mutex, NULL);
    errnum_assert (rc == 0, rc);
    pthread_mutexattr_destroy(&attr);
}

void nn_mutex_term (nn_mutex_t *self)
{
    int rc;

    rc = pthread_mutex_destroy (&self->mutex);
    errnum_assert (rc == 0, rc);
}

void nn_mutex_lock (nn_mutex_t *self)
{
    int rc;

    rc = pthread_mutex_lock (&self->mutex);
    errnum_assert (rc == 0, rc);
}

void nn_mutex_unlock (nn_mutex_t *self)
{
    int rc;

    rc = pthread_mutex_unlock (&self->mutex);
    errnum_assert (rc == 0, rc);
}

#endif
