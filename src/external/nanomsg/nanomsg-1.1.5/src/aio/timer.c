/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.
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

#include "timer.h"

#include "../utils/cont.h"
#include "../utils/fast.h"
#include "../utils/err.h"
#include "../utils/attr.h"

/*  Timer state reflects the state as seen by the user thread. It says nothing
    about the state of affairs in the worker thread. */
#define NN_TIMER_STATE_IDLE 1
#define NN_TIMER_STATE_ACTIVE 2
#define NN_TIMER_STATE_STOPPING 3

#define NN_TIMER_SRC_START_TASK 1
#define NN_TIMER_SRC_STOP_TASK 2

/*  Private functions. */
static void nn_timer_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_timer_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);

void nn_timer_init (struct nn_timer *self, int src, struct nn_fsm *owner)
{
    nn_fsm_init (&self->fsm, nn_timer_handler, nn_timer_shutdown,
        src, self, owner);
    self->state = NN_TIMER_STATE_IDLE;
    nn_worker_task_init (&self->start_task, NN_TIMER_SRC_START_TASK,
        &self->fsm);
    nn_worker_task_init (&self->stop_task, NN_TIMER_SRC_STOP_TASK, &self->fsm);
    nn_worker_timer_init (&self->wtimer, &self->fsm);
    nn_fsm_event_init (&self->done);
    self->worker = nn_fsm_choose_worker (&self->fsm);
    self->timeout = -1;
}

void nn_timer_term (struct nn_timer *self)
{
    nn_assert_state (self, NN_TIMER_STATE_IDLE);

    nn_fsm_event_term (&self->done);
    nn_worker_timer_term (&self->wtimer);
    nn_worker_task_term (&self->stop_task);
    nn_worker_task_term (&self->start_task);
    nn_fsm_term (&self->fsm);
}

int nn_timer_isidle (struct nn_timer *self)
{
     return nn_fsm_isidle (&self->fsm);
}

void nn_timer_start (struct nn_timer *self, int timeout)
{
    /*  Negative timeout make no sense. */
    nn_assert (timeout >= 0);

    self->timeout = timeout;
    nn_fsm_start (&self->fsm);
}

void nn_timer_stop (struct nn_timer *self)
{
    nn_fsm_stop (&self->fsm);
}

static void nn_timer_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_timer *timer;

    timer = nn_cont (self, struct nn_timer, fsm);

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {
        timer->state = NN_TIMER_STATE_STOPPING;
        nn_worker_execute (timer->worker, &timer->stop_task);
        return;
    }
    if (nn_slow (timer->state == NN_TIMER_STATE_STOPPING)) {
        if (src != NN_TIMER_SRC_STOP_TASK)
            return;
        nn_assert (type == NN_WORKER_TASK_EXECUTE);
        nn_worker_rm_timer (timer->worker, &timer->wtimer);
        timer->state = NN_TIMER_STATE_IDLE;
        nn_fsm_stopped (&timer->fsm, NN_TIMER_STOPPED);
        return;
    }

    nn_fsm_bad_state(timer->state, src, type);
}

static void nn_timer_handler (struct nn_fsm *self, int src, int type,
    void *srcptr)
{
    struct nn_timer *timer;

    timer = nn_cont (self, struct nn_timer, fsm);

    switch (timer->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/******************************************************************************/
    case NN_TIMER_STATE_IDLE:
        switch (src) {
        case NN_FSM_ACTION:
            switch (type) {
            case NN_FSM_START:

                /*  Send start event to the worker thread. */
                timer->state = NN_TIMER_STATE_ACTIVE;
                nn_worker_execute (timer->worker, &timer->start_task);
                return;
            default:
                nn_fsm_bad_action (timer->state, src, type);
            }
        default:
            nn_fsm_bad_source (timer->state, src, type);
        }

/******************************************************************************/
/*  ACTIVE state.                                                             */
/******************************************************************************/
    case NN_TIMER_STATE_ACTIVE:
        if (src == NN_TIMER_SRC_START_TASK) {
            nn_assert (type == NN_WORKER_TASK_EXECUTE);
            nn_assert (timer->timeout >= 0);
            nn_worker_add_timer (timer->worker, timer->timeout,
                &timer->wtimer);
            timer->timeout = -1;
            return;
        }
        if (srcptr == &timer->wtimer) {
            switch (type) {
            case NN_WORKER_TIMER_TIMEOUT:

                /*  Notify the user about the timeout. */
                nn_assert (timer->timeout == -1);
                nn_fsm_raise (&timer->fsm, &timer->done, NN_TIMER_TIMEOUT);
                return;

            default:
                nn_fsm_bad_action (timer->state, src, type);
            }
        }
        nn_fsm_bad_source (timer->state, src, type);

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (timer->state, src, type);
    }
}

