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

#ifndef NN_XSURVEYOR_INCLUDED
#define NN_XSURVEYOR_INCLUDED

#include "../../protocol.h"

#include "../utils/dist.h"
#include "../utils/fq.h"

struct nn_xsurveyor_data {
    struct nn_pipe *pipe;
    struct nn_dist_data outitem;
    struct nn_fq_data initem;
};

struct nn_xsurveyor {

    /*  The generic socket base class. */
    struct nn_sockbase sockbase;

    /*  Distributor to send messages. */
    struct nn_dist outpipes;

    /*  Fair-queuer to receive messages. */
    struct nn_fq inpipes;
};

void nn_xsurveyor_init (struct nn_xsurveyor *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint);
void nn_xsurveyor_term (struct nn_xsurveyor *self);

int nn_xsurveyor_add (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xsurveyor_rm (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xsurveyor_in (struct nn_sockbase *self, struct nn_pipe *pipe);
void nn_xsurveyor_out (struct nn_sockbase *self, struct nn_pipe *pipe);
int nn_xsurveyor_events (struct nn_sockbase *self);
int nn_xsurveyor_send (struct nn_sockbase *self, struct nn_msg *msg);
int nn_xsurveyor_recv (struct nn_sockbase *self, struct nn_msg *msg);
int nn_xsurveyor_setopt (struct nn_sockbase *self, int level, int option,
    const void *optval, size_t optvallen);
int nn_xsurveyor_getopt (struct nn_sockbase *self, int level, int option,
    void *optval, size_t *optvallen);

int nn_xsurveyor_ispeer (int socktype);

#endif

