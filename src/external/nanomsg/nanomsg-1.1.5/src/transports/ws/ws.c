/*
    Copyright (c) 2012-2013 250bpm s.r.o.  All rights reserved.
    Copyright (c) 2013 GoPivotal, Inc.  All rights reserved.
    Copyright (c) 2014 Wirebird Labs LLC.  All rights reserved.
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

#include "bws.h"
#include "cws.h"
#include "sws.h"

#include "../../ws.h"

#include "../utils/port.h"
#include "../utils/iface.h"

#include "../../utils/err.h"
#include "../../utils/alloc.h"
#include "../../utils/fast.h"
#include "../../utils/cont.h"

#include <string.h>

#if defined NN_HAVE_WINDOWS
#include "../../utils/win.h"
#else
#include <unistd.h>
#endif

/*  WebSocket-specific socket options. */
struct nn_ws_optset {
    struct nn_optset base;
    int msg_type;
};

static void nn_ws_optset_destroy (struct nn_optset *self);
static int nn_ws_optset_setopt (struct nn_optset *self, int option,
    const void *optval, size_t optvallen);
static int nn_ws_optset_getopt (struct nn_optset *self, int option,
    void *optval, size_t *optvallen);
static const struct nn_optset_vfptr nn_ws_optset_vfptr = {
    nn_ws_optset_destroy,
    nn_ws_optset_setopt,
    nn_ws_optset_getopt
};

/*  nn_transport interface. */
static int nn_ws_bind (struct nn_ep *);
static int nn_ws_connect (struct nn_ep *);
static struct nn_optset *nn_ws_optset (void);

struct nn_transport nn_ws = {
    "ws",
    NN_WS,
    NULL,
    NULL,
    nn_ws_bind,
    nn_ws_connect,
    nn_ws_optset,
};

static int nn_ws_bind (struct nn_ep *ep)
{
    return nn_bws_create (ep);
}

static int nn_ws_connect (struct nn_ep *ep)
{
    return nn_cws_create (ep);
}

static struct nn_optset *nn_ws_optset ()
{
    struct nn_ws_optset *optset;

    optset = nn_alloc (sizeof (struct nn_ws_optset), "optset (ws)");
    alloc_assert (optset);
    optset->base.vfptr = &nn_ws_optset_vfptr;

    /*  Default values for WebSocket options. */
    optset->msg_type = NN_WS_MSG_TYPE_BINARY;

    return &optset->base;   
}

static void nn_ws_optset_destroy (struct nn_optset *self)
{
    struct nn_ws_optset *optset;

    optset = nn_cont (self, struct nn_ws_optset, base);
    nn_free (optset);
}

static int nn_ws_optset_setopt (struct nn_optset *self, int option,
    const void *optval, size_t optvallen)
{
    struct nn_ws_optset *optset;
    int val;

    optset = nn_cont (self, struct nn_ws_optset, base);
    if (optvallen != sizeof (int)) {
        return -EINVAL;
    }
    val = *(int *)optval;

    switch (option) {
    case NN_WS_MSG_TYPE:
        switch (val) {
        case NN_WS_MSG_TYPE_TEXT:
        case NN_WS_MSG_TYPE_BINARY:
	    optset->msg_type = val;
            return 0;
        default:
            return -EINVAL;
        }
    default:
        return -ENOPROTOOPT;
    }
}

static int nn_ws_optset_getopt (struct nn_optset *self, int option,
    void *optval, size_t *optvallen)
{
    struct nn_ws_optset *optset;

    optset = nn_cont (self, struct nn_ws_optset, base);

    switch (option) {
    case NN_WS_MSG_TYPE:
        memcpy (optval, &optset->msg_type,
            *optvallen < sizeof (int) ? *optvallen : sizeof (int));
        *optvallen = sizeof (int);
        return 0;
    default:
        return -ENOPROTOOPT;
    }
}
