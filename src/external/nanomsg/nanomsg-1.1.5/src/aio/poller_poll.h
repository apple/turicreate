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

#include <poll.h>

#define NN_POLLER_HAVE_ASYNC_ADD 0

struct nn_poller_hndl {
    int index;
};

struct nn_poller {

    /*  Actual number of elements in the pollset. */
    int size;

    /*  Index of the event being processed at the moment. */
    int index;

    /*  Number of allocated elements in the pollset. */
    int capacity;

    /*  The pollset. */
    struct pollfd *pollset;

    /*  List of handles associated with elements in the pollset. Either points
        to the handle associated with the file descriptor (hndl) or is part
        of the list of removed pollitems (removed). */
    struct nn_hndls_item {
        struct nn_poller_hndl *hndl;
        int prev;
        int next;
    } *hndls;

    /*  List of removed pollitems, linked by indices. -1 means empty list. */
    int removed;
};

