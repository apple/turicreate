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

#include "atcp.h"

#include "../../tcp.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/attr.h"

#if defined NN_HAVE_WINDOWS
#include "../../utils/win.h"
#else
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#define NN_ATCP_STATE_IDLE 1
#define NN_ATCP_STATE_ACCEPTING 2
#define NN_ATCP_STATE_ACTIVE 3
#define NN_ATCP_STATE_STOPPING_STCP 4
#define NN_ATCP_STATE_STOPPING_USOCK 5
#define NN_ATCP_STATE_DONE 6
#define NN_ATCP_STATE_STOPPING_STCP_FINAL 7
#define NN_ATCP_STATE_STOPPING 8

#define NN_ATCP_SRC_USOCK 1
#define NN_ATCP_SRC_STCP 2
#define NN_ATCP_SRC_LISTENER 3

/*  Private functions. */
static void nn_atcp_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_atcp_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);

void nn_atcp_init (struct nn_atcp *self, int src,
    struct nn_ep *ep, struct nn_fsm *owner)
{
    nn_fsm_init (&self->fsm, nn_atcp_handler, nn_atcp_shutdown,
        src, self, owner);
    self->state = NN_ATCP_STATE_IDLE;
    self->ep = ep;
    nn_usock_init (&self->usock, NN_ATCP_SRC_USOCK, &self->fsm);
    self->listener = NULL;
    self->listener_owner.src = -1;
    self->listener_owner.fsm = NULL;
    nn_stcp_init (&self->stcp, NN_ATCP_SRC_STCP, ep, &self->fsm);
    nn_fsm_event_init (&self->accepted);
    nn_fsm_event_init (&self->done);
    nn_list_item_init (&self->item);
}

void nn_atcp_term (struct nn_atcp *self)
{
    nn_assert_state (self, NN_ATCP_STATE_IDLE);

    nn_list_item_term (&self->item);
    nn_fsm_event_term (&self->done);
    nn_fsm_event_term (&self->accepted);
    nn_stcp_term (&self->stcp);
    nn_usock_term (&self->usock);
    nn_fsm_term (&self->fsm);
}

int nn_atcp_isidle (struct nn_atcp *self)
{
    return nn_fsm_isidle (&self->fsm);
}

void nn_atcp_start (struct nn_atcp *self, struct nn_usock *listener)
{
    nn_assert_state (self, NN_ATCP_STATE_IDLE);

    /*  Take ownership of the listener socket. */
    self->listener = listener;
    self->listener_owner.src = NN_ATCP_SRC_LISTENER;
    self->listener_owner.fsm = &self->fsm;
    nn_usock_swap_owner (listener, &self->listener_owner);

    /*  Start the state machine. */
    nn_fsm_start (&self->fsm);
}

void nn_atcp_stop (struct nn_atcp *self)
{
    nn_fsm_stop (&self->fsm);
}

static void nn_atcp_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_atcp *atcp;

    atcp = nn_cont (self, struct nn_atcp, fsm);

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {
        if (!nn_stcp_isidle (&atcp->stcp)) {
            nn_ep_stat_increment (atcp->ep, NN_STAT_DROPPED_CONNECTIONS, 1);
            nn_stcp_stop (&atcp->stcp);
        }
        atcp->state = NN_ATCP_STATE_STOPPING_STCP_FINAL;
    }
    if (nn_slow (atcp->state == NN_ATCP_STATE_STOPPING_STCP_FINAL)) {
        if (!nn_stcp_isidle (&atcp->stcp))
            return;
        nn_usock_stop (&atcp->usock);
        atcp->state = NN_ATCP_STATE_STOPPING;
    }
    if (nn_slow (atcp->state == NN_ATCP_STATE_STOPPING)) {
        if (!nn_usock_isidle (&atcp->usock))
            return;
       if (atcp->listener) {
            nn_assert (atcp->listener_owner.fsm);
            nn_usock_swap_owner (atcp->listener, &atcp->listener_owner);
            atcp->listener = NULL;
            atcp->listener_owner.src = -1;
            atcp->listener_owner.fsm = NULL;
        }
        atcp->state = NN_ATCP_STATE_IDLE;
        nn_fsm_stopped (&atcp->fsm, NN_ATCP_STOPPED);
        return;
    }

    nn_fsm_bad_action(atcp->state, src, type);
}

static void nn_atcp_handler (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_atcp *atcp;
    int val;
    size_t sz;

    atcp = nn_cont (self, struct nn_atcp, fsm);

    switch (atcp->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/*  The state machine wasn't yet started.                                     */
/******************************************************************************/
    case NN_ATCP_STATE_IDLE:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_FSM_START:
                nn_usock_accept (&atcp->usock, atcp->listener);
                atcp->state = NN_ATCP_STATE_ACCEPTING;
                return;
            default:
                nn_fsm_bad_action (atcp->state, src, type);
            }

        default:
            nn_fsm_bad_source (atcp->state, src, type);
        }

/******************************************************************************/
/*  ACCEPTING state.                                                          */
/*  Waiting for incoming connection.                                          */
/******************************************************************************/
    case NN_ATCP_STATE_ACCEPTING:
        switch (src) {

        case NN_ATCP_SRC_USOCK:
            switch (type) {
            case NN_USOCK_ACCEPTED:
                nn_ep_clear_error (atcp->ep);

                /*  Set the relevant socket options. */
                sz = sizeof (val);
                nn_ep_getopt (atcp->ep, NN_SOL_SOCKET, NN_SNDBUF, &val, &sz);
                nn_assert (sz == sizeof (val));
                nn_usock_setsockopt (&atcp->usock, SOL_SOCKET, SO_SNDBUF,
                    &val, sizeof (val));
                sz = sizeof (val);
                nn_ep_getopt (atcp->ep, NN_SOL_SOCKET, NN_RCVBUF, &val, &sz);
                nn_assert (sz == sizeof (val));
                nn_usock_setsockopt (&atcp->usock, SOL_SOCKET, SO_RCVBUF,
                    &val, sizeof (val));
                sz = sizeof (val);
                nn_ep_getopt (atcp->ep, NN_TCP, NN_TCP_NODELAY, &val, &sz);
                nn_assert (sz == sizeof (val));
                nn_usock_setsockopt (&atcp->usock, IPPROTO_TCP, TCP_NODELAY,
                    &val, sizeof (val));

                /*  Return ownership of the listening socket to the parent. */
                nn_usock_swap_owner (atcp->listener, &atcp->listener_owner);
                atcp->listener = NULL;
                atcp->listener_owner.src = -1;
                atcp->listener_owner.fsm = NULL;
                nn_fsm_raise (&atcp->fsm, &atcp->accepted, NN_ATCP_ACCEPTED);

                /*  Start the stcp state machine. */
                nn_usock_activate (&atcp->usock);
                nn_stcp_start (&atcp->stcp, &atcp->usock);
                atcp->state = NN_ATCP_STATE_ACTIVE;

                nn_ep_stat_increment (atcp->ep,
                    NN_STAT_ACCEPTED_CONNECTIONS, 1);

                return;

            default:
                nn_fsm_bad_action (atcp->state, src, type);
            }

        case NN_ATCP_SRC_LISTENER:
            switch (type) {

            case NN_USOCK_ACCEPT_ERROR:
                nn_ep_set_error (atcp->ep, nn_usock_geterrno(atcp->listener));
                nn_ep_stat_increment (atcp->ep, NN_STAT_ACCEPT_ERRORS, 1);
                nn_usock_accept (&atcp->usock, atcp->listener);
                return;

            default:
                nn_fsm_bad_action (atcp->state, src, type);
            }

        default:
            nn_fsm_bad_source (atcp->state, src, type);
        }

/******************************************************************************/
/*  ACTIVE state.                                                             */
/******************************************************************************/
    case NN_ATCP_STATE_ACTIVE:
        switch (src) {

        case NN_ATCP_SRC_STCP:
            switch (type) {
            case NN_STCP_ERROR:
                nn_stcp_stop (&atcp->stcp);
                atcp->state = NN_ATCP_STATE_STOPPING_STCP;
                nn_ep_stat_increment (atcp->ep, NN_STAT_BROKEN_CONNECTIONS, 1);
                return;
            default:
                nn_fsm_bad_action (atcp->state, src, type);
            }

        default:
            nn_fsm_bad_source (atcp->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_STCP state.                                                      */
/******************************************************************************/
    case NN_ATCP_STATE_STOPPING_STCP:
        switch (src) {

        case NN_ATCP_SRC_STCP:
            switch (type) {
            case NN_USOCK_SHUTDOWN:
                return;
            case NN_STCP_STOPPED:
                nn_usock_stop (&atcp->usock);
                atcp->state = NN_ATCP_STATE_STOPPING_USOCK;
                return;
            default:
                nn_fsm_bad_action (atcp->state, src, type);
            }

        default:
            nn_fsm_bad_source (atcp->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_USOCK state.                                                      */
/******************************************************************************/
    case NN_ATCP_STATE_STOPPING_USOCK:
        switch (src) {

        case NN_ATCP_SRC_USOCK:
            switch (type) {
            case NN_USOCK_SHUTDOWN:
                return;
            case NN_USOCK_STOPPED:
                nn_fsm_raise (&atcp->fsm, &atcp->done, NN_ATCP_ERROR);
                atcp->state = NN_ATCP_STATE_DONE;
                return;
            default:
                nn_fsm_bad_action (atcp->state, src, type);
            }

        default:
            nn_fsm_bad_source (atcp->state, src, type);
        }

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (atcp->state, src, type);
    }
}

