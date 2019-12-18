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
#include "../src/pair.h"

#include "../src/utils/thread.c"
#include "testutil.h"

static void worker (NN_UNUSED void *arg)
{
    int rc;
    int s;
    char buf [3];

    /*  Test socket. */
    s = test_socket (AF_SP, NN_PAIR);

    /*  Launch blocking function to check that it will be unblocked once
        nn_term() is called from the main thread. */
    rc = nn_recv (s, buf, sizeof (buf), 0);
    nn_assert (rc == -1 && nn_errno () == EBADF);

    /*  Check that all subsequent operations fail in synchronous manner. */
    rc = nn_recv (s, buf, sizeof (buf), 0);
    nn_assert (rc == -1 && nn_errno () == EBADF);
    test_close (s);
}

int main ()
{
    int rc;
    int s;
    struct nn_thread thread;

    /*  Close the socket with no associated endpoints. */
    s = test_socket (AF_SP, NN_PAIR);
    test_close (s);

    /*  Test nn_term() before nn_close(). */
    nn_thread_init (&thread, worker, NULL);
    nn_sleep (100);
    nn_term ();

    /*  Check that it's not possible to create new sockets after nn_term(). */
    rc = nn_socket (AF_SP, NN_PAIR);
    nn_assert (rc == -1);
    errno_assert (nn_errno () == ETERM);

    /*  Wait till worker thread terminates. */
    nn_thread_term (&thread);

    return 0;
}

