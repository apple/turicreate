/*
    Copyright (c) 2013-2014 Martin Sustrik  All rights reserved.
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

#include "xbus.h"

#include "../../nn.h"
#include "../../bus.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/alloc.h"
#include "../../utils/attr.h"

#include <stddef.h>
#include <string.h>

/*  To make the algorithm super efficient we directly cast pipe pointers to
    pipe IDs (rather than maintaining a hash table). For this to work, it is
    neccessary for the pointer to fit in 64-bit ID. */
CT_ASSERT (sizeof (uint64_t) >= sizeof (struct nn_pipe*));

/*  Implementation of nn_sockbase's virtual functions. */
static void nn_xbus_destroy (struct nn_sockbase *self);
static const struct nn_sockbase_vfptr nn_xbus_sockbase_vfptr = {
    NULL,
    nn_xbus_destroy,
    nn_xbus_add,
    nn_xbus_rm,
    nn_xbus_in,
    nn_xbus_out,
    nn_xbus_events,
    nn_xbus_send,
    nn_xbus_recv,
    NULL,
    NULL
};

void nn_xbus_init (struct nn_xbus *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint)
{
    nn_sockbase_init (&self->sockbase, vfptr, hint);
    nn_dist_init (&self->outpipes);
    nn_fq_init (&self->inpipes);
}

void nn_xbus_term (struct nn_xbus *self)
{
    nn_fq_term (&self->inpipes);
    nn_dist_term (&self->outpipes);
    nn_sockbase_term (&self->sockbase);
}

static void nn_xbus_destroy (struct nn_sockbase *self)
{
    struct nn_xbus *xbus;

    xbus = nn_cont (self, struct nn_xbus, sockbase);

    nn_xbus_term (xbus);
    nn_free (xbus);
}

int nn_xbus_add (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xbus *xbus;
    struct nn_xbus_data *data;
    int rcvprio;
    size_t sz;

    xbus = nn_cont (self, struct nn_xbus, sockbase);

    sz = sizeof (rcvprio);
    nn_pipe_getopt (pipe, NN_SOL_SOCKET, NN_RCVPRIO, &rcvprio, &sz);
    nn_assert (sz == sizeof (rcvprio));
    nn_assert (rcvprio >= 1 && rcvprio <= 16);

    data = nn_alloc (sizeof (struct nn_xbus_data), "pipe data (xbus)");
    alloc_assert (data);
    nn_fq_add (&xbus->inpipes, &data->initem, pipe, rcvprio);
    nn_dist_add (&xbus->outpipes, &data->outitem, pipe);
    nn_pipe_setdata (pipe, data);

    return 0;
}

void nn_xbus_rm (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xbus *xbus;
    struct nn_xbus_data *data;

    xbus = nn_cont (self, struct nn_xbus, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_fq_rm (&xbus->inpipes, &data->initem);
    nn_dist_rm (&xbus->outpipes, &data->outitem);

    nn_free (data);
}

void nn_xbus_in (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xbus *xbus;
    struct nn_xbus_data *data;

    xbus = nn_cont (self, struct nn_xbus, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_fq_in (&xbus->inpipes, &data->initem);
}

void nn_xbus_out (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xbus *xbus;
    struct nn_xbus_data *data;

    xbus = nn_cont (self, struct nn_xbus, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_dist_out (&xbus->outpipes, &data->outitem);
}

int nn_xbus_events (struct nn_sockbase *self)
{
    return (nn_fq_can_recv (&nn_cont (self, struct nn_xbus,
        sockbase)->inpipes) ? NN_SOCKBASE_EVENT_IN : 0) | NN_SOCKBASE_EVENT_OUT;
}

int nn_xbus_send (struct nn_sockbase *self, struct nn_msg *msg)
{
    size_t hdrsz;
    struct nn_pipe *exclude;

    hdrsz = nn_chunkref_size (&msg->sphdr);
    if (hdrsz == 0)
        exclude = NULL;
    else if (hdrsz == sizeof (uint64_t)) {
        memcpy (&exclude, nn_chunkref_data (&msg->sphdr), sizeof (exclude));
        nn_chunkref_term (&msg->sphdr);
        nn_chunkref_init (&msg->sphdr, 0);
    }
    else
        return -EINVAL;

    return nn_dist_send (&nn_cont (self, struct nn_xbus, sockbase)->outpipes,
        msg, exclude);
}

int nn_xbus_recv (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;
    struct nn_xbus *xbus;
    struct nn_pipe *pipe;

    xbus = nn_cont (self, struct nn_xbus, sockbase);

    while (1) {

        /*  Get next message in fair-queued manner. */
        rc = nn_fq_recv (&xbus->inpipes, msg, &pipe);
        if (nn_slow (rc < 0))
            return rc;

        /*  The message should have no header. Drop malformed messages. */
        if (nn_chunkref_size (&msg->sphdr) == 0)
            break;
        nn_msg_term (msg);
    }

    /*  Add pipe ID to the message header. */
    nn_chunkref_term (&msg->sphdr);
    nn_chunkref_init (&msg->sphdr, sizeof (uint64_t));
    memset (nn_chunkref_data (&msg->sphdr), 0, sizeof (uint64_t));
    memcpy (nn_chunkref_data (&msg->sphdr), &pipe, sizeof (pipe));

    return 0;
}

static int nn_xbus_create (void *hint, struct nn_sockbase **sockbase)
{
    struct nn_xbus *self;

    self = nn_alloc (sizeof (struct nn_xbus), "socket (bus)");
    alloc_assert (self);
    nn_xbus_init (self, &nn_xbus_sockbase_vfptr, hint);
    *sockbase = &self->sockbase;

    return 0;
}

int nn_xbus_ispeer (int socktype)
{
    return socktype == NN_BUS ? 1 : 0;
}

struct nn_socktype nn_xbus_socktype = {
    AF_SP_RAW,
    NN_BUS,
    0,
    nn_xbus_create,
    nn_xbus_ispeer,
};
