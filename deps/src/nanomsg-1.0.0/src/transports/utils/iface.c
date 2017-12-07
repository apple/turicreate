/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.
    Copyright 2015 Garrett D'Amore <garrett@damore.org>

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

#include "iface.h"
#include "literal.h"

#include "../../utils/err.h"
#include "../../utils/closefd.h"

#include <string.h>

#ifndef NN_HAVE_WINDOWS
#include <sys/types.h>
#include <netinet/in.h>
#endif

/*  Private functions. */
static void nn_iface_any (int ipv4only, struct sockaddr_storage *result,
    size_t *resultlen);

/*  We no longer resolve interface names.  This feature was non-portable
    and fragile.  Only IP addresses may be used.  They are provided in
    the form of string literals. */
int nn_iface_resolve (const char *addr, size_t addrlen, int ipv4only,
    struct sockaddr_storage *result, size_t *resultlen)
{
    int rc;

    /*  Asterisk is a special name meaning "all interfaces". */
    if (addrlen == 1 && addr [0] == '*') {
        nn_iface_any (ipv4only, result, resultlen);
        return 0;
    }

    rc = nn_literal_resolve (addr, addrlen, ipv4only, result, resultlen);
    if (rc == -EINVAL)
        return -ENODEV;
    errnum_assert (rc == 0, -rc);
    return 0;
}

static void nn_iface_any (int ipv4only, struct sockaddr_storage *result,
    size_t *resultlen)
{
    if (ipv4only) {
        if (result) {
            result->ss_family = AF_INET;
            ((struct sockaddr_in*) result)->sin_addr.s_addr =
                htonl (INADDR_ANY);
        }
        if (resultlen)
            *resultlen = sizeof (struct sockaddr_in);
    }
    else {
        if (result) {
            result->ss_family = AF_INET6;
            memcpy (&((struct sockaddr_in6*) result)->sin6_addr,
                &in6addr_any, sizeof (in6addr_any));
        }
        if (resultlen)
            *resultlen = sizeof (struct sockaddr_in6);
    }
}
