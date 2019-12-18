/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
    Copyright 2015 Garrett D'Amore <garrett@damore.org>

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

#include "err.h"
#include "fast.h"
#include "closefd.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

int nn_efd_init (struct nn_efd *self)
{
    int rc;
    int flags;
    int p [2];

#if defined NN_HAVE_PIPE2
    rc = pipe2 (p, O_NONBLOCK | O_CLOEXEC);
    if ((rc == -1) &&  (errno == ENOTSUP)) {
        /*  Deal with unimplemented system call/libc mismatch (Linux). */
        rc = pipe (p);
    }
#else
    rc = pipe (p);
#endif
    if (rc != 0 && (errno == EMFILE || errno == ENFILE))
        return -EMFILE;
    errno_assert (rc == 0);
    self->r = p [0];
    self->w = p [1];

#if !defined NN_HAVE_PIPE2 && defined FD_CLOEXEC
    rc = fcntl (self->r, F_SETFD, FD_CLOEXEC);
    errno_assert (rc != -1);
    rc = fcntl (self->w, F_SETFD, FD_CLOEXEC);
    errno_assert (rc != -1);
#endif

#if !defined NN_HAVE_PIPE2
    flags = fcntl (self->r, F_GETFL, 0);
    if (flags == -1)
        flags = 0;
    rc = fcntl (self->r, F_SETFL, flags | O_NONBLOCK);
    errno_assert (rc != -1);
#endif

    return 0;
}

void nn_efd_term (struct nn_efd *self)
{
    /*  Close the read side first. */
    int fd = self->r;
    self->r = -1;
    nn_closefd (fd);

    fd = self->w;
    self->w = -1;
    nn_closefd (fd);
}

void nn_efd_stop (struct nn_efd *self)
{
    /*  Close the write side, which wakes up pollers with POLLHUP. */
    int fd = self->w;
    self->w = -1;
    nn_closefd (fd);
}

nn_fd nn_efd_getfd (struct nn_efd *self)
{
    return self->r;
}

void nn_efd_signal (struct nn_efd *self)
{
    ssize_t nbytes;
    char c = 101;
    int fd = self->w;

    if (nn_slow (fd < 0))
        return;
    nbytes = write (self->w, &c, 1);
    errno_assert (nbytes != -1);
    nn_assert (nbytes == 1);
}

void nn_efd_unsignal (struct nn_efd *self)
{
    ssize_t nbytes;
    uint8_t buf [16];

    while (1) {
        int fd = self->r;
        if (nn_slow (fd < 0))
            return;
        nbytes = read (fd, buf, sizeof (buf));
        if (nbytes < 0 && errno == EAGAIN)
            nbytes = 0;
        errno_assert (nbytes >= 0);
        if (nn_fast ((size_t) nbytes < sizeof (buf)))
            break;
    }
}

