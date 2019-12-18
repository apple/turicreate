/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.

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
#include "../src/pipeline.h"
#include "testutil.h"

#define SOCKET_ADDRESS "inproc://a"

int main ()
{
    int push1;
    int push2;
    int pull1;
    int pull2;

    /*  Test fan-out. */

    push1 = test_socket (AF_SP, NN_PUSH);
    test_bind (push1, SOCKET_ADDRESS);
    pull1 = test_socket (AF_SP, NN_PULL);
    test_connect (pull1, SOCKET_ADDRESS);
    pull2 = test_socket (AF_SP, NN_PULL);
    test_connect (pull2, SOCKET_ADDRESS);

    /*  Wait till both connections are established to get messages spread
        evenly between the two pull sockets. */
    nn_sleep (10);

    test_send (push1, "ABC");
    test_send (push1, "DEF");

    test_recv (pull1, "ABC");
    test_recv (pull2, "DEF");

    test_close (push1);
    test_close (pull1);
    test_close (pull2);

    /*  Test fan-in. */

    pull1 = test_socket (AF_SP, NN_PULL);
    test_bind (pull1, SOCKET_ADDRESS);
    push1 = test_socket (AF_SP, NN_PUSH);
    test_connect (push1, SOCKET_ADDRESS);
    push2 = test_socket (AF_SP, NN_PUSH);
    test_connect (push2, SOCKET_ADDRESS);

    test_send (push1, "ABC");
    test_send (push2, "DEF");

    test_recv (pull1, "ABC");
    test_recv (pull1, "DEF");

    test_close (pull1);
    test_close (push1);
    test_close (push2);

    return 0;
}

