/*
    Copyright (c) 2012-2014 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.

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

#include "xpull.h"

#include "../../nn.h"
#include "../../pipeline.h"

#include "../utils/fq.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/alloc.h"
#include "../../utils/attr.h"

struct nn_xpull_data {
    struct nn_fq_data fq;
};

struct nn_xpull {
    struct nn_sockbase sockbase;
    struct nn_fq fq;
};

/*  Private functions. */
static void nn_xpull_init (struct nn_xpull *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint);
static void nn_xpull_term (struct nn_xpull *self);

/*  Implementation of nn_sockbase's virtual functions. */
static void nn_xpull_destroy (struct nn_sockbase *self);
static int nn_xpull_add (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xpull_rm (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xpull_in (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xpull_out (struct nn_sockbase *self, struct nn_pipe *pipe);
static int nn_xpull_events (struct nn_sockbase *self);
static int nn_xpull_recv (struct nn_sockbase *self, struct nn_msg *msg);
static const struct nn_sockbase_vfptr nn_xpull_sockbase_vfptr = {
    NULL,
    nn_xpull_destroy,
    nn_xpull_add,
    nn_xpull_rm,
    nn_xpull_in,
    nn_xpull_out,
    nn_xpull_events,
    NULL,
    nn_xpull_recv,
    NULL,
    NULL
};

static void nn_xpull_init (struct nn_xpull *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint)
{
    nn_sockbase_init (&self->sockbase, vfptr, hint);
    nn_fq_init (&self->fq);
}

static void nn_xpull_term (struct nn_xpull *self)
{
    nn_fq_term (&self->fq);
    nn_sockbase_term (&self->sockbase);
}

void nn_xpull_destroy (struct nn_sockbase *self)
{
    struct nn_xpull *xpull;

    xpull = nn_cont (self, struct nn_xpull, sockbase);

    nn_xpull_term (xpull);
    nn_free (xpull);
}

static int nn_xpull_add (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xpull *xpull;
    struct nn_xpull_data *data;
    int rcvprio;
    size_t sz;

    xpull = nn_cont (self, struct nn_xpull, sockbase);

    sz = sizeof (rcvprio);
    nn_pipe_getopt (pipe, NN_SOL_SOCKET, NN_RCVPRIO, &rcvprio, &sz);
    nn_assert (sz == sizeof (rcvprio));
    nn_assert (rcvprio >= 1 && rcvprio <= 16);

    data = nn_alloc (sizeof (struct nn_xpull_data), "pipe data (pull)");
    alloc_assert (data);
    nn_pipe_setdata (pipe, data);
    nn_fq_add (&xpull->fq, &data->fq, pipe, rcvprio);

    return 0;
}

static void nn_xpull_rm (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xpull *xpull;
    struct nn_xpull_data *data;

    xpull = nn_cont (self, struct nn_xpull, sockbase);
    data = nn_pipe_getdata (pipe);
    nn_fq_rm (&xpull->fq, &data->fq);
    nn_free (data);
}

static void nn_xpull_in (NN_UNUSED struct nn_sockbase *self,
                         struct nn_pipe *pipe)
{
    struct nn_xpull *xpull;
    struct nn_xpull_data *data;

    xpull = nn_cont (self, struct nn_xpull, sockbase);
    data = nn_pipe_getdata (pipe);
    nn_fq_in (&xpull->fq, &data->fq);
}

static void nn_xpull_out (NN_UNUSED struct nn_sockbase *self,
                          NN_UNUSED struct nn_pipe *pipe)
{
    /*  We are not going to send any messages, so there's no point is
        maintaining a list of pipes ready for sending. */
}

static int nn_xpull_events (struct nn_sockbase *self)
{
    return nn_fq_can_recv (&nn_cont (self, struct nn_xpull, sockbase)->fq) ?
        NN_SOCKBASE_EVENT_IN : 0;
}

static int nn_xpull_recv (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;

    rc = nn_fq_recv (&nn_cont (self, struct nn_xpull, sockbase)->fq,
         msg, NULL);

    /*  Discard NN_PIPEBASE_PARSED flag. */
    return rc < 0 ? rc : 0;
}

int nn_xpull_create (void *hint, struct nn_sockbase **sockbase)
{
    struct nn_xpull *self;

    self = nn_alloc (sizeof (struct nn_xpull), "socket (pull)");
    alloc_assert (self);
    nn_xpull_init (self, &nn_xpull_sockbase_vfptr, hint);
    *sockbase = &self->sockbase;

    return 0;
}

int nn_xpull_ispeer (int socktype)
{
    return socktype == NN_PUSH ? 1 : 0;
}

struct nn_socktype nn_xpull_socktype = {
    AF_SP_RAW,
    NN_PULL,
    NN_SOCKTYPE_FLAG_NOSEND,
    nn_xpull_create,
    nn_xpull_ispeer,
};
