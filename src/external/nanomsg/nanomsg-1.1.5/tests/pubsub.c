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
#include "../src/pubsub.h"

#include "testutil.h"

#define SOCKET_ADDRESS "inproc://a"

int main ()
{
    int rc;
    int pub1;
    int pub2;
    int sub1;
    int sub2;
    char buf [8];
    size_t sz;

    pub1 = test_socket (AF_SP, NN_PUB);
    test_bind (pub1, SOCKET_ADDRESS);
    sub1 = test_socket (AF_SP, NN_SUB);
    rc = nn_setsockopt (sub1, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
    errno_assert (rc == 0);
    sz = sizeof (buf);
    rc = nn_getsockopt (sub1, NN_SUB, NN_SUB_SUBSCRIBE, buf, &sz);
    nn_assert (rc == -1 && nn_errno () == ENOPROTOOPT);
    test_connect (sub1, SOCKET_ADDRESS);
    sub2 = test_socket (AF_SP, NN_SUB);
    rc = nn_setsockopt (sub2, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
    errno_assert (rc == 0);
    test_connect (sub2, SOCKET_ADDRESS);

    /*  Wait till connections are established to prevent message loss. */
    nn_sleep (10);

    test_send (pub1, "0123456789012345678901234567890123456789");
    test_recv (sub1, "0123456789012345678901234567890123456789");
    test_recv (sub2, "0123456789012345678901234567890123456789");

    test_close (pub1);
    test_close (sub1);
    test_close (sub2);

    /*  Check receiving messages from two publishers. */

    sub1 = test_socket (AF_SP, NN_SUB);
    rc = nn_setsockopt (sub1, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
    errno_assert (rc == 0);
    test_bind (sub1, SOCKET_ADDRESS);
    pub1 = test_socket (AF_SP, NN_PUB);
    test_connect (pub1, SOCKET_ADDRESS);
    pub2 = test_socket (AF_SP, NN_PUB);
    test_connect (pub2, SOCKET_ADDRESS);
    nn_sleep (100);

    test_send (pub1, "0123456789012345678901234567890123456789");
    test_send (pub2, "0123456789012345678901234567890123456789");
    test_recv (sub1, "0123456789012345678901234567890123456789");
    test_recv (sub1, "0123456789012345678901234567890123456789");

    test_close (pub2);
    test_close (pub1);
    test_close (sub1);

    return 0;
}

