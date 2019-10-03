 /*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
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

#include "cinproc.h"
#include "binproc.h"
#include "ins.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/alloc.h"
#include "../../utils/attr.h"

#include <stddef.h>

#define NN_CINPROC_STATE_IDLE 1
#define NN_CINPROC_STATE_DISCONNECTED 2
#define NN_CINPROC_STATE_ACTIVE 3
#define NN_CINPROC_STATE_STOPPING 4

#define NN_CINPROC_ACTION_CONNECT 1

#define NN_CINPROC_SRC_SINPROC 1

/*  Implementation of nn_epbase callback interface. */
static void nn_cinproc_stop (struct nn_epbase *self);
static void nn_cinproc_destroy (struct nn_epbase *self);
static const struct nn_epbase_vfptr nn_cinproc_vfptr = {
    nn_cinproc_stop,
    nn_cinproc_destroy
};

/*  Private functions. */
static void nn_cinproc_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_cinproc_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_cinproc_connect (struct nn_ins_item *self,
    struct nn_ins_item *peer);

int nn_cinproc_create (void *hint, struct nn_epbase **epbase)
{
    struct nn_cinproc *self;

    self = nn_alloc (sizeof (struct nn_cinproc), "cinproc");
    alloc_assert (self);

    nn_ins_item_init (&self->item, &nn_cinproc_vfptr, hint);
    nn_fsm_init_root (&self->fsm, nn_cinproc_handler, nn_cinproc_shutdown,
        nn_epbase_getctx (&self->item.epbase));
    self->state = NN_CINPROC_STATE_IDLE;
    nn_sinproc_init (&self->sinproc, NN_CINPROC_SRC_SINPROC,
        &self->item.epbase, &self->fsm);

    /*  Start the state machine. */
    nn_fsm_start (&self->fsm);

    /*  Register the inproc endpoint into a global repository. */
    nn_ins_connect (&self->item, nn_cinproc_connect);

    *epbase = &self->item.epbase;
    return 0;
}

static void nn_cinproc_stop (struct nn_epbase *self)
{
    struct nn_cinproc *cinproc;

    cinproc = nn_cont (self, struct nn_cinproc, item.epbase);

    nn_fsm_stop (&cinproc->fsm);
}

static void nn_cinproc_destroy (struct nn_epbase *self)
{
    struct nn_cinproc *cinproc;

    cinproc = nn_cont (self, struct nn_cinproc, item.epbase);

    nn_sinproc_term (&cinproc->sinproc);
    nn_fsm_term (&cinproc->fsm);
    nn_ins_item_term (&cinproc->item);

    nn_free (cinproc);
}

static void nn_cinproc_connect (struct nn_ins_item *self,
    struct nn_ins_item *peer)
{
    struct nn_cinproc *cinproc;
    struct nn_binproc *binproc;

    cinproc = nn_cont (self, struct nn_cinproc, item);
    binproc = nn_cont (peer, struct nn_binproc, item);

    nn_assert_state (cinproc, NN_CINPROC_STATE_DISCONNECTED);
    nn_sinproc_connect (&cinproc->sinproc, &binproc->fsm);
    nn_fsm_action (&cinproc->fsm, NN_CINPROC_ACTION_CONNECT);
}

static void nn_cinproc_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_cinproc *cinproc;

    cinproc = nn_cont (self, struct nn_cinproc, fsm);

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {

        /*  First, unregister the endpoint from the global repository of inproc
            endpoints. This way, new connections cannot be created anymore. */
        nn_ins_disconnect (&cinproc->item);

        /*  Stop the existing connection. */
        nn_sinproc_stop (&cinproc->sinproc);
        cinproc->state = NN_CINPROC_STATE_STOPPING;
    }
    if (nn_slow (cinproc->state == NN_CINPROC_STATE_STOPPING)) {
        if (!nn_sinproc_isidle (&cinproc->sinproc))
            return;
        cinproc->state = NN_CINPROC_STATE_IDLE;
        nn_fsm_stopped_noevent (&cinproc->fsm);
        nn_epbase_stopped (&cinproc->item.epbase);
        return;
    }

    nn_fsm_bad_state(cinproc->state, src, type);
}

static void nn_cinproc_handler (struct nn_fsm *self, int src, int type,
    void *srcptr)
{
    struct nn_cinproc *cinproc;
    struct nn_sinproc *sinproc;

    cinproc = nn_cont (self, struct nn_cinproc, fsm);


    switch (cinproc->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/******************************************************************************/
    case NN_CINPROC_STATE_IDLE:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_FSM_START:
                cinproc->state = NN_CINPROC_STATE_DISCONNECTED;
                nn_epbase_stat_increment (&cinproc->item.epbase,
                    NN_STAT_INPROGRESS_CONNECTIONS, 1);
                return;
            default:
                nn_fsm_bad_action (cinproc->state, src, type);
            }

        default:
            nn_fsm_bad_source (cinproc->state, src, type);
        }

/******************************************************************************/
/*  DISCONNECTED state.                                                       */
/******************************************************************************/
    case NN_CINPROC_STATE_DISCONNECTED:
        switch (src) {

        case NN_FSM_ACTION:
            switch (type) {
            case NN_CINPROC_ACTION_CONNECT:
                cinproc->state = NN_CINPROC_STATE_ACTIVE;
                nn_epbase_stat_increment (&cinproc->item.epbase,
                    NN_STAT_INPROGRESS_CONNECTIONS, -1);
                nn_epbase_stat_increment (&cinproc->item.epbase,
                    NN_STAT_ESTABLISHED_CONNECTIONS, 1);
                return;
            default:
                nn_fsm_bad_action (cinproc->state, src, type);
            }

        case NN_SINPROC_SRC_PEER:
            sinproc = (struct nn_sinproc*) srcptr;
            switch (type) {
            case NN_SINPROC_CONNECT:
                nn_sinproc_accept (&cinproc->sinproc, sinproc);
                cinproc->state = NN_CINPROC_STATE_ACTIVE;
                nn_epbase_stat_increment (&cinproc->item.epbase,
                    NN_STAT_INPROGRESS_CONNECTIONS, -1);
                nn_epbase_stat_increment (&cinproc->item.epbase,
                    NN_STAT_ESTABLISHED_CONNECTIONS, 1);
                return;
            default:
                nn_fsm_bad_action (cinproc->state, src, type);
            }

        default:
            nn_fsm_bad_source (cinproc->state, src, type);
        }

/******************************************************************************/
/*  ACTIVE state.                                                             */
/******************************************************************************/
    case NN_CINPROC_STATE_ACTIVE:
        switch (src) {
        case NN_CINPROC_SRC_SINPROC:
            switch (type) {
            case NN_SINPROC_DISCONNECT:
                cinproc->state = NN_CINPROC_STATE_DISCONNECTED;
                nn_epbase_stat_increment (&cinproc->item.epbase,
                    NN_STAT_INPROGRESS_CONNECTIONS, 1);

                nn_sinproc_init (&cinproc->sinproc, NN_CINPROC_SRC_SINPROC,
                    &cinproc->item.epbase, &cinproc->fsm);
                return;

            default:
                nn_fsm_bad_action (cinproc->state, src, type);
            }

        default:
            nn_fsm_bad_source (cinproc->state, src, type);
        }

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (cinproc->state, src, type);
    }
}
