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

#include "testutil.h"

#include <string.h>

#define SOCKET_ADDRESS "inproc://a"

int main ()
{
    int rc;
    int sb;
    int sc;
    struct nn_iovec iov [2];
    struct nn_msghdr hdr;
    char buf [6];

    sb = test_socket (AF_SP, NN_PAIR);
    test_bind (sb, SOCKET_ADDRESS);
    sc = test_socket (AF_SP, NN_PAIR);
    test_connect (sc, SOCKET_ADDRESS);

    iov [0].iov_base = "AB";
    iov [0].iov_len = 2;
    iov [1].iov_base = "CDEF";
    iov [1].iov_len = 4;
    memset (&hdr, 0, sizeof (hdr));
    hdr.msg_iov = iov;
    hdr.msg_iovlen = 2;
    rc = nn_sendmsg (sc, &hdr, 0);
    errno_assert (rc >= 0);
    nn_assert (rc == 6);

    iov [0].iov_base = buf;
    iov [0].iov_len = 4;
    iov [1].iov_base = buf + 4;
    iov [1].iov_len = 2;
    memset (&hdr, 0, sizeof (hdr));
    hdr.msg_iov = iov;
    hdr.msg_iovlen = 2;
    rc = nn_recvmsg (sb, &hdr, 0);
    errno_assert (rc >= 0);
    nn_assert (rc == 6);
    nn_assert (memcmp (buf, "ABCDEF", 6) == 0);

    test_close (sc);
    test_close (sb);

    return 0;
}

