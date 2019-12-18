/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
    Copyright 2016 Garrett D'Amore <garrett@damore.org>

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

#ifndef NN_SOCK_INCLUDED
#define NN_SOCK_INCLUDED

#include "../protocol.h"
#include "../transport.h"

#include "../aio/ctx.h"
#include "../aio/fsm.h"

#include "../utils/efd.h"
#include "../utils/sem.h"
#include "../utils/list.h"

struct nn_pipe;

/*  The maximum implemented transport ID. */
#define NN_MAX_TRANSPORT 4

struct nn_sock
{
    /*  Socket state machine. */
    struct nn_fsm fsm;
    int state;

    /*  Pointer to the instance of the specific socket type. */
    struct nn_sockbase *sockbase;

    /*  Pointer to the socket type metadata. */
    const struct nn_socktype *socktype;

    int flags;

    struct nn_ctx ctx;
    struct nn_efd sndfd;
    struct nn_efd rcvfd;
    struct nn_sem termsem;
    struct nn_sem relesem;

    /*  List of all endpoints associated with the socket. */
    struct nn_list eps;

    /*  List of all endpoint being in the process of shutting down. */
    struct nn_list sdeps;

    /*  Next endpoint ID to assign to a new endpoint. */
    int eid;

    /*  Count of active holds against the socket. */
    int holds;

    /*  Socket-level socket options. */
    int sndbuf;
    int rcvbuf;
    int rcvmaxsize;
    int sndtimeo;
    int rcvtimeo;
    int reconnect_ivl;
    int reconnect_ivl_max;
    int maxttl;

    /*  Endpoint-specific options.  */
    struct nn_ep_options ep_template;

    /*  Transport-specific socket options. */
    struct nn_optset *optsets [NN_MAX_TRANSPORT];

    struct {

        /*****  The ever-incrementing counters  *****/

        /*  Successfully established nn_connect() connections  */
        uint64_t established_connections;
        /*  Successfully accepted connections  */
        uint64_t accepted_connections;
        /*  Forcedly closed connections  */
        uint64_t dropped_connections;
        /*  Connections closed by peer  */
        uint64_t broken_connections;
        /*  Errors trying to establish active connection  */
        uint64_t connect_errors;
        /*  Errors binding to specified port  */
        uint64_t bind_errors;
        /*  Errors accepting connections at nn_bind()'ed endpoint  */
        uint64_t accept_errors;

        /*  Messages sent  */
        uint64_t messages_sent;
        /*  Messages received  */
        uint64_t messages_received;
        /*  Bytes sent (sum length of data in messages sent)  */
        uint64_t bytes_sent;
        /*  Bytes recevied (sum length of data in messages received)  */
        uint64_t bytes_received;

        /*****  Level-style values *****/

        /*  Number of currently established connections  */
        int current_connections;
        /*  Number of connections currently in progress  */
        int inprogress_connections;
        /*  The currently set priority for sending data  */
        int current_snd_priority;
        /*  Number of endpoints having last_errno set to non-zero value  */
        int current_ep_errors;

    } statistics;

    /*  The socket name for statistics  */
    char socket_name[64];

    /* Win32 Security Attribute */
    void * sec_attr;
    size_t sec_attr_size;
    int outbuffersz;
    int inbuffersz;

};

/*  Initialise the socket. */
int nn_sock_init (struct nn_sock *self, const struct nn_socktype *socktype,
    int fd);

/*  Called by nn_close() to stop activity on the socket.  It doesn't block. */
void nn_sock_stop (struct nn_sock *self);

/*  Called by nn_close() to deallocate the socket. It's a blocking function
    and can return -EINTR. */
int nn_sock_term (struct nn_sock *self);

/*  Called by sockbase when stopping is done. */
void nn_sock_stopped (struct nn_sock *self);

/*  Returns the AIO context associated with the socket. */
struct nn_ctx *nn_sock_getctx (struct nn_sock *self);

/*  Returns 1 if the specified socket type is a valid peer for this socket,
    0 otherwise. */
int nn_sock_ispeer (struct nn_sock *self, int socktype);

/*  Add new endpoint to the socket. */
int nn_sock_add_ep (struct nn_sock *self, const struct nn_transport *transport,
    int bind, const char *addr);

/*  Remove the endpoint with the specified ID from the socket. */
int nn_sock_rm_ep (struct nn_sock *self, int eid);

/*  Send a message to the socket. */
int nn_sock_send (struct nn_sock *self, struct nn_msg *msg, int flags);

/*  Receive a message from the socket. */
int nn_sock_recv (struct nn_sock *self, struct nn_msg *msg, int flags);

/*  Set a socket option. */
int nn_sock_setopt (struct nn_sock *self, int level, int option,
    const void *optval, size_t optvallen);

/*  Retrieve a socket option. This function is to be called from the API. */
int nn_sock_getopt (struct nn_sock *self, int level, int option,
    void *optval, size_t *optvallen);

/*  Retrieve a socket option. This function is to be called from within
    the socket. */
int nn_sock_getopt_inner (struct nn_sock *self, int level, int option,
    void *optval, size_t *optvallen);

/*  Used by pipes. */
int nn_sock_add (struct nn_sock *self, struct nn_pipe *pipe);
void nn_sock_rm (struct nn_sock *self, struct nn_pipe *pipe);

/*  Monitoring callbacks  */
void nn_sock_report_error(struct nn_sock *self, struct nn_ep *ep,  int errnum);
void nn_sock_stat_increment(struct nn_sock *self, int name, int64_t increment);

/*  Holds and releases. */
int nn_sock_hold (struct nn_sock *self);
void nn_sock_rele (struct nn_sock *self);

#endif

