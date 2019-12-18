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
#include "../src/tcp.h"
#include "../src/utils/err.c"

#define MAX_SOCKETS 1000

int main ()
{
    int rc;
    int i;
    int socks [MAX_SOCKETS];

    /*  First, just create as much SP sockets as possible. */
    for (i = 0; i != MAX_SOCKETS; ++i) {
        socks [i] = nn_socket (AF_SP, NN_PAIR);
        if (socks [i] < 0) {
            errno_assert (nn_errno () == EMFILE);
            break;
        }
    }
    while (1) {
        --i;
        if (i == -1)
            break;
        rc = nn_close (socks [i]);
        errno_assert (rc == 0);
    }

    return 0;
}

