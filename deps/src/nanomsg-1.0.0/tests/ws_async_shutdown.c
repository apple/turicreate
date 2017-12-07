/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
    Copyright (c) 2015-2016 Jack R. Dunaway. All rights reserved.

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
#include "../src/utils/thread.c"

static char socket_address [128];

/*  Test condition of closing sockets that are blocking in another thread. */

#define TEST_LOOPS 10
#define TEST_THREADS 10

static void routine (NN_UNUSED void *arg)
{
    int s;
    int rc;
    char msg[1];

    nn_assert (arg);

    s = *(int *)arg;

    while (1) {
        rc = nn_recv (s, &msg, sizeof(msg), 0);
        if (rc == 0) {
            continue;
        }

        nn_assert (rc == -1);

        /*  A timeout is OK since PUB/SUB is lossy. */
        if (nn_errno () == ETIMEDOUT) {
            continue;
        }
        break;
    }
    /*  Socket is expected to be closed by caller.  */
    errno_assert (nn_errno () == EBADF);
}

int main (int argc, const char *argv[])
{
    int i;
    int j;
    int s;
    int sb;
    int rcvtimeo = 10;
    int sndtimeo = 0;
    int sockets [TEST_THREADS];
    struct nn_thread threads [TEST_THREADS];

    test_addr_from (socket_address, "ws", "127.0.0.1",
        get_test_port (argc, argv));

    for (i = 0; i != TEST_LOOPS; ++i) {

        sb = test_socket (AF_SP, NN_PUB);
        test_bind (sb, socket_address);
        test_setsockopt (sb, NN_SOL_SOCKET, NN_SNDTIMEO,
            &sndtimeo, sizeof (sndtimeo));

        for (j = 0; j < TEST_THREADS; j++){
            s = test_socket (AF_SP, NN_SUB);
            test_setsockopt (s, NN_SOL_SOCKET, NN_RCVTIMEO,
                &rcvtimeo, sizeof (rcvtimeo));
            test_setsockopt (s, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
            test_connect (s, socket_address);
            sockets [j] = s;
            nn_thread_init (&threads [j], routine, &sockets [j]);
        }

        /*  Allow all threads a bit of time to connect. */
        nn_sleep (100);

        test_send (sb, "");

        for (j = 0; j < TEST_THREADS; j++) {
            test_close (sockets [j]);
            nn_thread_term (&threads [j]);
        }

        test_close (sb);
    }

    return 0;
}
