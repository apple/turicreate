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

#include "chunkref.h"
#include "err.h"

#include <string.h>

/*  nn_chunkref should be reinterpreted as this structure in case the first
    byte ('tag') is 0xff. */
struct nn_chunkref_chunk {
    uint8_t tag;
    void *chunk;
};

/*  Check whether VSM are small enough for size to fit into the first byte
    of the structure. */
CT_ASSERT (NN_CHUNKREF_MAX < 255);

/*  Check whether nn_chunkref_chunk fits into nn_chunkref. */
CT_ASSERT (sizeof (struct nn_chunkref) >= sizeof (struct nn_chunkref_chunk));

void nn_chunkref_init (struct nn_chunkref *self, size_t size)
{
    int rc;
    struct nn_chunkref_chunk *ch;

    if (size < NN_CHUNKREF_MAX) {
        self->u.ref [0] = (uint8_t) size;
        return;
    }

    ch = (struct nn_chunkref_chunk*) self;
    ch->tag = 0xff;
    rc = nn_chunk_alloc (size, 0, &ch->chunk);
    errno_assert (rc == 0);
}

void nn_chunkref_init_chunk (struct nn_chunkref *self, void *chunk)
{
    struct nn_chunkref_chunk *ch;

    ch = (struct nn_chunkref_chunk*) self;
    ch->tag = 0xff;
    ch->chunk = chunk;
}

void nn_chunkref_term (struct nn_chunkref *self)
{
    struct nn_chunkref_chunk *ch;

    if (self->u.ref [0] == 0xff) {
        ch = (struct nn_chunkref_chunk*) self;
        nn_chunk_free (ch->chunk);
    }
}

void *nn_chunkref_getchunk (struct nn_chunkref *self)
{
    int rc;
    struct nn_chunkref_chunk *ch;
    void *chunk;

    if (self->u.ref [0] == 0xff) {
        ch = (struct nn_chunkref_chunk*) self;
        self->u.ref [0] = 0;
        return ch->chunk;
    }

    rc = nn_chunk_alloc (self->u.ref [0], 0, &chunk);
    errno_assert (rc == 0);
    memcpy (chunk, &self->u.ref [1], self->u.ref [0]);
    self->u.ref [0] = 0;
    return chunk;
}

void nn_chunkref_mv (struct nn_chunkref *dst, struct nn_chunkref *src)
{
    memcpy (dst, src, src->u.ref [0] == 0xff ?
        (int)sizeof (struct nn_chunkref_chunk) : src->u.ref [0] + 1);
}

void nn_chunkref_cp (struct nn_chunkref *dst, struct nn_chunkref *src)
{
    struct nn_chunkref_chunk *ch;

    if (src->u.ref [0] == 0xff) {
        ch = (struct nn_chunkref_chunk*) src;
        nn_chunk_addref (ch->chunk, 1);
    }
    memcpy (dst, src, sizeof (struct nn_chunkref));
}

void *nn_chunkref_data (struct nn_chunkref *self)
{
    return self->u.ref [0] == 0xff ?
        ((struct nn_chunkref_chunk*) self)->chunk :
        &self->u.ref [1];
}

size_t nn_chunkref_size (struct nn_chunkref *self)
{
    return self->u.ref [0] == 0xff ?
        nn_chunk_size (((struct nn_chunkref_chunk*) self)->chunk) :
        self->u.ref [0];
}

void nn_chunkref_trim (struct nn_chunkref *self, size_t n)
{
    struct nn_chunkref_chunk *ch;

    if (self->u.ref [0] == 0xff) {
        ch = (struct nn_chunkref_chunk*) self;
        ch->chunk = nn_chunk_trim (ch->chunk, n);
        return;
    }

    nn_assert (self->u.ref [0] >= n);
    memmove (&self->u.ref [1], &self->u.ref [1 + n], self->u.ref [0] - n);
    self->u.ref [0] -= (uint8_t) n;
}

void nn_chunkref_bulkcopy_start (struct nn_chunkref *self, uint32_t copies)
{
    struct nn_chunkref_chunk *ch;

    if (self->u.ref [0] == 0xff) {
        ch = (struct nn_chunkref_chunk*) self;
        nn_chunk_addref (ch->chunk, copies);
    }
}

void nn_chunkref_bulkcopy_cp (struct nn_chunkref *dst, struct nn_chunkref *src)
{
    memcpy (dst, src, sizeof (struct nn_chunkref));
}

