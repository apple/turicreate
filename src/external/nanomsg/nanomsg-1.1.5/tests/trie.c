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

#include "../src/protocols/pubsub/trie.c"
#include "../src/utils/alloc.c"
#include "../src/utils/err.c"

#include <stdio.h>

int main ()
{
    int rc;
    struct nn_trie trie;

    /*  Try matching with an empty trie. */
    nn_trie_init (&trie);
    rc = nn_trie_match (&trie, (const uint8_t*) "", 0);
    nn_assert (rc == 0);
    rc = nn_trie_match (&trie, (const uint8_t*) "ABC", 3);
    nn_assert (rc == 0);
    nn_trie_term (&trie);

    /*  Try matching with "all" subscription. */
    nn_trie_init (&trie);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "", 0);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "", 0);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "ABC", 3);
    nn_assert (rc == 1);
    nn_trie_term (&trie);

    /*  Try some simple matching. */
    nn_trie_init (&trie);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "ABC", 3);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "DEF", 3);
    nn_assert (rc == 0);
    rc = nn_trie_match (&trie, (const uint8_t*) "AB", 2);
    nn_assert (rc == 0);
    rc = nn_trie_match (&trie, (const uint8_t*) "ABC", 3);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "ABCDE", 5);
    nn_assert (rc == 1);
    nn_trie_term (&trie);

    /*  Try a long subcsription. */
    nn_trie_init (&trie);
    rc = nn_trie_subscribe (&trie,
        (const uint8_t*) "01234567890123456789012345678901234", 35);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "", 0);
    nn_assert (rc == 0);
    rc = nn_trie_match (&trie, (const uint8_t*) "012456789", 10);
    nn_assert (rc == 0);
    rc = nn_trie_match (&trie, (const uint8_t*) "012345678901234567", 18);
    nn_assert (rc == 0);
    rc = nn_trie_match (&trie,
        (const uint8_t*) "01234567890123456789012345678901234", 35);
    nn_assert (rc == 1);
    nn_trie_term (&trie);

    /*  Try matching with a sparse node involved. */
    nn_trie_init (&trie);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "ABC", 3);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "ADE", 3);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "A", 1);
    nn_assert (rc == 0);
    rc = nn_trie_match (&trie, (const uint8_t*) "AD", 2);
    nn_assert (rc == 0);
    nn_trie_term (&trie);

    /*  Try matching with a dense node involved. */
    nn_trie_init (&trie);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "A", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "B", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "C", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "0", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "E", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "F", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "1", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "@", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "b", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "f", 1);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "0", 1);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "A", 1);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "f", 1);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "000", 3);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "a", 1);
    nn_assert (rc == 0);
    rc = nn_trie_match (&trie, (const uint8_t*) "c", 1);
    nn_assert (rc == 0);
    nn_trie_term (&trie);

    /*  Check prefix splitting and compaction. */
    nn_trie_init (&trie);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "ABCD", 4);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "AB", 2);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "AB", 2);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "AB", 2);
    nn_assert (rc == 0);
    rc = nn_trie_match (&trie, (const uint8_t*) "ABCDEF", 6);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "ABEF", 4);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "ABCD", 4);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "ABCD", 4);
    nn_assert (rc == 0);
    rc = nn_trie_match (&trie, (const uint8_t*) "ABEF", 4);
    nn_assert (rc == 1);
    nn_trie_term (&trie);

    /*  Check whether there's no problem with removing all subscriptions. */
    nn_trie_init (&trie);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "A", 1);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "A", 1);
    nn_assert (rc == 1);
    rc = nn_trie_match (&trie, (const uint8_t*) "", 0);
    nn_assert (rc == 0);
    rc = nn_trie_match (&trie, (const uint8_t*) "A", 1);
    nn_assert (rc == 0);
    nn_trie_term (&trie);  

    /*  Check converting from sparse node to dense node and vice versa. */
    nn_trie_init (&trie);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "A", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "B", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "C", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "0", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "E", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "F", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "1", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "@", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "b", 1);
    nn_assert (rc == 1);
    rc = nn_trie_subscribe (&trie, (const uint8_t*) "f", 1);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "0", 1);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "f", 1);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "E", 1);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "B", 1);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "A", 1);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "1", 1);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "@", 1);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "F", 1);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "C", 1);
    nn_assert (rc == 1);
    rc = nn_trie_unsubscribe (&trie, (const uint8_t*) "b", 1);
    nn_assert (rc == 1);
    nn_trie_term (&trie);

    return 0;
}

