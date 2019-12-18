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

#include "alloc.h"

#if defined NN_ALLOC_MONITOR

#include "mutex.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

struct nn_alloc_hdr {
    size_t size;
    const char *name;
};

static struct nn_mutex nn_alloc_sync;
static size_t nn_alloc_bytes;
static size_t nn_alloc_blocks;

void nn_alloc_init (void)
{
    nn_mutex_init (&nn_alloc_sync);
    nn_alloc_bytes = 0;
    nn_alloc_blocks = 0;
}

void nn_alloc_term (void)
{
    nn_mutex_term (&nn_alloc_sync);
}

void *nn_alloc_ (size_t size, const char *name)
{
    uint8_t *chunk;

    chunk = malloc (sizeof (struct nn_alloc_hdr) + size);
    if (!chunk)
        return NULL;

    nn_mutex_lock (&nn_alloc_sync);
    ((struct nn_alloc_hdr*) chunk)->size = size;
    ((struct nn_alloc_hdr*) chunk)->name = name;
    nn_alloc_bytes += size;
    ++nn_alloc_blocks;
    printf ("Allocating %s (%zu bytes)\n", name, size);
    printf ("Current memory usage: %zu bytes in %zu blocks\n",
        nn_alloc_bytes, nn_alloc_blocks);
    nn_mutex_unlock (&nn_alloc_sync);

    return chunk + sizeof (struct nn_alloc_hdr);
}

void *nn_realloc (void *ptr, size_t size)
{
    struct nn_alloc_hdr *oldchunk;
    struct nn_alloc_hdr *newchunk;
    size_t oldsize;

    oldchunk = ((struct nn_alloc_hdr*) ptr) - 1;
    oldsize = oldchunk->size;
    newchunk = realloc (oldchunk, sizeof (struct nn_alloc_hdr) + size);
    if (!newchunk)
        return NULL;
    newchunk->size = size;

    nn_mutex_lock (&nn_alloc_sync);
    nn_alloc_bytes -= oldsize;
    nn_alloc_bytes += size;
    printf ("Reallocating %s (%zu bytes to %zu bytes)\n",
        newchunk->name, oldsize, size);
    printf ("Current memory usage: %zu bytes in %zu blocks\n",
        nn_alloc_bytes, nn_alloc_blocks);
    nn_mutex_unlock (&nn_alloc_sync);

    return newchunk + sizeof (struct nn_alloc_hdr);
}

void nn_free (void *ptr)
{
    struct nn_alloc_hdr *chunk;
    
    if (!ptr)
        return;
    chunk = ((struct nn_alloc_hdr*) ptr) - 1;

    nn_mutex_lock (&nn_alloc_sync);
    nn_alloc_bytes -= chunk->size;
    --nn_alloc_blocks;
    printf ("Deallocating %s (%zu bytes)\n", chunk->name, chunk->size);
    printf ("Current memory usage: %zu bytes in %zu blocks\n",
        nn_alloc_bytes, nn_alloc_blocks);
    nn_mutex_unlock (&nn_alloc_sync);

    free (chunk);
}

#else

#include <stdlib.h>

void nn_alloc_init (void)
{
}

void nn_alloc_term (void)
{
}

void *nn_alloc_ (size_t size)
{
    return malloc (size);
}

void *nn_realloc (void *ptr, size_t size)
{
    return realloc (ptr, size);
}

void nn_free (void *ptr)
{
    free (ptr);
}

#endif

