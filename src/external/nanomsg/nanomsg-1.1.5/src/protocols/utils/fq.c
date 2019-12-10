/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.

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

#include "fq.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"

#include <stddef.h>

void nn_fq_init (struct nn_fq *self)
{
    nn_priolist_init (&self->priolist);
}

void nn_fq_term (struct nn_fq *self)
{
    nn_priolist_term (&self->priolist);
}

void nn_fq_add (struct nn_fq *self, struct nn_fq_data *data,
    struct nn_pipe *pipe, int priority)
{
    nn_priolist_add (&self->priolist, &data->priodata, pipe, priority);
}

void nn_fq_rm (struct nn_fq *self, struct nn_fq_data *data)
{
    nn_priolist_rm (&self->priolist, &data->priodata);
}

void nn_fq_in (struct nn_fq *self, struct nn_fq_data *data)
{
    nn_priolist_activate (&self->priolist, &data->priodata);
}

int nn_fq_can_recv (struct nn_fq *self)
{
    return nn_priolist_is_active (&self->priolist);
}

int nn_fq_recv (struct nn_fq *self, struct nn_msg *msg, struct nn_pipe **pipe)
{
    int rc;
    struct nn_pipe *p;

    /*  Pipe is NULL only when there are no avialable pipes. */
    p = nn_priolist_getpipe (&self->priolist);
    if (nn_slow (!p))
        return -EAGAIN;

    /*  Receive the messsage. */
    rc = nn_pipe_recv (p, msg);
    errnum_assert (rc >= 0, -rc);

    /*  Return the pipe data to the user, if required. */
    if (pipe)
        *pipe = p;

    /*  Move to the next pipe. */
    nn_priolist_advance (&self->priolist, rc & NN_PIPE_RELEASE);

    return rc & ~NN_PIPE_RELEASE;
}

