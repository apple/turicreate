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

#ifndef NN_TIMER_INCLUDED
#define NN_TIMER_INCLUDED

#include "fsm.h"
#include "worker.h"

#define NN_TIMER_TIMEOUT 1
#define NN_TIMER_STOPPED 2

struct nn_timer {
    struct nn_fsm fsm;
    int state;
    struct nn_worker_task start_task;
    struct nn_worker_task stop_task;
    struct nn_worker_timer wtimer;
    struct nn_fsm_event done;
    struct nn_worker *worker;
    int timeout;
};

void nn_timer_init (struct nn_timer *self, int src, struct nn_fsm *owner);
void nn_timer_term (struct nn_timer *self);

int nn_timer_isidle (struct nn_timer *self);
void nn_timer_start (struct nn_timer *self, int timeout);
void nn_timer_stop (struct nn_timer *self);

#endif
