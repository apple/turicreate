/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.

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

#include "../src/nn.h"
#include "../src/pair.h"
#include "../src/inproc.h"

#include "testutil.h"
#include "../src/utils/attr.h"
#include "../src/utils/thread.c"

#if defined NN_HAVE_WINDOWS
#include "../src/utils/win.h"
#else
#include <sys/select.h>
#endif

/*  Test of polling via NN_SNDFD/NN_RCVFD mechanism. */

#define SOCKET_ADDRESS "inproc://a"

int sc;

void routine1 (NN_UNUSED void *arg)
{
   nn_sleep (10);
   test_send (sc, "ABC");
}

void routine2 (NN_UNUSED void *arg)
{
   nn_sleep (10);
   nn_term ();
}

#define NN_IN 1
#define NN_OUT 2

int getevents (int s, int events, int timeout)
{
    int rc;
    fd_set pollset;
#if defined NN_HAVE_WINDOWS
    SOCKET rcvfd;
    SOCKET sndfd;
#else
    int rcvfd;
    int sndfd;
    int maxfd;
#endif
    size_t fdsz;
    struct timeval tv;
    int revents;

#if !defined NN_HAVE_WINDOWS
    maxfd = 0;
#endif
    FD_ZERO (&pollset);

    if (events & NN_IN) {
        fdsz = sizeof (rcvfd);
        rc = nn_getsockopt (s, NN_SOL_SOCKET, NN_RCVFD, (char*) &rcvfd, &fdsz);
        errno_assert (rc == 0);
        nn_assert (fdsz == sizeof (rcvfd));
        FD_SET (rcvfd, &pollset);
#if !defined NN_HAVE_WINDOWS
        if (rcvfd + 1 > maxfd)
            maxfd = rcvfd + 1;
#endif
    }

    if (events & NN_OUT) {
        fdsz = sizeof (sndfd);
        rc = nn_getsockopt (s, NN_SOL_SOCKET, NN_SNDFD, (char*) &sndfd, &fdsz);
        errno_assert (rc == 0);
        nn_assert (fdsz == sizeof (sndfd));
        FD_SET (sndfd, &pollset);
#if !defined NN_HAVE_WINDOWS
        if (sndfd + 1 > maxfd)
            maxfd = sndfd + 1;
#endif
    }

    if (timeout >= 0) {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
    }
#if defined NN_HAVE_WINDOWS
    rc = select (0, &pollset, NULL, NULL, timeout < 0 ? NULL : &tv);
    wsa_assert (rc != SOCKET_ERROR);
#else
    rc = select (maxfd, &pollset, NULL, NULL, timeout < 0 ? NULL : &tv);
    errno_assert (rc >= 0);
#endif
    revents = 0;
    if ((events & NN_IN) && FD_ISSET (rcvfd, &pollset))
        revents |= NN_IN;
    if ((events & NN_OUT) && FD_ISSET (sndfd, &pollset))
        revents |= NN_OUT;
    return revents;
}

int main ()
{
    int rc;
    int sb;
    char buf [3];
    struct nn_thread thread;
    struct nn_pollfd pfd [2];

    /* Test nn_poll() function. */
    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, SOCKET_ADDRESS);
    sc = test_socket (AF_SP, NN_PAIR);
    test_connect (sc, SOCKET_ADDRESS);
    test_send (sc, "ABC");
    nn_sleep (100);
    pfd [0].fd = sb;
    pfd [0].events = NN_POLLIN | NN_POLLOUT;
    pfd [1].fd = sc;
    pfd [1].events = NN_POLLIN | NN_POLLOUT;
    rc = nn_poll (pfd, 2, -1);
    errno_assert (rc >= 0);
    nn_assert (rc == 2);
    nn_assert (pfd [0].revents == (NN_POLLIN | NN_POLLOUT));
    nn_assert (pfd [1].revents == NN_POLLOUT);
    test_close (sc);
    test_close (sb);

    /*  Create a simple topology. */
    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, SOCKET_ADDRESS);
    sc = test_socket (AF_SP, NN_PAIR);
    test_connect (sc, SOCKET_ADDRESS);

    /*  Check the initial state of the socket. */
    rc = getevents (sb, NN_IN | NN_OUT, 1000);
    nn_assert (rc == NN_OUT);

    /*  Poll for IN when there's no message available. The call should
        time out. */
    rc = getevents (sb, NN_IN, 10);
    nn_assert (rc == 0);

    /*  Send a message and start polling. This time IN event should be
        signaled. */
    test_send (sc, "ABC");
    rc = getevents (sb, NN_IN, 1000);
    nn_assert (rc == NN_IN);

    /*  Receive the message and make sure that IN is no longer signaled. */
    test_recv (sb, "ABC");
    rc = getevents (sb, NN_IN, 10);
    nn_assert (rc == 0);

    /*  Check signalling from a different thread. */
    nn_thread_init (&thread, routine1, NULL);
    rc = getevents (sb, NN_IN, 1000);
    nn_assert (rc == NN_IN);
    test_recv (sb, "ABC");
    nn_thread_term (&thread);

    /*  Check terminating the library from a different thread. */
    nn_thread_init (&thread, routine2, NULL);
    rc = nn_recv (sb, buf, sizeof (buf), 0);
    nn_assert (rc < 0 && nn_errno () == EBADF);
    nn_thread_term (&thread);

    /*  Clean up. */
    test_close (sc);
    test_close (sb);

    return 0;
}

