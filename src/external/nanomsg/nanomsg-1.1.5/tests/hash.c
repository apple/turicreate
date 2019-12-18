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

#include "../src/utils/err.c"
#include "../src/utils/list.c"
#include "../src/utils/hash.c"
#include "../src/utils/alloc.c"

int main ()
{
    struct nn_hash hash;
    uint32_t k;
    struct nn_hash_item *item;
    struct nn_hash_item *item5000 = NULL;

    nn_hash_init (&hash);

    /*  Insert 10000 elements into the hash table. */
    for (k = 0; k != 10000; ++k) {
        item = nn_alloc (sizeof (struct nn_hash_item), "item");
        nn_assert (item);
        if (k == 5000)
            item5000 = item;
        nn_hash_item_init (item);
        nn_hash_insert (&hash, k, item);
    }

    /*  Find one element and check whether it is the correct one. */
    nn_assert (nn_hash_get (&hash, 5000) == item5000);

    /*  Remove all the elements from the hash table and terminate it. */
    for (k = 0; k != 10000; ++k) {
        item = nn_hash_get (&hash, k);
        nn_hash_erase (&hash, item);
        nn_free (item);
    }
    nn_hash_term (&hash);

    return 0;
}

