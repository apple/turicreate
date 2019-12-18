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

#include "btcp.h"
#include "ctcp.h"

#include "../../tcp.h"

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

/*  TCP-specific socket options. */

struct nn_tcp_optset {
    struct nn_optset base;
    int nodelay;
};

static void nn_tcp_optset_destroy (struct nn_optset *self);
static int nn_tcp_optset_setopt (struct nn_optset *self, int option,
    const void *optval, size_t optvallen);
static int nn_tcp_optset_getopt (struct nn_optset *self, int option,
    void *optval, size_t *optvallen);
static const struct nn_optset_vfptr nn_tcp_optset_vfptr = {
    nn_tcp_optset_destroy,
    nn_tcp_optset_setopt,
    nn_tcp_optset_getopt
};

/*  nn_transport interface. */
static int nn_tcp_bind (struct nn_ep *ep);
static int nn_tcp_connect (struct nn_ep *ep);
static struct nn_optset *nn_tcp_optset (void);

struct nn_transport nn_tcp = {
    "tcp",
    NN_TCP,
    NULL,
    NULL,
    nn_tcp_bind,
    nn_tcp_connect,
    nn_tcp_optset,
};

static int nn_tcp_bind (struct nn_ep *ep)
{
    return nn_btcp_create (ep);
}

static int nn_tcp_connect (struct nn_ep *ep)
{
    return nn_ctcp_create (ep);
}

static struct nn_optset *nn_tcp_optset ()
{
    struct nn_tcp_optset *optset;

    optset = nn_alloc (sizeof (struct nn_tcp_optset), "optset (tcp)");
    alloc_assert (optset);
    optset->base.vfptr = &nn_tcp_optset_vfptr;

    /*  Default values for TCP socket options. */
    optset->nodelay = 0;

    return &optset->base;   
}

static void nn_tcp_optset_destroy (struct nn_optset *self)
{
    struct nn_tcp_optset *optset;

    optset = nn_cont (self, struct nn_tcp_optset, base);
    nn_free (optset);
}

static int nn_tcp_optset_setopt (struct nn_optset *self, int option,
    const void *optval, size_t optvallen)
{
    struct nn_tcp_optset *optset;
    int val;

    optset = nn_cont (self, struct nn_tcp_optset, base);

    /*  At this point we assume that all options are of type int. */
    if (optvallen != sizeof (int))
        return -EINVAL;
    val = *(int*) optval;

    switch (option) {
    case NN_TCP_NODELAY:
        if (nn_slow (val != 0 && val != 1))
            return -EINVAL;
        optset->nodelay = val;
        return 0;
    default:
        return -ENOPROTOOPT;
    }
}

static int nn_tcp_optset_getopt (struct nn_optset *self, int option,
    void *optval, size_t *optvallen)
{
    struct nn_tcp_optset *optset;
    int intval;

    optset = nn_cont (self, struct nn_tcp_optset, base);

    switch (option) {
    case NN_TCP_NODELAY:
        intval = optset->nodelay;
        break;
    default:
        return -ENOPROTOOPT;
    }
    memcpy (optval, &intval,
        *optvallen < sizeof (int) ? *optvallen : sizeof (int));
    *optvallen = sizeof (int);
    return 0;
}

