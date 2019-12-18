/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.

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

#include "../utils/attr.h"
#include "../utils/fast.h"
#include "../utils/err.h"
#include "../utils/closefd.h"

#include <unistd.h>

/*  NetBSD has different definition of udata. */
#if defined NN_HAVE_NETBSD
#define nn_poller_udata intptr_t
#else
#define nn_poller_udata void*
#endif

int nn_poller_init (struct nn_poller *self)
{
    self->kq = kqueue ();
    if (self->kq == -1) {
         if (errno == ENFILE || errno == EMFILE)
              return -EMFILE;
         errno_assert (0);
    }
    self->nevents = 0;
    self->index = 0;

    return 0;
}

void nn_poller_term (struct nn_poller *self)
{
    nn_closefd (self->kq);
}

void nn_poller_add (NN_UNUSED struct nn_poller *self, int fd,
    struct nn_poller_hndl *hndl)
{
    /*  Initialise the handle. */
    hndl->fd = fd;
    hndl->events = 0;
}

void nn_poller_rm (struct nn_poller *self, struct nn_poller_hndl *hndl)
{
    struct kevent ev;
    int i;

    if (hndl->events & NN_POLLER_EVENT_IN) {
        EV_SET (&ev, hndl->fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
        kevent (self->kq, &ev, 1, NULL, 0, NULL);
    }

    if (hndl->events & NN_POLLER_EVENT_OUT) {
        EV_SET (&ev, hndl->fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
        kevent (self->kq, &ev, 1, NULL, 0, NULL);
    }

    /*  Invalidate any subsequent events on this file descriptor. */
    for (i = self->index; i != self->nevents; ++i)
        if (self->events [i].ident == (unsigned) hndl->fd)
            self->events [i].udata = (nn_poller_udata) NULL;
}

void nn_poller_set_in (struct nn_poller *self, struct nn_poller_hndl *hndl)
{
    int rc;
    struct kevent ev;

    if (!(hndl->events & NN_POLLER_EVENT_IN)) {
        EV_SET (&ev, hndl->fd, EVFILT_READ, EV_ADD, 0, 0,
            (nn_poller_udata) hndl);
        rc = kevent (self->kq, &ev, 1, NULL, 0, NULL);
        if (rc != -1)
            hndl->events |= NN_POLLER_EVENT_IN;
    }
}

void nn_poller_reset_in (struct nn_poller *self, struct nn_poller_hndl *hndl)
{
    int rc;
    struct kevent ev;
    int i;

    if (hndl->events & NN_POLLER_EVENT_IN) {
        EV_SET (&ev, hndl->fd, EVFILT_READ, EV_DELETE, 0, 0, 0);
        rc = kevent (self->kq, &ev, 1, NULL, 0, NULL);
        hndl->events &= ~NN_POLLER_EVENT_IN;
    }

    /*  Invalidate any subsequent IN events on this file descriptor. */
    for (i = self->index; i != self->nevents; ++i)
        if (self->events [i].ident == (unsigned) hndl->fd &&
              self->events [i].filter == EVFILT_READ)
            self->events [i].udata = (nn_poller_udata) NULL;
}

void nn_poller_set_out (struct nn_poller *self, struct nn_poller_hndl *hndl)
{
    int rc;
    struct kevent ev;
    int fd = hndl->fd;

    if (!(hndl->events & NN_POLLER_EVENT_OUT)) {
        EV_SET (&ev, fd, EVFILT_WRITE, EV_ADD, 0, 0, (nn_poller_udata) hndl);
        rc = kevent (self->kq, &ev, 1, NULL, 0, NULL);
        if (rc != -1)
            hndl->events |= NN_POLLER_EVENT_OUT;
    }
}

void nn_poller_reset_out (struct nn_poller *self, struct nn_poller_hndl *hndl)
{
    int rc;
    struct kevent ev;
    int i;
    int fd = hndl->fd;

    if (hndl->events & NN_POLLER_EVENT_OUT) {
        EV_SET (&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, 0);
        rc = kevent (self->kq, &ev, 1, NULL, 0, NULL);
        if (rc != -1) {
            hndl->events &= ~NN_POLLER_EVENT_OUT;
        }
    }

    /*  Invalidate any subsequent OUT events on this file descriptor. */
    for (i = self->index; i != self->nevents; ++i)
        if (self->events [i].ident == (unsigned) hndl->fd &&
              self->events [i].filter == EVFILT_WRITE)
            self->events [i].udata = (nn_poller_udata) NULL;
}

int nn_poller_wait (struct nn_poller *self, int timeout)
{
    struct timespec ts;
    int nevents;

    /*  Clear all existing events. */
    self->nevents = 0;
    self->index = 0;

    /*  Wait for new events. */
#if defined NN_IGNORE_EINTR
again:
#endif
    ts.tv_sec = timeout / 1000;
    ts.tv_nsec = (timeout % 1000) * 1000000;
    nevents = kevent (self->kq, NULL, 0, &self->events [0],
        NN_POLLER_MAX_EVENTS, timeout >= 0 ? &ts : NULL);
    if (nevents == -1 && errno == EINTR)
#if defined NN_IGNORE_EINTR
        goto again;
#else
        return -EINTR;
#endif
    errno_assert (nevents != -1);

    self->nevents = nevents;
    return 0;
}

int nn_poller_event (struct nn_poller *self, int *event,
    struct nn_poller_hndl **hndl)
{
    /*  Skip over empty events. */
    while (self->index < self->nevents) {
        if (self->events [self->index].udata)
            break;
        ++self->index;
    }

    /*  If there is no stored event, let the caller know. */
    if (nn_slow (self->index >= self->nevents))
        return -EAGAIN;

    /*  Return next event to the caller. Remove the event from the set. */
    *hndl = (struct nn_poller_hndl*) self->events [self->index].udata;
    if (self->events [self->index].flags & EV_EOF)
        *event = NN_POLLER_ERR;
    else if (self->events [self->index].filter == EVFILT_WRITE)
        *event = NN_POLLER_OUT;
    else if (self->events [self->index].filter == EVFILT_READ)
        *event = NN_POLLER_IN;
    else
        nn_assert (0);
    ++self->index;

    return 0;
}

