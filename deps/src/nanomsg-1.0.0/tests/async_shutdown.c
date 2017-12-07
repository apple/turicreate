/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
    Copyright (c) 2015 Jack R. Dunaway.  All rights reserved.
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
#include "../src/pipeline.h"
#include "../src/tcp.h"

#include "testutil.h"
#include "../src/utils/attr.h"
#include "../src/utils/thread.c"
#include "../src/utils/atomic.c"

/*  Test condition of closing sockets that are blocking in another thread. */

#define TEST_LOOPS 10

struct nn_atomic active;

static void routine (NN_UNUSED void *arg)
{
    int s;
    int rc;
    int msg;

    nn_assert (arg);

    s = *((int *) arg);

    /*  We don't expect to actually receive a message here;
        therefore, the datatype of 'msg' is irrelevant. */
    rc = nn_recv (s, &msg, sizeof(msg), 0);

    errno_assert (rc == -1 && nn_errno () == EBADF);
}

int main (int argc, const char *argv[])
{
    int sb;
    int i;
    struct nn_thread thread;
    char socket_address[128];

    test_addr_from(socket_address, "tcp", "127.0.0.1",
            get_test_port(argc, argv));

    for (i = 0; i != TEST_LOOPS; ++i) {
        sb = test_socket (AF_SP, NN_PULL);
        test_bind (sb, socket_address);
        nn_sleep (100);
        nn_thread_init (&thread, routine, &sb);
        nn_sleep (100);
        test_close (sb);
        nn_thread_term (&thread);
    }

    return 0;
}
