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

#ifndef NN_TIMERSET_INCLUDED
#define NN_TIMERSET_INCLUDED

#include <stdint.h>

#include "../utils/list.h"

/*  This class stores a list of timeouts and reports the next one to expire
    along with the time till it happens. */

struct nn_timerset_hndl {
    struct nn_list_item list;
    uint64_t timeout;
};

struct nn_timerset {
    struct nn_list timeouts;
};

void nn_timerset_init (struct nn_timerset *self);
void nn_timerset_term (struct nn_timerset *self);
int nn_timerset_add (struct nn_timerset *self, int timeout,
    struct nn_timerset_hndl *hndl);
int nn_timerset_rm (struct nn_timerset *self, struct nn_timerset_hndl *hndl);
int nn_timerset_timeout (struct nn_timerset *self);
int nn_timerset_event (struct nn_timerset *self, struct nn_timerset_hndl **hndl);

void nn_timerset_hndl_init (struct nn_timerset_hndl *self);
void nn_timerset_hndl_term (struct nn_timerset_hndl *self);
int nn_timerset_hndl_isactive (struct nn_timerset_hndl *self);

#endif

