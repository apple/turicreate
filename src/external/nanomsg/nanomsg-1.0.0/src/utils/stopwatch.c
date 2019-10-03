/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.

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

#include "stopwatch.h"

#if defined NN_HAVE_WINDOWS

#include "win.h"

void nn_stopwatch_init (struct nn_stopwatch *self)
{
    LARGE_INTEGER time;

    QueryPerformanceCounter (&time);
    self->start = (uint64_t) (time.QuadPart);
}

uint64_t nn_stopwatch_term (struct nn_stopwatch *self)
{
    LARGE_INTEGER tps;
    LARGE_INTEGER time;

    QueryPerformanceFrequency (&tps);
    QueryPerformanceCounter (&time);
    return (uint64_t) ((time.QuadPart - self->start) * 1000000 / tps.QuadPart);
}

#else

#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

void nn_stopwatch_init (struct nn_stopwatch *self)
{
    int rc;
    struct timeval tv;

    rc = gettimeofday (&tv, NULL);
    assert (rc == 0);
    self->start = (uint64_t) (((uint64_t) tv.tv_sec) * 1000000 + tv.tv_usec);
}

uint64_t nn_stopwatch_term (struct nn_stopwatch *self)
{
    int rc;
    struct timeval tv;
    uint64_t end;

    rc = gettimeofday (&tv, NULL);
    assert (rc == 0);
    end = (uint64_t) (((uint64_t) tv.tv_sec) * 1000000 + tv.tv_usec);
    return end - self->start;
}

#endif
