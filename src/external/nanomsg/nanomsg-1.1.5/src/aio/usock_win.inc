/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
    Copyright 2018 Staysail Systems, Inc. <info@staysail.tech>
    Copyright 2018 Capitar IT Group BV <info@capitar.com>

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

#include "worker.h"

#include "../utils/err.h"
#include "../utils/cont.h"
#include "../utils/alloc.h"

#include <stddef.h>
#include <string.h>
#include <limits.h>

#define NN_USOCK_STATE_IDLE 1
#define NN_USOCK_STATE_STARTING 2
#define NN_USOCK_STATE_BEING_ACCEPTED 3
#define NN_USOCK_STATE_ACCEPTED 4
#define NN_USOCK_STATE_CONNECTING 5
#define NN_USOCK_STATE_ACTIVE 6
#define NN_USOCK_STATE_CANCELLING_IO 7
#define NN_USOCK_STATE_DONE 8
#define NN_USOCK_STATE_LISTENING 9
#define NN_USOCK_STATE_ACCEPTING 10
#define NN_USOCK_STATE_CANCELLING 11
#define NN_USOCK_STATE_STOPPING 12
#define NN_USOCK_STATE_STOPPING_ACCEPT 13

#define NN_USOCK_ACTION_ACCEPT 1
#define NN_USOCK_ACTION_BEING_ACCEPTED 2
#define NN_USOCK_ACTION_CANCEL 3
#define NN_USOCK_ACTION_LISTEN 4
#define NN_USOCK_ACTION_CONNECT 5
#define NN_USOCK_ACTION_ACTIVATE 6
#define NN_USOCK_ACTION_DONE 7
#define NN_USOCK_ACTION_ERROR 8

#define NN_USOCK_SRC_IN 1
#define NN_USOCK_SRC_OUT 2

/*  Private functions. */
static void nn_usock_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_usock_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);
static int nn_usock_cancel_io (struct nn_usock *self);
static void nn_usock_create_io_completion (struct nn_usock *self);
DWORD nn_usock_open_pipe (struct nn_usock *self, const char *name);
void nn_usock_accept_pipe (struct nn_usock *self, struct nn_usock *listener);

void nn_usock_init (struct nn_usock *self, int src, struct nn_fsm *owner)
{
    nn_fsm_init (&self->fsm, nn_usock_handler, nn_usock_shutdown,
        src, self, owner);
    self->state = NN_USOCK_STATE_IDLE;
    self->s = INVALID_SOCKET;
    self->isaccepted = 0;
    nn_worker_op_init (&self->in, NN_USOCK_SRC_IN, &self->fsm);
    nn_worker_op_init (&self->out, NN_USOCK_SRC_OUT, &self->fsm);
    self->domain = -1;
    self->type = -1;
    self->protocol = -1;

    /*  Intialise events raised by usock. */
    nn_fsm_event_init (&self->event_established);
    nn_fsm_event_init (&self->event_sent);
    nn_fsm_event_init (&self->event_received);
    nn_fsm_event_init (&self->event_error);

    /*  No accepting is going on at the moment. */
    self->asock = NULL;
    self->ainfo = NULL;

    /* NamedPipe-related stuff. */
    memset (&self->pipename, 0, sizeof (self->pipename));
    self->pipesendbuf = NULL;
    self->sec_attr = NULL;

    /* default size for both in and out buffers is 4096 */
    self->outbuffersz = 4096;
    self->inbuffersz = 4096;
}

void nn_usock_term (struct nn_usock *self)
{
    nn_assert_state (self, NN_USOCK_STATE_IDLE);

    if (self->ainfo)
        nn_free (self->ainfo);
    if (self->pipesendbuf)
        nn_free (self->pipesendbuf);
    nn_fsm_event_term (&self->event_error);
    nn_fsm_event_term (&self->event_received);
    nn_fsm_event_term (&self->event_sent);
    nn_fsm_event_term (&self->event_established);
    nn_worker_op_term (&self->out);
    nn_worker_op_term (&self->in);
    nn_fsm_term (&self->fsm);
}

int nn_usock_isidle (struct nn_usock *self)
{
    return nn_fsm_isidle (&self->fsm);
}

int nn_usock_start (struct nn_usock *self, int domain, int type, int protocol)
{
    int rc;
#if defined IPV6_V6ONLY
    DWORD only;
#endif
#if defined HANDLE_FLAG_INHERIT
    BOOL brc;
#endif

    /* NamedPipes aren't sockets. They don't need all the socket
       initialisation stuff. */
    if (domain != AF_UNIX) {

        /*  Open the underlying socket. */
        self->s = socket (domain, type, protocol);
        if (self->s == INVALID_SOCKET)
           return -nn_err_wsa_to_posix (WSAGetLastError ());

        /*  Disable inheriting the socket to the child processes. */
#if defined HANDLE_FLAG_INHERIT
        brc = SetHandleInformation (self->p, HANDLE_FLAG_INHERIT, 0);
        win_assert (brc);
#endif

        /*  IPv4 mapping for IPv6 sockets is disabled by default. Switch it on. */
#if defined IPV6_V6ONLY
        if (domain == AF_INET6) {
            only = 0;
            rc = setsockopt (self->s, IPPROTO_IPV6, IPV6_V6ONLY,
                (const char*) &only, sizeof (only));
            wsa_assert (rc != SOCKET_ERROR);
        }
#endif

        /*  Associate the socket with a worker thread/completion port. */
        nn_usock_create_io_completion (self);
    }

    /*  Remember the type of the socket. */
    self->domain = domain;
    self->type = type;
    self->protocol = protocol;

    /*  Start the state machine. */
    nn_fsm_start (&self->fsm);

    return 0;
}

void nn_usock_start_fd (struct nn_usock *self, int fd)
{
    nn_assert (0);
}

void nn_usock_stop (struct nn_usock *self)
{
    nn_fsm_stop (&self->fsm);
}

void nn_usock_swap_owner (struct nn_usock *self, struct nn_fsm_owner *owner)
{
    nn_fsm_swap_owner (&self->fsm, owner);
}

int nn_usock_setsockopt (struct nn_usock *self, int level, int optname,
    const void *optval, size_t optlen)
{
    int rc;

    /*  NamedPipes aren't sockets. We can't set socket options on them.
        For now we'll ignore the options. */
    if (self->domain == AF_UNIX)
        return 0;

    /*  The socket can be modified only before it's active. */
    nn_assert (self->state == NN_USOCK_STATE_STARTING ||
        self->state == NN_USOCK_STATE_ACCEPTED);

    nn_assert (optlen < INT_MAX);

    rc = setsockopt (self->s, level, optname, (char*) optval, (int) optlen);
    if (nn_slow (rc == SOCKET_ERROR))
        return -nn_err_wsa_to_posix (WSAGetLastError ());

    return 0;
}

int nn_usock_bind (struct nn_usock *self, const struct sockaddr *addr,
    size_t addrlen)
{
    int rc;
    ULONG opt;

    /*  In the case of named pipes, let's save the address
        for the later use. */
    if (self->domain == AF_UNIX) {
        if (addrlen > sizeof (struct sockaddr_un))
            return -EINVAL;
        memcpy (&self->pipename, addr, addrlen);
        return 0;
    }

    /*  You can set socket options only before the socket is connected. */
    nn_assert_state (self, NN_USOCK_STATE_STARTING);

    /*  On Windows, the bound port can be hijacked
        if SO_EXCLUSIVEADDRUSE is not set. */
    opt = 1;
    rc = setsockopt (self->s, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
        (const char*) &opt, sizeof (opt));
    wsa_assert (rc != SOCKET_ERROR);

    nn_assert (addrlen < INT_MAX);
    rc = bind (self->s, addr, (int) addrlen);
    if (nn_slow (rc == SOCKET_ERROR))
       return -nn_err_wsa_to_posix (WSAGetLastError ());

    return 0;
}

int nn_usock_listen (struct nn_usock *self, int backlog)
{
    int rc;

    /*  You can start listening only before the socket is connected. */
    nn_assert_state (self, NN_USOCK_STATE_STARTING);

    /*  Start listening for incoming connections. NamedPipes are already
        created in the listening state, so no need to do anything here. */
    if (self->domain != AF_UNIX) {
        rc = listen (self->s, backlog);
        if (nn_slow (rc == SOCKET_ERROR))
           return -nn_err_wsa_to_posix (WSAGetLastError ());
    }

    /*  Notify the state machine. */
    nn_fsm_action (&self->fsm, NN_USOCK_ACTION_LISTEN);

    return 0;
}

void nn_usock_accept (struct nn_usock *self, struct nn_usock *listener)
{
    int rc;
    BOOL brc;
    DWORD nbytes;
 
    /* NamedPipes have their own accepting mechanism. */
    if (listener->domain == AF_UNIX) {
        nn_usock_accept_pipe (self, listener);
        return;
    }

    rc = nn_usock_start (self, listener->domain, listener->type,
        listener->protocol);
    /*  TODO: EMFILE can be returned here. */
    errnum_assert (rc == 0, -rc);
    nn_fsm_action (&listener->fsm, NN_USOCK_ACTION_ACCEPT);
    nn_fsm_action (&self->fsm, NN_USOCK_ACTION_BEING_ACCEPTED);

    /*  If the memory for accept information is not yet allocated, do so.  */
    if (!listener->ainfo) {
        listener->ainfo = nn_alloc (512, "accept info");
        alloc_assert (listener->ainfo);
    }

    /*  Wait for the incoming connection. */
    memset (&listener->in.olpd, 0, sizeof (listener->in.olpd));
    brc = AcceptEx (listener->s, self->s, listener->ainfo, 0, 256, 256, &nbytes,
        &listener->in.olpd);

    /*  Immediate success. */
    if (nn_fast (brc == TRUE)) {
        nn_fsm_action (&listener->fsm, NN_USOCK_ACTION_DONE);
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_DONE);
        return;
    }

    /*  We don't expect a synchronous failure at this point. */
    wsa_assert (nn_slow (WSAGetLastError () == WSA_IO_PENDING));

    /*  Pair the two sockets. */
    nn_assert (!self->asock);
    self->asock = listener;
    nn_assert (!listener->asock);
    listener->asock = self;

    /*  Asynchronous accept. */
    nn_worker_op_start (&listener->in);
}

void nn_usock_activate (struct nn_usock *self)
{
    nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ACTIVATE);
}

void nn_usock_connect (struct nn_usock *self, const struct sockaddr *addr,
    size_t addrlen)
{
    BOOL brc;
    GUID fid = WSAID_CONNECTEX;
    LPFN_CONNECTEX pconnectex;
    DWORD nbytes;
    DWORD winerror;

    /*  Fail if the socket is already connected, closed or such. */
    nn_assert_state (self, NN_USOCK_STATE_STARTING);

    /*  Notify the state machine that we've started connecting. */
    nn_fsm_action (&self->fsm, NN_USOCK_ACTION_CONNECT);

    memset (&self->out.olpd, 0, sizeof (self->out.olpd));

    if (self->domain == AF_UNIX) {
        winerror = nn_usock_open_pipe (self, ((struct sockaddr_un*) addr)->sun_path);
    }
    else
    {
        /*  Get the pointer to connect function. */
        brc = WSAIoctl (self->s, SIO_GET_EXTENSION_FUNCTION_POINTER,
            &fid, sizeof (fid), &pconnectex, sizeof (pconnectex),
            &nbytes, NULL, NULL) == 0;
        wsa_assert (brc == TRUE);
        nn_assert (nbytes == sizeof (pconnectex));

        /*  Ensure it is safe to cast this value to what might be a smaller
            integer type to conform to the pconnectex function signature. */
        nn_assert (addrlen < INT_MAX);

        /*  Connect itself. */
        brc = pconnectex (self->s, addr, (int) addrlen, NULL, 0, NULL,
            &self->out.olpd);
        winerror = brc ? ERROR_SUCCESS : WSAGetLastError ();
    }

    /*  Immediate success. */
    if (nn_fast (winerror == ERROR_SUCCESS)) {
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_DONE);
        return;
    }

    /*  Immediate error. */
    if (nn_slow (winerror != WSA_IO_PENDING)) {
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ERROR);
        return;
    }

    /*  Asynchronous connect. */
    nn_worker_op_start (&self->out);
}

void nn_usock_send (struct nn_usock *self, const struct nn_iovec *iov,
    int iovcnt)
{
    int rc;
    BOOL brc;
    WSABUF wbuf [NN_USOCK_MAX_IOVCNT];
    int i;
    size_t len;
    size_t idx;
    DWORD error;

    /*  Make sure that the socket is actually alive. */
    nn_assert_state (self, NN_USOCK_STATE_ACTIVE);

    /*  Create a WinAPI-style iovec. */
    len = 0;
    nn_assert (iovcnt <= NN_USOCK_MAX_IOVCNT);
    for (i = 0; i != iovcnt; ++i) {
        wbuf [i].buf = (char FAR*) iov [i].iov_base;
        wbuf [i].len = (ULONG) iov [i].iov_len;
        len += iov [i].iov_len;
    }

    /*  Start the send operation. */
    memset (&self->out.olpd, 0, sizeof (self->out.olpd));
    if (self->domain == AF_UNIX)
    {
        /* TODO: Do not copy the buffer, find an efficent way to Write 
           multiple buffers that doesn't affect the state machine. */

        /*  Ensure the total buffer size does not exceed size limitation
            of WriteFile. */
        nn_assert (len <= MAXDWORD);

        nn_assert (!self->pipesendbuf);
        self->pipesendbuf = nn_alloc (len, "named pipe sendbuf");

        idx = 0;
        for (i = 0; i != iovcnt; ++i) {
            memcpy ((char*)(self->pipesendbuf) + idx, iov [i].iov_base, iov [i].iov_len);
            idx += iov [i].iov_len;
        }
        brc = WriteFile (self->p, self->pipesendbuf, (DWORD) len, NULL, &self->out.olpd);
        if (nn_fast (brc || GetLastError() == ERROR_IO_PENDING)) {
            nn_worker_op_start (&self->out);
            return;
        }
        error = GetLastError();
        win_assert (error == ERROR_NO_DATA);
        self->errnum = EINVAL;
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ERROR);
        return;
    }

    rc = WSASend (self->s, wbuf, iovcnt, NULL, 0, &self->out.olpd, NULL);
    if (nn_fast (rc == 0)) {
        nn_worker_op_start (&self->out);
        return;
    }
    error = WSAGetLastError();
    if (nn_fast (error == WSA_IO_PENDING)) {
        nn_worker_op_start (&self->out);
        return;
    }
    wsa_assert (error == WSAECONNABORTED || error == WSAECONNRESET ||
        error == WSAENETDOWN || error == WSAENETRESET ||
        error == WSAENOBUFS || error == WSAEWOULDBLOCK);
    self->errnum = nn_err_wsa_to_posix (error);
    nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ERROR);
}

void nn_usock_recv_start_wsock (void *arg)
{
    struct nn_usock *self = arg;
    WSABUF wbuf;
    DWORD flags;
    DWORD error;

    /*  Start the receive operation. */
    wbuf.len = (ULONG) self->in.resid;
    wbuf.buf = (char FAR*) self->in.buf;
    flags = MSG_WAITALL;
    memset (&self->in.olpd, 0, sizeof (self->in.olpd));

    if (WSARecv (self->s, &wbuf, 1, NULL, &flags, &self->in.olpd, NULL) == 0) {
        error = ERROR_SUCCESS;
    } else {
        error = WSAGetLastError ();
    }

    switch (error) {
    case ERROR_SUCCESS:
    case WSA_IO_PENDING:
        nn_worker_op_start (&self->in);
        return;

    default:
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ERROR);
        return;
    }
}

void nn_usock_recv_start_pipe (void *arg)
{
    struct nn_usock *self = arg;
    void *buf = self->in.buf;
    DWORD len = (DWORD) self->in.resid;
    DWORD error;

    /*  Start the receive operation. */
    memset (&self->in.olpd, 0, sizeof (self->in.olpd));

    if (ReadFile (self->p, buf, len, NULL, &self->in.olpd)) {
        error = ERROR_SUCCESS;
    } else {
        error = GetLastError ();
    }

    switch (error) {
    case ERROR_SUCCESS:
    case ERROR_IO_PENDING:
        nn_worker_op_start (&self->in);
        return;

    default:
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_ERROR);
        return;
   }
}

void nn_usock_recv (struct nn_usock *self, void *buf, size_t len, int *fd)
{
    /*  Passing file descriptors is not implemented on Windows platform. */
    if (fd)
        *fd = -1;

    /*  Make sure that the socket is actually alive. */
    nn_assert_state (self, NN_USOCK_STATE_ACTIVE);

    self->in.resid = len;
    self->in.buf = buf;
    self->in.arg = self;
    self->in.zero_is_error = 1;
    if (self->domain == AF_UNIX) {
        self->in.start = nn_usock_recv_start_pipe;
    }
    else {
        self->in.start = nn_usock_recv_start_wsock;
    }

    self->in.start (self->in.arg);
}

static void nn_usock_create_io_completion (struct nn_usock *self)
{
    struct nn_worker *worker;
    HANDLE cp;

    /*  Associate the socket with a worker thread/completion port. */
    worker = nn_fsm_choose_worker (&self->fsm);
    cp = CreateIoCompletionPort (
	    self->p,
        nn_worker_getcp(worker),
		(ULONG_PTR) NULL,
		0);
    nn_assert(cp);
}

static void nn_usock_create_pipe (struct nn_usock *self, const char *name)
{
    char fullname [256];
    /*  First, create a fully qualified name for the named pipe. */
    _snprintf(fullname, sizeof (fullname), "\\\\.\\pipe\\%s", name);

    self->p = CreateNamedPipeA (
        (char*) fullname,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE |
            PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
        PIPE_UNLIMITED_INSTANCES,
        self->outbuffersz,
        self->inbuffersz,
        0,
        self->sec_attr);

    /* TODO: How to properly handle self->p == INVALID_HANDLE_VALUE? */
    win_assert (self->p != INVALID_HANDLE_VALUE);

    self->isaccepted = 1;
    nn_usock_create_io_completion (self);
}

DWORD nn_usock_open_pipe (struct nn_usock *self, const char *name)
{
    char fullname [256];
    DWORD winerror;
	DWORD mode;
	BOOL brc;

    /*  First, create a fully qualified name for the named pipe. */
    _snprintf(fullname, sizeof (fullname), "\\\\.\\pipe\\%s", name);

    self->p = CreateFileA (
        fullname,
        GENERIC_READ | GENERIC_WRITE,
        0,
        self->sec_attr,
        OPEN_ALWAYS,
        FILE_FLAG_OVERLAPPED,
        NULL);

    if (self->p == INVALID_HANDLE_VALUE)
        return GetLastError ();

    mode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT;
    brc = SetNamedPipeHandleState (
	    self->p,
		&mode,
		NULL,
		NULL);
	if (!brc) {
        CloseHandle (self->p);
        self->p = INVALID_HANDLE_VALUE;
        return GetLastError ();
    }
    self->isaccepted = 0;
    nn_usock_create_io_completion (self);

    winerror = GetLastError ();
    if (winerror != ERROR_SUCCESS && winerror != ERROR_ALREADY_EXISTS)
        return winerror;

    return ERROR_SUCCESS;
}

void nn_usock_accept_pipe (struct nn_usock *self, struct nn_usock *listener)
{
    int rc;
    BOOL brc;
    DWORD winerror;

    /*  TODO: EMFILE can be returned here. */
    rc = nn_usock_start (self, listener->domain, listener->type,
        listener->protocol);
    errnum_assert(rc == 0, -rc);

    nn_fsm_action(&listener->fsm, NN_USOCK_ACTION_ACCEPT);
    nn_fsm_action(&self->fsm, NN_USOCK_ACTION_BEING_ACCEPTED);

    /*  If the memory for accept information is not yet allocated, do so now. */
    if (!listener->ainfo) {
        listener->ainfo = nn_alloc (512, "accept info");
        alloc_assert (listener->ainfo);
    }

    /*  Wait for the incoming connection. */
    memset (&listener->in.olpd, 0, sizeof(listener->in.olpd));
    nn_usock_create_pipe (self, listener->pipename.sun_path);
    brc = ConnectNamedPipe (self->p, (LPOVERLAPPED) &listener->in.olpd);

    /*  TODO: Can this function possibly succeed? */
    nn_assert (brc == 0);
    winerror = GetLastError();

    /*  Immediate success. */
    if (nn_fast (winerror == ERROR_PIPE_CONNECTED)) {
        nn_fsm_action (&listener->fsm, NN_USOCK_ACTION_DONE);
        nn_fsm_action (&self->fsm, NN_USOCK_ACTION_DONE);
        return;
    }

    /*  We don't expect a synchronous failure at this point. */
    wsa_assert (nn_slow (winerror == WSA_IO_PENDING));

    /*  Pair the two sockets. */
    nn_assert (!self->asock);
    self->asock = listener;
    nn_assert (!listener->asock);
    listener->asock = self;

    /*  Asynchronous accept. */
    nn_worker_op_start (&listener->in);
}

static void nn_usock_close (struct nn_usock *self)
{
    int rc;
    BOOL brc;

    if (self->domain == AF_UNIX) {
        if (self->p == INVALID_HANDLE_VALUE)
            return;
        if (self->isaccepted)
            DisconnectNamedPipe(self->p);
        brc = CloseHandle (self->p);
        self->p = INVALID_HANDLE_VALUE;
        win_assert (brc);
    }
    else
    {
        rc = closesocket (self->s);
        self->s = INVALID_SOCKET;
        wsa_assert (rc == 0);
    }
}

static void nn_usock_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr)
{
    struct nn_usock *usock;

    usock = nn_cont (self, struct nn_usock, fsm);

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {

        /*  Socket in ACCEPTING state cannot be closed.
            Stop the socket being accepted first. */
        nn_assert (usock->state != NN_USOCK_STATE_ACCEPTING);

        /*  Synchronous stop. */
        if (usock->state == NN_USOCK_STATE_IDLE)
            goto finish3;
        if (usock->state == NN_USOCK_STATE_DONE)
            goto finish2;
        if (usock->state == NN_USOCK_STATE_STARTING ||
              usock->state == NN_USOCK_STATE_ACCEPTED ||
              usock->state == NN_USOCK_STATE_LISTENING)
            goto finish1;

        /*  When socket that's being accepted is asked to stop, we have to
            ask the listener socket to stop accepting first. */
        if (usock->state == NN_USOCK_STATE_BEING_ACCEPTED) {
            nn_fsm_action (&usock->asock->fsm, NN_USOCK_ACTION_CANCEL);
            usock->state = NN_USOCK_STATE_STOPPING_ACCEPT;
            return;
        }

        /*  If we were already in the process of cancelling overlapped
            operations, we don't have to do anything. Continue waiting
            till cancelling is finished. */
        if (usock->state == NN_USOCK_STATE_CANCELLING_IO) {
            usock->state = NN_USOCK_STATE_STOPPING;
            return;
        }

        /*  Notify our parent that pipe socket is shutting down  */
        nn_fsm_raise (&usock->fsm, &usock->event_error, NN_USOCK_SHUTDOWN);

        /*  In all remaining states we'll simply cancel all overlapped
            operations. */
        if (nn_usock_cancel_io (usock) == 0)
            goto finish1;
        usock->state = NN_USOCK_STATE_STOPPING;
        return;
    }
    if (nn_slow (usock->state == NN_USOCK_STATE_STOPPING_ACCEPT)) {
        nn_assert (src == NN_FSM_ACTION && type == NN_USOCK_ACTION_DONE);
        goto finish1;
    }
    if (nn_slow (usock->state == NN_USOCK_STATE_STOPPING)) {
        if (!nn_worker_op_isidle (&usock->in) ||
              !nn_worker_op_isidle (&usock->out))
            return;
finish1:
        nn_usock_close(usock);
finish2:
        usock->state = NN_USOCK_STATE_IDLE;
        nn_fsm_stopped (&usock->fsm, NN_USOCK_STOPPED);
finish3:
        return;
    }

    nn_fsm_bad_state(usock->state, src, type);
}

static void nn_usock_handler (struct nn_fsm *self, int src, int type,
    void *srcptr)
{
    struct nn_usock *usock;

    usock = nn_cont (self, struct nn_usock, fsm);

    switch (usock->state) {

/*****************************************************************************/
/*  IDLE state.                                                              */
/*****************************************************************************/
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

/*****************************************************************************/
/*  STARTING state.                                                          */
/*****************************************************************************/
    case NN_USOCK_STATE_STARTING:
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
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/*****************************************************************************/
/*  BEING_ACCEPTED state.                                                    */
/*****************************************************************************/
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

/*****************************************************************************/
/*  ACCEPTED state.                                                          */
/*****************************************************************************/
    case NN_USOCK_STATE_ACCEPTED:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_ACTIVATE:
                usock->state = NN_USOCK_STATE_ACTIVE;
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/*****************************************************************************/
/*  CONNECTING state.                                                        */
/*****************************************************************************/
    case NN_USOCK_STATE_CONNECTING:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_DONE:
                usock->state = NN_USOCK_STATE_ACTIVE;
                nn_fsm_raise (&usock->fsm, &usock->event_established,
                    NN_USOCK_CONNECTED);
                return;
            case NN_USOCK_ACTION_ERROR:
                nn_usock_close(usock);
                usock->state = NN_USOCK_STATE_DONE;
                nn_fsm_raise (&usock->fsm, &usock->event_error, NN_USOCK_ERROR);
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        case NN_USOCK_SRC_OUT:
            switch (type) {
            case NN_WORKER_OP_DONE:
                usock->state = NN_USOCK_STATE_ACTIVE;
                nn_fsm_raise (&usock->fsm, &usock->event_established,
                    NN_USOCK_CONNECTED);
                return;
            case NN_WORKER_OP_ERROR:
                nn_usock_close(usock);
                usock->state = NN_USOCK_STATE_DONE;
                nn_fsm_raise (&usock->fsm, &usock->event_error, NN_USOCK_ERROR);
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/*****************************************************************************/
/*  ACTIVE state.                                                            */
/*****************************************************************************/
    case NN_USOCK_STATE_ACTIVE:
        switch (src) {
        case NN_USOCK_SRC_IN:
            switch (type) {
            case NN_WORKER_OP_DONE:
                nn_fsm_raise (&usock->fsm, &usock->event_received,
                    NN_USOCK_RECEIVED);
                return;
            case NN_WORKER_OP_ERROR:
                if (nn_usock_cancel_io (usock) == 0) {
                    nn_fsm_raise(&usock->fsm, &usock->event_error,
                        NN_USOCK_ERROR);
                    nn_usock_close (usock);
                    usock->state = NN_USOCK_STATE_DONE;
                    return;
                }
                usock->state = NN_USOCK_STATE_CANCELLING_IO;
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        case NN_USOCK_SRC_OUT:
            switch (type) {
            case NN_WORKER_OP_DONE:
                if (usock->pipesendbuf) {
                    nn_free(usock->pipesendbuf);
                    usock->pipesendbuf = NULL;
                }
                nn_fsm_raise (&usock->fsm, &usock->event_sent, NN_USOCK_SENT);
                return;
            case NN_WORKER_OP_ERROR:
                if (nn_usock_cancel_io (usock) == 0) {
                    nn_fsm_raise(&usock->fsm, &usock->event_error,
                        NN_USOCK_ERROR);
                    nn_usock_close(usock);
                    usock->state = NN_USOCK_STATE_DONE;
                    return;
                }
                usock->state = NN_USOCK_STATE_CANCELLING_IO;
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_ERROR:
                if (nn_usock_cancel_io (usock) == 0) {
                    nn_fsm_raise(&usock->fsm, &usock->event_error,
                        NN_USOCK_SHUTDOWN);
                    nn_usock_close(usock);
                    usock->state = NN_USOCK_STATE_DONE;
                    return;
                }
                usock->state = NN_USOCK_STATE_CANCELLING_IO;
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/*****************************************************************************/
/*  CANCELLING_IO state.                                                     */
/*****************************************************************************/
    case NN_USOCK_STATE_CANCELLING_IO:
        switch (src) {
        case NN_USOCK_SRC_IN:
        case NN_USOCK_SRC_OUT:
            if (!nn_worker_op_isidle (&usock->in) ||
                  !nn_worker_op_isidle (&usock->out))
                return;
            nn_fsm_raise(&usock->fsm, &usock->event_error, NN_USOCK_SHUTDOWN);
            nn_usock_close(usock);
            usock->state = NN_USOCK_STATE_DONE;
            return;
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/*****************************************************************************/
/*  DONE state.                                                              */
/*****************************************************************************/
    case NN_USOCK_STATE_DONE:
        nn_fsm_bad_source (usock->state, src, type);

/*****************************************************************************/
/*  LISTENING state.                                                         */
/*****************************************************************************/
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

/*****************************************************************************/
/*  ACCEPTING state.                                                         */
/*****************************************************************************/
    case NN_USOCK_STATE_ACCEPTING:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_USOCK_ACTION_DONE:
                usock->state = NN_USOCK_STATE_LISTENING;
                return;
            case NN_USOCK_ACTION_CANCEL:
                if (usock->p == INVALID_HANDLE_VALUE && usock->asock != NULL && usock->domain == AF_UNIX) {
                    usock->p = usock->asock->p;
                    nn_usock_cancel_io (usock);
                    usock->p = INVALID_HANDLE_VALUE;
                }
                else
                {
                    nn_usock_cancel_io(usock);
                }
                usock->state = NN_USOCK_STATE_CANCELLING;
                return;
            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        case NN_USOCK_SRC_IN:
            switch (type) {
            case NN_WORKER_OP_DONE:

                /*  Adjust the new usock object. */
                usock->asock->state = NN_USOCK_STATE_ACCEPTED;

                /*  Notify the user that connection was accepted. */
                nn_fsm_raise (&usock->asock->fsm,
                    &usock->asock->event_established, NN_USOCK_ACCEPTED);

                /*  Disassociate the listener socket from the accepted
                    socket. */
                usock->asock->asock = NULL;
                usock->asock = NULL;

                /*  Wait till the user starts accepting once again. */
                usock->state = NN_USOCK_STATE_LISTENING;

                return;

            case NN_WORKER_OP_ERROR:
                nn_usock_close(usock->asock);
                usock->asock->state = NN_USOCK_STATE_DONE;
                nn_fsm_raise (&usock->asock->fsm, &usock->asock->event_error,
                    NN_USOCK_ERROR);
                usock->asock->asock = NULL;
                usock->asock = NULL;
                /*  Wait till the user starts accepting once again. */
                usock->state = NN_USOCK_STATE_LISTENING;
                return;

            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/*****************************************************************************/
/*  CANCELLING state.                                                        */
/*****************************************************************************/
    case NN_USOCK_STATE_CANCELLING:
        switch (src) {
        case NN_USOCK_SRC_IN:
            switch (type) {
            case NN_WORKER_OP_DONE:
            case NN_WORKER_OP_ERROR:

                /*  TODO: The socket being accepted should be closed here. */

                usock->state = NN_USOCK_STATE_LISTENING;

                /*  Notify the accepted socket that it was stopped. */
                nn_fsm_action (&usock->asock->fsm, NN_USOCK_ACTION_DONE);

                return;

            default:
                nn_fsm_bad_action (usock->state, src, type);
            }
        default:
            nn_fsm_bad_source (usock->state, src, type);
        }

/*****************************************************************************/
/*  Invalid state.                                                           */
/*****************************************************************************/
    default:
        nn_fsm_bad_state (usock->state, src, type);
    }
}

/*****************************************************************************/
/*  State machine actions.                                                   */
/*****************************************************************************/

/*  Returns 0 if there's nothing to cancel or 1 otherwise. */
static int nn_usock_cancel_io (struct nn_usock *self)
{
    int rc;
    BOOL brc;

    /*  For some reason simple CancelIo doesn't seem to work here.
        We have to use CancelIoEx instead. */
    rc = 0;
    if (!nn_worker_op_isidle (&self->in)) {
        brc = CancelIoEx (self->p, &self->in.olpd);
        win_assert (brc || GetLastError () == ERROR_NOT_FOUND);
        rc = 1;
    }
    if (!nn_worker_op_isidle (&self->out)) {
        brc = CancelIoEx (self->p, &self->out.olpd);
        win_assert (brc || GetLastError () == ERROR_NOT_FOUND);
        rc = 1;
    }

    return rc;
}
