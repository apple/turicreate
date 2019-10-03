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

#ifndef NN_XREP_INCLUDED
#define NN_XREP_INCLUDED

#include "../../protocol.h"

#include "../../utils/hash.h"

#include "../utils/fq.h"

#include <stddef.h>

#define NN_XREP_OUT 1

struct nn_xrep_data {
    struct nn_pipe *pipe;
    struct nn_hash_item outitem;
    struct nn_fq_data initem;
    uint32_t flags;
};

struct nn_xrep {

    struct nn_sockbase sockbase;

    /*  Key to be assigned to the next added pipe. */
    uint32_t next_key;

    /*  Map of all registered pipes indexed by the peer ID. */
    struct nn_hash outpipes;

    /*  Fair-queuer to get messages from. */
    struct nn_fq inpipes;
};

void nn_xrep_init (struct nn_xrep *self, const struct nn_sockbase_vfptr *vfptr,
    void *hint);
void nn_xrep_term (struct nn_xrep *self);

int nn_xrep_add (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xrep_rm (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xrep_in (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xrep_out (struct nn_sockbase *self, struct nn_pipe *pipe);
int nn_xrep_events (struct nn_sockbase *self);
int nn_xrep_send (struct nn_sockbase *self, struct nn_msg *msg);
int nn_xrep_recv (struct nn_sockbase *self, struct nn_msg *msg);
int nn_xrep_setopt (struct nn_sockbase *self, int level, int option,
    const void *optval, size_t optvallen);
int nn_xrep_getopt (struct nn_sockbase *self, int level, int option,
    void *optval, size_t *optvallen);

int nn_xrep_ispeer (int socktype);

extern struct nn_socktype *nn_xrep_socktype;

#endif
