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

#include "../protocol.h"

#include "sock.h"

#include "../utils/err.h"
#include "../utils/attr.h"

void nn_sockbase_init (struct nn_sockbase *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint)
{
    self->vfptr = vfptr;
    self->sock = (struct nn_sock*) hint;
}

void nn_sockbase_term (NN_UNUSED struct nn_sockbase *self)
{
}

void nn_sockbase_stopped (struct nn_sockbase *self)
{
    nn_sock_stopped (self->sock);
}

struct nn_ctx *nn_sockbase_getctx (struct nn_sockbase *self)
{
    return nn_sock_getctx (self->sock);
}

int nn_sockbase_getopt (struct nn_sockbase *self, int option,
    void *optval, size_t *optvallen)
{
    return nn_sock_getopt_inner (self->sock, NN_SOL_SOCKET, option,
        optval, optvallen);
}

void nn_sockbase_stat_increment (struct nn_sockbase *self, int name,
    int increment)
{
    nn_sock_stat_increment (self->sock, name, increment);
}
