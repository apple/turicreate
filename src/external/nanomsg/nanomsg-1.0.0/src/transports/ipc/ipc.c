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

#include "ipc.h"
#include "bipc.h"
#include "cipc.h"

#include "../../ipc.h"

#include "../../utils/err.h"
#include "../../utils/alloc.h"
#include "../../utils/fast.h"
#include "../../utils/list.h"
#include "../../utils/cont.h"

#include <string.h>
#if defined NN_HAVE_WINDOWS
#include "../../utils/win.h"
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

/*  IPC-specific socket options. */
struct nn_ipc_optset {
    struct nn_optset base;

    /* Win32 Security Attribute */
    void* sec_attr;

    int outbuffersz;
    int inbuffersz;
};

static void nn_ipc_optset_destroy (struct nn_optset *self);
static int nn_ipc_optset_setopt (struct nn_optset *self, int option,
    const void *optval, size_t optvallen);
static int nn_ipc_optset_getopt (struct nn_optset *self, int option,
    void *optval, size_t *optvallen);
static const struct nn_optset_vfptr nn_ipc_optset_vfptr = {
    nn_ipc_optset_destroy,
    nn_ipc_optset_setopt,
    nn_ipc_optset_getopt
};

/*  nn_transport interface. */
static int nn_ipc_bind (void *hint, struct nn_epbase **epbase);
static int nn_ipc_connect (void *hint, struct nn_epbase **epbase);
static struct nn_optset *nn_ipc_optset (void);

static struct nn_transport nn_ipc_vfptr = {
    "ipc",
    NN_IPC,
    NULL,
    NULL,
    nn_ipc_bind,
    nn_ipc_connect,
    nn_ipc_optset,
    NN_LIST_ITEM_INITIALIZER
};

struct nn_transport *nn_ipc = &nn_ipc_vfptr;

static int nn_ipc_bind (void *hint, struct nn_epbase **epbase)
{
    return nn_bipc_create (hint, epbase);
}

static int nn_ipc_connect (void *hint, struct nn_epbase **epbase)
{
    return nn_cipc_create (hint, epbase);
}

static struct nn_optset *nn_ipc_optset ()
{
    struct nn_ipc_optset *optset;

    optset = nn_alloc (sizeof (struct nn_ipc_optset), "optset (ipc)");
    alloc_assert (optset);
    optset->base.vfptr = &nn_ipc_optset_vfptr;

    /*  Default values for the IPC options */
    optset->sec_attr = NULL;
    optset->outbuffersz = 4096;
    optset->inbuffersz = 4096;

    return &optset->base;
}

static void nn_ipc_optset_destroy (struct nn_optset *self)
{
    struct nn_ipc_optset *optset;

    optset = nn_cont (self, struct nn_ipc_optset, base);
    nn_free (optset);
}

static int nn_ipc_optset_setopt (struct nn_optset *self, int option,
    const void *optval, size_t optvallen)
{
    struct nn_ipc_optset *optset;

    optset = nn_cont (self, struct nn_ipc_optset, base);
    if (optvallen < sizeof (int)) {
        return -EINVAL;
    }

    switch (option) {
    case NN_IPC_SEC_ATTR:
        optset->sec_attr = (void *)optval;
        return 0;
    case NN_IPC_OUTBUFSZ:
        optset->outbuffersz = *(int *)optval;
        return 0;
    case NN_IPC_INBUFSZ:
        optset->inbuffersz = *(int *)optval;
        return 0;
    default:
        return -ENOPROTOOPT;
    }
}

static int nn_ipc_optset_getopt (struct nn_optset *self, int option,
    void *optval, size_t *optvallen)
{
    struct nn_ipc_optset *optset;

    optset = nn_cont (self, struct nn_ipc_optset, base);

    switch (option) {
    case NN_IPC_SEC_ATTR:
        memcpy(optval, &optset->sec_attr, sizeof(optset->sec_attr));
        *optvallen = sizeof(optset->sec_attr);
        return 0;
    case NN_IPC_OUTBUFSZ:
        *(int *)optval = optset->outbuffersz;
        *optvallen = sizeof (int);
        return 0;
    case NN_IPC_INBUFSZ:
        *(int *)optval = optset->inbuffersz;
        *optvallen = sizeof (int);
        return 0;
    default:
        return -ENOPROTOOPT;
    }
}
