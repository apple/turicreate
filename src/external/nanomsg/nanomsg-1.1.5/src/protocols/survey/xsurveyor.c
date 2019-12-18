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

#include "xsurveyor.h"

#include "../../nn.h"
#include "../../survey.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/alloc.h"
#include "../../utils/attr.h"

#include <stddef.h>

/*  Private functions. */
static void nn_xsurveyor_destroy (struct nn_sockbase *self);

/*  Implementation of nn_sockbase's virtual functions. */
static const struct nn_sockbase_vfptr nn_xsurveyor_sockbase_vfptr = {
    NULL,
    nn_xsurveyor_destroy,
    nn_xsurveyor_add,
    nn_xsurveyor_rm,
    nn_xsurveyor_in,
    nn_xsurveyor_out,
    nn_xsurveyor_events,
    nn_xsurveyor_send,
    nn_xsurveyor_recv,
    NULL,
    NULL
};

void nn_xsurveyor_init (struct nn_xsurveyor *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint)
{
    nn_sockbase_init (&self->sockbase, vfptr, hint);
    nn_dist_init (&self->outpipes);
    nn_fq_init (&self->inpipes);
}

void nn_xsurveyor_term (struct nn_xsurveyor *self)
{
    nn_fq_term (&self->inpipes);
    nn_dist_term (&self->outpipes);
    nn_sockbase_term (&self->sockbase);
}

static void nn_xsurveyor_destroy (struct nn_sockbase *self)
{
    struct nn_xsurveyor *xsurveyor;

    xsurveyor = nn_cont (self, struct nn_xsurveyor, sockbase);

    nn_xsurveyor_term (xsurveyor);
    nn_free (xsurveyor);
}

int nn_xsurveyor_add (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xsurveyor *xsurveyor;
    struct nn_xsurveyor_data *data;
    int rcvprio;
    size_t sz;

    xsurveyor = nn_cont (self, struct nn_xsurveyor, sockbase);

    sz = sizeof (rcvprio);
    nn_pipe_getopt (pipe, NN_SOL_SOCKET, NN_RCVPRIO, &rcvprio, &sz);
    nn_assert (sz == sizeof (rcvprio));
    nn_assert (rcvprio >= 1 && rcvprio <= 16);

    data = nn_alloc (sizeof (struct nn_xsurveyor_data),
        "pipe data (xsurveyor)");
    alloc_assert (data);
    data->pipe = pipe;
    nn_fq_add (&xsurveyor->inpipes, &data->initem, pipe, rcvprio);
    nn_dist_add (&xsurveyor->outpipes, &data->outitem, pipe);
    nn_pipe_setdata (pipe, data);

    return 0;
}

void nn_xsurveyor_rm (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xsurveyor *xsurveyor;
    struct nn_xsurveyor_data *data;

    xsurveyor = nn_cont (self, struct nn_xsurveyor, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_fq_rm (&xsurveyor->inpipes, &data->initem);
    nn_dist_rm (&xsurveyor->outpipes, &data->outitem);

    nn_free (data);
}

void nn_xsurveyor_in (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xsurveyor *xsurveyor;
    struct nn_xsurveyor_data *data;

    xsurveyor = nn_cont (self, struct nn_xsurveyor, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_fq_in (&xsurveyor->inpipes, &data->initem);
}

void nn_xsurveyor_out (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xsurveyor *xsurveyor;
    struct nn_xsurveyor_data *data;

    xsurveyor = nn_cont (self, struct nn_xsurveyor, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_dist_out (&xsurveyor->outpipes, &data->outitem);
}

int nn_xsurveyor_events (struct nn_sockbase *self)
{
    struct nn_xsurveyor *xsurveyor;
    int events;

    xsurveyor = nn_cont (self, struct nn_xsurveyor, sockbase);

    events = NN_SOCKBASE_EVENT_OUT;
    if (nn_fq_can_recv (&xsurveyor->inpipes))
        events |= NN_SOCKBASE_EVENT_IN;
    return events;
}

int nn_xsurveyor_send (struct nn_sockbase *self, struct nn_msg *msg)
{
    return nn_dist_send (
        &nn_cont (self, struct nn_xsurveyor, sockbase)->outpipes, msg, NULL);
}

int nn_xsurveyor_recv (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;
    struct nn_xsurveyor *xsurveyor;

    xsurveyor = nn_cont (self, struct nn_xsurveyor, sockbase);

    rc = nn_fq_recv (&xsurveyor->inpipes, msg, NULL);
    if (nn_slow (rc < 0))
        return rc;

    /*  Split the header from the body, if needed. */
    if (!(rc & NN_PIPE_PARSED)) {
        if (nn_slow (nn_chunkref_size (&msg->body) < sizeof (uint32_t))) {
            nn_msg_term (msg);
            return -EAGAIN;
        }
        nn_assert (nn_chunkref_size (&msg->sphdr) == 0);
        nn_chunkref_term (&msg->sphdr);
        nn_chunkref_init (&msg->sphdr, sizeof (uint32_t));
        memcpy (nn_chunkref_data (&msg->sphdr), nn_chunkref_data (&msg->body),
           sizeof (uint32_t));
        nn_chunkref_trim (&msg->body, sizeof (uint32_t));
    }

    return 0;
}

static int nn_xsurveyor_create (void *hint, struct nn_sockbase **sockbase)
{
    struct nn_xsurveyor *self;

    self = nn_alloc (sizeof (struct nn_xsurveyor), "socket (xsurveyor)");
    alloc_assert (self);
    nn_xsurveyor_init (self, &nn_xsurveyor_sockbase_vfptr, hint);
    *sockbase = &self->sockbase;

    return 0;
}

int nn_xsurveyor_ispeer (int socktype)
{
    return socktype == NN_RESPONDENT ? 1 : 0;
}

struct nn_socktype nn_xsurveyor_socktype = {
    AF_SP_RAW,
    NN_SURVEYOR,
    0,
    nn_xsurveyor_create,
    nn_xsurveyor_ispeer,
};
