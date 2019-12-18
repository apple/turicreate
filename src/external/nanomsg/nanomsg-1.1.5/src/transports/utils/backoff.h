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

#ifndef NN_BACKOFF_INCLUDED
#define NN_BACKOFF_INCLUDED

#include "../../aio/timer.h"

/*  Timer with exponential backoff. Actual wating time is (2^n-1)*minivl,
    meaning that first wait is 0 ms long, second one is minivl ms long etc. */

#define NN_BACKOFF_TIMEOUT NN_TIMER_TIMEOUT
#define NN_BACKOFF_STOPPED NN_TIMER_STOPPED

struct nn_backoff {
    struct nn_timer timer;
    int minivl;
    int maxivl;
    int n;
};

void nn_backoff_init (struct nn_backoff *self, int src, int minivl, int maxivl,
    struct nn_fsm *owner);
void nn_backoff_term (struct nn_backoff *self);

int nn_backoff_isidle (struct nn_backoff *self);
void nn_backoff_start (struct nn_backoff *self);
void nn_backoff_stop (struct nn_backoff *self);

void nn_backoff_reset (struct nn_backoff *self);

#endif

