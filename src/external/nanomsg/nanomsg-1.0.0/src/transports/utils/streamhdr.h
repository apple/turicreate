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

#ifndef NN_STREAMHDR_INCLUDED
#define NN_STREAMHDR_INCLUDED

#include "../../transport.h"

#include "../../aio/fsm.h"
#include "../../aio/usock.h"
#include "../../aio/timer.h"

/*  This state machine exchanges protocol headers on top of
    a stream-based bi-directional connection. */

#define NN_STREAMHDR_OK 1
#define NN_STREAMHDR_ERROR 2
#define NN_STREAMHDR_STOPPED 3

struct nn_streamhdr {

    /*  The state machine. */
    struct nn_fsm fsm;
    int state;

    /*  Used to timeout the protocol header exchange. */
    struct nn_timer timer;

    /*  The underlying socket. */
    struct nn_usock *usock;

    /*  The original owner of the underlying socket. */
    struct nn_fsm_owner usock_owner;

    /*  Handle to the pipe. */
    struct nn_pipebase *pipebase;

    /*  Protocol header. */
    uint8_t protohdr [8];

    /*  Event fired when the state machine ends. */
    struct nn_fsm_event done;
};

void nn_streamhdr_init (struct nn_streamhdr *self, int src,
    struct nn_fsm *owner);
void nn_streamhdr_term (struct nn_streamhdr *self);

int nn_streamhdr_isidle (struct nn_streamhdr *self);
void nn_streamhdr_start (struct nn_streamhdr *self, struct nn_usock *usock,
    struct nn_pipebase *pipebase);
void nn_streamhdr_stop (struct nn_streamhdr *self);

#endif
