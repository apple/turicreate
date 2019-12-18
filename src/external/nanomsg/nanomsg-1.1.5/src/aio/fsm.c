/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.
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

#include "fsm.h"
#include "ctx.h"

#include "../utils/err.h"
#include "../utils/attr.h"

#include <stddef.h>

#define NN_FSM_STATE_IDLE 1
#define NN_FSM_STATE_ACTIVE 2
#define NN_FSM_STATE_STOPPING 3

void nn_fsm_event_init (struct nn_fsm_event *self)
{
    self->fsm = NULL;
    self->src = -1;
    self->srcptr = NULL;
    self->type = -1;
    nn_queue_item_init (&self->item);
}

void nn_fsm_event_term (NN_UNUSED struct nn_fsm_event *self)
{
    /*  We don't term the queue item, although one might think we ought to.
        It turns out that there are some hairy recursions which can cause
        events to get submitted to queues even after the FSM is stopped.
        We could spend more effort fixing this, and perhaps we ought to.
        But given that the assertion itself is harmless, if an FSM event
        is orphaned it should be pretty harmless -- the event won't be
        processed but the FSM is shutting down anyway.  So skip it for now.
        Later, if we don't rewrite/gut the entire FSM machinery, we can
        revisit this. */
    /* nn_queue_item_term (&self->item); */
}

int nn_fsm_event_active (struct nn_fsm_event *self)
{
    return nn_queue_item_isinqueue (&self->item);
}

void nn_fsm_event_process (struct nn_fsm_event *self)
{
    int src;
    int type;
    void *srcptr;

    src = self->src;
    type = self->type;
    srcptr = self->srcptr;
    self->src = -1;
    self->type = -1;
    self->srcptr = NULL;

    nn_fsm_feed (self->fsm, src, type, srcptr);
}

void nn_fsm_feed (struct nn_fsm *self, int src, int type, void *srcptr)
{
    if (nn_slow (self->state != NN_FSM_STATE_STOPPING)) {
        self->fn (self, src, type, srcptr);
    } else {
        self->shutdown_fn (self, src, type, srcptr);
    }
}

void nn_fsm_init_root (struct nn_fsm *self, nn_fsm_fn fn,
    nn_fsm_fn shutdown_fn, struct nn_ctx *ctx)
{
    self->fn = fn;
    self->shutdown_fn = shutdown_fn;
    self->state = NN_FSM_STATE_IDLE;
    self->src = -1;
    self->srcptr = NULL;
    self->owner = NULL;
    self->ctx = ctx;
    nn_fsm_event_init (&self->stopped);
}

void nn_fsm_init (struct nn_fsm *self, nn_fsm_fn fn,
    nn_fsm_fn shutdown_fn, int src, void *srcptr, struct nn_fsm *owner)
{
    self->fn = fn;
    self->shutdown_fn = shutdown_fn;
    self->state = NN_FSM_STATE_IDLE;
    self->src = src;
    self->srcptr = srcptr;
    self->owner = owner;
    self->ctx = owner->ctx;
    nn_fsm_event_init (&self->stopped);
}

void nn_fsm_term (struct nn_fsm *self)
{
    nn_assert (nn_fsm_isidle (self));
    nn_fsm_event_term (&self->stopped);
}

void nn_fsm_start (struct nn_fsm *self)
{
    nn_assert (nn_fsm_isidle (self));
    self->fn (self, NN_FSM_ACTION, NN_FSM_START, NULL);
    self->state = NN_FSM_STATE_ACTIVE;
}

int nn_fsm_isidle (struct nn_fsm *self)
{
    return self->state == NN_FSM_STATE_IDLE &&
        !nn_fsm_event_active (&self->stopped) ? 1 : 0;
}

void nn_fsm_stop (struct nn_fsm *self)
{
    /*  If stopping of the state machine was already requested, do nothing. */
    if (self->state != NN_FSM_STATE_ACTIVE)
        return;

    self->state = NN_FSM_STATE_STOPPING;
    self->shutdown_fn (self, NN_FSM_ACTION, NN_FSM_STOP, NULL);
}

void nn_fsm_stopped (struct nn_fsm *self, int type)
{
    nn_assert_state (self, NN_FSM_STATE_STOPPING);
    nn_fsm_raise (self, &self->stopped, type);
    self->state = NN_FSM_STATE_IDLE;
}

void nn_fsm_stopped_noevent (struct nn_fsm *self)
{
    nn_assert_state (self, NN_FSM_STATE_STOPPING);
    self->state = NN_FSM_STATE_IDLE;
}

void nn_fsm_swap_owner (struct nn_fsm *self, struct nn_fsm_owner *owner)
{
    int oldsrc;
    struct nn_fsm *oldowner;

    oldsrc = self->src;
    oldowner = self->owner;
    self->src = owner->src;
    self->owner = owner->fsm;
    owner->src = oldsrc;
    owner->fsm = oldowner;
}

struct nn_worker *nn_fsm_choose_worker (struct nn_fsm *self)
{
    return nn_ctx_choose_worker (self->ctx);
}

void nn_fsm_action (struct nn_fsm *self, int type)
{
    nn_assert (type > 0);
    nn_fsm_feed (self, NN_FSM_ACTION, type, NULL);
}

void nn_fsm_raise_from_src (struct nn_fsm *self, struct nn_fsm_event *event, 
    int src, int type)
{
    event->fsm = self;
    event->src = src;
    event->srcptr = self->srcptr;
    event->type = type;
    nn_ctx_raise (self->ctx, event);
}

void nn_fsm_raise (struct nn_fsm *self, struct nn_fsm_event *event, int type)
{
    event->fsm = self->owner;
    event->src = self->src;
    event->srcptr = self->srcptr;
    event->type = type;
    nn_ctx_raise (self->ctx, event);
}

void nn_fsm_raiseto (struct nn_fsm *self, struct nn_fsm *dst,
    struct nn_fsm_event *event, int src, int type, void *srcptr)
{
    event->fsm = dst;
    event->src = src;
    event->srcptr = srcptr;
    event->type = type;
    nn_ctx_raiseto (self->ctx, event);
}

