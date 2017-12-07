/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
    Copyright 2016 Garrett D'Amore <garrett@damore.org>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/utils/err.c"
#include "../src/utils/sleep.c"

int main (int argc, char *argv [])
{
    const char *connect_to;
    size_t sz;
    int count;
    char *buf;
    int nbytes;
    int s;
    int rc;
    int i;
    int opt;

    if (argc != 4) {
        printf ("usage: remote_thr <connect-to> <msg-size> <msg-count>\n");
        return 1;
    }
    connect_to = argv [1];
    sz = atoi (argv [2]);
    count = atoi (argv [3]);

    s = nn_socket (AF_SP, NN_PAIR);
    nn_assert (s != -1);
    rc = nn_connect (s, connect_to);
    nn_assert (rc >= 0);

    opt = -1;
    rc = nn_setsockopt (s, NN_SOL_SOCKET, NN_RCVMAXSIZE, &opt, sizeof (opt));
    nn_assert (rc == 0);

    opt = 1000;
    rc = nn_setsockopt (s, NN_SOL_SOCKET, NN_LINGER, &opt, sizeof (opt));
    nn_assert (rc == 0);

    buf = malloc (sz);
    nn_assert (buf);
    memset (buf, 111, sz);

    nbytes = nn_send (s, buf, 0, 0);
    nn_assert (nbytes == 0);

    for (i = 0; i != count; i++) {
        nbytes = nn_send (s, buf, sz, 0);
        nn_assert (nbytes == (int)sz);
    }

    free (buf);

    /*  Linger doesn't always do the trick, so sleep a bit to be sure. */
    nn_sleep (1000);

    rc = nn_close (s);
    nn_assert (rc == 0);

    return 0;
}
