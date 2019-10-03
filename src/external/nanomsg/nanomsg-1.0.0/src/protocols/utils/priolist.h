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

#ifndef NN_PRIOLIST_INCLUDED
#define NN_PRIOLIST_INCLUDED

#include "../../protocol.h"

#include "../../utils/list.h"

/*  Prioritised list of pipes. */

#define NN_PRIOLIST_SLOTS 16

struct nn_priolist_data {

    /*  The underlying pipe itself. */
    struct nn_pipe *pipe;

    /*  Priority the pipe is assigned. Using this value we can find the
        nn_priolist_slot object that owns this pipe. */
    int priority;

    /*  The structure is a member in nn_priolist_slot's 'pipes' list. */
    struct nn_list_item item;
};

struct nn_priolist_slot {

    /*  The list of pipes on particular priority level. */
    struct nn_list pipes;

    /*  Pointer to the current pipe within the priority level. If there's no
        pipe available, the field is set to NULL. */
    struct nn_priolist_data *current;
};

struct nn_priolist {

    /*  Each slot holds pipes for a particular priority level. */
    struct nn_priolist_slot slots [NN_PRIOLIST_SLOTS];

    /*  The index of the slot holding the current pipe. It should be the
        highest-priority non-empty slot available. If there's no available
        pipe, this field is set to -1. */
    int current;
};

/*  Initialise the list. */
void nn_priolist_init (struct nn_priolist *self);

/*  Terminate the list. The list must be empty before it's terminated. */
void nn_priolist_term (struct nn_priolist *self);

/*  Add a new pipe to the list with a particular priority level. The pipe
    is not active at this point. Use nn_priolist_activate to activate it. */
void nn_priolist_add (struct nn_priolist *self, struct nn_priolist_data *data,
    struct nn_pipe *pipe, int priority);

/*  Remove the pipe from the list. */
void nn_priolist_rm (struct nn_priolist *self, struct nn_priolist_data *data);

/*  Activates a non-active pipe. The pipe must be added to the list prior to
    calling this function. */
void nn_priolist_activate (struct nn_priolist *self, struct nn_priolist_data *data);

/*  Returns 1 if there's at least a single active pipe in the list,
    0 otherwise. */
int nn_priolist_is_active (struct nn_priolist *self);

/*  Get the pointer to the current pipe. If there's no pipe in the list,
    NULL is returned. */
struct nn_pipe *nn_priolist_getpipe (struct nn_priolist *self);

/*  Moves to the next pipe in the list. If 'release' is set to 1, the current
    pipe is removed from the list. To re-insert it into the list use
    nn_priolist_activate function. */
void nn_priolist_advance (struct nn_priolist *self, int release);

/*  Returns current priority. Used for statistics only  */
int nn_priolist_get_priority (struct nn_priolist *self);

#endif
