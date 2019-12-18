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

#include "hash.h"
#include "fast.h"
#include "alloc.h"
#include "cont.h"
#include "err.h"

#define NN_HASH_INITIAL_SLOTS 32

static uint32_t nn_hash_key (uint32_t key);

void nn_hash_init (struct nn_hash *self)
{
    uint32_t i;

    self->slots = NN_HASH_INITIAL_SLOTS;
    self->items = 0;
    self->array = nn_alloc (sizeof (struct nn_list) * NN_HASH_INITIAL_SLOTS,
        "hash map");
    alloc_assert (self->array);
    for (i = 0; i != NN_HASH_INITIAL_SLOTS; ++i)
        nn_list_init (&self->array [i]);
}

void nn_hash_term (struct nn_hash *self)
{
    uint32_t i;

    for (i = 0; i != self->slots; ++i)
        nn_list_term (&self->array [i]);
    nn_free (self->array);
}

static void nn_hash_rehash (struct nn_hash *self)
{
    uint32_t i;
    uint32_t oldslots;
    struct nn_list *oldarray;
    struct nn_hash_item *hitm;
    uint32_t newslot;

    /*  Allocate new double-sized array of slots. */
    oldslots = self->slots;
    oldarray = self->array;
    self->slots *= 2;
    self->array = nn_alloc (sizeof (struct nn_list) * self->slots, "hash map");
    alloc_assert (self->array);
    for (i = 0; i != self->slots; ++i)
        nn_list_init (&self->array [i]);

    /*  Move the items from old slot array to new slot array. */
    for (i = 0; i != oldslots; ++i) {
        while (!nn_list_empty (&oldarray [i])) {
            hitm = nn_cont (nn_list_begin (&oldarray [i]),
                    struct nn_hash_item, list);
            nn_list_erase (&oldarray [i], &hitm->list);
            newslot = nn_hash_key (hitm->key) % self->slots;
            nn_list_insert (&self->array [newslot], &hitm->list,
                    nn_list_end (&self->array [newslot]));
        }

        nn_list_term (&oldarray [i]);
    }

    /*  Deallocate the old array of slots. */
    nn_free (oldarray);
}

void nn_hash_insert (struct nn_hash *self, uint32_t key,
    struct nn_hash_item *item)
{
    struct nn_list_item *it;
    uint32_t i;

    i = nn_hash_key (key) % self->slots;

    for (it = nn_list_begin (&self->array [i]);
          it != nn_list_end (&self->array [i]);
          it = nn_list_next (&self->array [i], it))
        nn_assert (nn_cont (it, struct nn_hash_item, list)->key != key);

    item->key = key;
    nn_list_insert (&self->array [i], &item->list,
        nn_list_end (&self->array [i]));
    ++self->items;

    /*  If the hash is getting full, double the amount of slots and
        re-hash all the items. */
    if (nn_slow (self->items * 2 > self->slots && self->slots < 0x80000000))
        nn_hash_rehash(self);
}

void nn_hash_erase (struct nn_hash *self, struct nn_hash_item *item)
{
    uint32_t slot;

    slot = nn_hash_key (item->key) % self->slots;
    nn_list_erase (&self->array [slot], &item->list);
	--self->items;
}

struct nn_hash_item *nn_hash_get (struct nn_hash *self, uint32_t key)
{
    uint32_t slot;
    struct nn_list_item *it;
    struct nn_hash_item *item;

    slot = nn_hash_key (key) % self->slots;

    for (it = nn_list_begin (&self->array [slot]);
          it != nn_list_end (&self->array [slot]);
          it = nn_list_next (&self->array [slot], it)) {
        item = nn_cont (it, struct nn_hash_item, list);
        if (item->key == key)
            return item;
    }

    return NULL;
}

uint32_t nn_hash_key (uint32_t key)
{
    /*  TODO: This is a randomly chosen hashing function. Give some thought
        to picking a more fitting one. */
    key = (key ^ 61) ^ (key >> 16);
    key += key << 3;
    key = key ^ (key >> 4);
    key = key * 0x27d4eb2d;
    key = key ^ (key >> 15);

    return key;
}

void nn_hash_item_init (struct nn_hash_item *self)
{
    nn_list_item_init (&self->list);
}

void nn_hash_item_term (struct nn_hash_item *self)
{
    nn_list_item_term (&self->list);
}

