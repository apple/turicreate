/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
    Copyright 2016 Franklin "Snaipe" Mathieu <franklinmathieu@gmail.com>
    Copyright 2017 Garrett D'Amore <garrett@damore.org>

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

/*  Stress test the TCP transport. */

#ifdef NN_HAVE_WSL
#define THREAD_COUNT 10
#else
#define THREAD_COUNT 100
#endif

#define TEST2_THREAD_COUNT 10
#define MESSAGES_PER_THREAD 10
#define TEST_LOOPS 10

struct nn_atomic active;

static char socket_address[128];

static void routine (NN_UNUSED void *arg)
{
    int s;

    s = nn_socket (AF_SP, NN_SUB);
    if (s < 0 && nn_errno () == EMFILE)
        return;
    errno_assert (s >= 0);
    test_connect (s, socket_address);
    test_close (s);
}

static void routine2 (NN_UNUSED void *arg)
{
    int s;
    int i;
    int ms;

    s = test_socket (AF_SP, NN_PULL);

    test_connect (s, socket_address);

    ms = 2000;
    test_setsockopt (s, NN_SOL_SOCKET, NN_RCVTIMEO, &ms, sizeof (ms));

    for (i = 0; i < MESSAGES_PER_THREAD; ++i) {
        test_recv (s, "hello");
    }

    test_close (s);
    nn_atomic_dec(&active, 1);
}

int main (int argc, const char *argv[])
{
    int sb;
    int i;
    int j;
    struct nn_thread threads [THREAD_COUNT];

    test_addr_from(socket_address, "tcp", "127.0.0.1",
            get_test_port(argc, argv));

    /*  Stress the shutdown algorithm. */

#if defined(SIGPIPE) && defined(SIG_IGN)
    signal (SIGPIPE, SIG_IGN);
#endif

    sb = test_socket (AF_SP, NN_PUB);
    test_bind (sb, socket_address);

    for (j = 0; j != TEST_LOOPS; ++j) {
        for (i = 0; i != THREAD_COUNT; ++i)
            nn_thread_init (&threads [i], routine, NULL);
        for (i = 0; i != THREAD_COUNT; ++i) {
            nn_thread_term (&threads [i]);
	}
    }

    test_close (sb);

    /*  Test race condition of sending message while socket shutting down  */

    sb = test_socket (AF_SP, NN_PUSH);
    test_bind (sb, socket_address);

    for (j = 0; j != TEST_LOOPS; ++j) {
	int ms;
        nn_atomic_init(&active, TEST2_THREAD_COUNT);
        for (i = 0; i != TEST2_THREAD_COUNT; ++i)
            nn_thread_init (&threads [i], routine2, &threads[i]);

	nn_sleep(100);
	ms = 200;
	test_setsockopt (sb, NN_SOL_SOCKET, NN_SNDTIMEO, &ms, sizeof (ms));
        while (active.n) {
            (void) nn_send (sb, "hello", 5, 0);
        }

        for (i = 0; i != TEST2_THREAD_COUNT; ++i)
            nn_thread_term (&threads [i]);
        nn_atomic_term(&active);
    }

    test_close (sb);

    return 0;
}
