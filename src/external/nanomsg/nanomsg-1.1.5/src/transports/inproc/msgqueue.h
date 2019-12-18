/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.

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

#ifndef NN_MSGQUEUE_INCLUDED
#define NN_MSGQUEUE_INCLUDED

#include "../../utils/msg.h"

#include <stddef.h>

/*  This class is a simple uni-directional message queue. */

/*  It's not 128 so that chunk including its footer fits into a memory page. */
#define NN_MSGQUEUE_GRANULARITY 126

struct nn_msgqueue_chunk {
    struct nn_msg msgs [NN_MSGQUEUE_GRANULARITY];
    struct nn_msgqueue_chunk *next;
};

struct nn_msgqueue {

    /*  Pointer to the position where next message should be written into
        the message queue. */
    struct {
        struct nn_msgqueue_chunk *chunk;
        int pos;
    } out;

    /*  Pointer to the first unread message in the message queue. */
    struct {
        struct nn_msgqueue_chunk *chunk;
        int pos;
    } in;

    /*  Number of messages in the queue. */
    size_t count;

    /*  Amount of memory used by messages in the queue. */
    size_t mem;

    /*   Maximal queue size (in bytes). */
    size_t maxmem;

    /*  One empty chunk is always cached so that in case of steady stream
        of messages through the pipe there are no memory allocations. */
    struct nn_msgqueue_chunk *cache;
};

/*  Initialise the message pipe. maxmem is the maximal queue size in bytes. */
void nn_msgqueue_init (struct nn_msgqueue *self, size_t maxmem);

/*  Terminate the message pipe. */
void nn_msgqueue_term (struct nn_msgqueue *self);

/*  Returns 1 if there are no messages in the queue, 0 otherwise. */
int nn_msgqueue_empty (struct nn_msgqueue *self);

/*  Writes a message to the pipe. -EAGAIN is returned if the message cannot
    be sent because the queue is full. */
int nn_msgqueue_send (struct nn_msgqueue *self, struct nn_msg *msg);

/*  Reads a message from the pipe. -EAGAIN is returned if there's no message
    to receive. */
int nn_msgqueue_recv (struct nn_msgqueue *self, struct nn_msg *msg);

#endif
