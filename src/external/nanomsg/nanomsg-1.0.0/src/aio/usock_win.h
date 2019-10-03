/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.

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

#include "fsm.h"
#include "worker.h"

#include "../utils/win.h"

struct nn_usock {

    /*  The state machine. */
    struct nn_fsm fsm;
    int state;

    union {

        /*  The actual underlying socket. Can be used as a HANDLE too. */
        SOCKET s;

        /*  Named pipe handle. Cannot be used as a SOCKET. */
        HANDLE p;
    };

    /*  For NamedPipes, closing an accepted pipe differs from other pipes.
        If the NamedPipe was accepted, this member is set to 1. 0 otherwise. */
    int isaccepted;

    /*  Asynchronous operations being executed on the socket. */
    struct nn_worker_op in;
    struct nn_worker_op out;

    /*  When accepting new socket, they have to be created with same
        type as the listening socket. Thus, in listening socket we
        have to store its exact type. */
    int domain;
    int type;
    int protocol;

    /*  Events raised by the usock. */
    struct nn_fsm_event event_established;
    struct nn_fsm_event event_sent;
    struct nn_fsm_event event_received;
    struct nn_fsm_event event_error;

    /*  In ACCEPTING state points to the socket being accepted.
        In BEING_ACCEPTED state points to the listener socket. */
    struct nn_usock *asock;

    /*  Buffer allocated for output of AcceptEx function. If accepting is not
        done on this socket, the field is set to NULL. */
    void *ainfo;

    /*  For NamedPipes, we store the address inside the socket. */
    struct sockaddr_un pipename;

    /*  For now we allocate a new buffer for each write to a named pipe. */
    void *pipesendbuf;

    /* Pointer to the security attribute structure */
    SECURITY_ATTRIBUTES *sec_attr;

    /* Out Buffer and In Buffer size */
    int outbuffersz;
    int inbuffersz;

    /*  Errno remembered in NN_USOCK_ERROR state  */
    int errnum;
};
