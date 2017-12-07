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

#include "btcp.h"
#include "atcp.h"

#include "../utils/port.h"
#include "../utils/iface.h"

#include "../../aio/fsm.h"
#include "../../aio/usock.h"

#include "../utils/backoff.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/alloc.h"
#include "../../utils/list.h"
#include "../../utils/fast.h"

#include <string.h>

#if defined NN_HAVE_WINDOWS
#include "../../utils/win.h"
#else
#include <unistd.h>
#include <netinet/in.h>
#endif

/*  The backlog is set relatively high so that there are not too many failed
    connection attemps during re-connection storms. */
#define NN_BTCP_BACKLOG 100

#define NN_BTCP_STATE_IDLE 1
#define NN_BTCP_STATE_ACTIVE 2
#define NN_BTCP_STATE_STOPPING_ATCP 3
#define NN_BTCP_STATE_STOPPING_USOCK 4
#define NN_BTCP_STATE_STOPPING_ATCPS 5

#define NN_BTCP_SRC_USOCK 1
#define NN_BTCP_SRC_ATCP 2

struct nn_btcp {

    /*  The state machine. */
    struct nn_fsm fsm;
    int state;

    /*  This object is a specific type of endpoint.
        Thus it is derived from epbase. */
    struct nn_epbase epbase;

    /*  The underlying listening TCP socket. */
    struct nn_usock usock;

    /*  The connection being accepted at the moment. */
    struct nn_atcp *atcp;

    /*  List of accepted connections. */
    struct nn_list atcps;
};

/*  nn_epbase virtual interface implementation. */
static void nn_btcp_stop (struct nn_epbase *self);
static void nn_btcp_destroy (struct nn_epbase *self);
const struct nn_epbase_vfptr nn_btcp_epbase_vfptr = {
    nn_btcp_stop,
    nn_btcp_destroy
};

/*  Private functions. */
static void nn_btcp_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_btcp_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);
static int nn_btcp_listen (struct nn_btcp *self);
static void nn_btcp_start_accepting (struct nn_btcp *self);

int nn_btcp_create (void *hint, struct nn_epbase **epbase)
{
    int rc;
    struct nn_btcp *self;
    const char *addr;
    const char *end;
    const char *pos;
    struct sockaddr_storage ss;
    size_t sslen;
    int ipv4only;
    size_t ipv4onlylen;

    /*  Allocate the new endpoint object. */
    self = nn_alloc (sizeof (struct nn_btcp), "btcp");
    alloc_assert (self);

    /*  Initalise the epbase. */
    nn_epbase_init (&self->epbase, &nn_btcp_epbase_vfptr, hint);
    addr = nn_epbase_getaddr (&self->epbase);

    /*  Parse the port. */
    end = addr + strlen (addr);
    pos = strrchr (addr, ':');
    if (nn_slow (!pos)) {
        nn_epbase_term (&self->epbase);
        return -EINVAL;
    }
    ++pos;
    rc = nn_port_resolve (pos, end - pos);
    if (nn_slow (rc < 0)) {
        nn_epbase_term (&self->epbase);
        return -EINVAL;
    }

    /*  Check whether IPv6 is to be used. */
    ipv4onlylen = sizeof (ipv4only);
    nn_epbase_getopt (&self->epbase, NN_SOL_SOCKET, NN_IPV4ONLY,
        &ipv4only, &ipv4onlylen);
    nn_assert (ipv4onlylen == sizeof (ipv4only));

    /*  Parse the address. */
    rc = nn_iface_resolve (addr, pos - addr - 1, ipv4only, &ss, &sslen);
    if (nn_slow (rc < 0)) {
        nn_epbase_term (&self->epbase);
        return -ENODEV;
    }

    /*  Initialise the structure. */
    nn_fsm_init_root (&self->fsm, nn_btcp_handler, nn_btcp_shutdown,
        nn_epbase_getctx (&self->epbase));
    self->state = NN_BTCP_STATE_IDLE;
    self->atcp = NULL;
    nn_list_init (&self->atcps);

    /*  Start the state machine. */
    nn_fsm_start (&self->fsm);

    nn_usock_init (&self->usock, NN_BTCP_SRC_USOCK, &self->fsm);

    rc = nn_btcp_listen (self);
    if (rc != 0) {
        nn_epbase_term (&self->epbase);
        return rc;
    }

    /*  Return the base class as an out parameter. */
    *epbase = &self->epbase;

    return 0;
}

static void nn_btcp_stop (struct nn_epbase *self)
{
    struct nn_btcp *btcp;

    btcp = nn_cont (self, struct nn_btcp, epbase);

    nn_fsm_stop (&btcp->fsm);
}

static void nn_btcp_destroy (struct nn_epbase *self)
{
    struct nn_btcp *btcp;

    btcp = nn_cont (self, struct nn_btcp, epbase);

    nn_assert_state (btcp, NN_BTCP_STATE_IDLE);
    nn_list_term (&btcp->atcps);
    nn_assert (btcp->atcp == NULL);
    nn_usock_term (&btcp->usock);
    nn_epbase_term (&btcp->epbase);
    nn_fsm_term (&btcp->fsm);

    nn_free (btcp);
}

static void nn_btcp_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr)
{
    struct nn_btcp *btcp;
    struct nn_list_item *it;
    struct nn_atcp *atcp;

    btcp = nn_cont (self, struct nn_btcp, fsm);

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {
        if (btcp->atcp) {
            nn_atcp_stop (btcp->atcp);
            btcp->state = NN_BTCP_STATE_STOPPING_ATCP;
        }
        else {
            btcp->state = NN_BTCP_STATE_STOPPING_USOCK;
        }
    }
    if (nn_slow (btcp->state == NN_BTCP_STATE_STOPPING_ATCP)) {
        if (!nn_atcp_isidle (btcp->atcp))
            return;
        nn_atcp_term (btcp->atcp);
        nn_free (btcp->atcp);
        btcp->atcp = NULL;
        nn_usock_stop (&btcp->usock);
        btcp->state = NN_BTCP_STATE_STOPPING_USOCK;
    }
    if (nn_slow (btcp->state == NN_BTCP_STATE_STOPPING_USOCK)) {
       if (!nn_usock_isidle (&btcp->usock))
            return;
        for (it = nn_list_begin (&btcp->atcps);
              it != nn_list_end (&btcp->atcps);
              it = nn_list_next (&btcp->atcps, it)) {
            atcp = nn_cont (it, struct nn_atcp, item);
            nn_atcp_stop (atcp);
        }
        btcp->state = NN_BTCP_STATE_STOPPING_ATCPS;
        goto atcps_stopping;
    }
    if (nn_slow (btcp->state == NN_BTCP_STATE_STOPPING_ATCPS)) {
        nn_assert (src == NN_BTCP_SRC_ATCP && type == NN_ATCP_STOPPED);
        atcp = (struct nn_atcp *) srcptr;
        nn_list_erase (&btcp->atcps, &atcp->item);
        nn_atcp_term (atcp);
        nn_free (atcp);

        /*  If there are no more atcp state machines, we can stop the whole
            btcp object. */
atcps_stopping:
        if (nn_list_empty (&btcp->atcps)) {
            btcp->state = NN_BTCP_STATE_IDLE;
            nn_fsm_stopped_noevent (&btcp->fsm);
            nn_epbase_stopped (&btcp->epbase);
            return;
        }

        return;
    }

    nn_fsm_bad_action(btcp->state, src, type);
}

static void nn_btcp_handler (struct nn_fsm *self, int src, int type,
    void *srcptr)
{
    struct nn_btcp *btcp;
    struct nn_atcp *atcp;

    btcp = nn_cont (self, struct nn_btcp, fsm);

    switch (btcp->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/******************************************************************************/
    case NN_BTCP_STATE_IDLE:
        nn_assert (src == NN_FSM_ACTION);
        nn_assert (type == NN_FSM_START);
        btcp->state = NN_BTCP_STATE_ACTIVE;
        return;

/******************************************************************************/
/*  ACTIVE state.                                                             */
/*  The execution is yielded to the atcp state machine in this state.         */
/******************************************************************************/
    case NN_BTCP_STATE_ACTIVE:
        if (src == NN_BTCP_SRC_USOCK) {
            /*  usock object cleaning up */
            nn_assert (type == NN_USOCK_SHUTDOWN || type == NN_USOCK_STOPPED);
            return;
        }

        /*  All other events come from child atcp objects. */
        nn_assert (src == NN_BTCP_SRC_ATCP);
        atcp = (struct nn_atcp*) srcptr;
        switch (type) {
        case NN_ATCP_ACCEPTED:
            nn_assert (btcp->atcp == atcp) ;
            nn_list_insert (&btcp->atcps, &atcp->item,
                nn_list_end (&btcp->atcps));
            btcp->atcp = NULL;
            nn_btcp_start_accepting (btcp);
            return;
        case NN_ATCP_ERROR:
            nn_atcp_stop (atcp);
            return;
        case NN_ATCP_STOPPED:
            nn_list_erase (&btcp->atcps, &atcp->item);
            nn_atcp_term (atcp);
            nn_free (atcp);
            return;
        default:
            nn_fsm_bad_action (btcp->state, src, type);
        }

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (btcp->state, src, type);
    }
}

static int nn_btcp_listen (struct nn_btcp *self)
{
    int rc;
    struct sockaddr_storage ss;
    size_t sslen;
    int ipv4only;
    size_t ipv4onlylen;
    const char *addr;
    const char *end;
    const char *pos;
    uint16_t port;

    /*  First, resolve the IP address. */
    addr = nn_epbase_getaddr (&self->epbase);
    memset (&ss, 0, sizeof (ss));

    /*  Parse the port. */
    end = addr + strlen (addr);
    pos = strrchr (addr, ':');
    if (pos == NULL) {
        return -EINVAL;
    }
    ++pos;
    rc = nn_port_resolve (pos, end - pos);
    if (rc <= 0)
        return rc;
    port = (uint16_t) rc;

    /*  Parse the address. */
    ipv4onlylen = sizeof (ipv4only);
    nn_epbase_getopt (&self->epbase, NN_SOL_SOCKET, NN_IPV4ONLY,
        &ipv4only, &ipv4onlylen);
    nn_assert (ipv4onlylen == sizeof (ipv4only));
    rc = nn_iface_resolve (addr, pos - addr - 1, ipv4only, &ss, &sslen);
    if (rc < 0) {
        return rc;
    }

    /*  Combine the port and the address. */
    switch (ss.ss_family) {
    case AF_INET:
        ((struct sockaddr_in*) &ss)->sin_port = htons (port);
        sslen = sizeof (struct sockaddr_in);
        break;
    case AF_INET6:
        ((struct sockaddr_in6*) &ss)->sin6_port = htons (port);
        sslen = sizeof (struct sockaddr_in6);
        break;
    default:
        nn_assert (0);
    }

    /*  Start listening for incoming connections. */
    rc = nn_usock_start (&self->usock, ss.ss_family, SOCK_STREAM, 0);
    if (rc < 0) {
        return rc;
    }

    rc = nn_usock_bind (&self->usock, (struct sockaddr*) &ss, (size_t) sslen);
    if (rc < 0) {
       nn_usock_stop (&self->usock);
       return rc;
    }

    rc = nn_usock_listen (&self->usock, NN_BTCP_BACKLOG);
    if (rc < 0) {
        nn_usock_stop (&self->usock);
        return rc;
    }
    nn_btcp_start_accepting(self);

    return 0;
}

/******************************************************************************/
/*  State machine actions.                                                    */
/******************************************************************************/

static void nn_btcp_start_accepting (struct nn_btcp *self)
{
    nn_assert (self->atcp == NULL);

    /*  Allocate new atcp state machine. */
    self->atcp = nn_alloc (sizeof (struct nn_atcp), "atcp");
    alloc_assert (self->atcp);
    nn_atcp_init (self->atcp, NN_BTCP_SRC_ATCP, &self->epbase, &self->fsm);

    /*  Start waiting for a new incoming connection. */
    nn_atcp_start (self->atcp, &self->usock);
}
