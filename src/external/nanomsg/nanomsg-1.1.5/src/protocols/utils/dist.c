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

#include "dist.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/attr.h"

#include <stddef.h>

void nn_dist_init (struct nn_dist *self)
{
    self->count = 0;
    nn_list_init (&self->pipes);
}

void nn_dist_term (struct nn_dist *self)
{
    nn_assert (self->count == 0);
    nn_list_term (&self->pipes);
}

void nn_dist_add (NN_UNUSED struct nn_dist *self,
    struct nn_dist_data *data, struct nn_pipe *pipe)
{
    data->pipe = pipe;
    nn_list_item_init (&data->item);
}

void nn_dist_rm (struct nn_dist *self, struct nn_dist_data *data)
{
    if (nn_list_item_isinlist (&data->item)) {
        --self->count;
        nn_list_erase (&self->pipes, &data->item);
    }
    nn_list_item_term (&data->item);
}

void nn_dist_out (struct nn_dist *self, struct nn_dist_data *data)
{
    ++self->count;
    nn_list_insert (&self->pipes, &data->item, nn_list_end (&self->pipes));
}

int nn_dist_send (struct nn_dist *self, struct nn_msg *msg,
    struct nn_pipe *exclude)
{
    int rc;
    struct nn_list_item *it;
    struct nn_dist_data *data;
    struct nn_msg copy;

    /*  TODO: We can optimise for the case when there's only one outbound
        pipe here. No message copying is needed in such case. */

    /*  In the specific case when there are no outbound pipes. There's nowhere
        to send the message to. Deallocate it. */
    if (nn_slow (self->count) == 0) {
        nn_msg_term (msg);
        return 0;
    }

    /*  Send the message to all the subscribers. */
    nn_msg_bulkcopy_start (msg, self->count);
    it = nn_list_begin (&self->pipes);
    while (it != nn_list_end (&self->pipes)) {
       data = nn_cont (it, struct nn_dist_data, item);
       nn_msg_bulkcopy_cp (&copy, msg);
       if (nn_fast (data->pipe == exclude)) {
           nn_msg_term (&copy);
       }
       else {
           rc = nn_pipe_send (data->pipe, &copy);
           errnum_assert (rc >= 0, -rc);
           if (rc & NN_PIPE_RELEASE) {
               --self->count;
               it = nn_list_erase (&self->pipes, it);
               continue;
           }
       }
       it = nn_list_next (&self->pipes, it);
    }
    nn_msg_term (msg);

    return 0;
}

