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

#include "xbus.h"

#include "../../nn.h"
#include "../../bus.h"

#include "../../utils/cont.h"
#include "../../utils/alloc.h"
#include "../../utils/err.h"

struct nn_bus {
    struct nn_xbus xbus;
};

/*  Private functions. */
static void nn_bus_init (struct nn_bus *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint);
static void nn_bus_term (struct nn_bus *self);

/*  Implementation of nn_sockbase's virtual functions. */
static void nn_bus_destroy (struct nn_sockbase *self);
static int nn_bus_send (struct nn_sockbase *self, struct nn_msg *msg);
static int nn_bus_recv (struct nn_sockbase *self, struct nn_msg *msg);
static const struct nn_sockbase_vfptr nn_bus_sockbase_vfptr = {
    NULL,
    nn_bus_destroy,
    nn_xbus_add,
    nn_xbus_rm,
    nn_xbus_in,
    nn_xbus_out,
    nn_xbus_events,
    nn_bus_send,
    nn_bus_recv,
    NULL,
    NULL
};

static void nn_bus_init (struct nn_bus *self,
    const struct nn_sockbase_vfptr *vfptr, void *hint)
{
    nn_xbus_init (&self->xbus, vfptr, hint);
}

static void nn_bus_term (struct nn_bus *self)
{
    nn_xbus_term (&self->xbus);
}

static void nn_bus_destroy (struct nn_sockbase *self)
{
    struct nn_bus *bus;

    bus = nn_cont (self, struct nn_bus, xbus.sockbase);

    nn_bus_term (bus);
    nn_free (bus);
}

static int nn_bus_send (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;
    struct nn_bus *bus;

    bus = nn_cont (self, struct nn_bus, xbus.sockbase);

    /*  Check for malformed messages. */
    if (nn_chunkref_size (&msg->sphdr))
        return -EINVAL;

    /*  Send the message. */
    rc = nn_xbus_send (&bus->xbus.sockbase, msg);
    errnum_assert (rc == 0, -rc);

    return 0;
}

static int nn_bus_recv (struct nn_sockbase *self, struct nn_msg *msg)
{
    int rc;
    struct nn_bus *bus;

    bus = nn_cont (self, struct nn_bus, xbus.sockbase);

    /*  Get next message. */
    rc = nn_xbus_recv (&bus->xbus.sockbase, msg);
    if (nn_slow (rc == -EAGAIN))
        return -EAGAIN;
    errnum_assert (rc == 0, -rc);
    nn_assert (nn_chunkref_size (&msg->sphdr) == sizeof (uint64_t));

    /*  Discard the header. */
    nn_chunkref_term (&msg->sphdr);
    nn_chunkref_init (&msg->sphdr, 0);
    
    return 0;
}

static int nn_bus_create (void *hint, struct nn_sockbase **sockbase)
{
    struct nn_bus *self;

    self = nn_alloc (sizeof (struct nn_bus), "socket (bus)");
    alloc_assert (self);
    nn_bus_init (self, &nn_bus_sockbase_vfptr, hint);
    *sockbase = &self->xbus.sockbase;

    return 0;
}

struct nn_socktype nn_bus_socktype = {
    AF_SP,
    NN_BUS,
    0,
    nn_bus_create,
    nn_xbus_ispeer,
};
