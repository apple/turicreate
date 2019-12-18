/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
    Copyright (c) 2015-2016 Jack R. Dunaway.  All rights reserved.
    Copyright 2017 Garrett D'Amore <garrett@damore.org>
    Copyright 2017 Capitar IT Group BV <info@capitar.com>


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

#include "efd.h"
#include "clock.h"

#if defined NN_USE_EVENTFD
    #include "efd_eventfd.inc"
#elif defined NN_USE_PIPE
    #include "efd_pipe.inc"
#elif defined NN_USE_SOCKETPAIR
    #include "efd_socketpair.inc"
#elif defined NN_USE_WINSOCK
    #include "efd_win.inc"
#else
    #error
#endif

#if defined NN_HAVE_POLL

#include <poll.h>

int nn_efd_wait (struct nn_efd *self, int timeout)
{
    int rc;
    struct pollfd pfd;

    pfd.fd = nn_efd_getfd (self);
    pfd.events = POLLIN;
    if (pfd.fd < 0)
        return -EBADF;

    rc = poll (&pfd, 1, timeout);
    switch (rc) {
    case 0:
        return -ETIMEDOUT;
    case -1:
        return (-errno);
    }
    return 0;
}

#elif defined NN_HAVE_WINDOWS

int nn_efd_wait (struct nn_efd *self, int timeout)
{
    int rc;
    WSAPOLLFD pfd;

    pfd.fd = self->r;
    if (nn_slow (pfd.fd == INVALID_SOCKET)) {
        return -EBADF;
    }
    pfd.events = POLLIN;
    rc = WSAPoll(&pfd, 1, timeout);

    switch (rc) {
    case 0:
        return -ETIMEDOUT;
    case SOCKET_ERROR:
        rc = nn_err_wsa_to_posix (WSAGetLastError ());
        errno = rc;

        /*  Treat these as a non-fatal errors, typically occuring when the
            socket is being closed from a separate thread during a blocking
            I/O operation. */
        if (rc == EINTR || rc == ENOTSOCK) {
            return self->r == INVALID_SOCKET ? -EBADF : -EINTR;
        }
        return (-rc);
    }
    return (0);
}

#else
    #error
#endif
