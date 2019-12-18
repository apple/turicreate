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

#ifndef NN_REP_INCLUDED
#define NN_REP_INCLUDED

#include "../../protocol.h"
#include "xrep.h"

struct nn_rep {
    struct nn_xrep xrep;
    uint32_t flags;
    struct nn_chunkref backtrace;
};

/*  Some users may want to extend the REP protocol similar to how REP extends XREP.
    Expose these methods to improve extensibility. */
void nn_rep_init (struct nn_rep *self,
const struct nn_sockbase_vfptr *vfptr, void *hint);
void nn_rep_term (struct nn_rep *self);

/*  Implementation of nn_sockbase's virtual functions. */
void nn_rep_destroy (struct nn_sockbase *self);
int nn_rep_events (struct nn_sockbase *self);
int nn_rep_send (struct nn_sockbase *self, struct nn_msg *msg);
int nn_rep_recv (struct nn_sockbase *self, struct nn_msg *msg);

#endif
