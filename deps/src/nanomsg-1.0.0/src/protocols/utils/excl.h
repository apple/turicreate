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

#ifndef NN_EXCL_INCLUDED
#define NN_EXCL_INCLUDED

#include "../../protocol.h"

#include <stddef.h>

/*  This is an object to handle a single pipe. To be used by socket types that
    can work with precisely one connection, e.g. PAIR. */

struct nn_excl {

    /*  The pipe being used at the moment. All other pipes will be rejected
        until this one terminates. NULL if there is no connected pipe. */
    struct nn_pipe *pipe;

    /*  Pipe ready for receiving. It's either equal to 'pipe' or NULL. */
    struct nn_pipe *inpipe;

    /*  Pipe ready for sending. It's either equal to 'pipe' or NULL. */
    struct nn_pipe *outpipe;

};

void nn_excl_init (struct nn_excl *self);
void nn_excl_term (struct nn_excl *self);
int nn_excl_add (struct nn_excl *self, struct nn_pipe *pipe);
void nn_excl_rm (struct nn_excl *self, struct nn_pipe *pipe);
void nn_excl_in (struct nn_excl *self, struct nn_pipe *pipe);
void nn_excl_out (struct nn_excl *self, struct nn_pipe *pipe);
int nn_excl_send (struct nn_excl *self, struct nn_msg *msg);
int nn_excl_recv (struct nn_excl *self, struct nn_msg *msg);
int nn_excl_can_send (struct nn_excl *self);
int nn_excl_can_recv (struct nn_excl *self);

#endif
