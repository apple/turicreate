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

#ifndef NN_INS_INCLUDED
#define NN_INS_INCLUDED

#include "../../transport.h"

#include "../../utils/list.h"

/*  Inproc naming system. A global repository of inproc endpoints. */

struct nn_ins_item {

    /*  Every ins_item is either in the list of bound or connected endpoints. */
    struct nn_list_item item;

    struct nn_ep *ep;

    /*  This is the local cache of the endpoint's protocol ID. This way we can
        check the value without actually locking the object. */
    int protocol;
};

void nn_ins_item_init (struct nn_ins_item *self, struct nn_ep *ep);
void nn_ins_item_term (struct nn_ins_item *self);

void nn_ins_init (void);
void nn_ins_term (void);

typedef void (*nn_ins_fn) (struct nn_ins_item *self, struct nn_ins_item *peer);

int nn_ins_bind (struct nn_ins_item *item, nn_ins_fn fn);
void nn_ins_connect (struct nn_ins_item *item, nn_ins_fn fn);
void nn_ins_disconnect (struct nn_ins_item *item);
void nn_ins_unbind (struct nn_ins_item *item);

#endif

