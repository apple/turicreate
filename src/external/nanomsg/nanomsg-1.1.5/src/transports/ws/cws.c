/*
    Copyright (c) 2012-2013 250bpm s.r.o.  All rights reserved.
    Copyright (c) 2014-2016 Jack R. Dunaway. All rights reserved.
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

#include "cws.h"
#include "sws.h"

#include "../../ws.h"

#include "../utils/dns.h"
#include "../utils/port.h"
#include "../utils/iface.h"
#include "../utils/backoff.h"
#include "../utils/literal.h"

#include "../../aio/fsm.h"
#include "../../aio/usock.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/alloc.h"
#include "../../utils/fast.h"
#include "../../utils/attr.h"

#include <string.h>

#if defined NN_HAVE_WINDOWS
#include "../../utils/win.h"
#else
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#define NN_CWS_STATE_IDLE 1
#define NN_CWS_STATE_RESOLVING 2
#define NN_CWS_STATE_STOPPING_DNS 3
#define NN_CWS_STATE_CONNECTING 4
#define NN_CWS_STATE_ACTIVE 5
#define NN_CWS_STATE_STOPPING_SWS 6
#define NN_CWS_STATE_STOPPING_USOCK 7
#define NN_CWS_STATE_WAITING 8
#define NN_CWS_STATE_STOPPING_BACKOFF 9
#define NN_CWS_STATE_STOPPING_SWS_FINAL 10
#define NN_CWS_STATE_STOPPING 11

#define NN_CWS_SRC_USOCK 1
#define NN_CWS_SRC_RECONNECT_TIMER 2
#define NN_CWS_SRC_DNS 3
#define NN_CWS_SRC_SWS 4

struct nn_cws {

    /*  The state machine. */
    struct nn_fsm fsm;
    int state;

    struct nn_ep *ep;

    /*  The underlying WS socket. */
    struct nn_usock usock;

    /*  Used to wait before retrying to connect. */
    struct nn_backoff retry;

    /*  Defines message validation and framing. */
    uint8_t msg_type;

    /*  State machine that handles the active part of the connection
        lifetime. */
    struct nn_sws sws;

    /*  Parsed parts of the connection URI. */
    struct nn_chunkref resource;
    struct nn_chunkref remote_host;
    struct nn_chunkref nic;
    int remote_port;
    size_t remote_hostname_len;

    /*  If a close handshake is performed, this flag signals to not
        begin automatic reconnect retries. */
    int peer_gone;

    /*  DNS resolver used to convert textual address into actual IP address
        along with the variable to hold the result. */
    struct nn_dns dns;
    struct nn_dns_result dns_result;
};

/*  nn_ep virtual interface implementation. */
static void nn_cws_stop (void *);
static void nn_cws_destroy (void *);
const struct nn_ep_ops nn_cws_ep_ops = {
    nn_cws_stop,
    nn_cws_destroy
};

/*  Private functions. */
static void nn_cws_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_cws_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_cws_start_resolving (struct nn_cws *self);
static void nn_cws_start_connecting (struct nn_cws *self,
    struct sockaddr_storage *ss, size_t sslen);

int nn_cws_create (struct nn_ep *ep)
{
    int rc;
    const char *addr;
    size_t addrlen;
    const char *semicolon;
    const char *hostname;
    size_t hostlen;
    const char *colon;
    const char *slash;
    const char *resource;
    size_t resourcelen;
    struct sockaddr_storage ss;
    size_t sslen;
    int ipv4only;
    size_t ipv4onlylen;
    struct nn_cws *self;
    int reconnect_ivl;
    int reconnect_ivl_max;
    int msg_type;
    size_t sz;

    /*  Allocate the new endpoint object. */
    self = nn_alloc (sizeof (struct nn_cws), "cws");
    alloc_assert (self);
    self->ep = ep;
    self->peer_gone = 0;

    /*  Initalise the endpoint. */
    nn_ep_tran_setup (ep, &nn_cws_ep_ops, self);

    /*  Check whether IPv6 is to be used. */
    ipv4onlylen = sizeof (ipv4only);
    nn_ep_getopt (ep, NN_SOL_SOCKET, NN_IPV4ONLY, &ipv4only, &ipv4onlylen);
    nn_assert (ipv4onlylen == sizeof (ipv4only));

    /*  Start parsing the address. */
    addr = nn_ep_getaddr (ep);
    addrlen = strlen (addr);
    semicolon = strchr (addr, ';');
    hostname = semicolon ? semicolon + 1 : addr;
    colon = strrchr (addr, ':');
    slash = colon ? strchr (colon, '/') : strchr (addr, '/');
    resource = slash ? slash : addr + addrlen;
    self->remote_hostname_len = colon ? colon - hostname : resource - hostname;

    /*  Host contains both hostname and port. */
    hostlen = resource - hostname;

    /*  Parse the port; assume port 80 if not explicitly declared. */
    if (colon != NULL) {
        rc = nn_port_resolve (colon + 1, resource - colon - 1);
        if (rc < 0) {
            nn_free(self);
            return -EINVAL;
        }
        self->remote_port = rc;
    }
    else {
        self->remote_port = 80;
    }

    /*  Check whether the host portion of the address is either a literal
        or a valid hostname. */
    if (nn_dns_check_hostname (hostname, self->remote_hostname_len) < 0 &&
          nn_literal_resolve (hostname, self->remote_hostname_len, ipv4only,
          &ss, &sslen) < 0) {
        nn_free(self);
        return -EINVAL;
    }

    /*  If local address is specified, check whether it is valid. */
    if (semicolon) {
        rc = nn_iface_resolve (addr, semicolon - addr, ipv4only, &ss, &sslen);
        if (rc < 0) {
            nn_free(self);
            return -ENODEV;
        }
    }

    /*  At this point, the address is valid, so begin allocating resources. */
    nn_chunkref_init (&self->remote_host, hostlen + 1);
    memcpy (nn_chunkref_data (&self->remote_host), hostname, hostlen);
    ((uint8_t *) nn_chunkref_data (&self->remote_host)) [hostlen] = '\0';

    if (semicolon) {
        nn_chunkref_init (&self->nic, semicolon - addr);
        memcpy (nn_chunkref_data (&self->nic),
            addr, semicolon - addr);
    }
    else {
        nn_chunkref_init (&self->nic, 1);
        memcpy (nn_chunkref_data (&self->nic), "*", 1);
    }

    /*  The requested resource is used in opening handshake. */
    resourcelen = strlen (resource);
    if (resourcelen) {
        nn_chunkref_init (&self->resource, resourcelen + 1);
        strncpy (nn_chunkref_data (&self->resource),
            resource, resourcelen + 1);
    }
    else {
        /*  No resource specified, so allocate base path. */
        nn_chunkref_init (&self->resource, 2);
        strncpy (nn_chunkref_data (&self->resource), "/", 2);
    }

    /*  Initialise the structure. */
    nn_fsm_init_root (&self->fsm, nn_cws_handler, nn_cws_shutdown,
        nn_ep_getctx (ep));
    self->state = NN_CWS_STATE_IDLE;
    nn_usock_init (&self->usock, NN_CWS_SRC_USOCK, &self->fsm);

    sz = sizeof (msg_type);
    nn_ep_getopt (ep, NN_WS, NN_WS_MSG_TYPE, &msg_type, &sz);
    nn_assert (sz == sizeof (msg_type));
    self->msg_type = (uint8_t) msg_type;

    sz = sizeof (reconnect_ivl);
    nn_ep_getopt (ep, NN_SOL_SOCKET, NN_RECONNECT_IVL, &reconnect_ivl, &sz);
    nn_assert (sz == sizeof (reconnect_ivl));
    sz = sizeof (reconnect_ivl_max);
    nn_ep_getopt (ep, NN_SOL_SOCKET, NN_RECONNECT_IVL_MAX,
        &reconnect_ivl_max, &sz);
    nn_assert (sz == sizeof (reconnect_ivl_max));
    if (reconnect_ivl_max == 0)
        reconnect_ivl_max = reconnect_ivl;
    nn_backoff_init (&self->retry, NN_CWS_SRC_RECONNECT_TIMER,
        reconnect_ivl, reconnect_ivl_max, &self->fsm);

    nn_sws_init (&self->sws, NN_CWS_SRC_SWS, ep, &self->fsm);
    nn_dns_init (&self->dns, NN_CWS_SRC_DNS, &self->fsm);

    /*  Start the state machine. */
    nn_fsm_start (&self->fsm);

    return 0;
}

static void nn_cws_stop (void *self)
{
    struct nn_cws *cws = self;

    nn_fsm_stop (&cws->fsm);
}

static void nn_cws_destroy (void *self)
{
    struct nn_cws *cws = self;

    nn_chunkref_term (&cws->resource);
    nn_chunkref_term (&cws->remote_host);
    nn_chunkref_term (&cws->nic);
    nn_dns_term (&cws->dns);
    nn_sws_term (&cws->sws);
    nn_backoff_term (&cws->retry);
    nn_usock_term (&cws->usock);
    nn_fsm_term (&cws->fsm);

    nn_free (cws);
}

static void nn_cws_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_cws *cws;

    cws = nn_cont (self, struct nn_cws, fsm);

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {
        if (!nn_sws_isidle (&cws->sws)) {
            nn_ep_stat_increment (cws->ep, NN_STAT_DROPPED_CONNECTIONS, 1);
            nn_sws_stop (&cws->sws);
        }
        cws->state = NN_CWS_STATE_STOPPING_SWS_FINAL;
    }
    if (nn_slow (cws->state == NN_CWS_STATE_STOPPING_SWS_FINAL)) {
        if (!nn_sws_isidle (&cws->sws))
            return;
        nn_backoff_stop (&cws->retry);
        nn_usock_stop (&cws->usock);
        nn_dns_stop (&cws->dns);
        cws->state = NN_CWS_STATE_STOPPING;
    }
    if (nn_slow (cws->state == NN_CWS_STATE_STOPPING)) {
        if (!nn_backoff_isidle (&cws->retry) ||
              !nn_usock_isidle (&cws->usock) ||
              !nn_dns_isidle (&cws->dns))
            return;
        cws->state = NN_CWS_STATE_IDLE;
        nn_fsm_stopped_noevent (&cws->fsm);
        nn_ep_stopped (cws->ep);
        return;
    }

    nn_fsm_bad_state (cws->state, src, type);
}

static void nn_cws_handler (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_cws *cws;

    cws = nn_cont (self, struct nn_cws, fsm);

    switch (cws->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/*  The state machine wasn't yet started.                                     */
/******************************************************************************/
    case NN_CWS_STATE_IDLE:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_FSM_START:
                nn_cws_start_resolving (cws);
                return;
            default:
                nn_fsm_bad_action (cws->state, src, type);
            }

        default:
            nn_fsm_bad_source (cws->state, src, type);
        }

/******************************************************************************/
/*  RESOLVING state.                                                          */
/*  Name of the host to connect to is being resolved to get an IP address.    */
/******************************************************************************/
    case NN_CWS_STATE_RESOLVING:
        switch (src) {

        case NN_CWS_SRC_DNS:
            switch (type) {
            case NN_DNS_DONE:
                nn_dns_stop (&cws->dns);
                cws->state = NN_CWS_STATE_STOPPING_DNS;
                return;
            default:
                nn_fsm_bad_action (cws->state, src, type);
            }

        default:
            nn_fsm_bad_source (cws->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_DNS state.                                                       */
/*  dns object was asked to stop but it haven't stopped yet.                  */
/******************************************************************************/
    case NN_CWS_STATE_STOPPING_DNS:
        switch (src) {

        case NN_CWS_SRC_DNS:
            switch (type) {
            case NN_DNS_STOPPED:
                if (cws->dns_result.error == 0) {
                    nn_cws_start_connecting (cws, &cws->dns_result.addr,
                        cws->dns_result.addrlen);
                    return;
                }
                nn_backoff_start (&cws->retry);
                cws->state = NN_CWS_STATE_WAITING;
                return;
            default:
                nn_fsm_bad_action (cws->state, src, type);
            }

        default:
            nn_fsm_bad_source (cws->state, src, type);
        }

/******************************************************************************/
/*  CONNECTING state.                                                         */
/*  Non-blocking connect is under way.                                        */
/******************************************************************************/
    case NN_CWS_STATE_CONNECTING:
        switch (src) {

        case NN_CWS_SRC_USOCK:
            switch (type) {
            case NN_USOCK_CONNECTED:
                nn_sws_start (&cws->sws, &cws->usock, NN_WS_CLIENT,
                    nn_chunkref_data (&cws->resource),
                    nn_chunkref_data (&cws->remote_host), cws->msg_type);
                cws->state = NN_CWS_STATE_ACTIVE;
                cws->peer_gone = 0;
                nn_ep_stat_increment (cws->ep,
                    NN_STAT_INPROGRESS_CONNECTIONS, -1);
                nn_ep_stat_increment (cws->ep,
                    NN_STAT_ESTABLISHED_CONNECTIONS, 1);
                nn_ep_clear_error (cws->ep);
                return;
            case NN_USOCK_ERROR:
                nn_ep_set_error (cws->ep, nn_usock_geterrno (&cws->usock));
                nn_usock_stop (&cws->usock);
                cws->state = NN_CWS_STATE_STOPPING_USOCK;
                nn_ep_stat_increment (cws->ep,
                    NN_STAT_INPROGRESS_CONNECTIONS, -1);
                nn_ep_stat_increment (cws->ep, NN_STAT_CONNECT_ERRORS, 1);
                return;
            default:
                nn_fsm_bad_action (cws->state, src, type);
            }

        default:
            nn_fsm_bad_source (cws->state, src, type);
        }

/******************************************************************************/
/*  ACTIVE state.                                                             */
/*  Connection is established and handled by the sws state machine.           */
/******************************************************************************/
    case NN_CWS_STATE_ACTIVE:
        switch (src) {

        case NN_CWS_SRC_SWS:
            switch (type) {
            case NN_SWS_RETURN_CLOSE_HANDSHAKE:
                /*  Peer closed connection without intention to reconnect, or
                    local endpoint failed remote because of invalid data. */
                nn_sws_stop (&cws->sws);
                cws->state = NN_CWS_STATE_STOPPING_SWS;
                cws->peer_gone = 1;
                return;
            case NN_SWS_RETURN_ERROR:
                nn_sws_stop (&cws->sws);
                cws->state = NN_CWS_STATE_STOPPING_SWS;
                nn_ep_stat_increment (cws->ep, NN_STAT_BROKEN_CONNECTIONS, 1);
                return;
            default:
                nn_fsm_bad_action (cws->state, src, type);
            }

        default:
            nn_fsm_bad_source (cws->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_SWS state.                                                       */
/*  sws object was asked to stop but it haven't stopped yet.                  */
/******************************************************************************/
    case NN_CWS_STATE_STOPPING_SWS:
        switch (src) {

        case NN_CWS_SRC_SWS:
            switch (type) {
            case NN_USOCK_SHUTDOWN:
                return;
            case NN_SWS_RETURN_STOPPED:
                nn_usock_stop (&cws->usock);
                cws->state = NN_CWS_STATE_STOPPING_USOCK;
                return;
            default:
                nn_fsm_bad_action (cws->state, src, type);
            }

        default:
            nn_fsm_bad_source (cws->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_USOCK state.                                                     */
/*  usock object was asked to stop but it haven't stopped yet.                */
/******************************************************************************/
    case NN_CWS_STATE_STOPPING_USOCK:
        switch (src) {

        case NN_CWS_SRC_USOCK:
            switch (type) {
            case NN_USOCK_SHUTDOWN:
                return;
            case NN_USOCK_STOPPED:
                /*  If the peer has confirmed itself gone with a Closing
                    Handshake, or if the local endpoint failed the remote,
                    don't try to reconnect. */
                if (!cws->peer_gone) {
                    nn_backoff_start (&cws->retry);
                    cws->state = NN_CWS_STATE_WAITING;
                }
                return;
            default:
                nn_fsm_bad_action (cws->state, src, type);
            }

        default:
            nn_fsm_bad_source (cws->state, src, type);
        }

/******************************************************************************/
/*  WAITING state.                                                            */
/*  Waiting before re-connection is attempted. This way we won't overload     */
/*  the system by continuous re-connection attemps.                           */
/******************************************************************************/
    case NN_CWS_STATE_WAITING:
        switch (src) {

        case NN_CWS_SRC_RECONNECT_TIMER:
            switch (type) {
            case NN_BACKOFF_TIMEOUT:
                nn_backoff_stop (&cws->retry);
                cws->state = NN_CWS_STATE_STOPPING_BACKOFF;
                return;
            default:
                nn_fsm_bad_action (cws->state, src, type);
            }

        default:
            nn_fsm_bad_source (cws->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_BACKOFF state.                                                   */
/*  backoff object was asked to stop, but it haven't stopped yet.             */
/******************************************************************************/
    case NN_CWS_STATE_STOPPING_BACKOFF:
        switch (src) {

        case NN_CWS_SRC_RECONNECT_TIMER:
            switch (type) {
            case NN_BACKOFF_STOPPED:
                nn_cws_start_resolving (cws);
                return;
            default:
                nn_fsm_bad_action (cws->state, src, type);
            }

        default:
            nn_fsm_bad_source (cws->state, src, type);
        }

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (cws->state, src, type);
    }
}

/******************************************************************************/
/*  State machine actions.                                                    */
/******************************************************************************/

static void nn_cws_start_resolving (struct nn_cws *self)
{
    int ipv4only;
    size_t ipv4onlylen;
    char *host;

    /*  Check whether IPv6 is to be used. */
    ipv4onlylen = sizeof (ipv4only);
    nn_ep_getopt (self->ep, NN_SOL_SOCKET, NN_IPV4ONLY,
        &ipv4only, &ipv4onlylen);
    nn_assert (ipv4onlylen == sizeof (ipv4only));

    host = nn_chunkref_data (&self->remote_host);
    nn_assert (strlen (host) > 0);

    nn_dns_start (&self->dns, host, self->remote_hostname_len, ipv4only,
        &self->dns_result);

    self->state = NN_CWS_STATE_RESOLVING;
}

static void nn_cws_start_connecting (struct nn_cws *self,
    struct sockaddr_storage *ss, size_t sslen)
{
    int rc;
    struct sockaddr_storage remote;
    size_t remotelen;
    struct sockaddr_storage local;
    size_t locallen;
    int ipv4only;
    int val;
    size_t sz;

    memset (&remote, 0, sizeof (remote));
    memset (&local, 0, sizeof (local));

    /*  Check whether IPv6 is to be used. */
    sz = sizeof (ipv4only);
    nn_ep_getopt (self->ep, NN_SOL_SOCKET, NN_IPV4ONLY, &ipv4only, &sz);
    nn_assert (sz == sizeof (ipv4only));

    rc = nn_iface_resolve (nn_chunkref_data (&self->nic),
    nn_chunkref_size (&self->nic), ipv4only, &local, &locallen);

    if (nn_slow (rc < 0)) {
        nn_backoff_start (&self->retry);
        self->state = NN_CWS_STATE_WAITING;
        return;
    }

    /*  Combine the remote address and the port. */
    remote = *ss;
    remotelen = sslen;
    if (remote.ss_family == AF_INET)
        ((struct sockaddr_in*) &remote)->sin_port = htons (self->remote_port);
    else if (remote.ss_family == AF_INET6)
        ((struct sockaddr_in6*) &remote)->sin6_port = htons (self->remote_port);
    else
        nn_assert (0);

    /*  Try to start the underlying socket. */
    rc = nn_usock_start (&self->usock, remote.ss_family, SOCK_STREAM, 0);
    if (nn_slow (rc < 0)) {
        nn_backoff_start (&self->retry);
        self->state = NN_CWS_STATE_WAITING;
        return;
    }

    /*  Set the relevant socket options. */
    sz = sizeof (val);
    nn_ep_getopt (self->ep, NN_SOL_SOCKET, NN_SNDBUF, &val, &sz);
    nn_assert (sz == sizeof (val));
    nn_usock_setsockopt (&self->usock, SOL_SOCKET, SO_SNDBUF,
        &val, sizeof (val));
    sz = sizeof (val);
    nn_ep_getopt (self->ep, NN_SOL_SOCKET, NN_RCVBUF, &val, &sz);
    nn_assert (sz == sizeof (val));
    nn_usock_setsockopt (&self->usock, SOL_SOCKET, SO_RCVBUF,
        &val, sizeof (val));

    /*  Bind the socket to the local network interface. */
    rc = nn_usock_bind (&self->usock, (struct sockaddr*) &local, locallen);
    if (nn_slow (rc != 0)) {
        nn_backoff_start (&self->retry);
        self->state = NN_CWS_STATE_WAITING;
        return;
    }

    /*  Start connecting. */
    nn_usock_connect (&self->usock, (struct sockaddr*) &remote, remotelen);
    self->state = NN_CWS_STATE_CONNECTING;
    nn_ep_stat_increment (self->ep, NN_STAT_INPROGRESS_CONNECTIONS, 1);
}
