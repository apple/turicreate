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

#include "xrespondent.h"

#include "../../nn.h"
#include "../../survey.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/alloc.h"
#include "../../utils/random.h"
#include "../../utils/wire.h"
#include "../../utils/attr.h"

/*  Private functions. */
static void nn_xrespondent_destroy (struct nn_sockbase *self);

/*  Implementation of nn_sockbase's virtual functions. */
static const struct nn_sockbase_vfptr nn_xrespondent_sockbase_vfptr = {
    NULL,
    nn_xrespondent_destroy,
    nn_xrespondent_add,
    nn_xrespondent_rm,
    nn_xrespondent_in,
    nn_xrespondent_out,
    nn_xrespondent_events,
    nn_xrespondent_send,
    nn_xrespondent_recv,
    NULL,
    NULL
};

void nn_xrespondent_init (struct nn_xrespondent *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint)
{
    nn_sockbase_init (&self->sockbase, vfptr, hint);

    /*  Pipes IDs should be random.  See RFC for info. */
    nn_random_generate (&self->next_key, sizeof (self->next_key));
    nn_hash_init (&self->outpipes);
    nn_fq_init (&self->inpipes);
}

void nn_xrespondent_term (struct nn_xrespondent *self)
{
    nn_fq_term (&self->inpipes);
    nn_hash_term (&self->outpipes);
    nn_sockbase_term (&self->sockbase);
}

static void nn_xrespondent_destroy (struct nn_sockbase *self)
{
    struct nn_xrespondent *xrespondent;

    xrespondent = nn_cont (self, struct nn_xrespondent, sockbase);

    nn_xrespondent_term (xrespondent);
    nn_free (xrespondent);
}

int nn_xrespondent_add (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xrespondent *xrespondent;
    struct nn_xrespondent_data *data;
    int rcvprio;
    size_t sz;

    xrespondent = nn_cont (self, struct nn_xrespondent, sockbase);

    sz = sizeof (rcvprio);
    nn_pipe_getopt (pipe, NN_SOL_SOCKET, NN_RCVPRIO, &rcvprio, &sz);
    nn_assert (sz == sizeof (rcvprio));
    nn_assert (rcvprio >= 1 && rcvprio <= 16);

    data = nn_alloc (sizeof (*data), "pipe data (xrespondent)");
    alloc_assert (data);

    data->pipe = pipe;
    nn_hash_item_init (&data->outitem);
    data->flags = 0;
    nn_hash_insert (&xrespondent->outpipes, xrespondent->next_key & 0x7fffffff,
        &data->outitem);
    xrespondent->next_key++;
    nn_fq_add (&xrespondent->inpipes, &data->initem, pipe, rcvprio);
    nn_pipe_setdata (pipe, data);

    return 0;
}

void nn_xrespondent_rm (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xrespondent *xrespondent;
    struct nn_xrespondent_data *data;

    xrespondent = nn_cont (self, struct nn_xrespondent, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_fq_rm (&xrespondent->inpipes, &data->initem);
    nn_hash_erase (&xrespondent->outpipes, &data->outitem);
    nn_hash_item_term (&data->outitem);

    nn_free (data);
}

void nn_xrespondent_in (struct nn_sockbase *self, struct nn_pipe *pipe)
{
    struct nn_xrespondent *xrespondent;
    struct nn_xrespondent_data *data;

    xrespondent = nn_cont (self, struct nn_xrespondent, sockbase);
    data = nn_pipe_getdata (pipe);

    nn_fq_in (&xrespondent->inpipes, &data->initem);
}

void nn_xrespondent_out (NN_UNUSED struct nn_sockbase *self, 
    struct nn_pipe *pipe)
{
    struct nn_xrespondent_data *data;

    data = nn_pipe_getdata (pipe);
    data->flags |= NN_XRESPONDENT_OUT;
}

int nn_xrespondent_events (struct nn_sockbase *self)
{
    return (nn_fq_can_recv (&nn_cont (self, struct nn_xrespondent,
        sockbase)->inpipes) ? NN_SOCKBASE_EVENT_IN : 0) | NN_SOCKBASE_EVENT_OUT;
}

int nn_xrespondent_send (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;
    uint32_t key;
    struct nn_xrespondent *xrespondent;
    struct nn_xrespondent_data *data;

    xrespondent = nn_cont (self, struct nn_xrespondent, sockbase);

    /*  We treat invalid peer ID as if the peer was non-existent. */
    if (nn_slow (nn_chunkref_size (&msg->sphdr) < sizeof (uint32_t))) {
        nn_msg_term (msg);
        return 0;
    }

    /*  Retrieve destination peer ID. Trim it from the header. */
    key = nn_getl (nn_chunkref_data (&msg->sphdr));
    nn_chunkref_trim (&msg->sphdr, 4);

    /*  Find the appropriate pipe to send the message to. If there's none,
        or if it's not ready for sending, silently drop the message. */
    data = nn_cont (nn_hash_get (&xrespondent->outpipes, key),
        struct nn_xrespondent_data, outitem);
    if (!data || !(data->flags & NN_XRESPONDENT_OUT)) {
        nn_msg_term (msg);
        return 0;
    }

    /*  Send the message. */
    rc = nn_pipe_send (data->pipe, msg);
    errnum_assert (rc >= 0, -rc);
    if (rc & NN_PIPE_RELEASE)
        data->flags &= ~NN_XRESPONDENT_OUT;

    return 0;
}

int nn_xrespondent_recv (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;
    struct nn_xrespondent *xrespondent;
    struct nn_pipe *pipe;
    int i;
    size_t sz;
    void *data;
    struct nn_chunkref ref;
    struct nn_xrespondent_data *pipedata;
    int maxttl;

    xrespondent = nn_cont (self, struct nn_xrespondent, sockbase);

    rc = nn_fq_recv (&xrespondent->inpipes, msg, &pipe);
    if (nn_slow (rc < 0))
        return rc;

    /*  Split the header (including survey ID) from the body, if needed. */
    if (!(rc & NN_PIPE_PARSED)) {

        sz = sizeof (maxttl);
        rc = nn_sockbase_getopt (self, NN_MAXTTL, &maxttl, &sz);
        errnum_assert (rc == 0, -rc);

        /*  Determine the size of the message header. */
        data = nn_chunkref_data (&msg->body);
        sz = nn_chunkref_size (&msg->body);
        i = 0;

        while (1) {
	    /*  Ignore the malformed surveys without the bottom of the stack. */
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
    
        /*  Ignore messages with too many hops. */
        if (i > maxttl) {
            nn_msg_term (msg);
            return -EAGAIN;
        }

        nn_assert (nn_chunkref_size (&msg->sphdr) == 0);
        nn_chunkref_term (&msg->sphdr);
        nn_chunkref_init (&msg->sphdr, i * sizeof (uint32_t));
        memcpy (nn_chunkref_data (&msg->sphdr), data, i * sizeof (uint32_t));
        nn_chunkref_trim (&msg->body, i * sizeof (uint32_t));
    }

    /*  Prepend the header by the pipe key. */
    pipedata = nn_pipe_getdata (pipe);
    nn_chunkref_init (&ref, nn_chunkref_size (&msg->sphdr) + sizeof (uint32_t));
    nn_putl (nn_chunkref_data (&ref), pipedata->outitem.key);
    memcpy (((uint8_t *) nn_chunkref_data (&ref)) + sizeof (uint32_t),
        nn_chunkref_data (&msg->sphdr), nn_chunkref_size (&msg->sphdr));
    nn_chunkref_term (&msg->sphdr);
    nn_chunkref_mv (&msg->sphdr, &ref);

    return 0;
}

static int nn_xrespondent_create (void *hint, struct nn_sockbase **sockbase)
{
    struct nn_xrespondent *self;

    self = nn_alloc (sizeof (struct nn_xrespondent), "socket (xrespondent)");
    alloc_assert (self);
    nn_xrespondent_init (self, &nn_xrespondent_sockbase_vfptr, hint);
    *sockbase = &self->sockbase;

    return 0;
}

int nn_xrespondent_ispeer (int socktype)
{
    return socktype == NN_SURVEYOR ? 1 : 0;
}

struct nn_socktype nn_xrespondent_socktype = {
    AF_SP_RAW,
    NN_RESPONDENT,
    0,
    nn_xrespondent_create,
    nn_xrespondent_ispeer,
};
