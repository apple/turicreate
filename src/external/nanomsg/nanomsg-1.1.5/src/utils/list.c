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

#include <stddef.h>

#include "list.h"
#include "err.h"
#include "attr.h"

void nn_list_init (struct nn_list *self)
{
    self->first = NULL;
    self->last = NULL;
}

void nn_list_term (struct nn_list *self)
{
    nn_assert (self->first == NULL);
    nn_assert (self->last == NULL);
}

int nn_list_empty (struct nn_list *self)
{
    return self->first ? 0 : 1;
}

struct nn_list_item *nn_list_begin (struct nn_list *self)
{
    return self->first;
}

struct nn_list_item *nn_list_end (NN_UNUSED struct nn_list *self)
{
    return NULL;
}

struct nn_list_item *nn_list_prev (struct nn_list *self,
    struct nn_list_item *it)
{
    if (!it)
        return self->last;
    nn_assert (it->prev != NN_LIST_NOTINLIST);
    return it->prev;
}

struct nn_list_item *nn_list_next (NN_UNUSED struct nn_list *self,
    struct nn_list_item *it)
{
    nn_assert (it->next != NN_LIST_NOTINLIST);
    return it->next;
}

void nn_list_insert (struct nn_list *self, struct nn_list_item *item,
    struct nn_list_item *it)
{
    nn_assert (!nn_list_item_isinlist (item));

    item->prev = it ? it->prev : self->last;
    item->next = it;
    if (item->prev)
        item->prev->next = item;
    if (item->next)
        item->next->prev = item;
    if (!self->first || self->first == it)
        self->first = item;
    if (!it)
        self->last = item;
}

struct nn_list_item *nn_list_erase (struct nn_list *self,
    struct nn_list_item *item)
{
    struct nn_list_item *next;

    nn_assert (nn_list_item_isinlist (item));

    if (item->prev)
        item->prev->next = item->next;
    else
        self->first = item->next;
    if (item->next)
        item->next->prev = item->prev;
    else
        self->last = item->prev;

    next = item->next;

    item->prev = NN_LIST_NOTINLIST;
    item->next = NN_LIST_NOTINLIST;

    return next;
}

void nn_list_item_init (struct nn_list_item *self)
{
    self->prev = NN_LIST_NOTINLIST;
    self->next = NN_LIST_NOTINLIST;
}

void nn_list_item_term (struct nn_list_item *self)
{
    nn_assert (!nn_list_item_isinlist (self));
}

int nn_list_item_isinlist (struct nn_list_item *self)
{
    return self->prev == NN_LIST_NOTINLIST ? 0 : 1;
}

