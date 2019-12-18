/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
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

#include "../src/utils/attr.h"

#include "../src/utils/err.c"
#include "../src/utils/thread.c"
#include "../src/utils/sleep.c"
#include "../src/utils/stopwatch.c"

#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t message_size;
static int roundtrip_count;

void worker (void *arg)
{
    int rc;
    int s;
    int i;
    char *buf;

    s = *(int *)arg;

    buf = malloc (message_size);
    assert (buf);

    for (i = 0; i != roundtrip_count; i++) {
        rc = nn_recv (s, buf, message_size, 0);
        assert (rc == (int)message_size);
        rc = nn_send (s, buf, message_size, 0);
        assert (rc == (int)message_size);
    }

    free (buf);
}

int main (int argc, char *argv [])
{
    int rc;
    int s;
    int w;
    int i;
    char *buf;
    struct nn_thread thread;
    struct nn_stopwatch stopwatch;
    uint64_t elapsed;
    double latency;

    if (argc != 3) {
        printf ("usage: inproc_lat <message-size> <roundtrip-count>\n");
        return 1;
    }

    message_size = atoi (argv [1]);
    roundtrip_count = atoi (argv [2]);

    s = nn_socket (AF_SP, NN_PAIR);
    assert (s != -1);
    rc = nn_bind (s, "inproc://inproc_lat");
    assert (rc >= 0);

    w = nn_socket (AF_SP, NN_PAIR);
    assert (w != -1);
    rc = nn_connect (w, "inproc://inproc_lat");
    assert (rc >= 0);

    buf = malloc (message_size);
    assert (buf);
    memset (buf, 111, message_size);

    /*  Wait a bit till the worker thread blocks in nn_recv(). */
    nn_thread_init (&thread, worker, &w);
    nn_sleep (100);

    nn_stopwatch_init (&stopwatch);

    for (i = 0; i != roundtrip_count; i++) {
        rc = nn_send (s, buf, message_size, 0);
        assert (rc == (int)message_size);
        rc = nn_recv (s, buf, message_size, 0);
        assert (rc == (int)message_size);
    }

    elapsed = nn_stopwatch_term (&stopwatch);

    latency = (double) elapsed / (roundtrip_count * 2);
    printf ("message size: %d [B]\n", (int) message_size);
    printf ("roundtrip count: %d\n", (int) roundtrip_count);
    printf ("average latency: %.3f [us]\n", (double) latency);

    nn_thread_term (&thread);
    free (buf);
    rc = nn_close (s);
    assert (rc == 0);
    rc = nn_close (w);
    assert (rc == 0);

    return 0;
}

