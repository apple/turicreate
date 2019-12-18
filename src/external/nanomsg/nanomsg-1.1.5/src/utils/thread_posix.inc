/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
    Copyright (c) 2014 Achille Roussel All rights reserved.

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

#include "err.h"

#include <signal.h>

static void *nn_thread_main_routine (void *arg)
{
    struct nn_thread *self;

    self = (struct nn_thread*) arg;

    /*  Run the thread routine. */
    self->routine (self->arg);
    return NULL;
}

void nn_thread_init (struct nn_thread *self,
    nn_thread_routine *routine, void *arg)
{
    int rc;
    sigset_t new_sigmask;
    sigset_t old_sigmask;
 
    /*  No signals should be processed by this thread. The library doesn't
        use signals and thus all the signals should be delivered to application
        threads, not to worker threads. */
    rc = sigfillset (&new_sigmask);
    errno_assert (rc == 0);
    rc = pthread_sigmask (SIG_BLOCK, &new_sigmask, &old_sigmask);
    errnum_assert (rc == 0, rc);

    self->routine = routine;
    self->arg = arg;
    rc = pthread_create (&self->handle, NULL, nn_thread_main_routine,
        (void*) self);
    errnum_assert (rc == 0, rc);

    /*  Restore signal set to what it was before. */
    rc = pthread_sigmask (SIG_SETMASK, &old_sigmask, NULL);
    errnum_assert (rc == 0, rc);
}

void nn_thread_term (struct nn_thread *self)
{
    int rc;

    rc = pthread_join (self->handle, NULL);
    errnum_assert (rc == 0, rc);
}
