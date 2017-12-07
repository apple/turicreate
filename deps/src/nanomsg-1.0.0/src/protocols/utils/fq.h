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

#ifndef NN_FQ_INCLUDED
#define NN_FQ_INCLUDED

#include "../../protocol.h"

#include "priolist.h"

/*  Fair-queuer. Retrieves messages from a set of pipes in round-robin
    manner. */

struct nn_fq_data {
    struct nn_priolist_data priodata;
};

struct nn_fq {
    struct nn_priolist priolist;
};

void nn_fq_init (struct nn_fq *self);
void nn_fq_term (struct nn_fq *self);
void nn_fq_add (struct nn_fq *self, struct nn_fq_data *data,
    struct nn_pipe *pipe, int priority);
void nn_fq_rm (struct nn_fq *self, struct nn_fq_data *data);
void nn_fq_in (struct nn_fq *self, struct nn_fq_data *data);
int nn_fq_can_recv (struct nn_fq *self);
int nn_fq_recv (struct nn_fq *self, struct nn_msg *msg, struct nn_pipe **pipe);

#endif
