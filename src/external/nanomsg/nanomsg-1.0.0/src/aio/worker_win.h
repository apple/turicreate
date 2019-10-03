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

#include "fsm.h"
#include "timerset.h"

#include "../utils/win.h"
#include "../utils/thread.h"

struct nn_worker_task {
    int src;
    struct nn_fsm *owner;
};

#define NN_WORKER_OP_DONE 1
#define NN_WORKER_OP_ERROR 2

struct nn_worker_op {
    int src;
    struct nn_fsm *owner;
    int state;

    /*  This structure is to be used by the user, not nn_worker_op itself.
        Actual usage is specific to the asynchronous operation in question. */
    OVERLAPPED olpd;
};

void nn_worker_op_init (struct nn_worker_op *self, int src,
    struct nn_fsm *owner);
void nn_worker_op_term (struct nn_worker_op *self);

/*  Call this function when asynchronous operation is started.
    If 'zeroiserror' is set to 1, zero bytes transferred will be treated
    as an error. */
void nn_worker_op_start (struct nn_worker_op *self, int zeroiserror);

int nn_worker_op_isidle (struct nn_worker_op *self);

struct nn_worker {
    HANDLE cp;
    struct nn_timerset timerset;
    struct nn_thread thread;
};

HANDLE nn_worker_getcp (struct nn_worker *self);
