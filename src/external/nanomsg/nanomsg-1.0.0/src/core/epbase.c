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

#include "../transport.h"

#include "ep.h"
#include "sock.h"
#include "../utils/attr.h"

void nn_epbase_init (struct nn_epbase *self,
    const struct nn_epbase_vfptr *vfptr, void *hint)
{
    self->vfptr = vfptr;
    self->ep = (struct nn_ep*) hint;
}

void nn_epbase_term (NN_UNUSED struct nn_epbase *self)
{
}

void nn_epbase_stopped (struct nn_epbase *self)
{
    nn_ep_stopped (self->ep);
}

struct nn_ctx *nn_epbase_getctx (struct nn_epbase *self)
{
    return nn_ep_getctx (self->ep);
}

const char *nn_epbase_getaddr (struct nn_epbase *self)
{
    return nn_ep_getaddr (self->ep);
}

void nn_epbase_getopt (struct nn_epbase *self, int level, int option,
    void *optval, size_t *optvallen)
{
    nn_ep_getopt (self->ep, level, option, optval, optvallen);
}

int nn_epbase_ispeer (struct nn_epbase *self, int socktype)
{
    return nn_ep_ispeer (self->ep, socktype);
}

void nn_epbase_set_error (struct nn_epbase *self, int errnum)
{
    nn_ep_set_error (self->ep, errnum);
}

void nn_epbase_clear_error (struct nn_epbase *self)
{
    nn_ep_clear_error (self->ep);
}

void nn_epbase_stat_increment(struct nn_epbase *self, int name, int increment) {
    nn_ep_stat_increment(self->ep, name, increment);
}
