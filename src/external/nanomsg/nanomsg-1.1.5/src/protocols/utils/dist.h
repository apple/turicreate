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

#ifndef NN_DIST_INCLUDED
#define NN_DIST_INCLUDED

#include "../../protocol.h"

#include "../../utils/list.h"

/*  Distributor. Sends messages to all the pipes. */

struct nn_dist_data {
    struct nn_list_item item;
    struct nn_pipe *pipe;
};

struct nn_dist {
    uint32_t count;
    struct nn_list pipes;
};

void nn_dist_init (struct nn_dist *self);
void nn_dist_term (struct nn_dist *self);
void nn_dist_add (struct nn_dist *self, 
    struct nn_dist_data *data, struct nn_pipe *pipe);
void nn_dist_rm (struct nn_dist *self, struct nn_dist_data *data);
void nn_dist_out (struct nn_dist *self, struct nn_dist_data *data);

/*  Sends the message to all the attached pipes except the one specified
    by 'exclude' parameter. If 'exclude' is NULL, message is sent to all
    attached pipes. */
int nn_dist_send (struct nn_dist *self, struct nn_msg *msg,
    struct nn_pipe *exclude);

#endif
