/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.

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

#ifndef NN_WORKER_INCLUDED
#define NN_WORKER_INCLUDED

#include "fsm.h"
#include "timerset.h"

#if defined NN_HAVE_WINDOWS
#include "worker_win.h"
#else
#include "worker_posix.h"
#endif

#define NN_WORKER_TIMER_TIMEOUT 1

struct nn_worker_timer {
    struct nn_fsm *owner;
    struct nn_timerset_hndl hndl;
};

void nn_worker_timer_init (struct nn_worker_timer *self,
    struct nn_fsm *owner);
void nn_worker_timer_term (struct nn_worker_timer *self);
int nn_worker_timer_isactive (struct nn_worker_timer *self);

#define NN_WORKER_TASK_EXECUTE 1

struct nn_worker_task;

void nn_worker_task_init (struct nn_worker_task *self, int src,
    struct nn_fsm *owner);
void nn_worker_task_term (struct nn_worker_task *self);

struct nn_worker;

int nn_worker_init (struct nn_worker *self);
void nn_worker_term (struct nn_worker *self);
void nn_worker_execute (struct nn_worker *self, struct nn_worker_task *task);
void nn_worker_cancel (struct nn_worker *self, struct nn_worker_task *task);

void nn_worker_add_timer (struct nn_worker *self, int timeout,
    struct nn_worker_timer *timer);
void nn_worker_rm_timer (struct nn_worker *self,
    struct nn_worker_timer *timer);

#endif

