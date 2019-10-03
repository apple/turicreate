/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
    Copyright 2015 Garrett D'Amore <garrett@damore.org>

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
#include "../src/ipc.h"

#include "testutil.h"
#include "../src/utils/thread.c"
#include "../src/utils/atomic.h"
#include "../src/utils/atomic.c"

/*  Stress test the IPC transport. */

#define THREAD_COUNT 10
#define TEST_LOOPS 10
#define SOCKET_ADDRESS "ipc://test-stress.ipc"

static void server(NN_UNUSED void *arg)
{
    int bytes;
    int count;
    int sock = nn_socket(AF_SP, NN_PULL);
    int res[TEST_LOOPS];
    nn_assert(sock >= 0);
    nn_assert(nn_bind(sock, SOCKET_ADDRESS) >= 0);
    count = THREAD_COUNT * TEST_LOOPS;
    memset(res, 0, sizeof (res));
    while (count > 0)
    {
        char *buf = NULL;
        int tid;
        int num;
        bytes = nn_recv(sock, &buf, NN_MSG, 0);
        nn_assert(bytes >= 0);
        nn_assert(bytes >= 2);
        nn_assert(buf[0] >= 'A' && buf[0] <= 'Z');
        nn_assert(buf[1] >= 'a' && buf[0] <= 'z');
        tid = buf[0]-'A';
        num = buf[1]-'a';
        nn_assert(tid < THREAD_COUNT);
        nn_assert(res[tid] == num);
        res[tid]=num+1;
        nn_freemsg(buf);
        count--;
    }
    nn_close(sock);
}

static void client(void *arg)
{
    intptr_t id = (intptr_t)arg;
    int bytes;
    char msg[3];
    size_t sz_msg;
    int i;

    msg[0] = 'A' + id%26;
    msg[1] = 'a';
    msg[2] = '\0';
    /*  '\0' too */
    sz_msg = strlen (msg) + 1;

    for (i = 0; i < TEST_LOOPS; i++) {
        int cli_sock = nn_socket (AF_SP, NN_PUSH);
        msg[1] = 'a' + i%26;
        nn_assert (cli_sock >= 0);
        nn_assert (nn_connect (cli_sock, SOCKET_ADDRESS) >= 0);
        /*  Give time to allow for connect to establish. */
        nn_sleep (50);
        bytes = nn_send (cli_sock, msg, sz_msg, 0);
        /*  This would better be handled via semaphore or condvar. */
        nn_sleep (100);
        nn_assert (bytes == sz_msg);
        nn_close (cli_sock);
    }
}

int main()
{
    int i;
    struct nn_thread srv_thread;
    struct nn_thread cli_threads[THREAD_COUNT];
    /*  Stress the shutdown algorithm. */
    nn_thread_init(&srv_thread, server, NULL);

    for (i = 0; i != THREAD_COUNT; ++i)
        nn_thread_init(&cli_threads[i], client, (void *)(intptr_t)i);
    for (i = 0; i != THREAD_COUNT; ++i)
        nn_thread_term(&cli_threads[i]);

    return 0;
}
