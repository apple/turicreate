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

#include "timerset.h"

#include "../utils/fast.h"
#include "../utils/cont.h"
#include "../utils/clock.h"
#include "../utils/err.h"

void nn_timerset_init (struct nn_timerset *self)
{
    nn_list_init (&self->timeouts);
}

void nn_timerset_term (struct nn_timerset *self)
{
    nn_list_term (&self->timeouts);
}

int nn_timerset_add (struct nn_timerset *self, int timeout,
    struct nn_timerset_hndl *hndl)
{
    struct nn_list_item *it;
    struct nn_timerset_hndl *ith;
    int first;

    /*  Compute the instant when the timeout will be due. */
    hndl->timeout = nn_clock_ms()  + timeout;

    /*  Insert it into the ordered list of timeouts. */
    for (it = nn_list_begin (&self->timeouts);
          it != nn_list_end (&self->timeouts);
          it = nn_list_next (&self->timeouts, it)) {
        ith = nn_cont (it, struct nn_timerset_hndl, list);
        if (hndl->timeout < ith->timeout)
            break;
    }

    /*  If the new timeout happens to be the first one to expire, let the user
        know that the current waiting interval has to be changed. */
    first = nn_list_begin (&self->timeouts) == it ? 1 : 0;
    nn_list_insert (&self->timeouts, &hndl->list, it);
    return first;
}

int nn_timerset_rm (struct nn_timerset *self, struct nn_timerset_hndl *hndl)
{
    int first;

    /*  Ignore if handle is not in the timeouts list. */
    if (!nn_list_item_isinlist (&hndl->list))
        return 0;

    /*  If it was the first timeout that was removed, the actual waiting time
        may have changed. We'll thus return 1 to let the user know. */
    first = nn_list_begin (&self->timeouts) == &hndl->list ? 1 : 0;
    nn_list_erase (&self->timeouts, &hndl->list);
    return first;
}

int nn_timerset_timeout (struct nn_timerset *self)
{
    int timeout;

    if (nn_fast (nn_list_empty (&self->timeouts)))
        return -1;

    timeout = (int) (nn_cont (nn_list_begin (&self->timeouts),
        struct nn_timerset_hndl, list)->timeout - nn_clock_ms());
    return timeout < 0 ? 0 : timeout;
}

int nn_timerset_event (struct nn_timerset *self, struct nn_timerset_hndl **hndl)
{
    struct nn_timerset_hndl *first;

    /*  If there's no timeout, there's no event to report. */
    if (nn_fast (nn_list_empty (&self->timeouts)))
        return -EAGAIN;

    /*  If no timeout have expired yet, there's no event to return. */
    first = nn_cont (nn_list_begin (&self->timeouts),
        struct nn_timerset_hndl, list);
    if (first->timeout > nn_clock_ms())
        return -EAGAIN;

    /*  Return the first timeout and remove it from the list of active
        timeouts. */
    nn_list_erase (&self->timeouts, &first->list);
    *hndl = first;
    return 0;
}

void nn_timerset_hndl_init (struct nn_timerset_hndl *self)
{
    nn_list_item_init (&self->list);
}

void nn_timerset_hndl_term (struct nn_timerset_hndl *self)
{
    nn_list_item_term (&self->list);
}

int nn_timerset_hndl_isactive (struct nn_timerset_hndl *self)
{
    return nn_list_item_isinlist (&self->list);
}

