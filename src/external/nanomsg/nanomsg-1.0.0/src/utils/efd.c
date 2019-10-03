/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
    Copyright 2016 Garrett D'Amore <garrett@damore.org>
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
    uint64_t expire;

    if (timeout > 0) {
        expire = nn_clock_ms() + timeout;
    } else {
        expire = timeout;
    }

    /*  In order to solve a problem where the poll call doesn't wake up
        when a file is closed, we sleep a maximum of 100 msec.  This is
        a somewhat unfortunate band-aid to prevent hangs caused by a race
        condition involving nn_close.  In the future this code should be
        replaced by a simpler design using condition variables. */
    for (;;) {
        pfd.fd = nn_efd_getfd (self);
        pfd.events = POLLIN;
        if (nn_slow (pfd.fd < 0))
            return -EBADF;

        switch (expire) {
        case 0:
            /* poll once */
            timeout = 0;
            break;

        case (uint64_t)-1:
            /* infinite wait */
            timeout = 100;
            break;

        default:
            /* bounded wait */
            timeout = (int)(expire - nn_clock_ms());
            if (timeout < 0) {
                return -ETIMEDOUT;
            }
            if (timeout > 100) {
                timeout = 100;
            }
        }
        rc = poll (&pfd, 1, timeout);
        if (nn_slow (rc < 0 && errno == EINTR))
            return -EINTR;
        errno_assert (rc >= 0);
        if (nn_slow (rc == 0)) {
            if (expire == 0)
                return -ETIMEDOUT;
            if ((expire != (uint64_t)-1) && (expire < nn_clock_ms())) {
                return -ETIMEDOUT;
            }
            continue;
	}
        return 0;
    }
}

#elif defined NN_HAVE_WINDOWS

int nn_efd_wait (struct nn_efd *self, int timeout)
{
    int rc;
    struct timeval tv;
    SOCKET fd = self->r;
    uint64_t expire;

    if (timeout > 0) {
        expire = nn_clock_ms() + timeout;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = timeout % 1000 * 1000;
    } else {
        expire = timeout;
    }

    for (;;) {
        if (nn_slow (fd == INVALID_SOCKET)) {
            return -EBADF;
        }
        FD_SET (fd, &self->fds);
        switch (expire) {
        case 0:
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            break;
        case (uint64_t)-1:
            tv.tv_sec = 0;
            tv.tv_usec = 100000;
            break;
        default:
            timeout = (int)(expire - nn_clock_ms());
            if (timeout < 0) {
                return -ETIMEDOUT;
            }
            if (timeout > 100) {
                tv.tv_sec = 0;
                tv.tv_usec = 100000;
            } else {
                tv.tv_sec = timeout / 1000;
                tv.tv_usec = timeout % 1000 * 1000;
            }
        }
        rc = select (0, &self->fds, NULL, NULL, &tv);

        if (nn_slow (rc == SOCKET_ERROR)) {
            rc = nn_err_wsa_to_posix (WSAGetLastError ());
            errno = rc;

            /*  Treat these as a non-fatal errors, typically occuring when the
                socket is being closed from a separate thread during a blocking
                I/O operation. */
            if (rc == EINTR || rc == ENOTSOCK)
                return self->r == INVALID_SOCKET ? -EBADF : -EINTR;
        } else if (rc == 0) {
            if (expire == 0)
                return -ETIMEDOUT;
            if ((expire != (uint64_t)-1) && (expire < nn_clock_ms())) {
                return -ETIMEDOUT;
            }
            continue;
	}

        wsa_assert (rc >= 0);
        return 0;
    }
}

#else
    #error
#endif
