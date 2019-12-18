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

#include "../src/nn.h"
#include "../src/bus.h"
#include "testutil.h"

#define SOCKET_ADDRESS_A "inproc://a"
#define SOCKET_ADDRESS_B "inproc://b"

int main ()
{
    int rc;
    int bus1;
    int bus2;
    int bus3;
    char buf [3];

    /*  Create a simple bus topology consisting of 3 nodes. */
    bus1 = test_socket (AF_SP, NN_BUS);
    test_bind (bus1, SOCKET_ADDRESS_A);
    bus2 = test_socket (AF_SP, NN_BUS);
    test_bind (bus2, SOCKET_ADDRESS_B);
    test_connect (bus2, SOCKET_ADDRESS_A);
    bus3 = test_socket (AF_SP, NN_BUS);
    test_connect (bus3, SOCKET_ADDRESS_A);
    test_connect (bus3, SOCKET_ADDRESS_B);

    /*  Send a message from each node. */
    test_send (bus1, "A");
    test_send (bus2, "AB");
    test_send (bus3, "ABC");

    /*  Check that two messages arrived at each node. */
    rc = nn_recv (bus1, buf, 3, 0);
    errno_assert (rc >= 0);
    nn_assert (rc == 2 || rc == 3);
    rc = nn_recv (bus1, buf, 3, 0);
    errno_assert (rc >= 0);
    nn_assert (rc == 2 || rc == 3);
    rc = nn_recv (bus2, buf, 3, 0);
    errno_assert (rc >= 0);
    nn_assert (rc == 1 || rc == 3);
    rc = nn_recv (bus2, buf, 3, 0);
    errno_assert (rc >= 0);
    nn_assert (rc == 1 || rc == 3);
    rc = nn_recv (bus3, buf, 3, 0);
    errno_assert (rc >= 0);
    nn_assert (rc == 1 || rc == 2);
    rc = nn_recv (bus3, buf, 3, 0);
    errno_assert (rc >= 0);
    nn_assert (rc == 1 || rc == 2);

    /*  Wait till both connections are established. */
    nn_sleep (10);

    test_close (bus3);
    test_close (bus2);
    test_close (bus1);

    return 0;
}

