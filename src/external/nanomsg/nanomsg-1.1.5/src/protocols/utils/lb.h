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

#ifndef NN_LB_INCLUDED
#define NN_LB_INCLUDED

#include "../../protocol.h"

#include "priolist.h"

/*  A load balancer. Round-robins messages to a set of pipes. */

struct nn_lb_data {
    struct nn_priolist_data priodata;
};

struct nn_lb {
    struct nn_priolist priolist;
};

void nn_lb_init (struct nn_lb *self);
void nn_lb_term (struct nn_lb *self);
void nn_lb_add (struct nn_lb *self, struct nn_lb_data *data,
    struct nn_pipe *pipe, int priority);
void nn_lb_rm (struct nn_lb *self, struct nn_lb_data *data);
void nn_lb_out (struct nn_lb *self, struct nn_lb_data *data);
int nn_lb_can_send (struct nn_lb *self);
int nn_lb_get_priority (struct nn_lb *self);
int nn_lb_send (struct nn_lb *self, struct nn_msg *msg, struct nn_pipe **to);

#endif
