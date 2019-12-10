/*
    Copyright (c) 2012-2014 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
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

#include "../nn.h"
#include "../transport.h"
#include "../protocol.h"

#include "global.h"
#include "sock.h"
#include "ep.h"

#include "../aio/pool.h"
#include "../aio/timer.h"

#include "../utils/err.h"
#include "../utils/alloc.h"
#include "../utils/mutex.h"
#include "../utils/condvar.h"
#include "../utils/once.h"
#include "../utils/list.h"
#include "../utils/cont.h"
#include "../utils/random.h"
#include "../utils/chunk.h"
#include "../utils/msg.h"
#include "../utils/attr.h"

#include "../pubsub.h"
#include "../pipeline.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined NN_HAVE_WINDOWS
#include "../utils/win.h"
#else
#include <unistd.h>
#endif

/*  Max number of concurrent SP sockets. Configureable at build time */
#ifndef NN_MAX_SOCKETS
#define NN_MAX_SOCKETS 512
#endif

/*  To save some space, list of unused socket slots uses uint16_t integers to
    refer to individual sockets. If there's a need to more that 0x10000 sockets,
    the type should be changed to uint32_t or int. */
CT_ASSERT (NN_MAX_SOCKETS <= 0x10000);

#define NN_CTX_FLAG_TERMED 1
#define NN_CTX_FLAG_TERMING 2
#define NN_CTX_FLAG_TERM (NN_CTX_FLAG_TERMED | NN_CTX_FLAG_TERMING)

#define NN_GLOBAL_SRC_STAT_TIMER 1

#define NN_GLOBAL_STATE_IDLE           1
#define NN_GLOBAL_STATE_ACTIVE         2
#define NN_GLOBAL_STATE_STOPPING_TIMER 3

/*  We could put these in an external header file, but there really is
    need to.  We are the only thing that needs them. */
extern struct nn_socktype nn_pair_socktype;
extern struct nn_socktype nn_xpair_socktype;
extern struct nn_socktype nn_pub_socktype;
extern struct nn_socktype nn_sub_socktype;
extern struct nn_socktype nn_xpub_socktype;
extern struct nn_socktype nn_xsub_socktype;
extern struct nn_socktype nn_rep_socktype;
extern struct nn_socktype nn_req_socktype;
extern struct nn_socktype nn_xrep_socktype;
extern struct nn_socktype nn_xreq_socktype;
extern struct nn_socktype nn_push_socktype;
extern struct nn_socktype nn_xpush_socktype;
extern struct nn_socktype nn_pull_socktype;
extern struct nn_socktype nn_xpull_socktype;
extern struct nn_socktype nn_respondent_socktype;
extern struct nn_socktype nn_surveyor_socktype;
extern struct nn_socktype nn_xrespondent_socktype;
extern struct nn_socktype nn_xsurveyor_socktype;
extern struct nn_socktype nn_bus_socktype;
extern struct nn_socktype nn_xbus_socktype;

/*  Array of known socket types. */
const struct nn_socktype *nn_socktypes[] = {
    &nn_pair_socktype,
    &nn_xpair_socktype,
    &nn_pub_socktype,
    &nn_sub_socktype,
    &nn_xpub_socktype,
    &nn_xsub_socktype,
    &nn_rep_socktype,
    &nn_req_socktype,
    &nn_xrep_socktype,
    &nn_xreq_socktype,
    &nn_push_socktype,
    &nn_xpush_socktype,
    &nn_pull_socktype,
    &nn_xpull_socktype,
    &nn_respondent_socktype,
    &nn_surveyor_socktype,
    &nn_xrespondent_socktype,
    &nn_xsurveyor_socktype,
    &nn_bus_socktype,
    &nn_xbus_socktype,
    NULL,
};

/* As with protocols, we could have these in a header file, but we are the
   only consumer, so just declare them inline. */

extern struct nn_transport nn_inproc;
extern struct nn_transport nn_ipc;
extern struct nn_transport nn_tcp;
extern struct nn_transport nn_ws;

const struct nn_transport *nn_transports[] = {
    &nn_inproc,
    &nn_ipc,
    &nn_tcp,
    &nn_ws,
    NULL,
};

struct nn_global {

    /*  The global table of existing sockets. The descriptor representing
        the socket is the index to this table. This pointer is also used to
        find out whether context is initialised. If it is NULL, context is
        uninitialised. */
    struct nn_sock **socks;

    /*  Stack of unused file descriptors. */
    uint16_t *unused;

    /*  Number of actual open sockets in the socket table. */
    size_t nsocks;

    /*  Combination of the flags listed above. */
    int flags;

    /*  Pool of worker threads. */
    struct nn_pool pool;

    /*  Timer and other machinery for submitting statistics  */
    int state;

    int print_errors;

    int inited;
    nn_mutex_t lock;
    nn_condvar_t cond;
};

/*  Singleton object containing the global state of the library. */
static struct nn_global self;
static nn_once_t once = NN_ONCE_INITIALIZER;


/*  Context creation- and termination-related private functions. */
static void nn_global_init (void);
static void nn_global_term (void);

/*  Private function that unifies nn_bind and nn_connect functionality.
    It returns the ID of the newly created endpoint. */
static int nn_global_create_ep (struct nn_sock *, const char *addr, int bind);

/*  Private socket creator which doesn't initialize global state and
    does no locking by itself */
static int nn_global_create_socket (int domain, int protocol);

/*  Socket holds. */
static int nn_global_hold_socket (struct nn_sock **sockp, int s);
static int nn_global_hold_socket_locked (struct nn_sock **sockp, int s);
static void nn_global_rele_socket(struct nn_sock *);

int nn_errno (void)
{
    return nn_err_errno ();
}

const char *nn_strerror (int errnum)
{
    return nn_err_strerror (errnum);
}

static void nn_global_init (void)
{
    int i;
    char *envvar;

#if defined NN_HAVE_WINDOWS
    int rc;
    WSADATA data;
#endif
    const struct nn_transport *tp;

    /*  Check whether the library was already initialised. If so, do nothing. */
    if (self.socks)
        return;

    /*  On Windows, initialise the socket library. */
#if defined NN_HAVE_WINDOWS
    rc = WSAStartup (MAKEWORD (2, 2), &data);
    nn_assert (rc == 0);
    nn_assert (LOBYTE (data.wVersion) == 2 &&
        HIBYTE (data.wVersion) == 2);
#endif

    /*  Initialise the memory allocation subsystem. */
    nn_alloc_init ();

    /*  Seed the pseudo-random number generator. */
    nn_random_seed ();

    /*  Allocate the global table of SP sockets. */
    self.socks = nn_alloc ((sizeof (struct nn_sock*) * NN_MAX_SOCKETS) +
        (sizeof (uint16_t) * NN_MAX_SOCKETS), "socket table");
    alloc_assert (self.socks);
    for (i = 0; i != NN_MAX_SOCKETS; ++i)
        self.socks [i] = NULL;
    self.nsocks = 0;
    self.flags = 0;

    /*  Print connection and accepting errors to the stderr  */
    envvar = getenv("NN_PRINT_ERRORS");
    /*  any non-empty string is true */
    self.print_errors = envvar && *envvar;

    /*  Allocate the stack of unused file descriptors. */
    self.unused = (uint16_t*) (self.socks + NN_MAX_SOCKETS);
    alloc_assert (self.unused);
    for (i = 0; i != NN_MAX_SOCKETS; ++i)
        self.unused [i] = NN_MAX_SOCKETS - i - 1;

    /*  Initialize transports if needed. */
    for (i = 0; (tp = nn_transports[i]) != NULL; i++) {
        if (tp->init != NULL) {
            tp->init ();
        }
    }

    /*  Start the worker threads. */
    nn_pool_init (&self.pool);
}

static void nn_global_term (void)
{
#if defined NN_HAVE_WINDOWS
    int rc;
#endif
    const struct nn_transport *tp;
    int i;

    /*  If there are no sockets remaining, uninitialise the global context. */
    nn_assert (self.socks);
    if (self.nsocks > 0)
        return;

    /*  Shut down the worker threads. */
    nn_pool_term (&self.pool);

    /*  Ask all the transport to deallocate their global resources. */
    for (i = 0; (tp = nn_transports[i]) != NULL; i++) {
        if (tp->term)
            tp->term ();
    }

    /*  Final deallocation of the nn_global object itself. */
    nn_free (self.socks);

    /*  This marks the global state as uninitialised. */
    self.socks = NULL;

    /*  Shut down the memory allocation subsystem. */
    nn_alloc_term ();

    /*  On Windows, uninitialise the socket library. */
#if defined NN_HAVE_WINDOWS
    rc = WSACleanup ();
    nn_assert (rc == 0);
#endif
}

void nn_term (void)
{
    int i;

    if (!self.inited) {
        return;
    }

    nn_mutex_lock (&self.lock);
    self.flags |= NN_CTX_FLAG_TERMING;
    nn_mutex_unlock (&self.lock);

    /* Make sure we really close resources, this will cause global
       resources to be freed too when the last socket is closed. */
    for (i = 0; i < NN_MAX_SOCKETS; i++) {
        (void) nn_close (i);
    }

    nn_mutex_lock (&self.lock);
    self.flags |= NN_CTX_FLAG_TERMED;
    self.flags &= ~NN_CTX_FLAG_TERMING;
    nn_condvar_broadcast(&self.cond);
    nn_mutex_unlock (&self.lock);
}

static void nn_lib_init(void)
{
    /*  This function is executed once to initialize global locks. */
    nn_mutex_init (&self.lock);
    nn_condvar_init (&self.cond);
    self.inited = 1;
}

void nn_init (void)
{
    nn_do_once (&once, nn_lib_init);

    nn_mutex_lock (&self.lock);
    /*  Wait for any in progress term to complete. */
    while (self.flags & NN_CTX_FLAG_TERMING) {
        nn_condvar_wait (&self.cond, &self.lock, -1);
    }
    self.flags &= ~NN_CTX_FLAG_TERMED;
    nn_mutex_unlock (&self.lock);
}

void *nn_allocmsg (size_t size, int type)
{
    int rc;
    void *result;

    rc = nn_chunk_alloc (size, type, &result);
    if (rc == 0)
        return result;
    errno = -rc;
    return NULL;
}

void *nn_reallocmsg (void *msg, size_t size)
{
    int rc;

    rc = nn_chunk_realloc (size, &msg);
    if (rc == 0)
        return msg;
    errno = -rc;
    return NULL;
}

int nn_freemsg (void *msg)
{
    nn_chunk_free (msg);
    return 0;
}

struct nn_cmsghdr *nn_cmsg_nxthdr_ (const struct nn_msghdr *mhdr,
    const struct nn_cmsghdr *cmsg)
{
    char *data;
    size_t sz;
    struct nn_cmsghdr *next;
    size_t headsz;

    /*  Early return if no message is provided. */
    if (nn_slow (mhdr == NULL))
        return NULL;

    /*  Get the actual data. */
    if (mhdr->msg_controllen == NN_MSG) {
        data = *((void**) mhdr->msg_control);
        sz = nn_chunk_size (data);
    }
    else {
        data = (char*) mhdr->msg_control;
        sz = mhdr->msg_controllen;
    }

    /*  Ancillary data allocation was not even large enough for one element. */
    if (nn_slow (sz < NN_CMSG_SPACE (0)))
        return NULL;

    /*  If cmsg is set to NULL we are going to return first property.
        Otherwise move to the next property. */
    if (!cmsg)
        next = (struct nn_cmsghdr*) data;
    else
        next = (struct nn_cmsghdr*)
            (((char*) cmsg) + NN_CMSG_ALIGN_ (cmsg->cmsg_len));

    /*  If there's no space for next property, treat it as the end
        of the property list. */
    headsz = ((char*) next) - data;
    if (headsz + NN_CMSG_SPACE (0) > sz ||
          headsz + NN_CMSG_ALIGN_ (next->cmsg_len) > sz)
        return NULL;

    /*  Success. */
    return next;
}

int nn_global_create_socket (int domain, int protocol)
{
    int rc;
    int s;
    int i;
    const struct nn_socktype *socktype;
    struct nn_sock *sock;

    /* The function is called with lock held */

    /*  Only AF_SP and AF_SP_RAW domains are supported. */
    if (domain != AF_SP && domain != AF_SP_RAW) {
        return -EAFNOSUPPORT;
    }

    /*  If socket limit was reached, report error. */
    if (self.nsocks >= NN_MAX_SOCKETS) {
        return -EMFILE;
    }

    /*  Find an empty socket slot. */
    s = self.unused [NN_MAX_SOCKETS - self.nsocks - 1];

    /*  Find the appropriate socket type. */
    for (i = 0; (socktype = nn_socktypes[i]) != NULL; i++) {
        if (socktype->domain == domain && socktype->protocol == protocol) {

            /*  Instantiate the socket. */
            if ((sock = nn_alloc (sizeof (struct nn_sock), "sock")) == NULL)
                return -ENOMEM;
            rc = nn_sock_init (sock, socktype, s);
            if (rc < 0) {
                nn_free (sock);
                return rc;
            }

            /*  Adjust the global socket table. */
            self.socks [s] = sock;
            ++self.nsocks;
            return s;
        }
    }
    /*  Specified socket type wasn't found. */
    return -EINVAL;
}

int nn_socket (int domain, int protocol)
{
    int rc;

    nn_do_once (&once, nn_lib_init);

    nn_mutex_lock (&self.lock);

    /*  If nn_term() was already called, return ETERM. */
    if (nn_slow (self.flags & NN_CTX_FLAG_TERM)) {
        nn_mutex_unlock (&self.lock);
        errno = ETERM;
        return -1;
    }

    /*  Make sure that global state is initialised. */
    nn_global_init ();

    rc = nn_global_create_socket (domain, protocol);

    if (rc < 0) {
        nn_global_term ();
        nn_mutex_unlock (&self.lock);
        errno = -rc;
        return -1;
    }

    nn_mutex_unlock (&self.lock);

    return rc;
}

int nn_close (int s)
{
    int rc;
    struct nn_sock *sock;

    nn_mutex_lock (&self.lock);
    rc = nn_global_hold_socket_locked (&sock, s);
    if (nn_slow (rc < 0)) {
        nn_mutex_unlock (&self.lock);
        errno = -rc;
        return -1;
    }

    /*  Start the shutdown process on the socket.  This will cause
        all other socket users, as well as endpoints, to begin cleaning up.
        This is done with the lock held to ensure that two instances
        of nn_close can't access the same socket. */
    nn_sock_stop (sock);

    /*  We have to drop both the hold we just acquired, as well as
        the original hold, in order for nn_sock_term to complete. */
    nn_sock_rele (sock);
    nn_sock_rele (sock);
    nn_mutex_unlock (&self.lock);

    /*  Now clean up.  The termination routine below will block until
        all other consumers of the socket have dropped their holds, and
        all endpoints have cleanly exited. */
    rc = nn_sock_term (sock);
    if (nn_slow (rc == -EINTR)) {
        nn_global_rele_socket (sock);
        errno = EINTR;
        return -1;
    }

    /*  Remove the socket from the socket table, add it to unused socket
        table. */
    nn_mutex_lock (&self.lock);
    self.socks [s] = NULL;
    self.unused [NN_MAX_SOCKETS - self.nsocks] = s;
    --self.nsocks;
    nn_free (sock);

    /*  Destroy the global context if there's no socket remaining. */
    nn_global_term ();

    nn_mutex_unlock (&self.lock);

    return 0;
}

int nn_setsockopt (int s, int level, int option, const void *optval,
    size_t optvallen)
{
    int rc;
    struct nn_sock *sock;

    rc = nn_global_hold_socket (&sock, s);
    if (nn_slow (rc < 0)) {
        errno = -rc;
        return -1;
    }

    if (nn_slow (!optval && optvallen)) {
        rc = -EFAULT;
        goto fail;
    }

    rc = nn_sock_setopt (sock, level, option, optval, optvallen);
    if (nn_slow (rc < 0))
        goto fail;
    errnum_assert (rc == 0, -rc);
    nn_global_rele_socket (sock);
    return 0;

fail:
    nn_global_rele_socket (sock);
    errno = -rc;
    return -1;
}

int nn_getsockopt (int s, int level, int option, void *optval,
    size_t *optvallen)
{
    int rc;
    struct nn_sock *sock;

    rc = nn_global_hold_socket (&sock, s);
    if (nn_slow (rc < 0)) {
        errno = -rc;
        return -1;
    }

    if (nn_slow (!optval && optvallen)) {
        rc = -EFAULT;
        goto fail;
    }

    rc = nn_sock_getopt (sock, level, option, optval, optvallen);
    if (nn_slow (rc < 0))
        goto fail;
    errnum_assert (rc == 0, -rc);
    nn_global_rele_socket (sock);
    return 0;

fail:
    nn_global_rele_socket (sock);
    errno = -rc;
    return -1;
}

int nn_bind (int s, const char *addr)
{
    int rc;
    struct nn_sock *sock;

    rc = nn_global_hold_socket (&sock, s);
    if (rc < 0) {
        errno = -rc;
        return -1;
    }

    rc = nn_global_create_ep (sock, addr, 1);
    if (nn_slow (rc < 0)) {
        nn_global_rele_socket (sock);
        errno = -rc;
        return -1;
    }

    nn_global_rele_socket (sock);
    return rc;
}

int nn_connect (int s, const char *addr)
{
    int rc;
    struct nn_sock *sock;

    rc = nn_global_hold_socket (&sock, s);
    if (nn_slow (rc < 0)) {
        errno = -rc;
        return -1;
    }

    rc = nn_global_create_ep (sock, addr, 0);
    if (rc < 0) {
        nn_global_rele_socket (sock);
        errno = -rc;
        return -1;
    }

    nn_global_rele_socket (sock);
    return rc;
}

int nn_shutdown (int s, int how)
{
    int rc;
    struct nn_sock *sock;

    rc = nn_global_hold_socket (&sock, s);
    if (nn_slow (rc < 0)) {
        errno = -rc;
        return -1;
    }

    rc = nn_sock_rm_ep (sock, how);
    if (nn_slow (rc < 0)) {
        nn_global_rele_socket (sock);
        errno = -rc;
        return -1;
    }
    nn_assert (rc == 0);

    nn_global_rele_socket (sock);
    return 0;
}

int nn_send (int s, const void *buf, size_t len, int flags)
{
    struct nn_iovec iov;
    struct nn_msghdr hdr;

    iov.iov_base = (void*) buf;
    iov.iov_len = len;

    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;
    hdr.msg_control = NULL;
    hdr.msg_controllen = 0;

    return nn_sendmsg (s, &hdr, flags);
}

int nn_recv (int s, void *buf, size_t len, int flags)
{
    struct nn_iovec iov;
    struct nn_msghdr hdr;

    iov.iov_base = buf;
    iov.iov_len = len;

    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;
    hdr.msg_control = NULL;
    hdr.msg_controllen = 0;

    return nn_recvmsg (s, &hdr, flags);
}

int nn_sendmsg (int s, const struct nn_msghdr *msghdr, int flags)
{
    int rc;
    size_t sz;
    size_t spsz;
    int i;
    struct nn_iovec *iov;
    struct nn_msg msg;
    void *chunk;
    int nnmsg;
    struct nn_cmsghdr *cmsg;
    struct nn_sock *sock;

    rc = nn_global_hold_socket (&sock, s);
    if (nn_slow (rc < 0)) {
        errno = -rc;
        return -1;
    }

    if (nn_slow (!msghdr)) {
        rc = -EINVAL;
        goto fail;
    }

    if (nn_slow (msghdr->msg_iovlen < 0)) {
        rc = -EMSGSIZE;
        goto fail;
    }

    if (msghdr->msg_iovlen == 1 && msghdr->msg_iov [0].iov_len == NN_MSG) {
        chunk = *(void**) msghdr->msg_iov [0].iov_base;
        if (nn_slow (chunk == NULL)) {
            rc = -EFAULT;
            goto fail;
        }
        sz = nn_chunk_size (chunk);
        nn_msg_init_chunk (&msg, chunk);
        nnmsg = 1;
    }
    else {

        /*  Compute the total size of the message. */
        sz = 0;
        for (i = 0; i != msghdr->msg_iovlen; ++i) {
            iov = &msghdr->msg_iov [i];
            if (nn_slow (iov->iov_len == NN_MSG)) {
               rc = -EINVAL;
               goto fail;
            }
            if (nn_slow (!iov->iov_base && iov->iov_len)) {
                rc = -EFAULT;
                goto fail;
            }
            if (nn_slow (sz + iov->iov_len < sz)) {
                rc = -EINVAL;
                goto fail;
            }
            sz += iov->iov_len;
        }

        /*  Create a message object from the supplied scatter array. */
        nn_msg_init (&msg, sz);
        sz = 0;
        for (i = 0; i != msghdr->msg_iovlen; ++i) {
            iov = &msghdr->msg_iov [i];
            memcpy (((uint8_t*) nn_chunkref_data (&msg.body)) + sz,
                iov->iov_base, iov->iov_len);
            sz += iov->iov_len;
        }

        nnmsg = 0;
    }

    /*  Add ancillary data to the message. */
    if (msghdr->msg_control) {

        /*  Copy all headers. */
        /*  TODO: SP_HDR should not be copied here! */
        if (msghdr->msg_controllen == NN_MSG) {
            chunk = *((void**) msghdr->msg_control);
            nn_chunkref_term (&msg.hdrs);
            nn_chunkref_init_chunk (&msg.hdrs, chunk);
        }
        else {
            nn_chunkref_term (&msg.hdrs);
            nn_chunkref_init (&msg.hdrs, msghdr->msg_controllen);
            memcpy (nn_chunkref_data (&msg.hdrs),
                msghdr->msg_control, msghdr->msg_controllen);
        }

        /* Search for SP_HDR property. */
        cmsg = NN_CMSG_FIRSTHDR (msghdr);
        while (cmsg) {
            if (cmsg->cmsg_level == PROTO_SP && cmsg->cmsg_type == SP_HDR) {
                unsigned char *ptr = NN_CMSG_DATA (cmsg);
                size_t clen = cmsg->cmsg_len - NN_CMSG_SPACE (0);
                if (clen > sizeof (size_t)) {
                    spsz = *(size_t *)(void *)ptr;
                    if (spsz <= (clen - sizeof (size_t))) {
                        /*  Copy body of SP_HDR property into 'sphdr'. */
                        nn_chunkref_term (&msg.sphdr);
                        nn_chunkref_init (&msg.sphdr, spsz);
                         memcpy (nn_chunkref_data (&msg.sphdr),
                             ptr + sizeof (size_t), spsz);
                    }
                }
                break;
            }
            cmsg = NN_CMSG_NXTHDR (msghdr, cmsg);
        }
    }

    /*  Send it further down the stack. */
    rc = nn_sock_send (sock, &msg, flags);
    if (nn_slow (rc < 0)) {

        /*  If we are dealing with user-supplied buffer, detach it from
            the message object. */
        if (nnmsg)
            nn_chunkref_init (&msg.body, 0);

        nn_msg_term (&msg);
        goto fail;
    }

    /*  Adjust the statistics. */
    nn_sock_stat_increment (sock, NN_STAT_MESSAGES_SENT, 1);
    nn_sock_stat_increment (sock, NN_STAT_BYTES_SENT, sz);

    nn_global_rele_socket (sock);

    return (int) sz;

fail:
    nn_global_rele_socket (sock);

    errno = -rc;
    return -1;
}

int nn_recvmsg (int s, struct nn_msghdr *msghdr, int flags)
{
    int rc;
    struct nn_msg msg;
    uint8_t *data;
    size_t sz;
    int i;
    struct nn_iovec *iov;
    void *chunk;
    size_t hdrssz;
    void *ctrl;
    size_t ctrlsz;
    size_t spsz;
    size_t sptotalsz;
    struct nn_cmsghdr *chdr;
    struct nn_sock *sock;

    rc = nn_global_hold_socket (&sock, s);
    if (nn_slow (rc < 0)) {
        errno = -rc;
        return -1;
    }

    if (nn_slow (!msghdr)) {
        rc = -EINVAL;
        goto fail;
    }

    if (nn_slow (msghdr->msg_iovlen < 0)) {
        rc = -EMSGSIZE;
        goto fail;
    }

    /*  Get a message. */
    rc = nn_sock_recv (sock, &msg, flags);
    if (nn_slow (rc < 0)) {
        goto fail;
    }

    if (msghdr->msg_iovlen == 1 && msghdr->msg_iov [0].iov_len == NN_MSG) {
        chunk = nn_chunkref_getchunk (&msg.body);
        *(void**) (msghdr->msg_iov [0].iov_base) = chunk;
        sz = nn_chunk_size (chunk);
    }
    else {

        /*  Copy the message content into the supplied gather array. */
        data = nn_chunkref_data (&msg.body);
        sz = nn_chunkref_size (&msg.body);
        for (i = 0; i != msghdr->msg_iovlen; ++i) {
            iov = &msghdr->msg_iov [i];
            if (nn_slow (iov->iov_len == NN_MSG)) {
                nn_msg_term (&msg);
                rc = -EINVAL;
                goto fail;
            }
            if (iov->iov_len > sz) {
                memcpy (iov->iov_base, data, sz);
                break;
            }
            memcpy (iov->iov_base, data, iov->iov_len);
            data += iov->iov_len;
            sz -= iov->iov_len;
        }
        sz = nn_chunkref_size (&msg.body);
    }

    /*  Retrieve the ancillary data from the message. */
    if (msghdr->msg_control) {

        spsz = nn_chunkref_size (&msg.sphdr);
        sptotalsz = NN_CMSG_SPACE (spsz+sizeof (size_t));
        ctrlsz = sptotalsz + nn_chunkref_size (&msg.hdrs);

        if (msghdr->msg_controllen == NN_MSG) {

            /* Allocate the buffer. */
            rc = nn_chunk_alloc (ctrlsz, 0, &ctrl);
            errnum_assert (rc == 0, -rc);

            /* Set output parameters. */
            *((void**) msghdr->msg_control) = ctrl;
        }
        else {

            /* Just use the buffer supplied by the user. */
            ctrl = msghdr->msg_control;
            ctrlsz = msghdr->msg_controllen;
        }

        /* If SP header alone won't fit into the buffer, return no ancillary
           properties. */
        if (ctrlsz >= sptotalsz) {
            char *ptr;

            /*  Fill in SP_HDR ancillary property. */
            chdr = (struct nn_cmsghdr*) ctrl;
            chdr->cmsg_len = sptotalsz;
            chdr->cmsg_level = PROTO_SP;
            chdr->cmsg_type = SP_HDR;
            ptr = (void *)chdr;
            ptr += sizeof (*chdr);
            *(size_t *)(void *)ptr = spsz;
            ptr += sizeof (size_t);
            memcpy (ptr, nn_chunkref_data (&msg.sphdr), spsz);

            /*  Fill in as many remaining properties as possible.
                Truncate the trailing properties if necessary. */
            hdrssz = nn_chunkref_size (&msg.hdrs);
            if (hdrssz > ctrlsz - sptotalsz)
                hdrssz = ctrlsz - sptotalsz;
            memcpy (((char*) ctrl) + sptotalsz,
                nn_chunkref_data (&msg.hdrs), hdrssz);
        }
    }

    nn_msg_term (&msg);

    /*  Adjust the statistics. */
    nn_sock_stat_increment (sock, NN_STAT_MESSAGES_RECEIVED, 1);
    nn_sock_stat_increment (sock, NN_STAT_BYTES_RECEIVED, sz);

    nn_global_rele_socket (sock);

    return (int) sz;

fail:
    nn_global_rele_socket (sock);

    errno = -rc;
    return -1;
}

uint64_t nn_get_statistic (int s, int statistic)
{
    int rc;
    struct nn_sock *sock;
    uint64_t val;

    rc = nn_global_hold_socket (&sock, s);
    if (nn_slow (rc < 0)) {
        errno = -rc;
        return (uint64_t)-1;
    }

    switch (statistic) {
    case NN_STAT_ESTABLISHED_CONNECTIONS:
        val = sock->statistics.established_connections;
        break;
    case NN_STAT_ACCEPTED_CONNECTIONS:
        val = sock->statistics.accepted_connections;
        break;
    case NN_STAT_DROPPED_CONNECTIONS:
        val = sock->statistics.dropped_connections;
        break;
    case NN_STAT_BROKEN_CONNECTIONS:
        val = sock->statistics.broken_connections;
        break;
    case NN_STAT_CONNECT_ERRORS:
        val = sock->statistics.connect_errors;
        break;
    case NN_STAT_BIND_ERRORS:
        val = sock->statistics.bind_errors;
        break;
    case NN_STAT_ACCEPT_ERRORS:
        val = sock->statistics.bind_errors;
        break;
    case NN_STAT_MESSAGES_SENT:
        val = sock->statistics.messages_sent;
        break;
    case NN_STAT_MESSAGES_RECEIVED:
        val = sock->statistics.messages_received;
        break;
    case NN_STAT_BYTES_SENT:
        val = sock->statistics.bytes_sent;
        break;
    case NN_STAT_BYTES_RECEIVED:
        val = sock->statistics.bytes_received;
        break;
    case NN_STAT_CURRENT_CONNECTIONS:
        val = sock->statistics.current_connections;
        break;
    case NN_STAT_INPROGRESS_CONNECTIONS:
        val = sock->statistics.inprogress_connections;
        break;
    case NN_STAT_CURRENT_SND_PRIORITY:
        val = sock->statistics.current_snd_priority;
        break;
    case NN_STAT_CURRENT_EP_ERRORS:
        val = sock->statistics.current_ep_errors;
        break;
    default:
        val = (uint64_t)-1;
        errno = EINVAL;
        break;
    }

    nn_global_rele_socket (sock);
    return val;
}

static int nn_global_create_ep (struct nn_sock *sock, const char *addr,
    int bind)
{
    int rc;
    const char *proto;
    const char *delim;
    size_t protosz;
    const struct nn_transport *tp;
    int i;

    /*  Check whether address is valid. */
    if (!addr)
        return -EINVAL;
    if (strlen (addr) >= NN_SOCKADDR_MAX)
        return -ENAMETOOLONG;

    /*  Separate the protocol and the actual address. */
    proto = addr;
    delim = strchr (addr, ':');
    if (!delim)
        return -EINVAL;
    if (delim [1] != '/' || delim [2] != '/')
        return -EINVAL;
    protosz = delim - addr;
    addr += protosz + 3;

    /*  Find the specified protocol. */
    tp = NULL;
    for (i = 0; ((tp = nn_transports[i]) != NULL); i++) {
        if (strlen (tp->name) == protosz &&
              memcmp (tp->name, proto, protosz) == 0)
            break;
    }

    /*  The protocol specified doesn't match any known protocol. */
    if (tp == NULL) {
        return -EPROTONOSUPPORT;
    }

    /*  Ask the socket to create the endpoint. */
    rc = nn_sock_add_ep (sock, tp, bind, addr);
    return rc;
}

const struct nn_transport *nn_global_transport (int id)
{
    const struct nn_transport *tp;
    int i;

    for (i = 0; (tp = nn_transports[i]) != NULL; i++) {
        if (tp->id == id)
            return tp;
    }
    return NULL;
}

struct nn_pool *nn_global_getpool ()
{
    return &self.pool;
}

int nn_global_print_errors ()
{
    return self.print_errors;
}

/*  Get the socket structure for a socket id.  This must be called under
    the global lock (self.lock.)  The socket itself will not be freed
    while the hold is active. */
int nn_global_hold_socket_locked(struct nn_sock **sockp, int s)
{
    struct nn_sock *sock;

    if (nn_slow (s < 0 || s >= NN_MAX_SOCKETS || self.socks == NULL))
        return -EBADF;

    sock = self.socks[s];
    if (nn_slow (sock == NULL)) {
        return -EBADF;
    }

    if (nn_slow (nn_sock_hold (sock) != 0)) {
        return -EBADF;
    }
    *sockp = sock;
    return 0;
}

int nn_global_hold_socket(struct nn_sock **sockp, int s)
{
    int rc;
    nn_mutex_lock(&self.lock);
    rc = nn_global_hold_socket_locked(sockp, s);
    nn_mutex_unlock(&self.lock);
    return rc;
}

void nn_global_rele_socket(struct nn_sock *sock)
{
    nn_mutex_lock(&self.lock);
    nn_sock_rele(sock);
    nn_mutex_unlock(&self.lock);
}
