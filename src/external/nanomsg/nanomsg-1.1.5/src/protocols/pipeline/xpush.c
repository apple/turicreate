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

#include "xpush.h"

#include "../../nn.h"
#include "../../pipeline.h"

#include "../utils/lb.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/alloc.h"
#include "../../utils/attr.h"

struct nn_xpush_data {
    struct nn_lb_data lb;
};

struct nn_xpush {
    struct nn_sockbase sockbase;
    struct nn_lb lb;
};

/*  Private functions. */
static void nn_xpush_init (struct nn_xpush *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint);
static void nn_xpush_term (struct nn_xpush *self);

/*  Implementation of nn_sockbase's virtual functions. */
static void nn_xpush_destroy (struct nn_sockbase *self);
static int nn_xpush_add (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xpush_rm (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xpush_in (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xpush_out (struct nn_sockbase *self, struct nn_pipe *pipe);
static int nn_xpush_events (struct nn_sockbase *self);
static int nn_xpush_send (struct nn_sockbase *self, struct nn_msg *msg);
static const struct nn_sockbase_vfptr nn_xpush_sockbase_vfptr = {
    NULL,
    nn_xpush_destroy,
    nn_xpush_add,
    nn_xpush_rm,
    nn_xpush_in,
    nn_xpush_out,
    nn_xpush_events,
    nn_xpush_send,
    NULL,
    NULL,
    NULL
};

static void nn_xpush_init (struct nn_xpush *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint)
{
    nn_sockbase_init (&self->sockbase, vfptr, hint);
    nn_lb_init (&self->lb);
}

static void nn_xpush_term (struct nn_xpush *self)
{
    nn_lb_term (&self->lb);
    nn_sockbase_term (&self->sockbase);
}

void nn_xpush_destroy (struct nn_sockbase *self)
{
    struct nn_xpush *xpush;

    xpush = nn_cont (self, struct nn_xpush, sockbase);

    nn_xpush_term (xpush);
    nn_free (xpush);
}

static int nn_xpush_add (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xpush *xpush;
    struct nn_xpush_data *data;
    int sndprio;
    size_t sz;

    xpush = nn_cont (self, struct nn_xpush, sockbase);

    sz = sizeof (sndprio);
    nn_pipe_getopt (pipe, NN_SOL_SOCKET, NN_SNDPRIO, &sndprio, &sz);
    nn_assert (sz == sizeof (sndprio));
    nn_assert (sndprio >= 1 && sndprio <= 16);

    data = nn_alloc (sizeof (struct nn_xpush_data), "pipe data (push)");
    alloc_assert (data);
    nn_pipe_setdata (pipe, data);
    nn_lb_add (&xpush->lb, &data->lb, pipe, sndprio);

    return 0;
}

static void nn_xpush_rm (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xpush *xpush;
    struct nn_xpush_data *data;

    xpush = nn_cont (self, struct nn_xpush, sockbase);
    data = nn_pipe_getdata (pipe);
    nn_lb_rm (&xpush->lb, &data->lb);
    nn_free (data);

    nn_sockbase_stat_increment (self, NN_STAT_CURRENT_SND_PRIORITY,
        nn_lb_get_priority (&xpush->lb));
}

static void nn_xpush_in (NN_UNUSED struct nn_sockbase *self,
    NN_UNUSED struct nn_pipe *pipe)
{
    /*  We are not going to receive any messages, so there's no need to store
        the list of inbound pipes. */
}

static void nn_xpush_out (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xpush *xpush;
    struct nn_xpush_data *data;

    xpush = nn_cont (self, struct nn_xpush, sockbase);
    data = nn_pipe_getdata (pipe);
    nn_lb_out (&xpush->lb, &data->lb);
    nn_sockbase_stat_increment (self, NN_STAT_CURRENT_SND_PRIORITY,
        nn_lb_get_priority (&xpush->lb));
}

static int nn_xpush_events (struct nn_sockbase *self)
{
    return nn_lb_can_send (&nn_cont (self, struct nn_xpush, sockbase)->lb) ?
        NN_SOCKBASE_EVENT_OUT : 0;
}

static int nn_xpush_send (struct nn_sockbase *self, struct nn_msg *msg)
{
    return nn_lb_send (&nn_cont (self, struct nn_xpush, sockbase)->lb,
        msg, NULL);
}

int nn_xpush_create (void *hint, struct nn_sockbase **sockbase)
{
    struct nn_xpush *self;

    self = nn_alloc (sizeof (struct nn_xpush), "socket (push)");
    alloc_assert (self);
    nn_xpush_init (self, &nn_xpush_sockbase_vfptr, hint);
    *sockbase = &self->sockbase;

    return 0;
}

int nn_xpush_ispeer (int socktype)
{
    return socktype == NN_PULL ? 1 : 0;
}

struct nn_socktype nn_xpush_socktype = {
    AF_SP_RAW,
    NN_PUSH,
    NN_SOCKTYPE_FLAG_NORECV,
    nn_xpush_create,
    nn_xpush_ispeer,
};
