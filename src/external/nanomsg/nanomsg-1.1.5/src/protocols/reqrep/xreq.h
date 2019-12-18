/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
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

#ifndef NN_XREQ_INCLUDED
#define NN_XREQ_INCLUDED

#include "../../protocol.h"

#include "../utils/lb.h"
#include "../utils/fq.h"

struct nn_xreq {
    struct nn_sockbase sockbase;
    struct nn_lb lb;
    struct nn_fq fq;
};

void nn_xreq_init (struct nn_xreq *self, const struct nn_sockbase_vfptr *vfptr,
    void *hint);
void nn_xreq_term (struct nn_xreq *self);

int nn_xreq_add (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xreq_rm (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xreq_in (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xreq_out (struct nn_sockbase *self, struct nn_pipe *pipe);
int nn_xreq_events (struct nn_sockbase *self);
int nn_xreq_send (struct nn_sockbase *self, struct nn_msg *msg);
int nn_xreq_send_to (struct nn_sockbase *self, struct nn_msg *msg,
    struct nn_pipe **to);
int nn_xreq_recv (struct nn_sockbase *self, struct nn_msg *msg);

int nn_xreq_ispeer (int socktype);

#endif
