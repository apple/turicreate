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

#include "xpub.h"

#include "../../nn.h"
#include "../../pubsub.h"

#include "../utils/dist.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/alloc.h"
#include "../../utils/list.h"
#include "../../utils/attr.h"

#include <stddef.h>

struct nn_xpub_data {
    struct nn_dist_data item;
};

struct nn_xpub {

    /*  The generic socket base class. */
    struct nn_sockbase sockbase;

    /*  Distributor. */
    struct nn_dist outpipes;
};

/*  Private functions. */
static void nn_xpub_init (struct nn_xpub *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint);
static void nn_xpub_term (struct nn_xpub *self);

/*  Implementation of nn_sockbase's virtual functions. */
static void nn_xpub_destroy (struct nn_sockbase *self);
static int nn_xpub_add (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xpub_rm (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xpub_in (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xpub_out (struct nn_sockbase *self, struct nn_pipe *pipe);
static int nn_xpub_events (struct nn_sockbase *self);
static int nn_xpub_send (struct nn_sockbase *self, struct nn_msg *msg);
static int nn_xpub_setopt (struct nn_sockbase *self, int level, int option,
    const void *optval, size_t optvallen);
static int nn_xpub_getopt (struct nn_sockbase *self, int level, int option,
    void *optval, size_t *optvallen);
static const struct nn_sockbase_vfptr nn_xpub_sockbase_vfptr = {
    NULL,
    nn_xpub_destroy,
    nn_xpub_add,
    nn_xpub_rm,
    nn_xpub_in,
    nn_xpub_out,
    nn_xpub_events,
    nn_xpub_send,
    NULL,
    nn_xpub_setopt,
    nn_xpub_getopt
};

static void nn_xpub_init (struct nn_xpub *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint)
{
    nn_sockbase_init (&self->sockbase, vfptr, hint);
    nn_dist_init (&self->outpipes);
}

static void nn_xpub_term (struct nn_xpub *self)
{
    nn_dist_term (&self->outpipes);
    nn_sockbase_term (&self->sockbase);
}

void nn_xpub_destroy (struct nn_sockbase *self)
{
    struct nn_xpub *xpub;

    xpub = nn_cont (self, struct nn_xpub, sockbase);

    nn_xpub_term (xpub);
    nn_free (xpub);
}

static int nn_xpub_add (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xpub *xpub;
    struct nn_xpub_data *data;

    xpub = nn_cont (self, struct nn_xpub, sockbase);

    data = nn_alloc (sizeof (struct nn_xpub_data), "pipe data (pub)");
    alloc_assert (data);
    nn_dist_add (&xpub->outpipes, &data->item, pipe);
    nn_pipe_setdata (pipe, data);

    return 0;
}

static void nn_xpub_rm (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xpub *xpub;
    struct nn_xpub_data *data;

    xpub = nn_cont (self, struct nn_xpub, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_dist_rm (&xpub->outpipes, &data->item);

    nn_free (data);
}

static void nn_xpub_in (NN_UNUSED struct nn_sockbase *self,
                       NN_UNUSED struct nn_pipe *pipe)
{
    /*  We shouldn't get any messages from subscribers. */
    nn_assert (0);
}

static void nn_xpub_out (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xpub *xpub;
    struct nn_xpub_data *data;

    xpub = nn_cont (self, struct nn_xpub, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_dist_out (&xpub->outpipes, &data->item);
}

static int nn_xpub_events (NN_UNUSED struct nn_sockbase *self)
{
    return NN_SOCKBASE_EVENT_OUT;
}

static int nn_xpub_send (struct nn_sockbase *self, struct nn_msg *msg)
{
    return nn_dist_send (&nn_cont (self, struct nn_xpub, sockbase)->outpipes,
        msg, NULL);
}

static int nn_xpub_setopt (NN_UNUSED struct nn_sockbase *self,
    NN_UNUSED int level, NN_UNUSED int option,
    NN_UNUSED const void *optval, NN_UNUSED size_t optvallen)
{
    return -ENOPROTOOPT;
}

static int nn_xpub_getopt (NN_UNUSED struct nn_sockbase *self,
    NN_UNUSED int level, NN_UNUSED int option,
    NN_UNUSED void *optval, NN_UNUSED size_t *optvallen)
{
    return -ENOPROTOOPT;
}

int nn_xpub_create (void *hint, struct nn_sockbase **sockbase)
{
    struct nn_xpub *self;

    self = nn_alloc (sizeof (struct nn_xpub), "socket (xpub)");
    alloc_assert (self);
    nn_xpub_init (self, &nn_xpub_sockbase_vfptr, hint);
    *sockbase = &self->sockbase;

    return 0;
}

int nn_xpub_ispeer (int socktype)
{
     return socktype == NN_SUB ? 1 : 0;
}

static struct nn_socktype nn_xpub_socktype_struct = {
    AF_SP_RAW,
    NN_PUB,
    NN_SOCKTYPE_FLAG_NORECV,
    nn_xpub_create,
    nn_xpub_ispeer,
    NN_LIST_ITEM_INITIALIZER
};

struct nn_socktype *nn_xpub_socktype = &nn_xpub_socktype_struct;
