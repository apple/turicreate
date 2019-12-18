/*
    Copyright (c) 2012-2014 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
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

#include "xsub.h"
#include "trie.h"

#include "../../nn.h"
#include "../../pubsub.h"

#include "../utils/fq.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/alloc.h"
#include "../../utils/attr.h"

struct nn_xsub_data {
    struct nn_fq_data fq;
};

struct nn_xsub {
    struct nn_sockbase sockbase;
    struct nn_fq fq;
    struct nn_trie trie;
};

/*  Private functions. */
static void nn_xsub_init (struct nn_xsub *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint);
static void nn_xsub_term (struct nn_xsub *self);

/*  Implementation of nn_sockbase's virtual functions. */
static void nn_xsub_destroy (struct nn_sockbase *self);
static int nn_xsub_add (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xsub_rm (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xsub_in (struct nn_sockbase *self, struct nn_pipe *pipe);
static void nn_xsub_out (struct nn_sockbase *self, struct nn_pipe *pipe);
static int nn_xsub_events (struct nn_sockbase *self);
static int nn_xsub_recv (struct nn_sockbase *self, struct nn_msg *msg);
static int nn_xsub_setopt (struct nn_sockbase *self, int level, int option,
    const void *optval, size_t optvallen);
static const struct nn_sockbase_vfptr nn_xsub_sockbase_vfptr = {
    NULL,
    nn_xsub_destroy,
    nn_xsub_add,
    nn_xsub_rm,
    nn_xsub_in,
    nn_xsub_out,
    nn_xsub_events,
    NULL,
    nn_xsub_recv,
    nn_xsub_setopt,
    NULL
};

static void nn_xsub_init (struct nn_xsub *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint)
{
    nn_sockbase_init (&self->sockbase, vfptr, hint);
    nn_fq_init (&self->fq);
    nn_trie_init (&self->trie);
}

static void nn_xsub_term (struct nn_xsub *self)
{
    nn_trie_term (&self->trie);
    nn_fq_term (&self->fq);
    nn_sockbase_term (&self->sockbase);
}

void nn_xsub_destroy (struct nn_sockbase *self)
{
    struct nn_xsub *xsub;

    xsub = nn_cont (self, struct nn_xsub, sockbase);

    nn_xsub_term (xsub);
    nn_free (xsub);
}

static int nn_xsub_add (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xsub *xsub;
    struct nn_xsub_data *data;
    int rcvprio;
    size_t sz;

    xsub = nn_cont (self, struct nn_xsub, sockbase);

    sz = sizeof (rcvprio);
    nn_pipe_getopt (pipe, NN_SOL_SOCKET, NN_RCVPRIO, &rcvprio, &sz);
    nn_assert (sz == sizeof (rcvprio));
    nn_assert (rcvprio >= 1 && rcvprio <= 16);

    data = nn_alloc (sizeof (struct nn_xsub_data), "pipe data (sub)");
    alloc_assert (data);
    nn_pipe_setdata (pipe, data);
    nn_fq_add (&xsub->fq, &data->fq, pipe, rcvprio);

    return 0;
}

static void nn_xsub_rm (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xsub *xsub;
    struct nn_xsub_data *data;

    xsub = nn_cont (self, struct nn_xsub, sockbase);
    data = nn_pipe_getdata (pipe);
    nn_fq_rm (&xsub->fq, &data->fq);
    nn_free (data);
}

static void nn_xsub_in (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xsub *xsub;
    struct nn_xsub_data *data;

    xsub = nn_cont (self, struct nn_xsub, sockbase);
    data = nn_pipe_getdata (pipe);
    nn_fq_in (&xsub->fq, &data->fq);
}

static void nn_xsub_out (NN_UNUSED struct nn_sockbase *self,
    NN_UNUSED struct nn_pipe *pipe)
{
    /*  We are not going to send any messages until subscription forwarding
        is implemented, so there's no point is maintaining a list of pipes
        ready for sending. */
}

static int nn_xsub_events (struct nn_sockbase *self)
{
    return nn_fq_can_recv (&nn_cont (self, struct nn_xsub, sockbase)->fq) ?
        NN_SOCKBASE_EVENT_IN : 0;
}

static int nn_xsub_recv (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;
    struct nn_xsub *xsub;

    xsub = nn_cont (self, struct nn_xsub, sockbase);

    /*  Loop while a matching message is found or when there are no more
        messages to receive. */
    while (1) {
        rc = nn_fq_recv (&xsub->fq, msg, NULL);
        if (nn_slow (rc == -EAGAIN))
            return -EAGAIN;
        errnum_assert (rc >= 0, -rc);
        rc = nn_trie_match (&xsub->trie, nn_chunkref_data (&msg->body),
            nn_chunkref_size (&msg->body));
        if (rc == 0) {
            nn_msg_term (msg);
            continue;
        }
        if (rc == 1)
            return 0;
        errnum_assert (0, -rc);
    }
}

static int nn_xsub_setopt (struct nn_sockbase *self, int level, int option,
        const void *optval, size_t optvallen)
{
    int rc;
    struct nn_xsub *xsub;

    xsub = nn_cont (self, struct nn_xsub, sockbase);

    if (level != NN_SUB)
        return -ENOPROTOOPT;

    if (option == NN_SUB_SUBSCRIBE) {
        rc = nn_trie_subscribe (&xsub->trie, optval, optvallen);
        if (rc >= 0)
            return 0;
        return rc;
    }

    if (option == NN_SUB_UNSUBSCRIBE) {
        rc = nn_trie_unsubscribe (&xsub->trie, optval, optvallen);
        if (rc >= 0)
            return 0;
        return rc;
    }

    return -ENOPROTOOPT;
}

int nn_xsub_create (void *hint, struct nn_sockbase **sockbase)
{
    struct nn_xsub *self;

    self = nn_alloc (sizeof (struct nn_xsub), "socket (xsub)");
    alloc_assert (self);
    nn_xsub_init (self, &nn_xsub_sockbase_vfptr, hint);
    *sockbase = &self->sockbase;

    return 0;
}

int nn_xsub_ispeer (int socktype)
{
    return socktype == NN_PUB ? 1 : 0;
}

struct nn_socktype nn_xsub_socktype = {
    AF_SP_RAW,
    NN_SUB,
    NN_SOCKTYPE_FLAG_NOSEND,
    nn_xsub_create,
    nn_xsub_ispeer,
};
