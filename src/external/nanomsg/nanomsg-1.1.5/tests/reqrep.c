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
#include "../src/reqrep.h"

#include "testutil.h"

#define SOCKET_ADDRESS "inproc://test"

int main ()
{
    int rc;
    int rep1;
    int rep2;
    int req1;
    int req2;
    int resend_ivl;
    char buf [7];
    int timeo;

    /*  Test req/rep with full socket types. */
    rep1 = test_socket (AF_SP, NN_REP);
    test_bind (rep1, SOCKET_ADDRESS);
    req1 = test_socket (AF_SP, NN_REQ);
    test_connect (req1, SOCKET_ADDRESS);
    req2 = test_socket (AF_SP, NN_REQ);
    test_connect (req2, SOCKET_ADDRESS);

    /*  Check invalid sequence of sends and recvs. */
    rc = nn_send (rep1, "ABC", 3, 0);
    nn_assert (rc == -1 && nn_errno () == EFSM);
    rc = nn_recv (req1, buf, sizeof (buf), 0);
    nn_assert (rc == -1 && nn_errno () == EFSM);

    /*  Check fair queueing the requests. */
    test_send (req2, "ABC");
    test_recv (rep1, "ABC");
    test_send (rep1, "ABC");
    test_recv (req2, "ABC");

    test_send (req1, "ABC");
    test_recv (rep1, "ABC");
    test_send (rep1, "ABC");
    test_recv (req1, "ABC");

    test_close (rep1);
    test_close (req1);
    test_close (req2);

    /*  Check load-balancing of requests. */
    req1 = test_socket (AF_SP, NN_REQ);
    test_bind (req1, SOCKET_ADDRESS);
    rep1 = test_socket (AF_SP, NN_REP);
    test_connect (rep1, SOCKET_ADDRESS);
    rep2 = test_socket (AF_SP, NN_REP);
    test_connect (rep2, SOCKET_ADDRESS);

    test_send (req1, "ABC");
    test_recv (rep1, "ABC");
    test_send (rep1, "ABC");
    test_recv (req1, "ABC");

    test_send (req1, "ABC");
    test_recv (rep2, "ABC");
    test_send (rep2, "ABC");
    test_recv (req1, "ABC");

    test_close (rep2);
    test_close (rep1);
    test_close (req1);

    /*  Test re-sending of the request. */
    rep1 = test_socket (AF_SP, NN_REP);
    test_bind (rep1, SOCKET_ADDRESS);
    req1 = test_socket (AF_SP, NN_REQ);
    test_connect (req1, SOCKET_ADDRESS);
    resend_ivl = 100;
    rc = nn_setsockopt (req1, NN_REQ, NN_REQ_RESEND_IVL,
        &resend_ivl, sizeof (resend_ivl));
    errno_assert (rc == 0);

    test_send (req1, "ABC");
    test_recv (rep1, "ABC");
    /*  The following waits for request to be resent  */
    test_recv (rep1, "ABC");

    test_close (req1);
    test_close (rep1);

    /*  Check sending a request when the peer is not available. (It should
        be sent immediatelly when the peer comes online rather than relying
        on the resend algorithm. */
    req1 = test_socket (AF_SP, NN_REQ);
    test_connect (req1, SOCKET_ADDRESS);
    test_send (req1, "ABC");

    rep1 = test_socket (AF_SP, NN_REP);
    test_bind (rep1, SOCKET_ADDRESS);
    timeo = 200;
    rc = nn_setsockopt (rep1, NN_SOL_SOCKET, NN_RCVTIMEO,
       &timeo, sizeof (timeo));
    errno_assert (rc == 0);
    test_recv (rep1, "ABC");

    test_close (req1);
    test_close (rep1);

    /*  Check removing socket request sent to (It should
        be sent immediatelly to other peer rather than relying
        on the resend algorithm). */
    req1 = test_socket (AF_SP, NN_REQ);
    test_bind (req1, SOCKET_ADDRESS);
    rep1 = test_socket (AF_SP, NN_REP);
    test_connect (rep1, SOCKET_ADDRESS);
    rep2 = test_socket (AF_SP, NN_REP);
    test_connect (rep2, SOCKET_ADDRESS);

    timeo = 200;
    rc = nn_setsockopt (rep1, NN_SOL_SOCKET, NN_RCVTIMEO,
       &timeo, sizeof (timeo));
    errno_assert (rc == 0);
    rc = nn_setsockopt (rep2, NN_SOL_SOCKET, NN_RCVTIMEO,
       &timeo, sizeof (timeo));
    errno_assert (rc == 0);

    test_send (req1, "ABC");
    /*  We got request through rep1  */
    test_recv (rep1, "ABC");
    /*  But instead replying we simulate crash  */
    test_close (rep1);
    /*  The rep2 should get request immediately  */
    test_recv (rep2, "ABC");
    /*  Let's check it's delivered well  */
    test_send (rep2, "REPLY");
    test_recv (req1, "REPLY");


    test_close (req1);
    test_close (rep2);

    /*  Test cancelling delayed request  */

    req1 = test_socket (AF_SP, NN_REQ);
    test_connect (req1, SOCKET_ADDRESS);
    test_send (req1, "ABC");
    test_send (req1, "DEF");

    rep1 = test_socket (AF_SP, NN_REP);
    test_bind (rep1, SOCKET_ADDRESS);
    timeo = 100;
    test_recv (rep1, "DEF");

    test_close (req1);
    test_close (rep1);

    return 0;
}

