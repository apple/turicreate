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

#include "xrep.h"

#include "../../nn.h"
#include "../../reqrep.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/alloc.h"
#include "../../utils/random.h"
#include "../../utils/wire.h"
#include "../../utils/attr.h"

#include <string.h>

/*  Private functions. */
static void nn_xrep_destroy (struct nn_sockbase *self);

static const struct nn_sockbase_vfptr nn_xrep_sockbase_vfptr = {
    NULL,
    nn_xrep_destroy,
    nn_xrep_add,
    nn_xrep_rm,
    nn_xrep_in,
    nn_xrep_out,
    nn_xrep_events,
    nn_xrep_send,
    nn_xrep_recv,
    NULL,
    NULL
};

void nn_xrep_init (struct nn_xrep *self, const struct nn_sockbase_vfptr *vfptr,
    void *hint)
{
    nn_sockbase_init (&self->sockbase, vfptr, hint);

    /*  Start assigning keys beginning with a random number. This way there
        are no key clashes even if the executable is re-started. */
    nn_random_generate (&self->next_key, sizeof (self->next_key));

    nn_hash_init (&self->outpipes);
    nn_fq_init (&self->inpipes);
}

void nn_xrep_term (struct nn_xrep *self)
{
    nn_fq_term (&self->inpipes);
    nn_hash_term (&self->outpipes);
    nn_sockbase_term (&self->sockbase);
}

static void nn_xrep_destroy (struct nn_sockbase *self)
{
    struct nn_xrep *xrep;

    xrep = nn_cont (self, struct nn_xrep, sockbase);

    nn_xrep_term (xrep);
    nn_free (xrep);
}

int nn_xrep_add (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xrep *xrep;
    struct nn_xrep_data *data;
    int rcvprio;
    size_t sz;

    xrep = nn_cont (self, struct nn_xrep, sockbase);

    sz = sizeof (rcvprio);
    nn_pipe_getopt (pipe, NN_SOL_SOCKET, NN_RCVPRIO, &rcvprio, &sz);
    nn_assert (sz == sizeof (rcvprio));
    nn_assert (rcvprio >= 1 && rcvprio <= 16);

    data = nn_alloc (sizeof (struct nn_xrep_data), "pipe data (xrep)");
    alloc_assert (data);
    data->pipe = pipe;
    nn_hash_item_init (&data->outitem);
    data->flags = 0;
    nn_hash_insert (&xrep->outpipes, xrep->next_key & 0x7fffffff,
        &data->outitem);
    ++xrep->next_key;
    nn_fq_add (&xrep->inpipes, &data->initem, pipe, rcvprio);
    nn_pipe_setdata (pipe, data);

    return 0;
}

void nn_xrep_rm (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xrep *xrep;
    struct nn_xrep_data *data;

    xrep = nn_cont (self, struct nn_xrep, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_fq_rm (&xrep->inpipes, &data->initem);
    nn_hash_erase (&xrep->outpipes, &data->outitem);
    nn_hash_item_term (&data->outitem);

    nn_free (data);
}

void nn_xrep_in (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xrep *xrep;
    struct nn_xrep_data *data;

    xrep = nn_cont (self, struct nn_xrep, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_fq_in (&xrep->inpipes, &data->initem);
}

void nn_xrep_out (NN_UNUSED struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xrep_data *data;

    data = nn_pipe_getdata (pipe);
    data->flags |= NN_XREP_OUT;
}

int nn_xrep_events (struct nn_sockbase *self)
{
    return (nn_fq_can_recv (&nn_cont (self, struct nn_xrep,
        sockbase)->inpipes) ? NN_SOCKBASE_EVENT_IN : 0) | NN_SOCKBASE_EVENT_OUT;
}

int nn_xrep_send (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;
    uint32_t key;
    struct nn_xrep *xrep;
    struct nn_xrep_data *data;

    xrep = nn_cont (self, struct nn_xrep, sockbase);

    /*  We treat invalid peer ID as if the peer was non-existent. */
    if (nn_slow (nn_chunkref_size (&msg->sphdr) < sizeof (uint32_t))) {
        nn_msg_term (msg);
        return 0;
    }

    /*  Retrieve the destination peer ID. Trim it from the header. */
    key = nn_getl (nn_chunkref_data (&msg->sphdr));
    nn_chunkref_trim (&msg->sphdr, 4);

    /*  Find the appropriate pipe to send the message to. If there's none,
        or if it's not ready for sending, silently drop the message. */
    data = nn_cont (nn_hash_get (&xrep->outpipes, key), struct nn_xrep_data,
        outitem);
    if (!data || !(data->flags & NN_XREP_OUT)) {
        nn_msg_term (msg);
        return 0;
    }

    /*  Send the message. */
    rc = nn_pipe_send (data->pipe, msg);
    errnum_assert (rc >= 0, -rc);
    if (rc & NN_PIPE_RELEASE)
        data->flags &= ~NN_XREP_OUT;

    return 0;
}

int nn_xrep_recv (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;
    struct nn_xrep *xrep;
    struct nn_pipe *pipe;
    int i;
    int maxttl;
    void *data;
    size_t sz;
    struct nn_chunkref ref;
    struct nn_xrep_data *pipedata;

    xrep = nn_cont (self, struct nn_xrep, sockbase);

    rc = nn_fq_recv (&xrep->inpipes, msg, &pipe);
    if (nn_slow (rc < 0))
        return rc;

    if (!(rc & NN_PIPE_PARSED)) {

        sz = sizeof (maxttl);
        rc = nn_sockbase_getopt (self, NN_MAXTTL, &maxttl, &sz);
        errnum_assert (rc == 0, -rc);

        /*  Determine the size of the message header. */
        data = nn_chunkref_data (&msg->body);
        sz = nn_chunkref_size (&msg->body);
        i = 0;
        while (1) {

            /*  Ignore the malformed requests without the bottom of the stack. */
            if (nn_slow ((i + 1) * sizeof (uint32_t) > sz)) {
                nn_msg_term (msg);
                return -EAGAIN;
            }

            /*  If the bottom of the backtrace stack is reached, proceed. */
            if (nn_getl ((uint8_t*)(((uint32_t*) data) + i)) & 0x80000000)
                break;

            ++i;
        }
        ++i;

        /* If we encountered too many hops, just toss the message */
        if (i > maxttl) {
            nn_msg_term (msg);
            return -EAGAIN;
        }

        /*  Split the header and the body. */
        nn_assert (nn_chunkref_size (&msg->sphdr) == 0);
        nn_chunkref_term (&msg->sphdr);
        nn_chunkref_init (&msg->sphdr, i * sizeof (uint32_t));
        memcpy (nn_chunkref_data (&msg->sphdr), data, i * sizeof (uint32_t));
        nn_chunkref_trim (&msg->body, i * sizeof (uint32_t));
    }

    /*  Prepend the header by the pipe key. */
    pipedata = nn_pipe_getdata (pipe);
    nn_chunkref_init (&ref,
        nn_chunkref_size (&msg->sphdr) + sizeof (uint32_t));
    nn_putl (nn_chunkref_data (&ref), pipedata->outitem.key);
    memcpy (((uint8_t*) nn_chunkref_data (&ref)) + sizeof (uint32_t),
        nn_chunkref_data (&msg->sphdr), nn_chunkref_size (&msg->sphdr));
    nn_chunkref_term (&msg->sphdr);
    nn_chunkref_mv (&msg->sphdr, &ref);

    return 0;
}

static int nn_xrep_create (void *hint, struct nn_sockbase **sockbase)
{
    struct nn_xrep *self;

    self = nn_alloc (sizeof (struct nn_xrep), "socket (xrep)");
    alloc_assert (self);
    nn_xrep_init (self, &nn_xrep_sockbase_vfptr, hint);
    *sockbase = &self->sockbase;

    return 0;
}

int nn_xrep_ispeer (int socktype)
{
    return socktype == NN_REQ ? 1 : 0;
}

struct nn_socktype nn_xrep_socktype = {
    AF_SP_RAW,
    NN_REP,
    0,
    nn_xrep_create,
    nn_xrep_ispeer,
};
