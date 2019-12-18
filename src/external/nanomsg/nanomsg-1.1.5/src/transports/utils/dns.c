/*
    Copyright (c) 2012-2013 Martin Sustrik  All rights reserved.

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

#include "dns.h"

#include "../../utils/err.h"

#include <string.h>

int nn_dns_check_hostname (const char *name, size_t namelen)
{
    int labelsz;

    /*  There has to be at least one label in the hostname.
        Additionally, hostnames are up to 255 characters long. */
    if (namelen < 1 || namelen > 255)
        return -EINVAL;

    /*  Hyphen can't be used as a first character of the hostname. */
    if (*name == '-')
        return -EINVAL;

    labelsz = 0;
    while (1) {

        /*  End of the hostname. */
        if (namelen == 0) {
            /*  Success! */
            return 0;
        }

        /*  End of a label. */
        if (*name == '.') {

            /*  The old label cannot be empty. */
            if (labelsz == 0)
                return -EINVAL;

            /*  Start new label. */
            labelsz = 0;
            ++name;
            --namelen;
            continue;
        }

        /*  Valid character. */
        if ((*name >= 'a' && *name <= 'z') ||
              (*name >= 'A' && *name <= 'Z') ||
              (*name >= '0' && *name <= '9') ||
              *name == '-') {
            ++name;
            --namelen;
            ++labelsz;

            /*  Labels longer than 63 charcters are not permitted. */
            if (labelsz > 63)
                return -EINVAL;

            continue;
        }

        /*  Invalid character. */
        return -EINVAL;
    }
}

#if defined NN_HAVE_GETADDRINFO_A && !defined NN_DISABLE_GETADDRINFO_A
#include "dns_getaddrinfo_a.inc"
#else
#include "dns_getaddrinfo.inc"
#endif
