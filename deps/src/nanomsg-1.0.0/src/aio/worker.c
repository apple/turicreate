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

#include "worker.h"

#if defined NN_HAVE_WINDOWS
#include "worker_win.inc"
#else
#include "worker_posix.inc"
#endif

void nn_worker_timer_init (struct nn_worker_timer *self, struct nn_fsm *owner)
{
    self->owner = owner;
    nn_timerset_hndl_init (&self->hndl);
}

void nn_worker_timer_term (struct nn_worker_timer *self)
{
    nn_timerset_hndl_term (&self->hndl);
}

int nn_worker_timer_isactive (struct nn_worker_timer *self)
{
    return nn_timerset_hndl_isactive (&self->hndl);
}
