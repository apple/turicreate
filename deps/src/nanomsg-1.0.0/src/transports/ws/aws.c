/*
    Copyright (c) 2012-2013 250bpm s.r.o.  All rights reserved.
    Copyright (c) 2014-2016 Jack R. Dunaway. All rights reserved.
    Copyright 2015 Garrett D'Amore <garrett@damore.org>

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

#include "aws.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/attr.h"
#include "../../ws.h"

#define NN_AWS_STATE_IDLE 1
#define NN_AWS_STATE_ACCEPTING 2
#define NN_AWS_STATE_ACTIVE 3
#define NN_AWS_STATE_STOPPING_SWS 4
#define NN_AWS_STATE_STOPPING_USOCK 5
#define NN_AWS_STATE_DONE 6
#define NN_AWS_STATE_STOPPING_SWS_FINAL 7
#define NN_AWS_STATE_STOPPING 8

#define NN_AWS_SRC_USOCK 1
#define NN_AWS_SRC_SWS 2
#define NN_AWS_SRC_LISTENER 3

/*  Private functions. */
static void nn_aws_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_aws_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);

void nn_aws_init (struct nn_aws *self, int src,
    struct nn_epbase *epbase, struct nn_fsm *owner)
{
    nn_fsm_init (&self->fsm, nn_aws_handler, nn_aws_shutdown,
        src, self, owner);
    self->state = NN_AWS_STATE_IDLE;
    self->epbase = epbase;
    nn_usock_init (&self->usock, NN_AWS_SRC_USOCK, &self->fsm);
    self->listener = NULL;
    self->listener_owner.src = -1;
    self->listener_owner.fsm = NULL;
    nn_sws_init (&self->sws, NN_AWS_SRC_SWS, epbase, &self->fsm);
    nn_fsm_event_init (&self->accepted);
    nn_fsm_event_init (&self->done);
    nn_list_item_init (&self->item);
}

void nn_aws_term (struct nn_aws *self)
{
    nn_assert_state (self, NN_AWS_STATE_IDLE);

    nn_list_item_term (&self->item);
    nn_fsm_event_term (&self->done);
    nn_fsm_event_term (&self->accepted);
    nn_sws_term (&self->sws);
    nn_usock_term (&self->usock);
    nn_fsm_term (&self->fsm);
}

int nn_aws_isidle (struct nn_aws *self)
{
    return nn_fsm_isidle (&self->fsm);
}

void nn_aws_start (struct nn_aws *self, struct nn_usock *listener)
{
    nn_assert_state (self, NN_AWS_STATE_IDLE);

    /*  Take ownership of the listener socket. */
    self->listener = listener;
    self->listener_owner.src = NN_AWS_SRC_LISTENER;
    self->listener_owner.fsm = &self->fsm;
    nn_usock_swap_owner (listener, &self->listener_owner);

    /*  Start the state machine. */
    nn_fsm_start (&self->fsm);
}

void nn_aws_stop (struct nn_aws *self)
{
    nn_fsm_stop (&self->fsm);
}

static void nn_aws_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_aws *aws;

    aws = nn_cont (self, struct nn_aws, fsm);

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {
        if (!nn_sws_isidle (&aws->sws)) {
            nn_epbase_stat_increment (aws->epbase,
                NN_STAT_DROPPED_CONNECTIONS, 1);
            nn_sws_stop (&aws->sws);
        }
        aws->state = NN_AWS_STATE_STOPPING_SWS_FINAL;
    }
    if (nn_slow (aws->state == NN_AWS_STATE_STOPPING_SWS_FINAL)) {
        if (!nn_sws_isidle (&aws->sws))
            return;
        nn_usock_stop (&aws->usock);
        aws->state = NN_AWS_STATE_STOPPING;
    }
    if (nn_slow (aws->state == NN_AWS_STATE_STOPPING)) {
        if (!nn_usock_isidle (&aws->usock))
            return;
       if (aws->listener) {
            nn_assert (aws->listener_owner.fsm);
            nn_usock_swap_owner (aws->listener, &aws->listener_owner);
            aws->listener = NULL;
            aws->listener_owner.src = -1;
            aws->listener_owner.fsm = NULL;
        }
        aws->state = NN_AWS_STATE_IDLE;
        nn_fsm_stopped (&aws->fsm, NN_AWS_STOPPED);
        return;
    }

    nn_fsm_bad_action (aws->state, src, type);
}

static void nn_aws_handler (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_aws *aws;
    int val;
    size_t sz;
    uint8_t msg_type;

    aws = nn_cont (self, struct nn_aws, fsm);

    switch (aws->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/*  The state machine wasn't yet started.                                     */
/******************************************************************************/
    case NN_AWS_STATE_IDLE:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_FSM_START:
                nn_usock_accept (&aws->usock, aws->listener);
                aws->state = NN_AWS_STATE_ACCEPTING;
                return;
            default:
                nn_fsm_bad_action (aws->state, src, type);
            }

        default:
            nn_fsm_bad_source (aws->state, src, type);
        }

/******************************************************************************/
/*  ACCEPTING state.                                                          */
/*  Waiting for incoming connection.                                          */
/******************************************************************************/
    case NN_AWS_STATE_ACCEPTING:
        switch (src) {

        case NN_AWS_SRC_USOCK:
            switch (type) {
            case NN_USOCK_ACCEPTED:
                nn_epbase_clear_error (aws->epbase);

                /*  Set the relevant socket options. */
                sz = sizeof (val);
                nn_epbase_getopt (aws->epbase, NN_SOL_SOCKET, NN_SNDBUF,
                    &val, &sz);
                nn_assert (sz == sizeof (val));
                nn_usock_setsockopt (&aws->usock, SOL_SOCKET, SO_SNDBUF,
                    &val, sizeof (val));
                sz = sizeof (val);
                nn_epbase_getopt (aws->epbase, NN_SOL_SOCKET, NN_RCVBUF,
                    &val, &sz);
                nn_assert (sz == sizeof (val));
                nn_usock_setsockopt (&aws->usock, SOL_SOCKET, SO_RCVBUF,
                    &val, sizeof (val));
                sz = sizeof (val);
                nn_epbase_getopt (aws->epbase, NN_WS, NN_WS_MSG_TYPE,
                    &val, &sz);
                msg_type = (uint8_t)val;

                /*   Since the WebSocket handshake must poll, the receive
                     timeout is set to zero. Later, it will be set again
                     to the value specified by the socket option. */
                val = 0;
                sz = sizeof (val);
                nn_usock_setsockopt (&aws->usock, SOL_SOCKET, SO_RCVTIMEO,
                    &val, sizeof (val));

                /*  Return ownership of the listening socket to the parent. */
                nn_usock_swap_owner (aws->listener, &aws->listener_owner);
                aws->listener = NULL;
                aws->listener_owner.src = -1;
                aws->listener_owner.fsm = NULL;
                nn_fsm_raise (&aws->fsm, &aws->accepted, NN_AWS_ACCEPTED);

                /*  Start the sws state machine. */
                nn_usock_activate (&aws->usock);
                nn_sws_start (&aws->sws, &aws->usock, NN_WS_SERVER,
                    NULL, NULL, msg_type);
                aws->state = NN_AWS_STATE_ACTIVE;

                nn_epbase_stat_increment (aws->epbase,
                    NN_STAT_ACCEPTED_CONNECTIONS, 1);

                return;

            default:
                nn_fsm_bad_action (aws->state, src, type);
            }

        case NN_AWS_SRC_LISTENER:
            switch (type) {

            case NN_USOCK_ACCEPT_ERROR:
                nn_epbase_set_error (aws->epbase,
                    nn_usock_geterrno (aws->listener));
                nn_epbase_stat_increment (aws->epbase,
                    NN_STAT_ACCEPT_ERRORS, 1);
                nn_usock_accept (&aws->usock, aws->listener);
                return;

            default:
                nn_fsm_bad_action (aws->state, src, type);
            }

        default:
            nn_fsm_bad_source (aws->state, src, type);
        }

/******************************************************************************/
/*  ACTIVE state.                                                             */
/******************************************************************************/
    case NN_AWS_STATE_ACTIVE:
        switch (src) {

        case NN_AWS_SRC_SWS:
            switch (type) {
            case NN_SWS_RETURN_CLOSE_HANDSHAKE:
                /*  Peer closed connection without intention to reconnect, or
                    local endpoint failed remote because of invalid data. */
                nn_sws_stop (&aws->sws);
                aws->state = NN_AWS_STATE_STOPPING_SWS;
                return;
            case NN_SWS_RETURN_ERROR:
                nn_sws_stop (&aws->sws);
                aws->state = NN_AWS_STATE_STOPPING_SWS;
                nn_epbase_stat_increment (aws->epbase,
                    NN_STAT_BROKEN_CONNECTIONS, 1);
                return;
            default:
                nn_fsm_bad_action (aws->state, src, type);
            }

        default:
            nn_fsm_bad_source (aws->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_SWS state.                                                       */
/******************************************************************************/
    case NN_AWS_STATE_STOPPING_SWS:
        switch (src) {

        case NN_AWS_SRC_SWS:
            switch (type) {
            case NN_USOCK_SHUTDOWN:
                return;
            case NN_SWS_RETURN_STOPPED:
                nn_usock_stop (&aws->usock);
                aws->state = NN_AWS_STATE_STOPPING_USOCK;
                return;
            default:
                nn_fsm_bad_action (aws->state, src, type);
            }

        default:
            nn_fsm_bad_source (aws->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_USOCK state.                                                     */
/******************************************************************************/
    case NN_AWS_STATE_STOPPING_USOCK:
        switch (src) {

        case NN_AWS_SRC_USOCK:
            switch (type) {
            case NN_USOCK_SHUTDOWN:
                return;
            case NN_USOCK_STOPPED:
                nn_fsm_raise (&aws->fsm, &aws->done, NN_AWS_ERROR);
                aws->state = NN_AWS_STATE_DONE;
                return;
            default:
                nn_fsm_bad_action (aws->state, src, type);
            }

        default:
            nn_fsm_bad_source (aws->state, src, type);
        }

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (aws->state, src, type);
    }
}
