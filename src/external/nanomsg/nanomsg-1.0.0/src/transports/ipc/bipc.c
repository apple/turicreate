/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
    Copyright 2016 Franklin "Snaipe" Mathieu <franklinmathieu@gmail.com>
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

#include "bipc.h"
#include "aipc.h"

#include "../../aio/fsm.h"
#include "../../aio/usock.h"

#include "../../utils/err.h"
#include "../../utils/cont.h"
#include "../../utils/alloc.h"
#include "../../utils/list.h"
#include "../../utils/fast.h"

#include <string.h>
#if defined NN_HAVE_WINDOWS
#include "../../utils/win.h"
#else
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#endif

#define NN_BIPC_BACKLOG 10

#define NN_BIPC_STATE_IDLE 1
#define NN_BIPC_STATE_ACTIVE 2
#define NN_BIPC_STATE_STOPPING_AIPC 3
#define NN_BIPC_STATE_STOPPING_USOCK 4
#define NN_BIPC_STATE_STOPPING_AIPCS 5

#define NN_BIPC_SRC_USOCK 1
#define NN_BIPC_SRC_AIPC 2

struct nn_bipc {

    /*  The state machine. */
    struct nn_fsm fsm;
    int state;

    /*  This object is a specific type of endpoint.
        Thus it is derived from epbase. */
    struct nn_epbase epbase;

    /*  The underlying listening IPC socket. */
    struct nn_usock usock;

    /*  The connection being accepted at the moment. */
    struct nn_aipc *aipc;

    /*  List of accepted connections. */
    struct nn_list aipcs;
};

/*  nn_epbase virtual interface implementation. */
static void nn_bipc_stop (struct nn_epbase *self);
static void nn_bipc_destroy (struct nn_epbase *self);
const struct nn_epbase_vfptr nn_bipc_epbase_vfptr = {
    nn_bipc_stop,
    nn_bipc_destroy
};

/*  Private functions. */
static void nn_bipc_handler (struct nn_fsm *self, int src, int type,
    void *srcptr);
static void nn_bipc_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr);
static int nn_bipc_listen (struct nn_bipc *self);
static void nn_bipc_start_accepting (struct nn_bipc *self);

int nn_bipc_create (void *hint, struct nn_epbase **epbase)
{
    struct nn_bipc *self;
    int rc;

    /*  Allocate the new endpoint object. */
    self = nn_alloc (sizeof (struct nn_bipc), "bipc");
    alloc_assert (self);


    /*  Initialise the structure. */
    nn_epbase_init (&self->epbase, &nn_bipc_epbase_vfptr, hint);
    nn_fsm_init_root (&self->fsm, nn_bipc_handler, nn_bipc_shutdown,
        nn_epbase_getctx (&self->epbase));
    self->state = NN_BIPC_STATE_IDLE;
    self->aipc = NULL;
    nn_list_init (&self->aipcs);

    /*  Start the state machine. */
    nn_fsm_start (&self->fsm);

    nn_usock_init (&self->usock, NN_BIPC_SRC_USOCK, &self->fsm);

    rc = nn_bipc_listen (self);
    if (rc != 0) {
        nn_epbase_term (&self->epbase);
        return rc;
    }

    /*  Return the base class as an out parameter. */
    *epbase = &self->epbase;

    return 0;
}

static void nn_bipc_stop (struct nn_epbase *self)
{
    struct nn_bipc *bipc;

    bipc = nn_cont (self, struct nn_bipc, epbase);

    nn_fsm_stop (&bipc->fsm);
}

static void nn_bipc_destroy (struct nn_epbase *self)
{
    struct nn_bipc *bipc;

    bipc = nn_cont (self, struct nn_bipc, epbase);

    nn_assert_state (bipc, NN_BIPC_STATE_IDLE);
    nn_list_term (&bipc->aipcs);
    nn_assert (bipc->aipc == NULL);
    nn_usock_term (&bipc->usock);
    nn_epbase_term (&bipc->epbase);
    nn_fsm_term (&bipc->fsm);

    nn_free (bipc);
}

static void nn_bipc_shutdown (struct nn_fsm *self, int src, int type,
    void *srcptr)
{
#if defined NN_HAVE_UNIX_SOCKETS
    const char *addr;
    int rc;
#endif

    struct nn_bipc *bipc;
    struct nn_list_item *it;
    struct nn_aipc *aipc;

    bipc = nn_cont (self, struct nn_bipc, fsm);

    if (nn_slow (src == NN_FSM_ACTION && type == NN_FSM_STOP)) {
        if (bipc->aipc) {
            nn_aipc_stop (bipc->aipc);
            bipc->state = NN_BIPC_STATE_STOPPING_AIPC;
        }
        else {
            bipc->state = NN_BIPC_STATE_STOPPING_USOCK;
        }
    }
    if (nn_slow (bipc->state == NN_BIPC_STATE_STOPPING_AIPC)) {
        if (!nn_aipc_isidle (bipc->aipc))
            return;
        nn_aipc_term (bipc->aipc);
        nn_free (bipc->aipc);
        bipc->aipc = NULL;

        /* On *nixes, unlink the domain socket file */
#if defined NN_HAVE_UNIX_SOCKETS
        addr = nn_epbase_getaddr (&bipc->epbase);
        rc = unlink(addr);
        errno_assert (rc == 0 || errno == ENOENT);
#endif

        nn_usock_stop (&bipc->usock);
        bipc->state = NN_BIPC_STATE_STOPPING_USOCK;
    }
    if (nn_slow (bipc->state == NN_BIPC_STATE_STOPPING_USOCK)) {
       if (!nn_usock_isidle (&bipc->usock))
            return;
        for (it = nn_list_begin (&bipc->aipcs);
              it != nn_list_end (&bipc->aipcs);
              it = nn_list_next (&bipc->aipcs, it)) {
            aipc = nn_cont (it, struct nn_aipc, item);
            nn_aipc_stop (aipc);
        }
        bipc->state = NN_BIPC_STATE_STOPPING_AIPCS;
        goto aipcs_stopping;
    }
    if (nn_slow (bipc->state == NN_BIPC_STATE_STOPPING_AIPCS)) {
        nn_assert (src == NN_BIPC_SRC_AIPC && type == NN_AIPC_STOPPED);
        aipc = (struct nn_aipc *) srcptr;
        nn_list_erase (&bipc->aipcs, &aipc->item);
        nn_aipc_term (aipc);
        nn_free (aipc);

        /*  If there are no more aipc state machines, we can stop the whole
            bipc object. */
aipcs_stopping:
        if (nn_list_empty (&bipc->aipcs)) {
            bipc->state = NN_BIPC_STATE_IDLE;
            nn_fsm_stopped_noevent (&bipc->fsm);
            nn_epbase_stopped (&bipc->epbase);
            return;
        }

        return;
    }

    nn_fsm_bad_state(bipc->state, src, type);
}

static void nn_bipc_handler (struct nn_fsm *self, int src, int type,
    void *srcptr)
{
    struct nn_bipc *bipc;
    struct nn_aipc *aipc;

    bipc = nn_cont (self, struct nn_bipc, fsm);

    switch (bipc->state) {

/******************************************************************************/
/*  IDLE state.                                                               */
/******************************************************************************/
    case NN_BIPC_STATE_IDLE:
        nn_assert (src == NN_FSM_ACTION);
        nn_assert (type == NN_FSM_START);
        bipc->state = NN_BIPC_STATE_ACTIVE;
        return;

/******************************************************************************/
/*  ACTIVE state.                                                             */
/*  The execution is yielded to the aipc state machine in this state.         */
/******************************************************************************/
    case NN_BIPC_STATE_ACTIVE:
        if (src == NN_BIPC_SRC_USOCK) {
            nn_assert (type == NN_USOCK_SHUTDOWN || type == NN_USOCK_STOPPED);
            return;
        }

        /* All other events come from child aipc objects. */
        nn_assert (src == NN_BIPC_SRC_AIPC);
        aipc = (struct nn_aipc*) srcptr;
        switch (type) {
        case NN_AIPC_ACCEPTED:

            nn_list_insert (&bipc->aipcs, &aipc->item,
                nn_list_end (&bipc->aipcs));
            bipc->aipc = NULL;
            nn_bipc_start_accepting (bipc);
            return;
        case NN_AIPC_ERROR:
            nn_aipc_stop (aipc);
            return;
        case NN_AIPC_STOPPED:
            nn_list_erase (&bipc->aipcs, &aipc->item);
            nn_aipc_term (aipc);
            nn_free (aipc);
            return;
        default:
            nn_fsm_bad_action (bipc->state, src, type);
        }

/******************************************************************************/
/*  Invalid state.                                                            */
/******************************************************************************/
    default:
        nn_fsm_bad_state (bipc->state, src, type);
    }
}

static int nn_bipc_listen (struct nn_bipc *self)
{
    int rc;
    struct sockaddr_storage ss;
    struct sockaddr_un *un;
    const char *addr;
#if defined NN_HAVE_UNIX_SOCKETS
    int fd;
#endif

    /*  First, create the AF_UNIX address. */
    addr = nn_epbase_getaddr (&self->epbase);
    memset (&ss, 0, sizeof (ss));
    un = (struct sockaddr_un*) &ss;
    nn_assert (strlen (addr) < sizeof (un->sun_path));
    ss.ss_family = AF_UNIX;
    strncpy (un->sun_path, addr, sizeof (un->sun_path));

    /*  Delete the IPC file left over by eventual previous runs of
        the application. We'll check whether the file is still in use by
        connecting to the endpoint. On Windows plaform, NamedPipe is used
        which does not have an underlying file. */
#if defined NN_HAVE_UNIX_SOCKETS
    fd = socket (AF_UNIX, SOCK_STREAM, 0);
    if (fd >= 0) {
        rc = fcntl (fd, F_SETFL, O_NONBLOCK);
        errno_assert (rc != -1 || errno == EINVAL);
        rc = connect (fd, (struct sockaddr*) &ss,
            sizeof (struct sockaddr_un));
        if (rc == -1 && errno == ECONNREFUSED) {
            rc = unlink (addr);
            errno_assert (rc == 0 || errno == ENOENT);
        }
        rc = close (fd);
        errno_assert (rc == 0);
    }
#endif

    /*  Start listening for incoming connections. */
    rc = nn_usock_start (&self->usock, AF_UNIX, SOCK_STREAM, 0);
    if (rc < 0) {
        return rc;
    }

    rc = nn_usock_bind (&self->usock,
        (struct sockaddr*) &ss, sizeof (struct sockaddr_un));
    if (rc < 0) {
        nn_usock_stop (&self->usock);
        return rc;
    }

    rc = nn_usock_listen (&self->usock, NN_BIPC_BACKLOG);
    if (rc < 0) {
        nn_usock_stop (&self->usock);
        return rc;
    }
    nn_bipc_start_accepting (self);

    return 0;
}

/******************************************************************************/
/*  State machine actions.                                                    */
/******************************************************************************/

static void nn_bipc_start_accepting (struct nn_bipc *self)
{
    nn_assert (self->aipc == NULL);

    /*  Allocate new aipc state machine. */
    self->aipc = nn_alloc (sizeof (struct nn_aipc), "aipc");
    alloc_assert (self->aipc);
    nn_aipc_init (self->aipc, NN_BIPC_SRC_AIPC, &self->epbase, &self->fsm);

    /*  Start waiting for a new incoming connection. */
    nn_aipc_start (self->aipc, &self->usock);
}
