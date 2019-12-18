/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
    Copyright 2015 Garrett D'Amore <garrett@damore.org>
    Copyright 2016 Franklin "Snaipe" Mathieu <franklinmathieu@gmail.com>

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
#include "../src/tcp.h"

#include "testutil.h"

/*  Tests TCP transport. */

int sc;

int main (int argc, const char *argv[])
{
    int rc;
    int sb;
    int i;
    int opt;
    size_t sz;
    int s1, s2;
    void * dummy_buf;
    char addr[128];
    char socket_address[128];

    int port = get_test_port(argc, argv);

    test_addr_from(socket_address, "tcp", "127.0.0.1", port);

    /*  Try closing bound but unconnected socket. */
    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, socket_address);
    test_close (sb);

    /*  Try closing a TCP socket while it not connected. At the same time
        test specifying the local address for the connection. */
    sc = test_socket (AF_SP, NN_PAIR);
    test_addr_from(addr, "tcp", "127.0.0.1;127.0.0.1", port);
    test_connect (sc, addr);
    test_close (sc);

    /*  Open the socket anew. */
    sc = test_socket (AF_SP, NN_PAIR);

    /*  Check NODELAY socket option. */
    sz = sizeof (opt);
    rc = nn_getsockopt (sc, NN_TCP, NN_TCP_NODELAY, &opt, &sz);
    errno_assert (rc == 0);
    nn_assert (sz == sizeof (opt));
    nn_assert (opt == 0);
    opt = 2;
    rc = nn_setsockopt (sc, NN_TCP, NN_TCP_NODELAY, &opt, sizeof (opt));
    nn_assert (rc < 0 && nn_errno () == EINVAL);
    opt = 1;
    rc = nn_setsockopt (sc, NN_TCP, NN_TCP_NODELAY, &opt, sizeof (opt));
    errno_assert (rc == 0);
    sz = sizeof (opt);
    rc = nn_getsockopt (sc, NN_TCP, NN_TCP_NODELAY, &opt, &sz);
    errno_assert (rc == 0);
    nn_assert (sz == sizeof (opt));
    nn_assert (opt == 1);

    /*  Try using invalid address strings. */
    rc = nn_connect (sc, "tcp://*:");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    rc = nn_connect (sc, "tcp://*:1000000");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    rc = nn_connect (sc, "tcp://*:some_port");
    nn_assert (rc < 0);
    rc = nn_connect (sc, "tcp://eth10000;127.0.0.1:5555");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == ENODEV);
    rc = nn_connect (sc, "tcp://127.0.0.1");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    rc = nn_bind (sc, "tcp://127.0.0.1:");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    rc = nn_bind (sc, "tcp://127.0.0.1:1000000");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    rc = nn_bind (sc, "tcp://eth10000:5555");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == ENODEV);
    rc = nn_connect (sc, "tcp://:5555");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    rc = nn_connect (sc, "tcp://-hostname:5555");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    rc = nn_connect (sc, "tcp://abc.123.---.#:5555");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    rc = nn_connect (sc, "tcp://[::1]:5555");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    rc = nn_connect (sc, "tcp://abc...123:5555");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    rc = nn_connect (sc, "tcp://.123:5555");
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);

    /*  Connect correctly. Do so before binding the peer socket. */
    test_connect (sc, socket_address);

    /*  Leave enough time for at least on re-connect attempt. */
    nn_sleep (200);

    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, socket_address);

    /*  Ping-pong test. */
    for (i = 0; i != 100; ++i) {

        test_send (sc, "ABC");
        test_recv (sb, "ABC");

        test_send (sb, "DEF");
        test_recv (sc, "DEF");
    }

    /*  Batch transfer test. */
    for (i = 0; i != 100; ++i) {
        test_send (sc, "0123456789012345678901234567890123456789");
    }
    for (i = 0; i != 100; ++i) {
        test_recv (sb, "0123456789012345678901234567890123456789");
    }

    test_close (sc);
    test_close (sb);

    /*  Test whether connection rejection is handled decently. */
    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, socket_address);
    s1 = test_socket (AF_SP, NN_PAIR);
    test_connect (s1, socket_address);
    s2 = test_socket (AF_SP, NN_PAIR);
    test_connect (s2, socket_address);
    nn_sleep (100);
    test_close (s2);
    test_close (s1);
    test_close (sb);

    /*  Test two sockets binding to the same address. */
    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, socket_address);
    s1 = test_socket (AF_SP, NN_PAIR);

    rc = nn_bind (s1, socket_address);
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EADDRINUSE);

    sc = test_socket (AF_SP, NN_PAIR);
    test_connect (sc, socket_address);
    nn_sleep (100);
    test_send (sb, "ABC");
    test_recv (sc, "ABC");
    test_close (sb);
    test_close (sc);
    test_close (s1);

    /*  Test NN_RCVMAXSIZE limit */
    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, socket_address);
    s1 = test_socket (AF_SP, NN_PAIR);
    test_connect (s1, socket_address);
    opt = 4;
    rc = nn_setsockopt (sb, NN_SOL_SOCKET, NN_RCVMAXSIZE, &opt, sizeof (opt));
    nn_assert (rc == 0);
    nn_sleep (100);
    test_send (s1, "ABC");
    test_recv (sb, "ABC");
    test_send (s1, "0123456789012345678901234567890123456789");
    rc = nn_recv (sb, &dummy_buf, NN_MSG, NN_DONTWAIT);
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EAGAIN);
    test_close (sb);
    test_close (s1);

    /*  Test that NN_RCVMAXSIZE can be -1, but not lower */
    sb = test_socket (AF_SP, NN_PAIR);
    opt = -1;
    rc = nn_setsockopt (sb, NN_SOL_SOCKET, NN_RCVMAXSIZE, &opt, sizeof (opt));
    nn_assert (rc >= 0);
    opt = -2;
    rc = nn_setsockopt (sb, NN_SOL_SOCKET, NN_RCVMAXSIZE, &opt, sizeof (opt));
    nn_assert (rc < 0);
    errno_assert (nn_errno () == EINVAL);
    test_close (sb);

    /*  Test closing a socket that is waiting to connect. */
    sc = test_socket (AF_SP, NN_PAIR);
    test_connect (sc, socket_address);
    nn_sleep (100);
    test_close (sc);

    return 0;
}
