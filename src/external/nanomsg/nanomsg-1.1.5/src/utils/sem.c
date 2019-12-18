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

#include "sem.h"
#include "err.h"
#include "fast.h"


#if defined NN_HAVE_WINDOWS

void nn_sem_init (struct nn_sem *self)
{
    self->h = CreateEvent (NULL, FALSE, FALSE, NULL);
    win_assert (self->h);
}

void nn_sem_term (struct nn_sem *self)
{
    BOOL brc;

    brc = CloseHandle (self->h);
    win_assert (brc);
}

void nn_sem_post (struct nn_sem *self)
{
    BOOL brc;

    brc = SetEvent (self->h);
    win_assert (brc);
}

int nn_sem_wait (struct nn_sem *self)
{
    DWORD rc;

    rc = WaitForSingleObject (self->h, INFINITE);
    win_assert (rc != WAIT_FAILED);
    nn_assert (rc == WAIT_OBJECT_0);

    return 0;
}

#elif defined NN_HAVE_SEMAPHORE

void nn_sem_init (struct nn_sem *self)
{
    int rc;

    rc = sem_init (&self->sem, 0, 0);
    errno_assert (rc == 0);
}

void nn_sem_term (struct nn_sem *self)
{
    int rc;

    rc = sem_destroy (&self->sem);
    errno_assert (rc == 0);
}

void nn_sem_post (struct nn_sem *self)
{
    int rc;

    rc = sem_post (&self->sem);
    errno_assert (rc == 0);
}

int nn_sem_wait (struct nn_sem *self)
{
    int rc;

    rc = sem_wait (&self->sem);
    if (nn_slow (rc < 0 && errno == EINTR))
        return -EINTR;
    errno_assert (rc == 0);
    return 0;
}

#else

/*  Simulate semaphores with condition variables. */

void nn_sem_init (struct nn_sem *self)
{
    int rc;

    rc = pthread_mutex_init (&self->mutex, NULL);
    errnum_assert (rc == 0, rc);
    rc = pthread_cond_init (&self->cond, NULL);
    errnum_assert (rc == 0, rc);
    self->signaled = 0;
}

void nn_sem_term (struct nn_sem *self)
{
    int rc;

    rc = pthread_cond_destroy (&self->cond);
    errnum_assert (rc == 0, rc);
    rc = pthread_mutex_destroy (&self->mutex);
    errnum_assert (rc == 0, rc);
}

void nn_sem_post (struct nn_sem *self)
{
    int rc;

    rc = pthread_mutex_lock (&self->mutex);
    errnum_assert (rc == 0, rc);
    nn_assert (self->signaled == 0);
    self->signaled = 1;
    rc = pthread_cond_signal (&self->cond);
    errnum_assert (rc == 0, rc);
    rc = pthread_mutex_unlock (&self->mutex);
    errnum_assert (rc == 0, rc);
}

int nn_sem_wait (struct nn_sem *self)
{
    int rc;

    /*  With OSX, semaphores are global named objects. They are not useful for
        our use case. To get a similar object we exploit the implementation
        detail of pthread_cond_wait() in Darwin kernel: It exits if signal is
        caught. Note that this behaviour is not mandated by POSIX
        and may break with future versions of Darwin. */
    rc = pthread_mutex_lock (&self->mutex);
    errnum_assert (rc == 0, rc);
    if (nn_fast (self->signaled)) {
        rc = pthread_mutex_unlock (&self->mutex);
        errnum_assert (rc == 0, rc);
        return 0;
    }
    rc = pthread_cond_wait (&self->cond, &self->mutex);
    errnum_assert (rc == 0, rc);
    if (nn_slow (!self->signaled)) {
        rc = pthread_mutex_unlock (&self->mutex);
        errnum_assert (rc == 0, rc);
        return -EINTR;
    }
    self->signaled = 0;
    rc = pthread_mutex_unlock (&self->mutex);
    errnum_assert (rc == 0, rc);

    return 0;
}


#endif

