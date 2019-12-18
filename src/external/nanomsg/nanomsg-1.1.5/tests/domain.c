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

/*  Test the NN_DOMAIN and NN_PROTOCOL socket options. */

int main ()
{
    int rc;
    int s;
    int op;
    size_t opsz;

    s = test_socket (AF_SP, NN_PAIR);

    opsz = sizeof (op);
    rc = nn_getsockopt (s, NN_SOL_SOCKET, NN_DOMAIN, &op, &opsz);
    errno_assert (rc == 0);
    nn_assert (opsz == sizeof (op));
    nn_assert (op == AF_SP);

    opsz = sizeof (op);
    rc = nn_getsockopt (s, NN_SOL_SOCKET, NN_PROTOCOL, &op, &opsz);
    errno_assert (rc == 0);
    nn_assert (opsz == sizeof (op));
    nn_assert (op == NN_PAIR);

    test_close (s);

    return 0;
}

