/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.

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

#include "ins.h"

#include "../../utils/mutex.h"
#include "../../utils/alloc.h"
#include "../../utils/list.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/err.h"

struct nn_ins {

    /*  Synchronises access to this object. */
    struct nn_mutex sync;

    /*  List of all bound inproc endpoints. */
    /*  TODO: O(n) lookup, shouldn't we do better? Hash? */
    struct nn_list bound;

    /*  List of all connected inproc endpoints. */
    /*  TODO: O(n) lookup, shouldn't we do better? Hash? */
    struct nn_list connected;
};

/*  Global instance of the nn_ins object. It contains the lists of all
    inproc endpoints in the current process. */
static struct nn_ins self;

void nn_ins_item_init (struct nn_ins_item *self, struct nn_ep *ep)
{
    self->ep = ep;
    nn_list_item_init (&self->item);
}

void nn_ins_item_term (struct nn_ins_item *self)
{
    nn_list_item_term (&self->item);
}

void nn_ins_init (void)
{
    nn_mutex_init (&self.sync);
    nn_list_init (&self.bound);
    nn_list_init (&self.connected);
}
void nn_ins_term (void)
{
    nn_list_term (&self.connected);
    nn_list_term (&self.bound);
    nn_mutex_term (&self.sync);
}

int nn_ins_bind (struct nn_ins_item *item, nn_ins_fn fn)
{
    struct nn_list_item *it;
    struct nn_ins_item *bitem;
    struct nn_ins_item *citem;

    nn_mutex_lock (&self.sync);

    /*  Check whether the endpoint isn't already bound. */
    /*  TODO:  This is an O(n) algorithm! */
    for (it = nn_list_begin (&self.bound); it != nn_list_end (&self.bound);
          it = nn_list_next (&self.bound, it)) {
        bitem = nn_cont (it, struct nn_ins_item, item);

        if (strncmp (nn_ep_getaddr(bitem->ep), nn_ep_getaddr(item->ep),
            NN_SOCKADDR_MAX) == 0) {

            nn_mutex_unlock (&self.sync);
            return -EADDRINUSE;
        }
    }

    /*  Insert the entry into the endpoint repository. */
    nn_list_insert (&self.bound, &item->item,
        nn_list_end (&self.bound));

    /*  During this process new pipes may be created. */
    for (it = nn_list_begin (&self.connected);
          it != nn_list_end (&self.connected);
          it = nn_list_next (&self.connected, it)) {
        citem = nn_cont (it, struct nn_ins_item, item);
        if (strncmp (nn_ep_getaddr(item->ep), nn_ep_getaddr(citem->ep),
            NN_SOCKADDR_MAX) == 0) {

            /*  Check whether the two sockets are compatible. */
            if (!nn_ep_ispeer_ep (item->ep, citem->ep))
                continue;

            fn (item, citem);
        }
    }

    nn_mutex_unlock (&self.sync);

    return 0;
}

void nn_ins_connect (struct nn_ins_item *item, nn_ins_fn fn)
{
    struct nn_list_item *it;
    struct nn_ins_item *bitem;

    nn_mutex_lock (&self.sync);

    /*  Insert the entry into the endpoint repository. */
    nn_list_insert (&self.connected, &item->item,
        nn_list_end (&self.connected));

    /*  During this process a pipe may be created. */
    for (it = nn_list_begin (&self.bound);
          it != nn_list_end (&self.bound);
          it = nn_list_next (&self.bound, it)) {
        bitem = nn_cont (it, struct nn_ins_item, item);

        if (strncmp (nn_ep_getaddr(item->ep), nn_ep_getaddr(bitem->ep),
            NN_SOCKADDR_MAX) == 0) {

            /*  Check whether the two sockets are compatible. */
            if (!nn_ep_ispeer_ep (item->ep, bitem->ep))
                break;

            /*  Call back to cinproc to create actual connection. */
            fn (item, bitem);

            break;
        }
    }

    nn_mutex_unlock (&self.sync);
}

void nn_ins_disconnect (struct nn_ins_item *item)
{
    nn_mutex_lock (&self.sync);
    nn_list_erase (&self.connected, &item->item);
    nn_mutex_unlock (&self.sync);
}

void nn_ins_unbind (struct nn_ins_item *item)
{
    nn_mutex_lock (&self.sync);
    nn_list_erase (&self.bound, &item->item);
    nn_mutex_unlock (&self.sync);
}

