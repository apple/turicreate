/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
    Copyright 2017 Garrett D'Amore <garrett@damore.org>

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
#include "../src/pubsub.h"
#include "../src/ipc.h"

#include "testutil.h"

/*  Tests IPC transport. */

#define SOCKET_ADDRESS "ipc://test.ipc"

int main ()
{
#ifndef NN_HAVE_WSL
    int sb;
    int sc;
    int i;
    int s1, s2;
    void * dummy_buf;
    int rc;
    int opt;
    size_t opt_sz = sizeof (opt);

    int size;
    char * buf;

    /*  Try closing a IPC socket while it not connected. */
    sc = test_socket (AF_SP, NN_PAIR);
    test_connect (sc, SOCKET_ADDRESS);
    test_close (sc);

    /*  Open the socket anew. */
    sc = test_socket (AF_SP, NN_PAIR);
    test_connect (sc, SOCKET_ADDRESS);

    /*  Leave enough time for at least one re-connect attempt. */
    nn_sleep (200);

    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, SOCKET_ADDRESS);

    /*  Ping-pong test. */
    for (i = 0; i != 1; ++i) {
        test_send (sc, "0123456789012345678901234567890123456789");
        test_recv (sb, "0123456789012345678901234567890123456789");
        test_send (sb, "0123456789012345678901234567890123456789");
        test_recv (sc, "0123456789012345678901234567890123456789");
    }

    /*  Batch transfer test. */
    for (i = 0; i != 100; ++i) {
        test_send (sc, "XYZ");
    }
    for (i = 0; i != 100; ++i) {
        test_recv (sb, "XYZ");
    }

    /*  Send something large enough to trigger overlapped I/O on Windows. */
    size = 10000;
    buf = malloc (size);
    for (i = 0; i < size; ++i) {
        buf[i] = 48 + i % 10;
    }
    buf[size-1] = '\0';
    test_send (sc, buf);
    test_recv (sb, buf);
    free (buf);

    test_close (sc);
    test_close (sb);

    /*  Test whether connection rejection is handled decently. */
    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, SOCKET_ADDRESS);
    s1 = test_socket (AF_SP, NN_PAIR);
    test_connect (s1, SOCKET_ADDRESS);
    s2 = test_socket (AF_SP, NN_PAIR);
    test_connect (s2, SOCKET_ADDRESS);
    nn_sleep (100);
    test_close (s2);
    test_close (s1);
    test_close (sb);

/*  On Windows, CreateNamedPipeA does not run exclusively.
    We should look at fixing this, but it will require
    changing the usock code for Windows.  In the meantime just
    disable this test on Windows. */
#if !defined(NN_HAVE_WINDOWS)
    /*  Test two sockets binding to the same address. */
    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, SOCKET_ADDRESS);
    s1 = test_socket (AF_SP, NN_PAIR);
    rc = nn_bind (s1, SOCKET_ADDRESS);
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EADDRINUSE);
    sc = test_socket (AF_SP, NN_PAIR);
    test_connect (sc, SOCKET_ADDRESS);
    nn_sleep (100);
    test_send (sb, "ABC");
    test_recv (sc, "ABC");
    test_close (sb);
    test_close (sc);
    test_close (s1);
#endif

    /*  Test NN_RCVMAXSIZE limit */
    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, SOCKET_ADDRESS);
    s1 = test_socket (AF_SP, NN_PAIR);
    test_connect (s1, SOCKET_ADDRESS);
    opt = 4;
    rc = nn_setsockopt (sb, NN_SOL_SOCKET, NN_RCVMAXSIZE, &opt, opt_sz);
    nn_assert (rc == 0);
    nn_sleep (100);
    test_send (s1, "ABCD");
    test_recv (sb, "ABCD");
    test_send (s1, "ABCDE");
    /*  Without sleep nn_recv returns EAGAIN even for string
        of acceptable size, so false positives are possible. */
    nn_sleep (100);
    rc = nn_recv (sb, &dummy_buf, NN_MSG, NN_DONTWAIT);
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EAGAIN);
    test_close (sb);
    test_close (s1);

    /*  Test that NN_RCVMAXSIZE can be -1, but not lower */
    sb = test_socket (AF_SP, NN_PAIR);
    opt = -1;
    rc = nn_setsockopt (sb, NN_SOL_SOCKET, NN_RCVMAXSIZE, &opt, opt_sz);
    nn_assert (rc >= 0);
    opt = -2;
    rc = nn_setsockopt (sb, NN_SOL_SOCKET, NN_RCVMAXSIZE, &opt, opt_sz);
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    test_close (sb);

    /*  Test closing a socket that is waiting to connect. */
    sc = test_socket (AF_SP, NN_PAIR);
    test_connect (sc, SOCKET_ADDRESS);
    nn_sleep (100);
    test_close (sc);
#endif /* NN_HAVE_WSL */

    return 0;
}
