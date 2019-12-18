/*
    Copyright (c) 2012-2014 Martin Sustrik  All rights reserved.
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

#include "xreq.h"

#include "../../nn.h"
#include "../../reqrep.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/alloc.h"
#include "../../utils/attr.h"

struct nn_xreq_data {
    struct nn_lb_data lb;
    struct nn_fq_data fq;
};

/*  Private functions. */
static void nn_xreq_destroy (struct nn_sockbase *self);

static const struct nn_sockbase_vfptr nn_xreq_sockbase_vfptr = {
    NULL,
    nn_xreq_destroy,
    nn_xreq_add,
    nn_xreq_rm,
    nn_xreq_in,
    nn_xreq_out,
    nn_xreq_events,
    nn_xreq_send,
    nn_xreq_recv,
    NULL,
    NULL
};

void nn_xreq_init (struct nn_xreq *self, const struct nn_sockbase_vfptr *vfptr,
    void *hint)
{
    nn_sockbase_init (&self->sockbase, vfptr, hint);
    nn_lb_init (&self->lb);
    nn_fq_init (&self->fq);
}

void nn_xreq_term (struct nn_xreq *self)
{
    nn_fq_term (&self->fq);
    nn_lb_term (&self->lb);
    nn_sockbase_term (&self->sockbase);
}

static void nn_xreq_destroy (struct nn_sockbase *self)
{
    struct nn_xreq *xreq;

    xreq = nn_cont (self, struct nn_xreq, sockbase);

    nn_xreq_term (xreq);
    nn_free (xreq);
}

int nn_xreq_add (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xreq *xreq;
    struct nn_xreq_data *data;
    int sndprio;
    int rcvprio;
    size_t sz;

    xreq = nn_cont (self, struct nn_xreq, sockbase);

    sz = sizeof (sndprio);
    nn_pipe_getopt (pipe, NN_SOL_SOCKET, NN_SNDPRIO, &sndprio, &sz);
    nn_assert (sz == sizeof (sndprio));
    nn_assert (sndprio >= 1 && sndprio <= 16);

    sz = sizeof (rcvprio);
    nn_pipe_getopt (pipe, NN_SOL_SOCKET, NN_RCVPRIO, &rcvprio, &sz);
    nn_assert (sz == sizeof (rcvprio));
    nn_assert (rcvprio >= 1 && rcvprio <= 16);

    data = nn_alloc (sizeof (struct nn_xreq_data), "pipe data (req)");
    alloc_assert (data);
    nn_pipe_setdata (pipe, data);
    nn_lb_add (&xreq->lb, &data->lb, pipe, sndprio);
    nn_fq_add (&xreq->fq, &data->fq, pipe, rcvprio);

    return 0;
}

void nn_xreq_rm (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xreq *xreq;
    struct nn_xreq_data *data;

    xreq = nn_cont (self, struct nn_xreq, sockbase);
    data = nn_pipe_getdata (pipe);
    nn_lb_rm (&xreq->lb, &data->lb);
    nn_fq_rm (&xreq->fq, &data->fq);
    nn_free (data);

    nn_sockbase_stat_increment (self, NN_STAT_CURRENT_SND_PRIORITY,
        nn_lb_get_priority (&xreq->lb));
}

void nn_xreq_in (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xreq *xreq;
    struct nn_xreq_data *data;

    xreq = nn_cont (self, struct nn_xreq, sockbase);
    data = nn_pipe_getdata (pipe);
    nn_fq_in (&xreq->fq, &data->fq);
}

void nn_xreq_out (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xreq *xreq;
    struct nn_xreq_data *data;

    xreq = nn_cont (self, struct nn_xreq, sockbase);
    data = nn_pipe_getdata (pipe);
    nn_lb_out (&xreq->lb, &data->lb);

    nn_sockbase_stat_increment (self, NN_STAT_CURRENT_SND_PRIORITY,
        nn_lb_get_priority (&xreq->lb));
}

int nn_xreq_events (struct nn_sockbase *self)
{
    struct nn_xreq *xreq;

    xreq = nn_cont (self, struct nn_xreq, sockbase);

    return (nn_fq_can_recv (&xreq->fq) ? NN_SOCKBASE_EVENT_IN : 0) |
        (nn_lb_can_send (&xreq->lb) ? NN_SOCKBASE_EVENT_OUT : 0);
}

int nn_xreq_send (struct nn_sockbase *self, struct nn_msg *msg)
{
    return nn_xreq_send_to (self, msg, NULL);
}

int nn_xreq_send_to (struct nn_sockbase *self, struct nn_msg *msg,
    struct nn_pipe **to)
{
    int rc;

    /*  If request cannot be sent due to the pushback, drop it silenly. */
    rc = nn_lb_send (&nn_cont (self, struct nn_xreq, sockbase)->lb, msg, to);
    if (nn_slow (rc == -EAGAIN))
        return -EAGAIN;
    errnum_assert (rc >= 0, -rc);

    return 0;
}

int nn_xreq_recv (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;

    rc = nn_fq_recv (&nn_cont (self, struct nn_xreq, sockbase)->fq, msg, NULL);
    if (rc == -EAGAIN)
        return -EAGAIN;
    errnum_assert (rc >= 0, -rc);

    if (!(rc & NN_PIPE_PARSED)) {

        /*  Ignore malformed replies. */
        if (nn_slow (nn_chunkref_size (&msg->body) < sizeof (uint32_t))) {
            nn_msg_term (msg);
            return -EAGAIN;
        }

        /*  Split the message into the header and the body. */
        nn_assert (nn_chunkref_size (&msg->sphdr) == 0);
        nn_chunkref_term (&msg->sphdr);
        nn_chunkref_init (&msg->sphdr, sizeof (uint32_t));
        memcpy (nn_chunkref_data (&msg->sphdr), nn_chunkref_data (&msg->body),
            sizeof (uint32_t));
        nn_chunkref_trim (&msg->body, sizeof (uint32_t));
    }

    return 0;
}

static int nn_xreq_create (void *hint, struct nn_sockbase **sockbase)
{
    struct nn_xreq *self;

    self = nn_alloc (sizeof (struct nn_xreq), "socket (xreq)");
    alloc_assert (self);
    nn_xreq_init (self, &nn_xreq_sockbase_vfptr, hint);
    *sockbase = &self->sockbase;

    return 0;
}

int nn_xreq_ispeer (int socktype)
{
    return socktype == NN_REP ? 1 : 0;
}

struct nn_socktype nn_xreq_socktype = {
    AF_SP_RAW,
    NN_REQ,
    0,
    nn_xreq_create,
    nn_xreq_ispeer,
};
