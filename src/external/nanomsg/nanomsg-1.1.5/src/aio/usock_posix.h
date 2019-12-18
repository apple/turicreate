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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>

struct nn_usock {

    /*  State machine base class. */
    struct nn_fsm fsm;
    int state;

    /*  The worker thread the usock is associated with. */
    struct nn_worker *worker;

    /*  The underlying OS socket and handle that represents it in the poller. */
    int s;
    struct nn_worker_fd wfd;

    /*  Members related to receiving data. */
    struct {

        /*  The buffer being filled in at the moment. */
        uint8_t *buf;
        size_t len;

        /*  Buffer for batch-reading inbound data. */
        uint8_t *batch;

        /*  Size of the batch buffer. */
        size_t batch_len;

        /*  Current position in the batch buffer. The data preceding this
            position were already received by the user. The data that follow
            will be received in the future. */
        size_t batch_pos;

        /*  File descriptor received via SCM_RIGHTS, if any. */
        int *pfd;
    } in;

    /*  Members related to sending data. */
    struct {

        /*  msghdr being sent at the moment. */
        struct msghdr hdr;

        /*  List of buffers being sent at the moment. Referenced from 'hdr'. */
        struct iovec iov [NN_USOCK_MAX_IOVCNT];
    } out;

    /*  Asynchronous tasks for the worker. */
    struct nn_worker_task task_connecting;
    struct nn_worker_task task_connected;
    struct nn_worker_task task_accept;
    struct nn_worker_task task_send;
    struct nn_worker_task task_recv;
    struct nn_worker_task task_stop;

    /*  Events raised by the usock. */
    struct nn_fsm_event event_established;
    struct nn_fsm_event event_sent;
    struct nn_fsm_event event_received;
    struct nn_fsm_event event_error;

    /*  In ACCEPTING state points to the socket being accepted.
        In BEING_ACCEPTED state points to the listener socket. */
    struct nn_usock *asock;

    /*  Errno remembered in NN_USOCK_ERROR state  */
    int errnum;
};
