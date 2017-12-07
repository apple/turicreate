/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
    Copyright (c) 2015-2016 Jack R. Dunaway.  All rights reserved.

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

#ifndef NN_POLLER_INCLUDED
#define NN_POLLER_INCLUDED

#define NN_POLLER_IN 1
#define NN_POLLER_OUT 2
#define NN_POLLER_ERR 3

#if defined NN_USE_EPOLL
    #include "poller_epoll.h"
#elif defined NN_USE_KQUEUE
    #include "poller_kqueue.h"
#elif defined NN_USE_POLL
    #include "poller_poll.h"
#else
    #error
#endif

int nn_poller_init (struct nn_poller *self);
void nn_poller_term (struct nn_poller *self);
void nn_poller_add (struct nn_poller *self, int fd,
    struct nn_poller_hndl *hndl);
void nn_poller_rm (struct nn_poller *self, struct nn_poller_hndl *hndl);
void nn_poller_set_in (struct nn_poller *self, struct nn_poller_hndl *hndl);
void nn_poller_reset_in (struct nn_poller *self, struct nn_poller_hndl *hndl);
void nn_poller_set_out (struct nn_poller *self, struct nn_poller_hndl *hndl);
void nn_poller_reset_out (struct nn_poller *self, struct nn_poller_hndl *hndl);
int nn_poller_wait (struct nn_poller *self, int timeout);
int nn_poller_event (struct nn_poller *self, int *event,
    struct nn_poller_hndl **hndl);

#endif
