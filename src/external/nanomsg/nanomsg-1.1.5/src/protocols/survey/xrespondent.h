/*
    Copyright (c) 201-2013 Martin Sustrik  All rights reserved.
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

#ifndef NN_XRESPONDENT_INCLUDED
#define NN_XRESPONDENT_INCLUDED

#include "../../protocol.h"

#include "../../utils/hash.h"
#include "../utils/fq.h"

#define NN_XRESPONDENT_OUT 1

struct nn_xrespondent_data {
    struct nn_pipe *pipe;
    struct nn_hash_item outitem;
    struct nn_fq_data initem;
    uint32_t flags;
};

struct nn_xrespondent {
    struct nn_sockbase sockbase;

    /*  Key to be assigned to the next added pipe. */
    uint32_t next_key;

    /*  Map of all registered pipes indexed by the peer ID. */
    struct nn_hash outpipes;

    /*  Fair-queuer to get surveys from. */
    struct nn_fq inpipes;
};

void nn_xrespondent_init (struct nn_xrespondent *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint);
void nn_xrespondent_term (struct nn_xrespondent *self);

int nn_xrespondent_add (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xrespondent_rm (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xrespondent_in (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xrespondent_out (struct nn_sockbase *self, struct nn_pipe *pipe);
int nn_xrespondent_events (struct nn_sockbase *self);
int nn_xrespondent_send (struct nn_sockbase *self, struct nn_msg *msg);
int nn_xrespondent_recv (struct nn_sockbase *self, struct nn_msg *msg);

int nn_xrespondent_ispeer (int socktype);

#endif
