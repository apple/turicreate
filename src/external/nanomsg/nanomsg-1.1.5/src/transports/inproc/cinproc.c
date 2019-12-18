 /*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
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

#include "cinproc.h"
#include "binproc.h"
#include "ins.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/alloc.h"
#include "../../utils/attr.h"

#include <stddef.h>

#define NN_CINPROC_STATE_IDLE 1
#define NN_CINPROC_STATE_ACTIVE 2
#define NN_CINPROC_STATE_STOPPING 3

#define NN_CINPROC_ACTION_CONNECT 1

#define NN_CINPROC_SRC_SINPROC 1

/*  Implementation of nn_ep callback interface. */
static void nn_cinproc_stop (void *);
static void nn_cinproc_destroy (void *);
static const struct nn_ep_ops nn_cinproc_ops = {
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

int nn_cinproc_create (struct nn_ep *ep)
{
    struct nn_cinproc *self;

    self = nn_alloc (sizeof (struct nn_cinproc), "cinproc");
    alloc_assert (self);

    nn_ep_tran_setup (ep, &nn_cinproc_ops, self);

    nn_ins_item_init (&self->item, ep);
    nn_fsm_init_root (&self->fsm, nn_cinproc_handler, nn_cinproc_shutdown,
        nn_ep_getctx (ep));
    self->state = NN_CINPROC_STATE_IDLE;
    nn_list_init (&self->sinprocs);

    nn_ep_stat_increment (ep, NN_STAT_INPROGRESS_CONNECTIONS, 1);

    /*  Start the state machine. */
    nn_fsm_start (&self->fsm);

    /*  Register the inproc endpoint into a global repository. */
    nn_ins_connect (&self->item, nn_cinproc_connect);

    return 0;
}

static void nn_cinproc_stop (void *self)
{
    struct nn_cinproc *cinproc = self;

    nn_fsm_stop (&cinproc->fsm);
}

static void nn_cinproc_destroy (void *self)
{
    struct nn_cinproc *cinproc = self;

    nn_list_term (&cinproc->sinprocs);
    nn_fsm_term (&cinproc->fsm);
    nn_ins_item_term (&cinproc->item);

    nn_free (cinproc);
}

static void nn_cinproc_connect (struct nn_ins_item *self,
    struct nn_ins_item *peer)
{
    struct nn_cinproc *cinproc;
    struct nn_binproc *binproc;
    struct nn_sinproc *sinproc;

    cinproc = nn_cont (self, struct nn_cinproc, item);
    binproc = nn_cont (peer, struct nn_binproc, item);

    nn_assert_state (cinproc, NN_CINPROC_STATE_ACTIVE);

    sinproc = nn_alloc (sizeof (struct nn_sinproc), "sinproc");
    alloc_assert (sinproc);
    nn_sinproc_init (sinproc, NN_CINPROC_SRC_SINPROC,
        cinproc->item.ep, &cinproc->fsm);

    nn_list_insert (&cinproc->sinprocs, &sinproc->item,
        nn_list_end (&cinproc->sinprocs));

    nn_sinproc_connect (sinproc, &binproc->fsm);

    nn_ep_stat_increment (cinproc->item.ep, NN_STAT_INPROGRESS_CONNECTIONS, -1);
    nn_ep_stat_increment (cinproc->item.ep, NN_STAT_ESTABLISHED_CONNECTIONS, 1);
}

static void nn_cinproc_shutdown (struct nn_fsm *self, int src, int type,
    NN_UNUSED void *srcptr)
{
    struct nn_cinproc *cinproc;
    struct nn_sinproc *sinproc;
    struct nn_list_item *it;

    cinproc = nn_cont (self, struct nn_cinproc, fsm);

    if (src == NN_FSM_ACTION && type == NN_FSM_STOP) {

        /*  First, unregister the endpoint from the global repository of inproc
            endpoints. This way, new connections cannot be created anymore. */
        nn_ins_disconnect (&cinproc->item);

        /*  Stop the existing connection. */
        for (it = nn_list_begin (&cinproc->sinprocs);
              it != nn_list_end (&cinproc->sinprocs);
              it = nn_list_next (&cinproc->sinprocs, it)) {
            sinproc = nn_cont (it, struct nn_sinproc, item);
            nn_sinproc_stop (sinproc);
        }
        cinproc->state = NN_CINPROC_STATE_STOPPING;
        goto finish;
    }
    if (cinproc->state == NN_CINPROC_STATE_STOPPING) {
        sinproc = (struct nn_sinproc *) srcptr;
        nn_list_erase (&cinproc->sinprocs, &sinproc->item);
        nn_sinproc_term (sinproc);
        nn_free (sinproc);

finish:
        if (!nn_list_empty (&cinproc->sinprocs))
            return;
        cinproc->state = NN_CINPROC_STATE_IDLE;
        nn_fsm_stopped_noevent (&cinproc->fsm);
        nn_ep_stopped (cinproc->item.ep);
        return;
    }

    nn_fsm_bad_state(cinproc->state, src, type);
}

static void nn_cinproc_handler (struct nn_fsm *self, int src, int type,
    void *srcptr)
{
    struct nn_cinproc *cinproc;
    struct nn_sinproc *sinproc;
    struct nn_sinproc *peer;

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
                cinproc->state = NN_CINPROC_STATE_ACTIVE;
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
        case NN_SINPROC_SRC_PEER:
            peer = (struct nn_sinproc*) srcptr;

            switch (type) {
            case NN_SINPROC_CONNECT:
                sinproc = nn_alloc (sizeof (struct nn_sinproc), "sinproc");
                alloc_assert (sinproc);
                nn_sinproc_init (sinproc, NN_CINPROC_SRC_SINPROC,
                    cinproc->item.ep, &cinproc->fsm);
                nn_list_insert (&cinproc->sinprocs, &sinproc->item,
                    nn_list_end (&cinproc->sinprocs));
                nn_sinproc_accept (sinproc, peer);
                nn_ep_stat_increment (cinproc->item.ep,
                    NN_STAT_INPROGRESS_CONNECTIONS, -1);
                nn_ep_stat_increment (cinproc->item.ep,
                    NN_STAT_ESTABLISHED_CONNECTIONS, 1);
                return;
            default:
                nn_fsm_bad_action (cinproc->state, src, type);
            }

        case NN_CINPROC_SRC_SINPROC:
            switch (type) {
            case NN_SINPROC_DISCONNECT:
                nn_ep_stat_increment (cinproc->item.ep,
                    NN_STAT_INPROGRESS_CONNECTIONS, 1);
                return;
            }
            return;

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
