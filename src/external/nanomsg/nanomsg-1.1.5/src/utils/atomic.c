/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.

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

#include "atomic.h"
#include "err.h"
#include "attr.h"

void nn_atomic_init (struct nn_atomic *self, uint32_t n)
{
    self->n = n;
#if defined NN_ATOMIC_MUTEX
    nn_mutex_init (&self->sync);
#endif
}

#if defined NN_ATOMIC_MUTEX
void nn_atomic_term (struct nn_atomic *self)
{
    nn_mutex_term (&self->sync);
}
#else
void nn_atomic_term (NN_UNUSED struct nn_atomic *self)
{
}
#endif

uint32_t nn_atomic_inc (struct nn_atomic *self, uint32_t n)
{
#if defined NN_ATOMIC_WINAPI
    return (uint32_t) InterlockedExchangeAdd ((LONG*) &self->n, n);
#elif defined NN_ATOMIC_SOLARIS
    return atomic_add_32_nv (&self->n, n) - n;
#elif defined NN_ATOMIC_GCC_BUILTINS
    return (uint32_t) __sync_fetch_and_add (&self->n, n);
#elif defined NN_ATOMIC_MUTEX
    uint32_t res;
    nn_mutex_lock (&self->sync);
    res = self->n;
    self->n += n;
    nn_mutex_unlock (&self->sync);
    return res;
#else
#error
#endif
}

uint32_t nn_atomic_dec (struct nn_atomic *self, uint32_t n)
{
#if defined NN_ATOMIC_WINAPI
    return (uint32_t) InterlockedExchangeAdd ((LONG*) &self->n, -((LONG) n));
#elif defined NN_ATOMIC_SOLARIS
    return atomic_add_32_nv (&self->n, -((int32_t) n)) + n;
#elif defined NN_ATOMIC_GCC_BUILTINS
    return (uint32_t) __sync_fetch_and_sub (&self->n, n);
#elif defined NN_ATOMIC_MUTEX
    uint32_t res;
    nn_mutex_lock (&self->sync);
    res = self->n;
    self->n -= n;
    nn_mutex_unlock (&self->sync);
    return res;
#else
#error
#endif
}

