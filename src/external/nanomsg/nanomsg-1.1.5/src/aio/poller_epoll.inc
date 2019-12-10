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

#include "../utils/fast.h"
#include "../utils/err.h"
#include "../utils/closefd.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int nn_poller_init (struct nn_poller *self)
{
#ifndef EPOLL_CLOEXEC
    int rc;
#endif

#ifdef EPOLL_CLOEXEC
    self->ep = epoll_create1 (EPOLL_CLOEXEC);
#else
    /*  Size parameter is unused, we can safely set it to 1. */
    self->ep = epoll_create (1);
    rc = fcntl (self->ep, F_SETFD, FD_CLOEXEC);
    errno_assert (rc != -1);
#endif 
    if (self->ep == -1) {
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
    nn_closefd (self->ep);
}

void nn_poller_add (struct nn_poller *self, int fd,
    struct nn_poller_hndl *hndl)
{
    struct epoll_event ev;

    /*  Initialise the handle and add the file descriptor to the pollset. */
    hndl->fd = fd;
    hndl->events = 0;
    memset (&ev, 0, sizeof (ev));
    ev.events = 0;
    ev.data.ptr = (void*) hndl;
    epoll_ctl (self->ep, EPOLL_CTL_ADD, fd, &ev);
}

void nn_poller_rm (struct nn_poller *self, struct nn_poller_hndl *hndl)
{
    int i;

    /*  Remove the file descriptor from the pollset. */
    epoll_ctl (self->ep, EPOLL_CTL_DEL, hndl->fd, NULL);

    /*  Invalidate any subsequent events on this file descriptor. */
    for (i = self->index; i != self->nevents; ++i)
        if (self->events [i].data.ptr == hndl)
            self->events [i].events = 0;
}

void nn_poller_set_in (struct nn_poller *self, struct nn_poller_hndl *hndl)
{
    struct epoll_event ev;

    /*  If already polling for IN, do nothing. */
    if (nn_slow (hndl->events & EPOLLIN))
        return;

    /*  Start polling for IN. */
    hndl->events |= EPOLLIN;
    memset (&ev, 0, sizeof (ev));
    ev.events = hndl->events;
    ev.data.ptr = (void*) hndl;
    epoll_ctl (self->ep, EPOLL_CTL_MOD, hndl->fd, &ev);
}

void nn_poller_reset_in (struct nn_poller *self, struct nn_poller_hndl *hndl)
{
    int i;
    struct epoll_event ev;

    /*  If not polling for IN, do nothing. */
    if (nn_slow (!(hndl->events & EPOLLIN)))
        return;

    /*  Stop polling for IN. */
    hndl->events &= ~EPOLLIN;
    memset (&ev, 0, sizeof (ev));
    ev.events = hndl->events;
    ev.data.ptr = (void*) hndl;
    epoll_ctl (self->ep, EPOLL_CTL_MOD, hndl->fd, &ev);

    /*  Invalidate any subsequent IN events on this file descriptor. */
    for (i = self->index; i != self->nevents; ++i)
        if (self->events [i].data.ptr == hndl)
            self->events [i].events &= ~EPOLLIN;
}

void nn_poller_set_out (struct nn_poller *self, struct nn_poller_hndl *hndl)
{
    struct epoll_event ev;
    int fd = hndl->fd;

    /*  If already polling for OUT, do nothing. */
    if (nn_slow (hndl->events & EPOLLOUT))
        return;

    /*  Start polling for OUT. */
    hndl->events |= EPOLLOUT;
    memset (&ev, 0, sizeof (ev));
    ev.events = hndl->events;
    ev.data.ptr = (void*) hndl;
    epoll_ctl (self->ep, EPOLL_CTL_MOD, fd, &ev);
}

void nn_poller_reset_out (struct nn_poller *self, struct nn_poller_hndl *hndl)
{
    int i;
    struct epoll_event ev;

    /*  If not polling for OUT, do nothing. */
    if (nn_slow (!(hndl->events & EPOLLOUT)))
        return;

    /*  Stop polling for OUT. */
    hndl->events &= ~EPOLLOUT;
    memset (&ev, 0, sizeof (ev));
    ev.events = hndl->events;
    ev.data.ptr = (void*) hndl;
    epoll_ctl (self->ep, EPOLL_CTL_MOD, hndl->fd, &ev);

    /*  Invalidate any subsequent OUT events on this file descriptor. */
    for (i = self->index; i != self->nevents; ++i)
        if (self->events [i].data.ptr == hndl)
            self->events [i].events &= ~EPOLLOUT;
}

int nn_poller_wait (struct nn_poller *self, int timeout)
{
    int nevents;

    /*  Clear all existing events. */
    self->nevents = 0;
    self->index = 0;

    /*  Wait for new events. */
    while (1) {
        nevents = epoll_wait (self->ep, self->events,
            NN_POLLER_MAX_EVENTS, timeout);
        if (nn_slow (nevents == -1 && errno == EINTR))
            continue;
        break;
    }
    errno_assert (self->nevents != -1);
    self->nevents = nevents;
    return 0;
}

int nn_poller_event (struct nn_poller *self, int *event,
    struct nn_poller_hndl **hndl)
{
    /*  Skip over empty events. */
    while (self->index < self->nevents) {
        if (self->events [self->index].events != 0)
            break;
        ++self->index;
    }

    /*  If there is no stored event, let the caller know. */
    if (nn_slow (self->index >= self->nevents))
        return -EAGAIN;

    /*  Return next event to the caller. Remove the event from the set. */
    *hndl = (struct nn_poller_hndl*) self->events [self->index].data.ptr;
    if (nn_fast (self->events [self->index].events & EPOLLIN)) {
        *event = NN_POLLER_IN;
        self->events [self->index].events &= ~EPOLLIN;
        return 0;
    }
    else if (nn_fast (self->events [self->index].events & EPOLLOUT)) {
        *event = NN_POLLER_OUT;
        self->events [self->index].events &= ~EPOLLOUT;
        return 0;
    }
    else {
        *event = NN_POLLER_ERR;
        ++self->index;
        return 0;
    }
}

