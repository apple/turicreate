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

#include "random.h"
#include "clock.h"
#include "fast.h"

#ifdef NN_HAVE_WINDOWS
#include "win.h"
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include <string.h>

static uint64_t nn_random_state;

void nn_random_seed ()
{
    uint64_t pid;

#ifdef NN_HAVE_WINDOWS
    pid = (uint64_t) GetCurrentProcessId ();
#else
    pid = (uint64_t) getpid ();
#endif

    /*  The initial state for pseudo-random number generator is computed from
        the exact timestamp and process ID. */
    memcpy (&nn_random_state, "\xfa\x9b\x23\xe3\x07\xcc\x61\x1f", 8);
    nn_random_state ^= pid + nn_clock_ms();
}

void nn_random_generate (void *buf, size_t len)
{
    uint8_t *pos;

    pos = (uint8_t*) buf;

    while (1) {

        /*  Generate a pseudo-random integer. */
        nn_random_state = nn_random_state * 1103515245 + 12345;

        /*  Move the bytes to the output buffer. */
        memcpy (pos, &nn_random_state, len > 8 ? 8 : len);
        if (nn_fast (len <= 8))
            return;
        len -= 8;
        pos += 8;
    }
}

