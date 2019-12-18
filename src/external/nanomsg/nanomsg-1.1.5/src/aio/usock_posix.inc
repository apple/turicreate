/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
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

#include "../utils/alloc.h"
#include "../utils/closefd.h"
#include "../utils/cont.h"
#include "../utils/fast.h"
#include "../utils/err.h"
#include "../utils/attr.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>

#define NN_USOCK_STATE_IDLE 1
#define NN_USOCK_STATE_STARTING 2
#define NN_USOCK_STATE_BEING_ACCEPTED 3
#define NN_USOCK_STATE_ACCEPTED 4
#define NN_USOCK_STATE_CONNECTING 5
#define NN_USOCK_STATE_ACTIVE 6
#define NN_USOCK_STATE_REMOVING_FD 7
#define NN_USOCK_STATE_DONE 8
#define NN_USOCK_STATE_LISTENING 9
#define NN_USOCK_STATE_ACCEPTING 10
#define NN_USOCK_STATE_CANCELLING 11
#define NN_USOCK_STATE_STOPPING 12
#define NN_USOCK_STATE_STOPPING_ACCEPT 13
#define NN_USOCK_STATE_ACCEPTING_ERROR 14

#define NN_USOCK_ACTION_ACCEPT 1
#define NN_USOCK_ACTION_BEING_ACCEPTED 2
#define NN_USOCK_ACTION_CANCEL 3
#define NN_USOCK_ACTION_LISTEN 4
#define NN_USOCK_ACTION_CONNECT 5
#define NN_USOCK_ACTION_ACTIVATE 6
#define NN_USOCK_ACTION_DONE 7
#define NN_USOCK_ACTION_ERROR 8
#define NN_USOCK_ACTION_STARTED 9

#define NN_USOCK_SRC_FD 1
#define NN_USOCK_SRC_TASK_CONNECTING 2
#define NN_USOCK_SRC_TASK_CONNECTED 3
#define NN_USOCK_SRC_TASK_ACCEPT 4
#define NN_USOCK_SRC_TASK_SEND 5
#define NN_USOCK_SRC_TASK_RECV 6
#define NN_USOCK_SRC_TASK_STOP 7

/*  Private functions. */
static void nn_usock_init_from_fd (struct nn_usock *self, int s);
static int nn_usock_send_raw (struct nn_usock *self, struct msghdr *hdr);
static int nn_usock_recv_raw (struct nn_usock *self, void *buf, size_t *len);
static int nn_usock_geterr (struct nn_usock *self);
static void nn_usock_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_usock_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);

void nn_usock_init (struct nn_usock *self, int src, struct nn_fsm *owner)
{
    /*  Initalise the state machine. */
    nn_fsm_init (&self->fsm, nn_usock_handler, nn_usock_shutdown,
        src, self, owner);
    self->state = NN_USOCK_STATE_IDLE;

    /*  Choose a worker thread to handle this socket. */
    self->worker = nn_fsm_choose_worker (&self->fsm);

    /*  Actual file descriptor will be generated during 'start' step. */
    self->s = -1;
    self->errnum = 0;

    self->in.buf = NULL;
    self->in.len = 0;
    self->in.batch = NULL;
    self->in.batch_len = 0;
    self->in.batch_pos = 0;
    self->in.pfd = NULL;

    memset (&self->out.hdr, 0, sizeof (struct msghdr));

    /*  Initialise tasks for the worker thread. */
    nn_worker_fd_init (&self->wfd, NN_USOCK_SRC_FD, &self->fsm);
    nn_worker_task_init (&self->task_connecting, NN_USOCK_SRC_TASK_CONNECTING,
        &self->fsm);
    nn_worker_task_init (&self->task_connected, NN_USOCK_SRC_TASK_CONNECTED,
        &self->fsm);
    nn_worker_task_init (&self->task_accept, NN_USOCK_SRC_TASK_ACCEPT,
        &self->fsm);
    nn_worker_task_init (&self->task_send, NN_USOCK_SRC_TASK_SEND, &self->fsm);
    nn_worker_task_init (&self->task_recv, NN_USOCK_SRC_TASK_RECV, &self->fsm);
    nn_worker_task_init (&self->task_stop, NN_USOCK_SRC_TASK_STOP, &self->fsm);

    /*  Intialise events raised by usock. */
    nn_fsm_event_init (&self->event_established);
    nn_fsm_event_init (&self->event_sent);
    nn_fsm_event_init (&self->event_received);
    nn_fsm_event_init (&self->event_error);

    /*  accepting is not going on at the moment. */
    self->asock = NULL;
}

void nn_usock_term (struct nn_usock *self)
{
    nn_assert_state (self, NN_USOCK_STATE_IDLE);

    if (self->in.batch)
        nn_free (self->in.batch);

    nn_fsm_event_term (&self->event_error);
    nn_fsm_event_term (&self->event_received);
    nn_fsm_event_term (&self->event_sent);
    nn_fsm_event_term (&self->event_established);

    nn_worker_cancel (self->worker, &self->task_stop);
    nn_worker_cancel (self->worker, &self->task_recv);
    nn_worker_cancel (self->worker, &self->task_send);
    nn_worker_cancel (self->worker, &self->task_accept);
    nn_worker_cancel (self->worker, &self->task_connected);
    nn_worker_cancel (self->worker, &self->task_connecting);

    nn_worker_task_term (&self->task_stop);
    nn_worker_task_term (&self->task_recv);
    nn_worker_task_term (&self->task_send);
    nn_worker_task_term (&self->task_accept);
    nn_worker_task_term (&self->task_connected);
    nn_worker_task_term (&self->task_connecting);
    nn_worker_fd_term (&self->wfd);

    nn_fsm_term (&self->fsm);
}

int nn_usock_isidle (struct nn_usock *self)
{
    return nn_fsm_isidle (&self->fsm);
}

int nn_usock_start (struct nn_usock *self, int domain, int type, int protocol)
{
    int s;

    /*  If the operating system allows to directly open the socket with CLOEXEC
        flag, do so. That way there are no race conditions. */
#ifdef SOCK_CLOEXEC
    type |= SOCK_CLOEXEC;
#endif

    /* Open the underlying socket. */
    s = socket (domain, type, protocol);
    if (nn_slow (s < 0))
       return -errno;

    nn_usock_init_from_fd (self, s);

    /*  Start the state machine. */
    nn_fsm_start (&self->fsm);

    return 0;
}

void nn_usock_start_fd (struct nn_usock *self, int fd)
{
    nn_usock_init_from_fd (self, fd);
    nn_fsm_start (&self->fsm);
    nn_fsm_action (&self->fsm, NN_USOCK_ACTION_STARTED);
}

static void nn_usock_init_from_fd (struct nn_usock *self, int s)
{
    int rc;
    int opt;

    nn_assert (self->state == NN_USOCK_STATE_IDLE ||
        self->state == NN_USOCK_STATE_BEING_ACCEPTED);

    /*  Store the file descriptor. */
    nn_assert (self->s == -1);
    self->s = s;

    /* Setting FD_CLOEXEC option immediately after socket creation is the
        second best option after using SOCK_CLOEXEC. There is a race condition
        here (if process is forked between socket creation and setting
        the option) but the problem is pretty unlikely to happen. */
#if defined FD_CLOEXEC
    rc = fcntl (self->s, F_SETFD, FD_CLOEXEC);
#if defined NN_HAVE_OSX
    errno_assert (rc != -1 || errno == EINVAL);
#else
    errno_assert (rc != -1);
#endif
#endif

    /* If applicable, prevent SIGPIPE signal when writing to the connection
        already closed by the peer. */
#ifdef SO_NOSIGPIPE
    opt = 1;
    rc = setsockopt (self->s, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof (opt));
#if defined NN_HAVE_OSX
    errno_assert (rc == 0 || errno == EINVAL);
#else
    errno_assert (rc == 0);
#endif
#endif

    /* Switch the socket to the non-blocking mode. All underlying sockets
        are always used in the callbackhronous mode. */
    opt = fcntl (self->s, F_GETFL, 0);
    if (opt == -1)
        opt = 0;
    if (!(opt & O_NONBLOCK)) {
        rc = fcntl (self->s, F_SETFL, opt | O_NONBLOCK);
#if defined NN_HAVE_OSX
        errno_assert (rc != -1 || errno == EINVAL);
#else
        errno_assert (rc != -1);
#endif
    }
}

void nn_usock_stop (struct nn_usock *self)
{
    nn_fsm_stop (&self->fsm);
}

void nn_usock_async_stop (struct nn_usock *self)
{
    nn_worker_execute (self->worker, &self->task_stop);
    nn_fsm_raise (&self->fsm, &self->event_error, NN_USOCK_SHUTDOWN);
}

void nn_usock_swap_owner (struct nn_usock *self, struct nn_fsm_owner *owner)
{
    nn_fsm_swap_owner (&self->fsm, owner);
}

int nn_usock_setsockopt (struct nn_usock *self, int level, int optname,
    const void *optval, size_t optlen)
{
    int rc;

    /*  The socket can be modified only before it's active. */
    nn_assert (self->state == NN_USOCK_STATE_STARTING ||
        self->state == NN_USOCK_STATE_ACCEPTED);

    /*  EINVAL errors are ignored on OSX platform. The reason for that is buggy
        OSX behaviour where setsockopt returns EINVAL if the peer have already
        disconnected. Thus, nn_usock_setsockopt() can succeed on OSX even though
        the option value was invalid, but the peer have already closed the
        connection. This behaviour should be relatively harmless. */
    rc = setsockopt (self->s, level, optname, optval, (socklen_t) optlen);
#if defined NN_HAVE_OSX
    if (nn_slow (rc != 0 && errno != EINVAL))
        return -errno;
#else
    if (nn_slow (rc != 0))
        return -errno;
#endif

    return 0;
}

int nn_usock_bind (struct nn_usock *self, const struct sockaddr *addr,
    size_t addrlen)
{
    int rc;
    int opt;

    /*  The socket can be bound only before it's connected. */
    nn_assert_state (self, NN_USOCK_STATE_STARTING);

    /*  Windows Subsystem for Linux - SO_REUSEADDR is different,
        and the Windows semantics are very wrong for us. */
#ifndef NN_HAVE_WSL
    /*  Allow re-using the address. */
    opt = 1;
    rc = setsockopt (self->s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));
    errno_assert (rc == 0);
#endif

    rc = bind (self->s, addr, (socklen_t) addrlen);
    if (nn_slow (rc != 0))
        return -errno;

    return 0;
}

int nn_usock_listen (struct nn_usock *self, int backlog)
{
    int rc;

    /*  You can start listening only before the socket is connected. */
    nn_assert_state (self, NN_USOCK_STATE_STARTING);

    /*  Start listening for incoming connections. */
    rc = listen (self->s, backlog);
    if (nn_slow (rc != 0))
        return -errno;

    /*  Notify the state machine. */
    nn_fsm_action (&self->fsm, NN_USOCK_ACTION_LISTEN);

    return 0;
}

void nn_usock_accept (struct nn_usock *self, struct nn_usock *listener)
{
    int s;

    /*  Start the actual accepting. */
    if (nn_fsm_isidle(&self->fsm)) {
        nn_fsm_start (&self->fsm);
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_BEING_ACCEPTED);
    }
    nn_fsm_action (&listener->fsm, NN_USOCK_ACTION_ACCEPT);

    /*  Try to accept new connection in synchronous manner. */
#if NN_HAVE_ACCEPT4
    s = accept4 (listener->s, NULL, NULL, SOCK_CLOEXEC);
    if ((s < 0) && (errno == ENOTSUP)) {
        /*  Apparently some old versions of Linux have a stub for this in libc,
            without any of the underlying kernel support. */
        s = accept (listener->s, NULL, NULL);
    }
#else
    s = accept (listener->s, NULL, NULL);
#endif

    /*  Immediate success. */
    if (nn_fast (s >= 0)) {
        /*  Disassociate the listener socket from the accepted
            socket. Is useful if we restart accepting on ACCEPT_ERROR  */
        listener->asock = NULL;
        self->asock = NULL;

        nn_usock_init_from_fd (self, s);
        nn_fsm_action (&listener->fsm, NN_USOCK_ACTION_DONE);
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_DONE);
        return;
    }

    /*  Detect a failure. Note that in ECONNABORTED case we simply ignore
        the error and wait for next connection in asynchronous manner. */
    errno_assert (errno == EAGAIN || errno == EWOULDBLOCK ||
        errno == ECONNABORTED || errno == ENFILE || errno == EMFILE ||
        errno == ENOBUFS || errno == ENOMEM);

    /*  Pair the two sockets.  They are already paired in case
        previous attempt failed on ACCEPT_ERROR  */
    nn_assert (!self->asock || self->asock == listener);
    self->asock = listener;
    nn_assert (!listener->asock || listener->asock == self);
    listener->asock = self;

    /*  Some errors are just ok to ignore for now.  We also stop repeating
        any errors until next IN_FD event so that we are not in a tight loop
        and allow processing other events in the meantime  */
    if (nn_slow (errno != EAGAIN && errno != EWOULDBLOCK
        && errno != ECONNABORTED && errno != listener->errnum))
    {
        listener->errnum = errno;
        listener->state = NN_USOCK_STATE_ACCEPTING_ERROR;
        nn_fsm_raise (&listener->fsm,
            &listener->event_error, NN_USOCK_ACCEPT_ERROR);
        return;
    }

    /*  Ask the worker thread to wait for the new connection. */
    nn_worker_execute (listener->worker, &listener->task_accept);
}

void nn_usock_activate (struct nn_usock *self)
{
    nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ACTIVATE);
}

void nn_usock_connect (struct nn_usock *self, const struct sockaddr *addr,
    size_t addrlen)
{
    int rc;

    /*  Notify the state machine that we've started connecting. */
    nn_fsm_action (&self->fsm, NN_USOCK_ACTION_CONNECT);

    /* Do the connect itself. */
    rc = connect (self->s, addr, (socklen_t) addrlen);

    /* Immediate success. */
    if (nn_fast (rc == 0)) {
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_DONE);
        return;
    }

    /*  Immediate error. */
    if (nn_slow (errno != EINPROGRESS)) {
        self->errnum = errno;
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ERROR);
        return;
    }

    /*  Start asynchronous connect. */
    nn_worker_execute (self->worker, &self->task_connecting);
}

void nn_usock_send (struct nn_usock *self, const struct nn_iovec *iov,
    int iovcnt)
{
    int rc;
    int i;
    int out;

    /*  Make sure that the socket is actually alive. */
    if (self->state != NN_USOCK_STATE_ACTIVE) {
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ERROR);
        return;
    }

    /*  Copy the iovecs to the socket. */
    nn_assert (iovcnt <= NN_USOCK_MAX_IOVCNT);
    self->out.hdr.msg_iov = self->out.iov;
    out = 0;
    for (i = 0; i != iovcnt; ++i) {
        if (iov [i].iov_len == 0)
            continue;
        self->out.iov [out].iov_base = iov [i].iov_base;
        self->out.iov [out].iov_len = iov [i].iov_len;
        out++;
    }
    self->out.hdr.msg_iovlen = out;

    /*  Try to send the data immediately. */
    rc = nn_usock_send_raw (self, &self->out.hdr);

    /*  Success. */
    if (nn_fast (rc == 0)) {
        nn_fsm_raise (&self->fsm, &self->event_sent, NN_USOCK_SENT);
        return;
    }

    /*  Errors. */
    if (nn_slow (rc != -EAGAIN)) {
        errnum_assert (rc == -ECONNRESET, -rc);
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ERROR);
        return;
    }

    /*  Ask the worker thread to send the remaining data. */
    nn_worker_execute (self->worker, &self->task_send);
}

void nn_usock_recv (struct nn_usock *self, void *buf, size_t len, int *fd)
{
    int rc;
    size_t nbytes;

    /*  Make sure that the socket is actually alive. */
    if (self->state != NN_USOCK_STATE_ACTIVE) {
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ERROR);
        return;
    }

    /*  Try to receive the data immediately. */
    nbytes = len;
    self->in.pfd = fd;
    rc = nn_usock_recv_raw (self, buf, &nbytes);
    if (nn_slow (rc < 0)) {
        errnum_assert (rc == -ECONNRESET, -rc);
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ERROR);
        return;
    }

    /*  Success. */
    if (nn_fast (nbytes == len)) {
        nn_fsm_raise (&self->fsm, &self->event_received, NN_USOCK_RECEIVED);
        return;
    }

    /*  There are still data to receive in the background. */
    self->in.buf = ((uint8_t*) buf) + nbytes;
    self->in.len = len - nbytes;

    /*  Ask the worker thread to receive the remaining data. */
    nn_worker_execute (self->worker, &self->task_recv);
}

static int nn_internal_tasks (struct nn_usock *usock, int src, int type)
{

/******************************************************************************/
/*  Internal tasks sent from the user thread to the worker thread.            */
/******************************************************************************/
    switch (src) {
    case NN_USOCK_SRC_TASK_SEND:
        nn_assert (type == NN_WORKER_TASK_EXECUTE);
        nn_worker_set_out (usock->worker, &usock->wfd);
        return 1;
    case NN_USOCK_SRC_TASK_RECV:
        nn_assert (type == NN_WORKER_TASK_EXECUTE);
        nn_worker_set_in (usock->worker, &usock->wfd);
        return 1;
    case NN_USOCK_SRC_TASK_CONNECTED:
        nn_assert (type == NN_WORKER_TASK_EXECUTE);
        nn_worker_add_fd (usock->worker, usock->s, &usock->wfd);
        return 1;
    case NN_USOCK_SRC_TASK_CONNECTING:
        nn_assert (type == NN_WORKER_TASK_EXECUTE);
        nn_worker_add_fd (usock->worker, usock->s, &usock->wfd);
        nn_worker_set_out (usock->worker, &usock->wfd);
        return 1;
    case NN_USOCK_SRC_TASK_ACCEPT:
        nn_assert (type == NN_WORKER_TASK_EXECUTE);
        nn_worker_add_fd (usock->worker, usock->s, &usock->wfd);
        nn_worker_set_in (usock->worker, &usock->wfd);
        return 1;
    }

    return 0;
}

static void nn_usock_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_usock *usock;

    usock = nn_cont (self, struct nn_usock, fsm);

    if (nn_internal_tasks (usock, src, type))
        return;

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {

        /*  Socket in ACCEPTING or CANCELLING state cannot be closed.
            Stop the socket being accepted first. */
        nn_assert (usock->state != NN_USOCK_STATE_ACCEPTING &&
            usock->state != NN_USOCK_STATE_CANCELLING);

        usock->errnum = 0;

        /*  Synchronous stop. */
        if (usock->state == NN_USOCK_STATE_IDLE)
            goto finish3;
        if (usock->state == NN_USOCK_STATE_DONE)
            goto finish2;
        if (usock->state == NN_USOCK_STATE_STARTING ||
              usock->state == NN_USOCK_STATE_ACCEPTED ||
              usock->state == NN_USOCK_STATE_ACCEPTING_ERROR ||
              usock->state == NN_USOCK_STATE_LISTENING)
            goto finish1;

        /*  When socket that's being accepted is asked to stop, we have to
            ask the listener socket to stop accepting first. */
        if (usock->state == NN_USOCK_STATE_BEING_ACCEPTED) {
            nn_fsm_action (&usock->asock->fsm, NN_USOCK_ACTION_CANCEL);
            usock->state = NN_USOCK_STATE_STOPPING_ACCEPT;
            return;
        }

        /*  Asynchronous stop. */
        if (usock->state != NN_USOCK_STATE_REMOVING_FD)
            nn_usock_async_stop (usock);
        usock->state = NN_USOCK_STATE_STOPPING;
        return;
    }
    if (nn_slow (usock->state == NN_USOCK_STATE_STOPPING_ACCEPT)) {
        nn_assert (src == NN_FSM_ACTION && type == NN_USOCK_ACTION_DONE);
        goto finish2;
    }
    if (nn_slow (usock->state == NN_USOCK_STATE_STOPPING)) {
        if (src != NN_USOCK_SRC_TASK_STOP)
            return;
        nn_assert (type == NN_WORKER_TASK_EXECUTE);
        nn_worker_rm_fd (usock->worker, &usock->wfd);
finish1:
        nn_closefd (usock->s);
        usock->s = -1;
finish2:
        usock->state = NN_USOCK_STATE_IDLE;
        nn_fsm_stopped (&usock->fsm, NN_USOCK_STOPPED);
finish3:
        return;
    }

    nn_fsm_bad_state(usock->state, src, type);
}

static void nn_usock_handler (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    int rc;
    struct nn_usock *usock;
    int s;
    size_t sz;
    int sockerr;

    usock = nn_cont (self, struct nn_usock, fsm);

    if(nn_internal_tasks(usock, src, type))
        return;

    switch (usock->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/*  nn_usock object is initialised, but underlying OS socket is not yet       */
/*  created.                                                                  */
/******************************************************************************/
    case NN_USOCK_STATE_IDLE:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_FSM_START:
                usock->state = NN_USOCK_STATE_STARTING;
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/******************************************************************************/
/*  STARTING state.                                                           */
/*  Underlying OS socket is created, but it's not yet passed to the worker    */
/*  thread. In this state we can set socket options, local and remote         */
/*  address etc.                                                              */
/******************************************************************************/
    case NN_USOCK_STATE_STARTING:

        /*  Events from the owner of the usock. */
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_LISTEN:
                usock->state = NN_USOCK_STATE_LISTENING;
                return;
            case NN_USOCK_ACTION_CONNECT:
                usock->state = NN_USOCK_STATE_CONNECTING;
                return;
            case NN_USOCK_ACTION_BEING_ACCEPTED:
                usock->state = NN_USOCK_STATE_BEING_ACCEPTED;
                return;
            case NN_USOCK_ACTION_STARTED:
                nn_worker_add_fd (usock->worker, usock->s, &usock->wfd);
                usock->state = NN_USOCK_STATE_ACTIVE;
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/******************************************************************************/
/*  BEING_ACCEPTED state.                                                     */
/*  accept() was called on the usock. Now the socket is waiting for a new     */
/*  connection to arrive.                                                     */
/******************************************************************************/
    case NN_USOCK_STATE_BEING_ACCEPTED:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_DONE:
                usock->state = NN_USOCK_STATE_ACCEPTED;
                nn_fsm_raise (&usock->fsm, &usock->event_established,
                    NN_USOCK_ACCEPTED);
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/******************************************************************************/
/*  ACCEPTED state.                                                           */
/*  Connection was accepted, now it can be tuned. Afterwards, it'll move to   */
/*  the active state.                                                         */
/******************************************************************************/
    case NN_USOCK_STATE_ACCEPTED:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_ACTIVATE:
                nn_worker_add_fd (usock->worker, usock->s, &usock->wfd);
                usock->state = NN_USOCK_STATE_ACTIVE;
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/******************************************************************************/
/*  CONNECTING state.                                                         */
/*  Asynchronous connecting is going on.                                      */
/******************************************************************************/
    case NN_USOCK_STATE_CONNECTING:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_DONE:
                usock->state = NN_USOCK_STATE_ACTIVE;
                nn_worker_execute (usock->worker, &usock->task_connected);
                nn_fsm_raise (&usock->fsm, &usock->event_established,
                    NN_USOCK_CONNECTED);
                return;
            case NN_USOCK_ACTION_ERROR:
                nn_closefd (usock->s);
                usock->s = -1;
                usock->state = NN_USOCK_STATE_DONE;
                nn_fsm_raise (&usock->fsm, &usock->event_error,
                    NN_USOCK_ERROR);
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        case NN_USOCK_SRC_FD:
            switch (type) {
            case NN_WORKER_FD_OUT:
                nn_worker_reset_out (usock->worker, &usock->wfd);
                usock->state = NN_USOCK_STATE_ACTIVE;
                sockerr = nn_usock_geterr(usock);
                if (sockerr == 0) {
                    nn_fsm_raise (&usock->fsm, &usock->event_established,
                        NN_USOCK_CONNECTED);
                } else {
                    usock->errnum = sockerr;
                    nn_worker_rm_fd (usock->worker, &usock->wfd);
                    rc = close (usock->s);
                    errno_assert (rc == 0);
                    usock->s = -1;
                    usock->state = NN_USOCK_STATE_DONE;
                    nn_fsm_raise (&usock->fsm,
                        &usock->event_error, NN_USOCK_ERROR);
                }
                return;
            case NN_WORKER_FD_ERR:
                nn_worker_rm_fd (usock->worker, &usock->wfd);
                nn_closefd (usock->s);
                usock->s = -1;
                usock->state = NN_USOCK_STATE_DONE;
                nn_fsm_raise (&usock->fsm, &usock->event_error, NN_USOCK_ERROR);
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/******************************************************************************/
/*  ACTIVE state.                                                             */
/*  Socket is connected. It can be used for sending and receiving data.       */
/******************************************************************************/
    case NN_USOCK_STATE_ACTIVE:
        switch (src) {
        case NN_USOCK_SRC_FD:
            switch (type) {
            case NN_WORKER_FD_IN:
                sz = usock->in.len;
                rc = nn_usock_recv_raw (usock, usock->in.buf, &sz);
                if (nn_fast (rc == 0)) {
                    usock->in.len -= sz;
                    usock->in.buf += sz;
                    if (!usock->in.len) {
                        nn_worker_reset_in (usock->worker, &usock->wfd);
                        nn_fsm_raise (&usock->fsm, &usock->event_received,
                            NN_USOCK_RECEIVED);
                    }
                    return;
                }
                errnum_assert (rc == -ECONNRESET, -rc);
                goto error;
            case NN_WORKER_FD_OUT:
                rc = nn_usock_send_raw (usock, &usock->out.hdr);
                if (nn_fast (rc == 0)) {
                    nn_worker_reset_out (usock->worker, &usock->wfd);
                    nn_fsm_raise (&usock->fsm, &usock->event_sent,
                        NN_USOCK_SENT);
                    return;
                }
                if (nn_fast (rc == -EAGAIN))
                    return;
                errnum_assert (rc == -ECONNRESET, -rc);
                goto error;
            case NN_WORKER_FD_ERR:
error:
                nn_worker_rm_fd (usock->worker, &usock->wfd);
                nn_closefd (usock->s);
                usock->s = -1;
                usock->state = NN_USOCK_STATE_DONE;
                nn_fsm_raise (&usock->fsm, &usock->event_error, NN_USOCK_ERROR);
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_ERROR:
                usock->state = NN_USOCK_STATE_REMOVING_FD;
                nn_usock_async_stop (usock);
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source(usock->state, src, type);
        }

/******************************************************************************/
/*  REMOVING_FD state.                                                        */
/******************************************************************************/
    case NN_USOCK_STATE_REMOVING_FD:
        switch (src) {
        case NN_USOCK_SRC_TASK_STOP:
            switch (type) {
            case NN_WORKER_TASK_EXECUTE:
                nn_worker_rm_fd (usock->worker, &usock->wfd);
                nn_closefd (usock->s);
                usock->s = -1;
                usock->state = NN_USOCK_STATE_DONE;
                nn_fsm_raise (&usock->fsm, &usock->event_error, NN_USOCK_ERROR);
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }

        /*  Events from the file descriptor are ignored while it is being
            removed. */
        case NN_USOCK_SRC_FD:
            return;

        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_ERROR:
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/******************************************************************************/
/*  DONE state.                                                               */
/*  Socket is closed. The only thing that can be done in this state is        */
/*  stopping the usock.                                                       */
/******************************************************************************/
    case NN_USOCK_STATE_DONE:
        return;

/******************************************************************************/
/*  LISTENING state.                                                          */
/*  Socket is listening for new incoming connections, however, user is not    */
/*  accepting a new connection.                                               */
/******************************************************************************/
    case NN_USOCK_STATE_LISTENING:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_ACCEPT:
                usock->state = NN_USOCK_STATE_ACCEPTING;
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/******************************************************************************/
/*  ACCEPTING state.                                                          */
/*  User is waiting asynchronouslyfor a new inbound connection                */
/*  to be accepted.                                                           */
/******************************************************************************/
    case NN_USOCK_STATE_ACCEPTING:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_DONE:
                usock->state = NN_USOCK_STATE_LISTENING;
                return;
            case NN_USOCK_ACTION_CANCEL:
                usock->state = NN_USOCK_STATE_CANCELLING;
                nn_worker_execute (usock->worker, &usock->task_stop);
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        case NN_USOCK_SRC_FD:
            switch (type) {
            case NN_WORKER_FD_IN:

                /*  New connection arrived in asynchronous manner. */
#if NN_HAVE_ACCEPT4
                s = accept4 (usock->s, NULL, NULL, SOCK_CLOEXEC);
#else
                s = accept (usock->s, NULL, NULL);
#endif

                /*  ECONNABORTED is an valid error. New connection was closed
                    by the peer before we were able to accept it. If it happens
                    do nothing and wait for next incoming connection. */
                if (nn_slow (s < 0 && errno == ECONNABORTED))
                    return;

                /*  Resource allocation errors. It's not clear from POSIX
                    specification whether the new connection is closed in this
                    case or whether it remains in the backlog. In the latter
                    case it would be wise to wait here for a while to prevent
                    busy looping. */
                if (nn_slow (s < 0 && (errno == ENFILE || errno == EMFILE ||
                      errno == ENOBUFS || errno == ENOMEM))) {
                    usock->errnum = errno;
                    usock->state = NN_USOCK_STATE_ACCEPTING_ERROR;

                    /*  Wait till the user starts accepting once again. */
                    nn_worker_rm_fd (usock->worker, &usock->wfd);

                    nn_fsm_raise (&usock->fsm,
                        &usock->event_error, NN_USOCK_ACCEPT_ERROR);
                    return;
                }

                /* Any other error is unexpected. */
                errno_assert (s >= 0);

                /*  Initialise the new usock object. */
                nn_usock_init_from_fd (usock->asock, s);
                usock->asock->state = NN_USOCK_STATE_ACCEPTED;

                /*  Notify the user that connection was accepted. */
                nn_fsm_raise (&usock->asock->fsm,
                    &usock->asock->event_established, NN_USOCK_ACCEPTED);

                /*  Disassociate the listener socket from the accepted
                    socket. */
                usock->asock->asock = NULL;
                usock->asock = NULL;

                /*  Wait till the user starts accepting once again. */
                nn_worker_rm_fd (usock->worker, &usock->wfd);
                usock->state = NN_USOCK_STATE_LISTENING;

                return;

            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/******************************************************************************/
/*  ACCEPTING_ERROR state.                                                    */
/*  Waiting the socket to accept the error and restart                        */
/******************************************************************************/
    case NN_USOCK_STATE_ACCEPTING_ERROR:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_ACCEPT:
                usock->state = NN_USOCK_STATE_ACCEPTING;
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/******************************************************************************/
/*  CANCELLING state.                                                         */
/******************************************************************************/
    case NN_USOCK_STATE_CANCELLING:
        switch (src) {
        case NN_USOCK_SRC_TASK_STOP:
            switch (type) {
            case NN_WORKER_TASK_EXECUTE:
                nn_worker_rm_fd (usock->worker, &usock->wfd);
                usock->state = NN_USOCK_STATE_LISTENING;

                /*  Notify the accepted socket that it was stopped. */
                nn_fsm_action (&usock->asock->fsm, NN_USOCK_ACTION_DONE);

                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        case NN_USOCK_SRC_FD:
            switch (type) {
            case NN_WORKER_FD_IN:
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/******************************************************************************/
/*  Invalid state                                                             */
/******************************************************************************/
    default:
        nn_fsm_bad_state (usock->state, src, type);
    }
}

static int nn_usock_send_raw (struct nn_usock *self, struct msghdr *hdr)
{
    ssize_t nbytes;

    /*  Try to send the data. */
#if defined MSG_NOSIGNAL
    nbytes = sendmsg (self->s, hdr, MSG_NOSIGNAL);
#else
    nbytes = sendmsg (self->s, hdr, 0);
#endif

    /*  Handle errors. */
    if (nn_slow (nbytes < 0)) {
        if (nn_fast (errno == EAGAIN || errno == EWOULDBLOCK))
            nbytes = 0;
        else {

            /*  If the connection fails, return ECONNRESET. */
            return -ECONNRESET;
        }
    }

    /*  Some bytes were sent. Adjust the iovecs accordingly. */
    while (nbytes) {
        if (nbytes >= (ssize_t)hdr->msg_iov->iov_len) {
            --hdr->msg_iovlen;
            if (!hdr->msg_iovlen) {
                nn_assert (nbytes == (ssize_t)hdr->msg_iov->iov_len);
                return 0;
            }
            nbytes -= hdr->msg_iov->iov_len;
            ++hdr->msg_iov;
        }
        else {
            *((uint8_t**) &(hdr->msg_iov->iov_base)) += nbytes;
            hdr->msg_iov->iov_len -= nbytes;
            return -EAGAIN;
        }
    }

    if (hdr->msg_iovlen > 0)
        return -EAGAIN;

    return 0;
}

static int nn_usock_recv_raw (struct nn_usock *self, void *buf, size_t *len)
{
    size_t sz;
    size_t length;
    ssize_t nbytes;
    struct iovec iov;
    struct msghdr hdr;
    unsigned char ctrl [256];
#if defined NN_HAVE_MSG_CONTROL
    struct cmsghdr *cmsg;
#endif
    int fd;

    /*  If batch buffer doesn't exist, allocate it. The point of delayed
        deallocation to allow non-receiving sockets, such as TCP listening
        sockets, to do without the batch buffer. */
    if (nn_slow (!self->in.batch)) {
        self->in.batch = nn_alloc (NN_USOCK_BATCH_SIZE, "AIO batch buffer");
        alloc_assert (self->in.batch);
    }

    /*  Try to satisfy the recv request by data from the batch buffer. */
    length = *len;
    sz = self->in.batch_len - self->in.batch_pos;
    if (sz) {
        if (sz > length)
            sz = length;
        memcpy (buf, self->in.batch + self->in.batch_pos, sz);
        self->in.batch_pos += sz;
        buf = ((char*) buf) + sz;
        length -= sz;
        if (!length)
            return 0;
    }

    /*  If recv request is greater than the batch buffer, get the data directly
        into the place. Otherwise, read data to the batch buffer. */
    if (length > NN_USOCK_BATCH_SIZE) {
        iov.iov_base = buf;
        iov.iov_len = length;
    }
    else {
        iov.iov_base = self->in.batch;
        iov.iov_len = NN_USOCK_BATCH_SIZE;
    }
    memset (&hdr, 0, sizeof (hdr));
    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;
#if defined NN_HAVE_MSG_CONTROL
    hdr.msg_control = ctrl;
    hdr.msg_controllen = sizeof (ctrl);
#else
    *((int*) ctrl) = -1;
    hdr.msg_accrights = ctrl;
    hdr.msg_accrightslen = sizeof (int);
#endif
    nbytes = recvmsg (self->s, &hdr, 0);

    /*  Handle any possible errors. */
    if (nn_slow (nbytes <= 0)) {

        if (nn_slow (nbytes == 0))
            return -ECONNRESET;

        /*  Zero bytes received. */
        if (nn_fast (errno == EAGAIN || errno == EWOULDBLOCK))
            nbytes = 0;
        else {

            /*  If the peer closes the connection, return ECONNRESET. */
            return -ECONNRESET;
        }
    }

    /*  Extract the associated file descriptor, if any. */
    if (nbytes > 0) {
#if defined NN_HAVE_MSG_CONTROL
        cmsg = CMSG_FIRSTHDR (&hdr);
        while (cmsg) {
            if (cmsg->cmsg_level == SOL_SOCKET &&
                  cmsg->cmsg_type == SCM_RIGHTS) {
                if (self->in.pfd) {
                    memcpy (self->in.pfd, CMSG_DATA (cmsg),
                        sizeof (*self->in.pfd));
                    self->in.pfd = NULL;
                }
                else {
                    memcpy (&fd, CMSG_DATA (cmsg), sizeof (fd));
                    nn_closefd (fd);
                }
                break;
            }
            cmsg = CMSG_NXTHDR (&hdr, cmsg);
        }
#else
        if (hdr.msg_accrightslen > 0) {
            nn_assert (hdr.msg_accrightslen == sizeof (int));
            if (self->in.pfd) {
                memcpy (self->in.pfd, hdr.msg_accrights,
                    sizeof (*self->in.pfd));
                self->in.pfd = NULL;
            }
            else {
                memcpy (&fd, hdr.msg_accrights, sizeof (fd));
                nn_closefd (fd);
            }
        }
#endif
    }

    /*  If the data were received directly into the place we can return
        straight away. */
    if (length > NN_USOCK_BATCH_SIZE) {
        length -= nbytes;
        *len -= length;
        return 0;
    }

    /*  New data were read to the batch buffer. Copy the requested amount of it
        to the user-supplied buffer. */
    self->in.batch_len = nbytes;
    self->in.batch_pos = 0;
    if (nbytes) {
        sz = nbytes > (ssize_t)length ? length : (size_t)nbytes;
        memcpy (buf, self->in.batch, sz);
        length -= sz;
        self->in.batch_pos += sz;
    }

    *len -= length;
    return 0;
}

static int nn_usock_geterr (struct nn_usock *self)
{
    int rc;
    int opt;
#if defined NN_HAVE_HPUX
    int optsz;
#else
    socklen_t optsz;
#endif

    opt = 0;
    optsz = sizeof (opt);
    rc = getsockopt (self->s, SOL_SOCKET, SO_ERROR, &opt, &optsz);

    /*  The following should handle both Solaris and UNIXes derived from BSD. */
    if (rc == -1)
        return errno;
    errno_assert (rc == 0);
    nn_assert (optsz == sizeof (opt));
    return opt;
}
