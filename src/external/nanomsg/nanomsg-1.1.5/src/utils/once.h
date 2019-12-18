/*
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

#ifndef NN_ONCE_INCLUDED
#define NN_ONCE_INCLUDED

#ifdef NN_HAVE_WINDOWS
#include "win.h"
struct nn_once {
    INIT_ONCE once;
};
#define	NN_ONCE_INITIALIZER	{ INIT_ONCE_STATIC_INIT }

#else /* !NN_HAVE_WINDOWS */

#include <pthread.h>

struct nn_once {
    pthread_once_t once;
};
#define NN_ONCE_INITIALIZER	{ PTHREAD_ONCE_INIT }

#endif /* NN_HAVE_WINDOWS */

typedef struct nn_once nn_once_t;
void nn_do_once (nn_once_t *once, void (*func)(void));

#endif /* NN_ONCE_INCLUDED */
