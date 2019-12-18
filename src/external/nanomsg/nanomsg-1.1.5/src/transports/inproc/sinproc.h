/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.
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

#ifndef NN_SINPROC_INCLUDED
#define NN_SINPROC_INCLUDED

#include "msgqueue.h"

#include "../../transport.h"

#include "../../aio/fsm.h"

#include "../../utils/msg.h"
#include "../../utils/list.h"

#define NN_SINPROC_CONNECT 1
#define NN_SINPROC_READY 2
#define NN_SINPROC_ACCEPTED 3
#define NN_SINPROC_SENT 4
#define NN_SINPROC_RECEIVED 5
#define NN_SINPROC_DISCONNECT 6
#define NN_SINPROC_STOPPED 7

/*  We use a random value here to prevent accidental clashes with the peer's
    internal source IDs. */
#define NN_SINPROC_SRC_PEER 27713

struct nn_sinproc {

    /*  The state machine. */
    struct nn_fsm fsm;
    int state;

    /*  Any combination of the flags defined in the .c file. */
    int flags;

    /*  Pointer to the peer inproc session, if connected. NULL otherwise. */
    struct nn_sinproc *peer;

    /*  Pipe connecting this inproc connection to the nanomsg core. */
    struct nn_pipebase pipebase;

    /*  Inbound message queue. The messages contained are meant to be received
        by the user later on. */
    struct nn_msgqueue msgqueue;

    /*  This message is the one being sent from this session to the peer
        session. It holds the data only temporarily, until the peer moves
        it to its msgqueue. */
    struct nn_msg msg;

    /*  Outbound events. I.e. event sent by this sinproc to the peer sinproc. */
    struct nn_fsm_event event_connect;

    /*  Inbound events. I.e. events sent by the peer sinproc to this inproc. */
    struct nn_fsm_event event_sent;
    struct nn_fsm_event event_received;
    struct nn_fsm_event event_disconnect;

    /*  This member is used only if we are on the bound side. binproc object
        has a list of sinprocs it handles. */
    struct nn_list_item item;
};

void nn_sinproc_init (struct nn_sinproc *self, int src,
    struct nn_ep *ep, struct nn_fsm *owner);
void nn_sinproc_term (struct nn_sinproc *self);
int nn_sinproc_isidle (struct nn_sinproc *self);

/*  Connect and accept are two different ways to start the state machine. */
void nn_sinproc_connect (struct nn_sinproc *self, struct nn_fsm *peer);
void nn_sinproc_accept (struct nn_sinproc *self, struct nn_sinproc *peer);
void nn_sinproc_stop (struct nn_sinproc *self);

#endif
