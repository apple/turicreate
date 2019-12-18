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

#ifndef NN_CHUNK_INCLUDED
#define NN_CHUNK_INCLUDED

#include <stddef.h>
#include <stdint.h>

/*  Allocates the chunk using the allocation mechanism specified by 'type'. */
int nn_chunk_alloc (size_t size, int type, void **result);

/*  Resizes a chunk previously allocated with nn_chunk_alloc. */
int nn_chunk_realloc (size_t size, void **chunk);

/*  Releases a reference to the chunk and once the reference count had dropped
    to zero, deallocates the chunk. */
void nn_chunk_free (void *p);

/*  Increases the reference count of the chunk by 'n'. */
void nn_chunk_addref (void *p, uint32_t n);

/*  Returns size of the chunk buffer. */
size_t nn_chunk_size (void *p);

/*  Trims n bytes from the beginning of the chunk. Returns pointer to the new
    chunk. */
void *nn_chunk_trim (void *p, size_t n);

#endif

