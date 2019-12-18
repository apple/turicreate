/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
    Copyright 2016 Garrett D'Amore <garrett@damore.org>
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
#include "../src/survey.h"
#include "../src/tcp.h"

#include "testutil.h"
#include "../src/utils/attr.h"
#include "../src/utils/thread.c"

static char socket_address_a[128];
static char socket_address_b[128];
int dev0;
int dev1;

void device (NN_UNUSED void *arg)
{
    int rc;

    /*  Run the device. */
    rc = nn_device (dev0, dev1);
    nn_assert (rc < 0 && nn_errno () == EBADF);

    /*  Clean up. */
    test_close (dev0);
    test_close (dev1);
}

int main (int argc, const char *argv[])
{
    int end0;
    int end1;
    struct nn_thread thread1;
    int timeo;
    int maxttl;
    size_t sz;
    int rc;

    int port = get_test_port(argc, argv);

    test_addr_from(socket_address_a, "tcp", "127.0.0.1", port);
    test_addr_from(socket_address_b, "tcp", "127.0.0.1", port + 1);

    /*  Intialise the device sockets. */
    dev0 = test_socket (AF_SP_RAW, NN_RESPONDENT);
    dev1 = test_socket (AF_SP_RAW, NN_SURVEYOR);

    test_bind (dev0, socket_address_a);
    test_bind (dev1, socket_address_b);

    /*  Start the device. */
    nn_thread_init (&thread1, device, NULL);

    end0 = test_socket (AF_SP, NN_SURVEYOR);
    end1 = test_socket (AF_SP, NN_RESPONDENT);

    /*  Test the bi-directional device TTL */ 
    test_connect (end0, socket_address_a);
    test_connect (end1, socket_address_b);

    /*  Wait for TCP to establish. */
    nn_sleep (100);

    /*  Set up max receive timeout. */
    timeo = 100;
    test_setsockopt (end0, NN_SOL_SOCKET, NN_RCVTIMEO, &timeo, sizeof (timeo));
    timeo = 100;
    test_setsockopt (end1, NN_SOL_SOCKET, NN_RCVTIMEO, &timeo, sizeof (timeo));

    /*  Test default TTL is 8. */
    sz = sizeof (maxttl);
    maxttl = -1;
    rc = nn_getsockopt(end1, NN_SOL_SOCKET, NN_MAXTTL, &maxttl, &sz);
    nn_assert (rc == 0);
    nn_assert (sz == sizeof (maxttl));
    nn_assert (maxttl == 8);

    /*  Test to make sure option TTL cannot be set below 1. */
    maxttl = -1;
    rc = nn_setsockopt(end1, NN_SOL_SOCKET, NN_MAXTTL, &maxttl, sizeof (maxttl));
    nn_assert (rc < 0 && nn_errno () == EINVAL);
    nn_assert (maxttl == -1);
    maxttl = 0;
    rc = nn_setsockopt(end1, NN_SOL_SOCKET, NN_MAXTTL, &maxttl, sizeof (maxttl));
    nn_assert (rc < 0 && nn_errno () == EINVAL);
    nn_assert (maxttl == 0);

    /*  Test to set non-integer size */
    maxttl = 8;
    rc = nn_setsockopt(end1, NN_SOL_SOCKET, NN_MAXTTL, &maxttl, 1);
    nn_assert (rc < 0 && nn_errno () == EINVAL);
    nn_assert (maxttl == 8);

    /*  Pass a message between endpoints. */
    test_send (end0, "SURVEY");
    test_recv (end1, "SURVEY");

    /*  Now send a reply. */
    test_send (end1, "REPLYXYZ");
    test_recv (end0, "REPLYXYZ");

    /*  Now set the max TTL. */
    maxttl = 1;
    test_setsockopt (end0, NN_SOL_SOCKET, NN_MAXTTL, &maxttl, sizeof (maxttl));
    test_setsockopt (end1, NN_SOL_SOCKET, NN_MAXTTL, &maxttl, sizeof (maxttl));

    test_send (end0, "DROPTHIS");
    test_drop (end1, ETIMEDOUT);

    maxttl = 2;
    test_setsockopt (end0, NN_SOL_SOCKET, NN_MAXTTL, &maxttl, sizeof (maxttl));
    test_setsockopt (end1, NN_SOL_SOCKET, NN_MAXTTL, &maxttl, sizeof (maxttl));
    test_send (end0, "DONTDROP");
    test_recv (end1, "DONTDROP");

    /*  Clean up. */
    test_close (end0);
    test_close (end1);

    /*  Shut down the devices. */
    nn_term ();
    nn_thread_term (&thread1);

    return 0;
}
