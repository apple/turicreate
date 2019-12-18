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

#ifndef NN_MSG_INCLUDED
#define NN_MSG_INCLUDED

#include "chunkref.h"

#include <stddef.h>

struct nn_msg {

    /*  Contains SP message header. This field directly corresponds
        to SP message header as defined in SP RFCs. There's no leading
        cmsghdr or trailing padding. */
    struct nn_chunkref sphdr;

    /*  Contains any additional transport-level message headers. Format of this
        buffer is a list of cmsgs as defined by POSIX (see "ancillary data"). */
    struct nn_chunkref hdrs;

    /*  Contains application level message payload. */
    struct nn_chunkref body;
};

/*  Initialises a message with body 'size' bytes long and empty header. */
void nn_msg_init (struct nn_msg *self, size_t size);

/*  Initialise message with body provided in the form of chunk pointer. */
void nn_msg_init_chunk (struct nn_msg *self, void *chunk);

/*  Frees resources allocate with the message. */
void nn_msg_term (struct nn_msg *self);

/*  Moves the content of the message from src to dst. dst should not be
    initialised prior to the operation. src will be uninitialised after the
    operation. */
void nn_msg_mv (struct nn_msg *dst, struct nn_msg *src);

/*  Copies a message from src to dst. dst should not be
    initialised prior to the operation. */
void nn_msg_cp (struct nn_msg *dst, struct nn_msg *src);

/*  Bulk copying is done by first invoking nn_msg_bulkcopy_start on the source
    message and specifying how many copies of the message will be made. Then,
    nn_msg_bulkcopy_cp should be used 'copies' of times to make individual
    copies of the source message. Note: Bulk copying is more efficient than
    making each copy separately. */
void nn_msg_bulkcopy_start (struct nn_msg *self, uint32_t copies);
void nn_msg_bulkcopy_cp (struct nn_msg *dst, struct nn_msg *src);

/** Replaces the message body with entirely new data.  This allows protocols
    that substantially rewrite or preprocess the userland message to be written. */
void nn_msg_replace_body(struct nn_msg *self, struct nn_chunkref newBody);

#endif

