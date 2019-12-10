/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
    Copyright 2017 Garrett D'Amore <garrett@damore.org>
    Copyright 2018 Staysail Systems, Inc. <info@staysail.tech>
    Copyright 2018 Capitar IT Group BV <info@capitar.com>


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

#include <string.h>
#include <stdint.h>

int nn_efd_init (struct nn_efd *self)
{
    SOCKET listener;
    int rc;
    struct sockaddr_in addr;
    socklen_t addrlen;
    int one;
    BOOL nodelay;
    u_long nonblock;

    /*  Unfortunately, on Windows the only way to send signal to a file
        descriptor (SOCKET) is to create a full-blown TCP connecting on top of
        the loopback interface. */
    self->w = INVALID_SOCKET;
    self->r = INVALID_SOCKET;

    /*  Create listening socket. */
    listener = socket (AF_INET, SOCK_STREAM, 0);
    if (listener == SOCKET_ERROR)
        goto wsafail;

    one = 1;
    rc = setsockopt (listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, 
        (char*) &one, sizeof (one));
    if (rc == SOCKET_ERROR)
        goto wsafail;

    /*  Bind the listening socket to the local port. */
    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    addr.sin_port = 0;

    rc = bind (listener, (const struct sockaddr*) &addr, sizeof (addr));
    if (rc == SOCKET_ERROR)
        goto wsafail;

    /*  Get the port we bound to (will be ephemeral.) */
    addrlen = sizeof (addr);
    rc = getsockname (listener, (struct sockaddr *) &addr, &addrlen);
    if (rc == SOCKET_ERROR)
        goto wsafail;

    /*  Start listening for the incomming connections. In normal case we are
        going to accept just a single connection, so backlog buffer of size
        1 is sufficient. */
    rc = listen (listener, 1);
    if (rc == SOCKET_ERROR)
        goto wsafail;

    /*  The following code is in the loop, because windows sometimes delays
        WSAEADDRINUSE error to the `connect` call. But retrying the connection
        works like a charm. Still we want to limit number of retries  */

    /*  Create the writer socket. */
    self->w = socket (AF_INET, SOCK_STREAM, 0);
    if (listener == SOCKET_ERROR)
        goto wsafail;

    /*  Set TCP_NODELAY on the writer socket to make efd as fast as possible.
        There's only one byte going to be written, so batching would not make
        sense anyway. */
        nodelay = 1;
    rc = setsockopt (self->w, IPPROTO_TCP, TCP_NODELAY, (char*) &nodelay,
        sizeof (nodelay));
    if (nn_slow (rc == SOCKET_ERROR))
        goto wsafail;

    /*  Connect the writer socket to the listener socket. */
    rc = connect (self->w, (struct sockaddr*) &addr, sizeof (addr));
    if (rc == SOCKET_ERROR)
        goto wsafail;

    /*  Accept new incoming connection. */
    addrlen = sizeof (addr);
    self->r = accept (listener, (struct sockaddr*) &addr, &addrlen);
    if (self->r == INVALID_SOCKET)
        goto wsafail;

    /*  Close the listener, we don't need it anymore. */
    (void) closesocket (listener);

    /*  Make the receiving socket non-blocking. */
    nonblock = 1;
    rc = ioctlsocket (self->r, FIONBIO, &nonblock);
    wsa_assert (rc != SOCKET_ERROR);

    return 0;

wsafail:
    rc = nn_err_wsa_to_posix (WSAGetLastError ());
    return -rc;
}

void nn_efd_stop (struct nn_efd *self)
{
    int rc;
    SOCKET s = self->w;
    self->w = INVALID_SOCKET;

    if (s != INVALID_SOCKET) {
        rc = closesocket (s);
        wsa_assert (rc != INVALID_SOCKET);
    }
}

void nn_efd_term (struct nn_efd *self)
{
    int rc;
    SOCKET s;

    s = self->r;
    self->r = INVALID_SOCKET;
    if (s != INVALID_SOCKET) {
        rc = closesocket (s);
        wsa_assert (rc != INVALID_SOCKET);
    }
    s = self->w;
    self->w = INVALID_SOCKET;
    if (s != INVALID_SOCKET) {
       rc = closesocket (s);
       wsa_assert (rc != INVALID_SOCKET);
    }
}

nn_fd nn_efd_getfd (struct nn_efd *self)
{
    return self->r;
}

void nn_efd_signal (struct nn_efd *self)
{
    int rc;
    unsigned char c = 0xec;
    SOCKET s = self->w;

    if (nn_fast (s != INVALID_SOCKET)) {
        rc = send (s, (char*) &c, 1, 0);
        wsa_assert (rc != SOCKET_ERROR);
        nn_assert (rc == 1);
    }
}

void nn_efd_unsignal (struct nn_efd *self)
{
    int rc;
    uint8_t buf [16];

    while (1) {
        SOCKET s = self->r;
        if (nn_slow (s == INVALID_SOCKET))
            break;
        rc = recv (s, (char*) buf, sizeof (buf), 0);
        if (rc == SOCKET_ERROR && WSAGetLastError () == WSAEWOULDBLOCK)
            rc = 0;
        wsa_assert (rc != SOCKET_ERROR);
        if ((rc == SOCKET_ERROR) || (rc < sizeof (buf)))
            break;
    }
}

