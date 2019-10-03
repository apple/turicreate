/*
    Copyright (c) 2012 Martin Sustrik  All rights reserved.
    Copyright (c) 2012 Julien Ammous

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

#if defined NN_HAVE_WINDOWS
#include "win.h"
#elif defined NN_HAVE_OSX
#include <mach/mach_time.h>
#elif defined NN_HAVE_CLOCK_MONOTONIC || defined NN_HAVE_GETHRTIME
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "clock.h"
#include "fast.h"
#include "err.h"
#include "attr.h"

uint64_t nn_clock_ms (void)
{
#if defined NN_HAVE_WINDOWS

    LARGE_INTEGER tps;
    LARGE_INTEGER time;
    double tpms;

    QueryPerformanceFrequency (&tps);
    QueryPerformanceCounter (&time);
    tpms = (double) (tps.QuadPart / 1000);
    return (uint64_t) (time.QuadPart / tpms);

#elif defined NN_HAVE_OSX

    static mach_timebase_info_data_t nn_clock_timebase_info;
    uint64_t ticks;

    /*  If the global timebase info is not initialised yet, init it. */
    if (nn_slow (!nn_clock_timebase_info.denom))
        mach_timebase_info (&nn_clock_timebase_info);

    ticks = mach_absolute_time ();
    return ticks * nn_clock_timebase_info.numer /
        nn_clock_timebase_info.denom / 1000000;

#elif defined NN_HAVE_CLOCK_MONOTONIC

    int rc;
    struct timespec tv;

    rc = clock_gettime (CLOCK_MONOTONIC, &tv);
    errno_assert (rc == 0);
    return tv.tv_sec * (uint64_t) 1000 + tv.tv_nsec / 1000000;

#elif defined NN_HAVE_GETHRTIME

    return gethrtime () / 1000000;

#else

    int rc;
    struct timeval tv;

    /*  Gettimeofday is slow on some systems. Moreover, it's not necessarily
        monotonic. Thus, it's used as a last resort mechanism. */
    rc = gettimeofday (&tv, NULL);
    errno_assert (rc == 0);
    return tv.tv_sec * (uint64_t) 1000 + tv.tv_usec / 1000;

#endif
}
