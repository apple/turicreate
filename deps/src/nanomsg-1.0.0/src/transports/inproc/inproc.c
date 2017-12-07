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

#include "inproc.h"
#include "ins.h"
#include "binproc.h"
#include "cinproc.h"

#include "../../inproc.h"

#include <string.h>

/*  nn_transport interface. */
static void nn_inproc_init (void);
static void nn_inproc_term (void);
static int nn_inproc_bind (void *hint, struct nn_epbase **epbase);
static int nn_inproc_connect (void *hint, struct nn_epbase **epbase);

static struct nn_transport nn_inproc_vfptr = {
    "inproc",
    NN_INPROC,
    nn_inproc_init,
    nn_inproc_term,
    nn_inproc_bind,
    nn_inproc_connect,
    NULL,
    NN_LIST_ITEM_INITIALIZER
};

struct nn_transport *nn_inproc = &nn_inproc_vfptr;

static void nn_inproc_init (void)
{
    nn_ins_init ();
}

static void nn_inproc_term (void)
{
    nn_ins_term ();
}

static int nn_inproc_bind (void *hint, struct nn_epbase **epbase)
{
    return nn_binproc_create (hint, epbase);
}

static int nn_inproc_connect (void *hint, struct nn_epbase **epbase)
{
    return nn_cinproc_create (hint, epbase);
}
