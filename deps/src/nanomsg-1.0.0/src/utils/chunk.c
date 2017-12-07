/*
    Copyright (c) 2013 Martin Sustrik  All rights reserved.
    Copyright (c) 2014 Achille Roussel All rights reserved.

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

#include "chunk.h"
#include "atomic.h"
#include "alloc.h"
#include "fast.h"
#include "wire.h"
#include "err.h"

#include <string.h>

#define NN_CHUNK_TAG 0xdeadcafe
#define NN_CHUNK_TAG_DEALLOCATED 0xbeadfeed

typedef void (*nn_chunk_free_fn) (void *p);

struct nn_chunk {

    /*  Number of places the chunk is referenced from. */
    struct nn_atomic refcount;

    /*  Size of the message in bytes. */
    size_t size;

    /*  Deallocation function. */
    nn_chunk_free_fn ffn;

    /*  The structure if followed by optional empty space, a 32 bit unsigned
        integer specifying the size of said empty space, a 32 bit tag and
        the message data itself. */
};

/*  Private functions. */
static struct nn_chunk *nn_chunk_getptr (void *p);
static void *nn_chunk_getdata (struct nn_chunk *c);
static void nn_chunk_default_free (void *p);
static size_t nn_chunk_hdrsize ();

int nn_chunk_alloc (size_t size, int type, void **result)
{
    size_t sz;
    struct nn_chunk *self;
    const size_t hdrsz = nn_chunk_hdrsize ();

    /*  Compute total size to be allocated. Check for overflow. */
    sz = hdrsz + size;
    if (nn_slow (sz < hdrsz))
        return -ENOMEM;

    /*  Allocate the actual memory depending on the type. */
    switch (type) {
    case 0:
        self = nn_alloc (sz, "message chunk");
        break;
    default:
        return -EINVAL;
    }
    if (nn_slow (!self))
        return -ENOMEM;

    /*  Fill in the chunk header. */
    nn_atomic_init (&self->refcount, 1);
    self->size = size;
    self->ffn = nn_chunk_default_free;

    /*  Fill in the size of the empty space between the chunk header
        and the message. */
    nn_putl ((uint8_t*) ((uint32_t*) (self + 1)), 0);

    /*  Fill in the tag. */
    nn_putl ((uint8_t*) ((((uint32_t*) (self + 1))) + 1), NN_CHUNK_TAG);

    *result = nn_chunk_getdata (self);
    return 0;
}

int nn_chunk_realloc (size_t size, void **chunk)
{
    struct nn_chunk *self;
    struct nn_chunk *new_chunk;
    void *new_ptr;
    size_t hdr_size;
    size_t new_size;
    int rc;

    self = nn_chunk_getptr (*chunk);

    /*  Check if we only have one reference to this object, in that case we can
        reallocate the memory chunk. */
    if (self->refcount.n == 1) {

        /* Compute new size, check for overflow. */
        hdr_size = nn_chunk_hdrsize ();
        new_size = hdr_size + size;
        if (nn_slow (new_size < hdr_size))
            return -ENOMEM;

        /*  Reallocate memory chunk. */
        new_chunk = nn_realloc (self, new_size);
        if (nn_slow (new_chunk == NULL))
            return -ENOMEM;

        new_chunk->size = size;
        *chunk = nn_chunk_getdata (new_chunk);
    }

    /*  There are many references to this memory chunk, we have to create a new
        one and copy the data. */
    else {
        new_ptr = NULL;
        rc = nn_chunk_alloc (size, 0, &new_ptr);

        if (nn_slow (rc != 0)) {
            return rc;
        }

        memcpy (new_ptr, nn_chunk_getdata (self), self->size);
        *chunk = new_ptr;
        nn_atomic_dec (&self->refcount, 1);
    }

    return 0;
}

void nn_chunk_free (void *p)
{
    struct nn_chunk *self;

    self = nn_chunk_getptr (p);

    /*  Decrement the reference count. Actual deallocation happens only if
        it drops to zero. */
    if (nn_atomic_dec (&self->refcount, 1) <= 1) {

        /*  Mark chunk as deallocated. */
        nn_putl ((uint8_t*) (((uint32_t*) p) - 1), NN_CHUNK_TAG_DEALLOCATED);

        /*  Deallocate the resources held by the chunk. */
        nn_atomic_term (&self->refcount);

        /*  Deallocate the memory block according to the allocation
            mechanism specified. */
        self->ffn (self);
    }
}

void nn_chunk_addref (void *p, uint32_t n)
{
    struct nn_chunk *self;

    self = nn_chunk_getptr (p);

    nn_atomic_inc (&self->refcount, n);
}


size_t nn_chunk_size (void *p)
{
    return nn_chunk_getptr (p)->size;
}

void *nn_chunk_trim (void *p, size_t n)
{
    struct nn_chunk *self;
    const size_t hdrsz = sizeof (struct nn_chunk) + 2 * sizeof (uint32_t);
    size_t empty_space;

    self = nn_chunk_getptr (p);

    /*  Sanity check. We cannot trim more bytes than there are in the chunk. */
    nn_assert (n <= self->size);

    /*  Adjust the chunk header. */
    p = ((uint8_t*) p) + n;
    nn_putl ((uint8_t*) (((uint32_t*) p) - 1), NN_CHUNK_TAG);
    empty_space = (uint8_t*) p - (uint8_t*) self - hdrsz;
    nn_assert(empty_space < UINT32_MAX);
    nn_putl ((uint8_t*) (((uint32_t*) p) - 2), (uint32_t) empty_space);

    /*  Adjust the size of the message. */
    self->size -= n;

    return p;
}

static struct nn_chunk *nn_chunk_getptr (void *p)
{
    uint32_t off;

    nn_assert (nn_getl ((uint8_t*) p - sizeof (uint32_t)) == NN_CHUNK_TAG);
    off = nn_getl ((uint8_t*) p - 2 * sizeof (uint32_t));

    return (struct  nn_chunk*) ((uint8_t*) p - 2 *sizeof (uint32_t) - off -
        sizeof (struct nn_chunk));
}

static void *nn_chunk_getdata (struct nn_chunk *self)
{
    return ((uint8_t*) (self + 1)) + 2 * sizeof (uint32_t);
}

static void nn_chunk_default_free (void *p)
{
    nn_free (p);
}

static size_t nn_chunk_hdrsize ()
{
    return sizeof (struct nn_chunk) + 2 * sizeof (uint32_t);
}
