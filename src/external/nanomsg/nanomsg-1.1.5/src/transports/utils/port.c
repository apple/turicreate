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

#include "port.h"

#include "../../utils/err.h"

int nn_port_resolve (const char *port, size_t portlen)
{
    int res;
    size_t i;

    res = 0;
    for (i = 0; i != portlen; ++i) {
        if (port [i] < '0' || port [i] > '9')
            return -EINVAL;
        res *= 10;
        res += port [i] - '0';
        if (res > 0xffff)
            return -EINVAL;
    }

    /*  Port 0 has special meaning (assign an ephemeral port to the socket),
        thus it is illegal to use it in the connection string. */
    if (res == 0)
        return -EINVAL;

    return res;
}

