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

#include "wire.h"

#if defined NN_HAVE_WINDOWS
#include "win.h"
#else
#include <arpa/inet.h>
#endif

uint16_t nn_gets (const uint8_t *buf)
{
    return (((uint16_t) buf [0]) << 8) |
           ((uint16_t) buf [1]);
}

void nn_puts (uint8_t *buf, uint16_t val)
{
    buf [0] = (uint8_t) (((val) >> 8) & 0xff);
    buf [1] = (uint8_t) (val & 0xff);
}

uint32_t nn_getl (const uint8_t *buf)
{
    return (((uint32_t) buf [0]) << 24) |
           (((uint32_t) buf [1]) << 16) |
           (((uint32_t) buf [2]) << 8) |
           ((uint32_t) buf [3]);
}

void nn_putl (uint8_t *buf, uint32_t val)
{
    buf [0] = (uint8_t) (((val) >> 24) & 0xff);
    buf [1] = (uint8_t) (((val) >> 16) & 0xff);
    buf [2] = (uint8_t) (((val) >> 8) & 0xff);
    buf [3] = (uint8_t) (val & 0xff);
}

uint64_t nn_getll (const uint8_t *buf)
{
    return (((uint64_t) buf [0]) << 56) |
           (((uint64_t) buf [1]) << 48) |
           (((uint64_t) buf [2]) << 40) |
           (((uint64_t) buf [3]) << 32) |
           (((uint64_t) buf [4]) << 24) |
           (((uint64_t) buf [5]) << 16) |
           (((uint64_t) buf [6]) << 8) |
           (((uint64_t) buf [7] << 0));
}

void nn_putll (uint8_t *buf, uint64_t val)
{
    buf [0] = (uint8_t) ((val >> 56) & 0xff);
    buf [1] = (uint8_t) ((val >> 48) & 0xff);
    buf [2] = (uint8_t) ((val >> 40) & 0xff);
    buf [3] = (uint8_t) ((val >> 32) & 0xff);
    buf [4] = (uint8_t) ((val >> 24) & 0xff);
    buf [5] = (uint8_t) ((val >> 16) & 0xff);
    buf [6] = (uint8_t) ((val >> 8) & 0xff);
    buf [7] = (uint8_t) (val & 0xff);
}

