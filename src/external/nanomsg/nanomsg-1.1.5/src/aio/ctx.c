/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.

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

#include "ctx.h"

#include "../utils/err.h"
#include "../utils/cont.h"
#include "../utils/fast.h"

void nn_ctx_init (struct nn_ctx *self, struct nn_pool *pool,
    nn_ctx_onleave onleave)
{
    nn_mutex_init (&self->sync);
    self->pool = pool;
    nn_queue_init (&self->events);
    nn_queue_init (&self->eventsto);
    self->onleave = onleave;
}

void nn_ctx_term (struct nn_ctx *self)
{
    nn_queue_term (&self->eventsto);
    nn_queue_term (&self->events);
    nn_mutex_term (&self->sync);
}

void nn_ctx_enter (struct nn_ctx *self)
{
    nn_mutex_lock (&self->sync);
}

void nn_ctx_leave (struct nn_ctx *self)
{
    struct nn_queue_item *item;
    struct nn_fsm_event *event;
    struct nn_queue eventsto;

    /*  Process any queued events before leaving the context. */
    while (1) {
        item = nn_queue_pop (&self->events);
        event = nn_cont (item, struct nn_fsm_event, item);
        if (!event)
            break;
        nn_fsm_event_process (event);
    }

    /*  Notify the owner that we are leaving the context. */
    if (nn_fast (self->onleave != NULL))
        self->onleave (self);

    /*  Shortcut in the case there are no external events. */
    if (nn_queue_empty (&self->eventsto)) {
        nn_mutex_unlock (&self->sync);
        return;
    }

    /*  Make a copy of the queue of the external events so that it does not
        get corrupted once we unlock the context. */
    eventsto = self->eventsto;
    nn_queue_init (&self->eventsto);

    nn_mutex_unlock (&self->sync);

    /*  Process any queued external events. Before processing each event
        lock the context it belongs to. */
    while (1) {
        item = nn_queue_pop (&eventsto);
        event = nn_cont (item, struct nn_fsm_event, item);
        if (!event)
            break;
        nn_ctx_enter (event->fsm->ctx);
        nn_fsm_event_process (event);
        nn_ctx_leave (event->fsm->ctx);
    }

    nn_queue_term (&eventsto);
}

struct nn_worker *nn_ctx_choose_worker (struct nn_ctx *self)
{
    return nn_pool_choose_worker (self->pool);
}

void nn_ctx_raise (struct nn_ctx *self, struct nn_fsm_event *event)
{
    nn_queue_push (&self->events, &event->item);
}

void nn_ctx_raiseto (struct nn_ctx *self, struct nn_fsm_event *event)
{
    nn_queue_push (&self->eventsto, &event->item);
}

