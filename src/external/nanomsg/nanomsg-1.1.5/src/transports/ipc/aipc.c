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

#include "aipc.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/attr.h"

#define NN_AIPC_STATE_IDLE 1
#define NN_AIPC_STATE_ACCEPTING 2
#define NN_AIPC_STATE_ACTIVE 3
#define NN_AIPC_STATE_STOPPING_SIPC 4
#define NN_AIPC_STATE_STOPPING_USOCK 5
#define NN_AIPC_STATE_DONE 6
#define NN_AIPC_STATE_STOPPING_SIPC_FINAL 7
#define NN_AIPC_STATE_STOPPING 8

#define NN_AIPC_SRC_USOCK 1
#define NN_AIPC_SRC_SIPC 2
#define NN_AIPC_SRC_LISTENER 3

/*  Private functions. */
static void nn_aipc_handler (struct nn_fsm *self, int src, int type,
   void *srcptr);
static void nn_aipc_shutdown (struct nn_fsm *self, int src, int type,
   void *srcptr);

void nn_aipc_init (struct nn_aipc *self, int src,
    struct nn_ep *ep, struct nn_fsm *owner)
{
    nn_fsm_init (&self->fsm, nn_aipc_handler, nn_aipc_shutdown,
        src, self, owner);
    self->state = NN_AIPC_STATE_IDLE;
    self->ep = ep;
    nn_usock_init (&self->usock, NN_AIPC_SRC_USOCK, &self->fsm);
    self->listener = NULL;
    self->listener_owner.src = -1;
    self->listener_owner.fsm = NULL;
    nn_sipc_init (&self->sipc, NN_AIPC_SRC_SIPC, ep, &self->fsm);
    nn_fsm_event_init (&self->accepted);
    nn_fsm_event_init (&self->done);
    nn_list_item_init (&self->item);
}

void nn_aipc_term (struct nn_aipc *self)
{
    nn_assert_state (self, NN_AIPC_STATE_IDLE);

    nn_list_item_term (&self->item);
    nn_fsm_event_term (&self->done);
    nn_fsm_event_term (&self->accepted);
    nn_sipc_term (&self->sipc);
    nn_usock_term (&self->usock);
    nn_fsm_term (&self->fsm);
}

int nn_aipc_isidle (struct nn_aipc *self)
{
    return nn_fsm_isidle (&self->fsm);
}

void nn_aipc_start (struct nn_aipc *self, struct nn_usock *listener)
{
#if defined NN_HAVE_WINDOWS
    size_t sz;
#endif
    nn_assert_state (self, NN_AIPC_STATE_IDLE);

    /*  Take ownership of the listener socket. */
    self->listener = listener;
    self->listener_owner.src = NN_AIPC_SRC_LISTENER;
    self->listener_owner.fsm = &self->fsm;
    nn_usock_swap_owner (listener, &self->listener_owner);

#if defined NN_HAVE_WINDOWS
    /* Get/Set security attribute pointer*/
    nn_ep_getopt (self->ep, NN_IPC, NN_IPC_SEC_ATTR, &self->usock.sec_attr, &sz);
    nn_ep_getopt (self->ep, NN_IPC, NN_IPC_OUTBUFSZ, &self->usock.outbuffersz, &sz);
    nn_ep_getopt (self->ep, NN_IPC, NN_IPC_INBUFSZ, &self->usock.inbuffersz, &sz);
#endif

    /*  Start the state machine. */
    nn_fsm_start (&self->fsm);
}

void nn_aipc_stop (struct nn_aipc *self)
{
    nn_fsm_stop (&self->fsm);
}

static void nn_aipc_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_aipc *aipc;

    aipc = nn_cont (self, struct nn_aipc, fsm);

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {
        if (!nn_sipc_isidle (&aipc->sipc)) {
            nn_ep_stat_increment (aipc->ep, NN_STAT_DROPPED_CONNECTIONS, 1);
            nn_sipc_stop (&aipc->sipc);
        }
        aipc->state = NN_AIPC_STATE_STOPPING_SIPC_FINAL;
    }
    if (nn_slow (aipc->state == NN_AIPC_STATE_STOPPING_SIPC_FINAL)) {
        if (!nn_sipc_isidle (&aipc->sipc))
            return;
        nn_usock_stop (&aipc->usock);
        aipc->state = NN_AIPC_STATE_STOPPING;
    }
    if (nn_slow (aipc->state == NN_AIPC_STATE_STOPPING)) {
        if (!nn_usock_isidle (&aipc->usock))
            return;
       if (aipc->listener) {
            nn_assert (aipc->listener_owner.fsm);
            nn_usock_swap_owner (aipc->listener, &aipc->listener_owner);
            aipc->listener = NULL;
            aipc->listener_owner.src = -1;
            aipc->listener_owner.fsm = NULL;
        }
        aipc->state = NN_AIPC_STATE_IDLE;
        nn_fsm_stopped (&aipc->fsm, NN_AIPC_STOPPED);
        return;
    }

    nn_fsm_bad_state(aipc->state, src, type);
}

static void nn_aipc_handler (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_aipc *aipc;
    int val;
    size_t sz;

    aipc = nn_cont (self, struct nn_aipc, fsm);

    switch (aipc->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/*  The state machine wasn't yet started.                                     */
/******************************************************************************/
    case NN_AIPC_STATE_IDLE:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_FSM_START:
                nn_usock_accept (&aipc->usock, aipc->listener);
                aipc->state = NN_AIPC_STATE_ACCEPTING;
                return;
            default:
                nn_fsm_bad_action (aipc->state, src, type);
            }

        default:
            nn_fsm_bad_source (aipc->state, src, type);
        }

/******************************************************************************/
/*  ACCEPTING state.                                                          */
/*  Waiting for incoming connection.                                          */
/******************************************************************************/
    case NN_AIPC_STATE_ACCEPTING:
        switch (src) {

        case NN_AIPC_SRC_USOCK:
            switch (type) {
            case NN_USOCK_ACCEPTED:
                nn_ep_clear_error (aipc->ep);

                /*  Set the relevant socket options. */
                sz = sizeof (val);
                nn_ep_getopt (aipc->ep, NN_SOL_SOCKET, NN_SNDBUF, &val, &sz);
                nn_assert (sz == sizeof (val));
                nn_usock_setsockopt (&aipc->usock, SOL_SOCKET, SO_SNDBUF,
                    &val, sizeof (val));
                sz = sizeof (val);
                nn_ep_getopt (aipc->ep, NN_SOL_SOCKET, NN_RCVBUF, &val, &sz);
                nn_assert (sz == sizeof (val));
                nn_usock_setsockopt (&aipc->usock, SOL_SOCKET, SO_RCVBUF,
                    &val, sizeof (val));

                /*  Return ownership of the listening socket to the parent. */
                nn_usock_swap_owner (aipc->listener, &aipc->listener_owner);
                aipc->listener = NULL;
                aipc->listener_owner.src = -1;
                aipc->listener_owner.fsm = NULL;
                nn_fsm_raise (&aipc->fsm, &aipc->accepted, NN_AIPC_ACCEPTED);

                /*  Start the sipc state machine. */
                nn_usock_activate (&aipc->usock);
                nn_sipc_start (&aipc->sipc, &aipc->usock);
                aipc->state = NN_AIPC_STATE_ACTIVE;

                nn_ep_stat_increment (aipc->ep,
                    NN_STAT_ACCEPTED_CONNECTIONS, 1);

                return;

            default:
                nn_fsm_bad_action (aipc->state, src, type);
            }

        case NN_AIPC_SRC_LISTENER:
            switch (type) {
            case NN_USOCK_ACCEPT_ERROR:
                nn_ep_set_error (aipc->ep, nn_usock_geterrno (aipc->listener));
                nn_ep_stat_increment (aipc->ep, NN_STAT_ACCEPT_ERRORS, 1);
                nn_usock_accept (&aipc->usock, aipc->listener);

                return;

            default:
                nn_fsm_bad_action (aipc->state, src, type);
            }

        default:
            nn_fsm_bad_source (aipc->state, src, type);
        }

/******************************************************************************/
/*  ACTIVE state.                                                             */
/******************************************************************************/
    case NN_AIPC_STATE_ACTIVE:
        switch (src) {

        case NN_AIPC_SRC_SIPC:
            switch (type) {
            case NN_SIPC_ERROR:
                nn_sipc_stop (&aipc->sipc);
                aipc->state = NN_AIPC_STATE_STOPPING_SIPC;
                nn_ep_stat_increment (aipc->ep, NN_STAT_BROKEN_CONNECTIONS, 1);
                return;
            default:
                nn_fsm_bad_action (aipc->state, src, type);
            }

        default:
            nn_fsm_bad_source (aipc->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_SIPC state.                                                      */
/******************************************************************************/
    case NN_AIPC_STATE_STOPPING_SIPC:
        switch (src) {

        case NN_AIPC_SRC_SIPC:
            switch (type) {
            case NN_USOCK_SHUTDOWN:
                return;
            case NN_SIPC_STOPPED:
                nn_usock_stop (&aipc->usock);
                aipc->state = NN_AIPC_STATE_STOPPING_USOCK;
                return;
            default:
                nn_fsm_bad_action (aipc->state, src, type);
            }

        default:
            nn_fsm_bad_source (aipc->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_USOCK state.                                                      */
/******************************************************************************/
    case NN_AIPC_STATE_STOPPING_USOCK:
        switch (src) {

        case NN_AIPC_SRC_USOCK:
            switch (type) {
            case NN_USOCK_SHUTDOWN:
                return;
            case NN_USOCK_STOPPED:
                nn_fsm_raise (&aipc->fsm, &aipc->done, NN_AIPC_ERROR);
                aipc->state = NN_AIPC_STATE_DONE;
                return;
            default:
                nn_fsm_bad_action (aipc->state, src, type);
            }

        default:
            nn_fsm_bad_source (aipc->state, src, type);
        }

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (aipc->state, src, type);
    }
}
