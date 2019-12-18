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

#ifndef NN_XBUS_INCLUDED
#define NN_XBUS_INCLUDED

#include "../../protocol.h"

#include "../utils/dist.h"
#include "../utils/fq.h"

struct nn_xbus_data {
    struct nn_dist_data outitem;
    struct nn_fq_data initem;
};

struct nn_xbus {
    struct nn_sockbase sockbase;
    struct nn_dist outpipes;
    struct nn_fq inpipes;
};

void nn_xbus_init (struct nn_xbus *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint);
void nn_xbus_term (struct nn_xbus *self);

int nn_xbus_add (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xbus_rm (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xbus_in (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xbus_out (struct nn_sockbase *self, struct nn_pipe *pipe);
int nn_xbus_events (struct nn_sockbase *self);
int nn_xbus_send (struct nn_sockbase *self, struct nn_msg *msg);
int nn_xbus_recv (struct nn_sockbase *self, struct nn_msg *msg);

int nn_xbus_ispeer (int socktype);

#endif
