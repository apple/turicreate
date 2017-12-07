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

#include "../transport.h"

#include "ep.h"
#include "sock.h"

#include "../utils/err.h"
#include "../utils/cont.h"
#include "../utils/fast.h"
#include "../utils/attr.h"

#include <string.h>

#define NN_EP_STATE_IDLE 1
#define NN_EP_STATE_ACTIVE 2
#define NN_EP_STATE_STOPPING 3

#define NN_EP_ACTION_STOPPED 1

/*  Private functions. */
static void nn_ep_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_ep_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);

int nn_ep_init (struct nn_ep *self, int src, struct nn_sock *sock, int eid,
    struct nn_transport *transport, int bind, const char *addr)
{
    int rc;

    nn_fsm_init (&self->fsm, nn_ep_handler, nn_ep_shutdown,
        src, self, &sock->fsm);
    self->state = NN_EP_STATE_IDLE;

    self->epbase = NULL;
    self->sock = sock;
    self->eid = eid;
    self->last_errno = 0;
    nn_list_item_init (&self->item);
    memcpy (&self->options, &sock->ep_template, sizeof(struct nn_ep_options));

    /*  Store the textual form of the address. */
    nn_assert (strlen (addr) <= NN_SOCKADDR_MAX);
    strcpy (self->addr, addr);

    /*  Create transport-specific part of the endpoint. */
    if (bind)
        rc = transport->bind ((void*) self, &self->epbase);
    else
        rc = transport->connect ((void*) self, &self->epbase);

    /*  Endpoint creation failed. */
    if (rc < 0) {
        nn_list_item_term (&self->item);
        nn_fsm_term (&self->fsm);
        return rc;
    }

    return 0;
}

void nn_ep_term (struct nn_ep *self)
{
    nn_assert_state (self, NN_EP_STATE_IDLE);

    self->epbase->vfptr->destroy (self->epbase);
    nn_list_item_term (&self->item);
    nn_fsm_term (&self->fsm);
}

void nn_ep_start (struct nn_ep *self)
{
    nn_fsm_start (&self->fsm);
}

void nn_ep_stop (struct nn_ep *self)
{
    nn_fsm_stop (&self->fsm);
}

void nn_ep_stopped (struct nn_ep *self)
{
    /*  TODO: Do the following in a more sane way. */
    self->fsm.stopped.fsm = &self->fsm;
    self->fsm.stopped.src = NN_FSM_ACTION;
    self->fsm.stopped.srcptr = NULL;
    self->fsm.stopped.type = NN_EP_ACTION_STOPPED;
    nn_ctx_raise (self->fsm.ctx, &self->fsm.stopped);
}

struct nn_ctx *nn_ep_getctx (struct nn_ep *self)
{
    return nn_sock_getctx (self->sock);
}

const char *nn_ep_getaddr (struct nn_ep *self)
{
    return self->addr;
}

void nn_ep_getopt (struct nn_ep *self, int level, int option,
    void *optval, size_t *optvallen)
{
    int rc;

    rc = nn_sock_getopt_inner (self->sock, level, option, optval, optvallen);
    errnum_assert (rc == 0, -rc);
}

int nn_ep_ispeer (struct nn_ep *self, int socktype)
{
    return nn_sock_ispeer (self->sock, socktype);
}


static void nn_ep_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_ep *ep;

    ep = nn_cont (self, struct nn_ep, fsm);

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {
        ep->epbase->vfptr->stop (ep->epbase);
        ep->state = NN_EP_STATE_STOPPING;
        return;
    }
    if (nn_slow (ep->state == NN_EP_STATE_STOPPING)) {
        if (src != NN_FSM_ACTION || type != NN_EP_ACTION_STOPPED)
            return;
        ep->state = NN_EP_STATE_IDLE;
        nn_fsm_stopped (&ep->fsm, NN_EP_STOPPED);
        return;
    }

    nn_fsm_bad_state (ep->state, src, type);
}


static void nn_ep_handler (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_ep *ep;

    ep = nn_cont (self, struct nn_ep, fsm);

    switch (ep->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/******************************************************************************/
    case NN_EP_STATE_IDLE:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_FSM_START:
                ep->state = NN_EP_STATE_ACTIVE;
                return;
            default:
                nn_fsm_bad_action (ep->state, src, type);
            }

        default:
            nn_fsm_bad_source (ep->state, src, type);
        }

/******************************************************************************/
/*  ACTIVE state.                                                             */
/*  We don't expect any events in this state. The only thing that can be done */
/*  is closing the endpoint.                                                  */
/******************************************************************************/
    case NN_EP_STATE_ACTIVE:
        nn_fsm_bad_source (ep->state, src, type);

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (ep->state, src, type);
    }
}

void nn_ep_set_error(struct nn_ep *self, int errnum)
{
    if (self->last_errno == errnum)
        /*  Error is still there, no need to report it again  */
        return;
    if (self->last_errno == 0)
        nn_sock_stat_increment (self->sock, NN_STAT_CURRENT_EP_ERRORS, 1);
    self->last_errno = errnum;
    nn_sock_report_error (self->sock, self, errnum);
}

void nn_ep_clear_error (struct nn_ep *self)
{
    if (self->last_errno == 0)
        /*  Error is already clear, no need to report it  */
        return;
    nn_sock_stat_increment (self->sock, NN_STAT_CURRENT_EP_ERRORS, -1);
    self->last_errno = 0;
    nn_sock_report_error (self->sock, self, 0);
}

void nn_ep_stat_increment (struct nn_ep *self, int name, int increment)
{
    nn_sock_stat_increment (self->sock, name, increment);
}
