/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
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

#include "xsurveyor.h"

#include "../../nn.h"
#include "../../survey.h"

#include "../../aio/fsm.h"
#include "../../aio/timer.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/fast.h"
#include "../../utils/wire.h"
#include "../../utils/alloc.h"
#include "../../utils/random.h"
#include "../../utils/attr.h"

#include <string.h>

#define NN_SURVEYOR_DEFAULT_DEADLINE 1000

#define NN_SURVEYOR_STATE_IDLE 1
#define NN_SURVEYOR_STATE_PASSIVE 2
#define NN_SURVEYOR_STATE_ACTIVE 3
#define NN_SURVEYOR_STATE_CANCELLING 4
#define NN_SURVEYOR_STATE_STOPPING_TIMER 5
#define NN_SURVEYOR_STATE_STOPPING 6

#define NN_SURVEYOR_ACTION_START 1
#define NN_SURVEYOR_ACTION_CANCEL 2

#define NN_SURVEYOR_SRC_DEADLINE_TIMER 1

#define NN_SURVEYOR_TIMEDOUT 1

struct nn_surveyor {

    /*  The underlying raw SP socket. */
    struct nn_xsurveyor xsurveyor;

    /*  The state machine. */
    struct nn_fsm fsm;
    int state;

    /*  Survey ID of the current survey. */
    uint32_t surveyid;

    /*  Timer for timing out the survey. */
    struct nn_timer timer;

    /*  When starting the survey, the message is temporarily stored here. */
    struct nn_msg tosend;

    /*  Protocol-specific socket options. */
    int deadline;

    /*  Flag if surveyor has timed out */
    int timedout;
};

/*  Private functions. */
static void nn_surveyor_init (struct nn_surveyor *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint);
static void nn_surveyor_term (struct nn_surveyor *self);
static void nn_surveyor_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_surveyor_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);
static int nn_surveyor_inprogress (struct nn_surveyor *self);
static void nn_surveyor_resend (struct nn_surveyor *self);

/*  Implementation of nn_sockbase's virtual functions. */
static void nn_surveyor_stop (struct nn_sockbase *self);
static void nn_surveyor_destroy (struct nn_sockbase *self);
static int nn_surveyor_events (struct nn_sockbase *self);
static int nn_surveyor_send (struct nn_sockbase *self, struct nn_msg *msg);
static int nn_surveyor_recv (struct nn_sockbase *self, struct nn_msg *msg);
static int nn_surveyor_setopt (struct nn_sockbase *self, int level, int option,
    const void *optval, size_t optvallen);
static int nn_surveyor_getopt (struct nn_sockbase *self, int level, int option,
    void *optval, size_t *optvallen);
static const struct nn_sockbase_vfptr nn_surveyor_sockbase_vfptr = {
    nn_surveyor_stop,
    nn_surveyor_destroy,
    nn_xsurveyor_add,
    nn_xsurveyor_rm,
    nn_xsurveyor_in,
    nn_xsurveyor_out,
    nn_surveyor_events,
    nn_surveyor_send,
    nn_surveyor_recv,
    nn_surveyor_setopt,
    nn_surveyor_getopt
};

static void nn_surveyor_init (struct nn_surveyor *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint)
{
    nn_xsurveyor_init (&self->xsurveyor, vfptr, hint);
    nn_fsm_init_root (&self->fsm, nn_surveyor_handler, nn_surveyor_shutdown,
        nn_sockbase_getctx (&self->xsurveyor.sockbase));
    self->state = NN_SURVEYOR_STATE_IDLE;

    /*  Start assigning survey IDs beginning with a random number. This way
        there should be no key clashes even if the executable is re-started. */
    nn_random_generate (&self->surveyid, sizeof (self->surveyid));

    nn_timer_init (&self->timer, NN_SURVEYOR_SRC_DEADLINE_TIMER, &self->fsm);
    nn_msg_init (&self->tosend, 0);
    self->deadline = NN_SURVEYOR_DEFAULT_DEADLINE;
    self->timedout = 0;

    /*  Start the state machine. */
    nn_fsm_start (&self->fsm);
}

static void nn_surveyor_term (struct nn_surveyor *self)
{
    nn_msg_term (&self->tosend);
    nn_timer_term (&self->timer);
    nn_fsm_term (&self->fsm);
    nn_xsurveyor_term (&self->xsurveyor);
}

void nn_surveyor_stop (struct nn_sockbase *self)
{
    struct nn_surveyor *surveyor;

    surveyor = nn_cont (self, struct nn_surveyor, xsurveyor.sockbase);

    nn_fsm_stop (&surveyor->fsm);
}

void nn_surveyor_destroy (struct nn_sockbase *self)
{
    struct nn_surveyor *surveyor;

    surveyor = nn_cont (self, struct nn_surveyor, xsurveyor.sockbase);

    nn_surveyor_term (surveyor);
    nn_free (surveyor);
}

static int nn_surveyor_inprogress (struct nn_surveyor *self)
{
    /*  Return 1 if there's a survey going on. 0 otherwise. */
    return self->state == NN_SURVEYOR_STATE_IDLE ||
        self->state == NN_SURVEYOR_STATE_PASSIVE ||
        self->state == NN_SURVEYOR_STATE_STOPPING ? 0 : 1;
}

static int nn_surveyor_events (struct nn_sockbase *self)
{
    int rc;
    struct nn_surveyor *surveyor;

    surveyor = nn_cont (self, struct nn_surveyor, xsurveyor.sockbase);

    /*  Determine the actual readability/writability of the socket. */
    rc = nn_xsurveyor_events (&surveyor->xsurveyor.sockbase);

    /*  If there's no survey going on we'll signal IN to interrupt polling
        when the survey expires. nn_recv() will return -EFSM afterwards. */
    if (!nn_surveyor_inprogress (surveyor))
        rc |= NN_SOCKBASE_EVENT_IN;

    return rc;
}

static int nn_surveyor_send (struct nn_sockbase *self, struct nn_msg *msg)
{
    struct nn_surveyor *surveyor;

    surveyor = nn_cont (self, struct nn_surveyor, xsurveyor.sockbase);

    /*  Generate new survey ID. */
    ++surveyor->surveyid;
    surveyor->surveyid |= 0x80000000;

    /*  Tag the survey body with survey ID. */
    nn_assert (nn_chunkref_size (&msg->sphdr) == 0);
    nn_chunkref_term (&msg->sphdr);
    nn_chunkref_init (&msg->sphdr, 4);
    nn_putl (nn_chunkref_data (&msg->sphdr), surveyor->surveyid);

    /*  Store the survey, so that it can be sent later on. */
    nn_msg_term (&surveyor->tosend);
    nn_msg_mv (&surveyor->tosend, msg);
    nn_msg_init (msg, 0);

    /*  Cancel any ongoing survey, if any. */
    if (nn_slow (nn_surveyor_inprogress (surveyor))) {

        /*  First check whether the survey can be sent at all. */
        if (!(nn_xsurveyor_events (&surveyor->xsurveyor.sockbase) &
              NN_SOCKBASE_EVENT_OUT))
            return -EAGAIN;

        /*  Cancel the current survey. */
        nn_fsm_action (&surveyor->fsm, NN_SURVEYOR_ACTION_CANCEL);

        return 0;
    }

    /*  Notify the state machine that the survey was started. */
    nn_fsm_action (&surveyor->fsm, NN_SURVEYOR_ACTION_START);

    return 0;
}

static int nn_surveyor_recv (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;
    struct nn_surveyor *surveyor;
    uint32_t surveyid;

    surveyor = nn_cont (self, struct nn_surveyor, xsurveyor.sockbase);

    /*  If no survey is going on return EFSM error. */
    if (nn_slow (!nn_surveyor_inprogress (surveyor))) {
        if (surveyor->timedout == NN_SURVEYOR_TIMEDOUT) {
            surveyor->timedout = 0;
            return -ETIMEDOUT;
        } else
            return -EFSM;
    }

    while (1) {

        /*  Get next response. */
        rc = nn_xsurveyor_recv (&surveyor->xsurveyor.sockbase, msg);
        if (nn_slow (rc == -EAGAIN))
            return -EAGAIN;
        errnum_assert (rc == 0, -rc);

        /*  Get the survey ID. Ignore any stale responses. */
        /*  TODO: This should be done asynchronously! */
        if (nn_slow (nn_chunkref_size (&msg->sphdr) != sizeof (uint32_t)))
            continue;
        surveyid = nn_getl (nn_chunkref_data (&msg->sphdr));
        if (nn_slow (surveyid != surveyor->surveyid))
            continue;

        /*  Discard the header and return the message to the user. */
        nn_chunkref_term (&msg->sphdr);
        nn_chunkref_init (&msg->sphdr, 0);
        break;
    }

    return 0;
}

static int nn_surveyor_setopt (struct nn_sockbase *self, int level, int option,
    const void *optval, size_t optvallen)
{
    struct nn_surveyor *surveyor;

    surveyor = nn_cont (self, struct nn_surveyor, xsurveyor.sockbase);

    if (level != NN_SURVEYOR)
        return -ENOPROTOOPT;

    if (option == NN_SURVEYOR_DEADLINE) {
        if (nn_slow (optvallen != sizeof (int)))
            return -EINVAL;
        surveyor->deadline = *(int*) optval;
        return 0;
    }

    return -ENOPROTOOPT;
}

static int nn_surveyor_getopt (struct nn_sockbase *self, int level, int option,
    void *optval, size_t *optvallen)
{
    struct nn_surveyor *surveyor;

    surveyor = nn_cont (self, struct nn_surveyor, xsurveyor.sockbase);

    if (level != NN_SURVEYOR)
        return -ENOPROTOOPT;

    if (option == NN_SURVEYOR_DEADLINE) {
        if (nn_slow (*optvallen < sizeof (int)))
            return -EINVAL;
        *(int*) optval = surveyor->deadline;
        *optvallen = sizeof (int);
        return 0;
    }

    return -ENOPROTOOPT;
}

static void nn_surveyor_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_surveyor *surveyor;

    surveyor = nn_cont (self, struct nn_surveyor, fsm);

    if (nn_slow (src== NN_FSM_ACTION && type == NN_FSM_STOP)) {
        nn_timer_stop (&surveyor->timer);
        surveyor->state = NN_SURVEYOR_STATE_STOPPING;
    }
    if (nn_slow (surveyor->state == NN_SURVEYOR_STATE_STOPPING)) {
        if (!nn_timer_isidle (&surveyor->timer))
            return;
        surveyor->state = NN_SURVEYOR_STATE_IDLE;
        nn_fsm_stopped_noevent (&surveyor->fsm);
        nn_sockbase_stopped (&surveyor->xsurveyor.sockbase);
        return;
    }

    nn_fsm_bad_state(surveyor->state, src, type);
}

static void nn_surveyor_handler (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_surveyor *surveyor;

    surveyor = nn_cont (self, struct nn_surveyor, fsm);

    switch (surveyor->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/*  The socket was created recently.                                          */
/******************************************************************************/
    case NN_SURVEYOR_STATE_IDLE:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_FSM_START:
                surveyor->state = NN_SURVEYOR_STATE_PASSIVE;
                return;
            default:
                nn_fsm_bad_action (surveyor->state, src, type);
            }

        default:
            nn_fsm_bad_source (surveyor->state, src, type);
        }

/******************************************************************************/
/*  PASSIVE state.                                                            */
/*  There's no survey going on.                                               */
/******************************************************************************/
    case NN_SURVEYOR_STATE_PASSIVE:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_SURVEYOR_ACTION_START:
                nn_surveyor_resend (surveyor);
                nn_timer_start (&surveyor->timer, surveyor->deadline);
                surveyor->state = NN_SURVEYOR_STATE_ACTIVE;
                return;

            default:
                nn_fsm_bad_action (surveyor->state, src, type);
            }

        default:
            nn_fsm_bad_source (surveyor->state, src, type);
        }

/******************************************************************************/
/*  ACTIVE state.                                                             */
/*  Survey was sent, waiting for responses.                                   */
/******************************************************************************/
    case NN_SURVEYOR_STATE_ACTIVE:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_SURVEYOR_ACTION_CANCEL:
                nn_timer_stop (&surveyor->timer);
                surveyor->state = NN_SURVEYOR_STATE_CANCELLING;
                return;
            default:
                nn_fsm_bad_action (surveyor->state, src, type);
            }

        case NN_SURVEYOR_SRC_DEADLINE_TIMER:
            switch (type) {
            case NN_TIMER_TIMEOUT:
                nn_timer_stop (&surveyor->timer);
                surveyor->state = NN_SURVEYOR_STATE_STOPPING_TIMER;
                surveyor->timedout = NN_SURVEYOR_TIMEDOUT;
                return;
            default:
                nn_fsm_bad_action (surveyor->state, src, type);
            }

        default:
            nn_fsm_bad_source (surveyor->state, src, type);
        }

/******************************************************************************/
/*  CANCELLING state.                                                         */
/*  Survey was cancelled, but the old timer haven't stopped yet. The new      */
/*  survey thus haven't been sent and is stored in 'tosend'.                  */
/******************************************************************************/
    case NN_SURVEYOR_STATE_CANCELLING:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_SURVEYOR_ACTION_CANCEL:
                return;
            default:
                nn_fsm_bad_action (surveyor->state, src, type);
            }

        case NN_SURVEYOR_SRC_DEADLINE_TIMER:
            switch (type) {
            case NN_TIMER_STOPPED:
                nn_surveyor_resend (surveyor);
                nn_timer_start (&surveyor->timer, surveyor->deadline);
                surveyor->state = NN_SURVEYOR_STATE_ACTIVE;
                return;
            default:
                nn_fsm_bad_action (surveyor->state, src, type);
            }

        default:
            nn_fsm_bad_source (surveyor->state, src, type);
        }

/******************************************************************************/
/*  STOPPING_TIMER state.                                                     */
/*  Survey timeout expired. Now we are stopping the timer.                    */
/******************************************************************************/
    case NN_SURVEYOR_STATE_STOPPING_TIMER:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_SURVEYOR_ACTION_CANCEL:
                surveyor->state = NN_SURVEYOR_STATE_CANCELLING;
                return;
            default:
                nn_fsm_bad_action (surveyor->state, src, type);
            }

        case NN_SURVEYOR_SRC_DEADLINE_TIMER:
            switch (type) {
            case NN_TIMER_STOPPED:
                surveyor->state = NN_SURVEYOR_STATE_PASSIVE;
                return;
            default:
                nn_fsm_bad_action (surveyor->state, src, type);
            }

        default:
            nn_fsm_bad_source (surveyor->state, src, type);
        }

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (surveyor->state, src, type);
    }
}

static void nn_surveyor_resend (struct nn_surveyor *self)
{
    int rc;
    struct nn_msg msg;

    nn_msg_cp (&msg, &self->tosend);
    rc = nn_xsurveyor_send (&self->xsurveyor.sockbase, &msg);
    errnum_assert (rc == 0, -rc);
}

static int nn_surveyor_create (void *hint, struct nn_sockbase **sockbase)
{
    struct nn_surveyor *self;

    self = nn_alloc (sizeof (struct nn_surveyor), "socket (surveyor)");
    alloc_assert (self);
    nn_surveyor_init (self, &nn_surveyor_sockbase_vfptr, hint);
    *sockbase = &self->xsurveyor.sockbase;

    return 0;
}

struct nn_socktype nn_surveyor_socktype = {
    AF_SP,
    NN_SURVEYOR,
    0,
    nn_surveyor_create,
    nn_xsurveyor_ispeer,
};
