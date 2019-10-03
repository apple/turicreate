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

#include "../nn.h"

#include "../utils/err.h"
#include "../utils/fast.h"
#include "../utils/fd.h"
#include "../utils/attr.h"
#include "../utils/thread.h"
#include "device.h"

#include <string.h>

int nn_custom_device(struct nn_device_recipe *device, int s1, int s2,
    int flags)
{
    return nn_device_entry (device, s1, s2, flags);
}

int nn_device (int s1, int s2)
{
    return nn_custom_device (&nn_ordinary_device, s1, s2, 0);
}

int nn_device_entry (struct nn_device_recipe *device, int s1, int s2,
    NN_UNUSED int flags)
{
    int rc;
    int op1;
    int op2;
    nn_fd s1rcv;
    nn_fd s1snd;
    nn_fd s2rcv;
    nn_fd s2snd;
    size_t opsz;

    /*  At least one socket must be specified. */
    if (device->required_checks & NN_CHECK_AT_LEAST_ONE_SOCKET) {
        if (s1 < 0 && s2 < 0) {
            errno = EBADF;
            return -1;
        }
    }

    /*  Handle the case when there's only one socket in the device. */
    if (device->required_checks & NN_CHECK_ALLOW_LOOPBACK) {
        if (s2 < 0)
            return nn_device_loopback (device, s1);
        if (s1 < 0)
            return nn_device_loopback (device, s2);
    }

    /*  Check whether both sockets are "raw" sockets. */
    if (device->required_checks & NN_CHECK_REQUIRE_RAW_SOCKETS) {
        opsz = sizeof (op1);
        rc = nn_getsockopt (s1, NN_SOL_SOCKET, NN_DOMAIN, &op1, &opsz);
        if (rc != 0)
            return -1;
        nn_assert (opsz == sizeof (op1));
        opsz = sizeof (op2);
        rc = nn_getsockopt (s2, NN_SOL_SOCKET, NN_DOMAIN, &op2, &opsz);
        if (rc != 0)
            return -1;
        nn_assert (opsz == sizeof (op2));
        if (op1 != AF_SP_RAW || op2 != AF_SP_RAW) {
            errno = EINVAL;
            return -1;
        }
    }

    /*  Check whether both sockets are from the same protocol. */
    if (device->required_checks & NN_CHECK_SAME_PROTOCOL_FAMILY) {
        opsz = sizeof (op1);
        rc = nn_getsockopt (s1, NN_SOL_SOCKET, NN_PROTOCOL, &op1, &opsz);
        if (rc != 0)
            return -1;
        nn_assert (opsz == sizeof (op1));
        opsz = sizeof (op2);
        rc = nn_getsockopt (s2, NN_SOL_SOCKET, NN_PROTOCOL, &op2, &opsz);
        if (rc != 0)
            return -1;
        nn_assert (opsz == sizeof (op2));
        if (op1 / 16 != op2 / 16) {
            errno = EINVAL;
            return -1;
        }
    }

    /*  Get the file descriptors for polling. */
    opsz = sizeof (s1rcv);
    rc = nn_getsockopt (s1, NN_SOL_SOCKET, NN_RCVFD, &s1rcv, &opsz);
    if (rc < 0) {
        if (nn_errno () != ENOPROTOOPT)
            return -1;
        s1rcv = -1;
    } else {
        nn_assert (rc == 0);
        nn_assert (opsz == sizeof (s1rcv));
        nn_assert (s1rcv >= 0);
    }
    opsz = sizeof (s1snd);
    rc = nn_getsockopt (s1, NN_SOL_SOCKET, NN_SNDFD, &s1snd, &opsz);
    if (rc < 0) {
        if (nn_errno () != ENOPROTOOPT)
            return -1;
        s1snd = -1;
    } else {
        nn_assert (rc == 0);
        nn_assert (opsz == sizeof (s1snd));
        nn_assert (s1snd >= 0);
    }
    opsz = sizeof (s2rcv);
    rc = nn_getsockopt (s2, NN_SOL_SOCKET, NN_RCVFD, &s2rcv, &opsz);
    if (rc < 0) {
        if (nn_errno () != ENOPROTOOPT)
            return -1;
        s2rcv = -1;
    } else {
        nn_assert (rc == 0);
        nn_assert (opsz == sizeof (s2rcv));
        nn_assert (s2rcv >= 0);
    }
    opsz = sizeof (s2snd);
    rc = nn_getsockopt (s2, NN_SOL_SOCKET, NN_SNDFD, &s2snd, &opsz);
    if (rc < 0) {
        if (nn_errno () != ENOPROTOOPT)
            return -1;
        s2snd = -1;
    } else {
        nn_assert (rc == 0);
        nn_assert (opsz == sizeof (s2snd));
        nn_assert (s2snd >= 0);
    }
    if (device->required_checks & NN_CHECK_SOCKET_DIRECTIONALITY) {
        /*  Check the directionality of the sockets. */
        if (s1rcv != -1 && s2snd == -1) {
            errno = EINVAL;
            return -1;
        }
        if (s1snd != -1 && s2rcv == -1) {
            errno = EINVAL;
            return -1;
        }
        if (s2rcv != -1 && s1snd == -1) {
            errno = EINVAL;
            return -1;
        }
        if (s2snd != -1 && s1rcv == -1) {
            errno = EINVAL;
            return -1;
        }
    }

    /*  Two-directional device. */
    if (device->required_checks & NN_CHECK_ALLOW_BIDIRECTIONAL) {
        if (s1rcv != -1 && s1snd != -1 && s2rcv != -1 && s2snd != -1)
            return nn_device_twoway (device, s1, s2);
    }

    if (device->required_checks & NN_CHECK_ALLOW_UNIDIRECTIONAL) {
        /*  Single-directional device passing messages from s1 to s2. */
        if (s1rcv != -1 && s1snd == -1 && s2rcv == -1 && s2snd != -1)
            return nn_device_oneway (device, s1, s2);

        /*  Single-directional device passing messages from s2 to s1. */
        if (s1rcv == -1 && s1snd != -1 && s2rcv != -1 && s2snd == -1)
            return nn_device_oneway (device, s2, s1);
    }

    /*  This should never happen. */
    nn_assert (0);
}

int nn_device_loopback (struct nn_device_recipe *device, int s)
{
    int rc;
    int op;
    size_t opsz;

    /*  Check whether the socket is a "raw" socket. */
    opsz = sizeof (op);
    rc = nn_getsockopt (s, NN_SOL_SOCKET, NN_DOMAIN, &op, &opsz);
    if (nn_slow (rc != 0))
        return -1;
    nn_assert (opsz == sizeof (op));
    if (op != AF_SP_RAW) {
        errno = EINVAL;
        return -1;
    }

    for (;;) {
        rc = nn_device_mvmsg (device, s, s, 0);
        if (nn_slow (rc < 0))
            return -1;
    }
}

struct nn_device_forwarder_args {
    struct nn_device_recipe *device;
    int s1;
    int s2;
    int rc;
    int err;
};

static void nn_device_forwarder (void *a)
{
    struct nn_device_forwarder_args *args = a;
    for (;;) {
        args->rc = nn_device_mvmsg (args->device, args->s1, args->s2, 0);
        if (nn_slow (args->rc < 0)) {
            args->err = nn_errno ();
            return;
        }
    }
}

int nn_device_twoway (struct nn_device_recipe *device, int s1, int s2)
{
    struct nn_thread t1;
    struct nn_thread t2;
    struct nn_device_forwarder_args a1;
    struct nn_device_forwarder_args a2;

    a1.device = device;
    a1.s1 = s1;
    a1.s2 = s2;

    a2.device = device;
    a2.s1 = s2;
    a2.s2 = s1;

    nn_thread_init (&t1, nn_device_forwarder, &a1);
    nn_thread_init (&t2, nn_device_forwarder, &a2);

    nn_thread_term (&t1);
    nn_thread_term (&t2);

    if (a1.rc != 0) {
        errno = a1.err;
        return (a1.rc);
    }
    errno = a2.err;
    return a2.rc;
}

int nn_device_oneway (struct nn_device_recipe *device, int s1, int s2)
{
    int rc;

    while (1) {
        rc = nn_device_mvmsg (device, s1, s2, 0);
        if (nn_slow (rc < 0))
            return -1;
    }
}

int nn_device_mvmsg (struct nn_device_recipe *device,
    int from, int to, int flags)
{
    int rc;
    void *body;
    void *control;
    struct nn_iovec iov;
    struct nn_msghdr hdr;

    iov.iov_base = &body;
    iov.iov_len = NN_MSG;
    memset (&hdr, 0, sizeof (hdr));
    hdr.msg_iov = &iov;
    hdr.msg_iovlen = 1;
    hdr.msg_control = &control;
    hdr.msg_controllen = NN_MSG;
    rc = nn_recvmsg (from, &hdr, flags);
    if (nn_slow (rc < 0 && (nn_errno () == ETERM || nn_errno () == EBADF))) {
        return -1;
    }
    errno_assert (rc >= 0);

    rc = device->nn_device_rewritemsg (device, from, to, flags, &hdr, rc);
    if (nn_slow (rc == -1))
        return -1;
    else if (rc == 0)
        return 0;
    nn_assert(rc == 1);

    rc = nn_sendmsg (to, &hdr, flags);
    if (nn_slow (rc < 0 && (nn_errno () == ETERM || nn_errno () == EBADF))) {
        return -1;
    }
    errno_assert (rc >= 0);
    return 0;
}

int nn_device_rewritemsg (NN_UNUSED struct nn_device_recipe *device,
    NN_UNUSED int from, NN_UNUSED int to, NN_UNUSED int flags,
    NN_UNUSED struct nn_msghdr *msghdr, NN_UNUSED int bytes)
{
    return 1; /* always forward */
}
