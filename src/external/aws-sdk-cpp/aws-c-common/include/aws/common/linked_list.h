#ifndef AWS_COMMON_LINKED_LIST_H
#define AWS_COMMON_LINKED_LIST_H

/*
 * Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws/common/common.h>

#include <stddef.h>

struct aws_linked_list_node {
    struct aws_linked_list_node *next;
    struct aws_linked_list_node *prev;
};

struct aws_linked_list {
    struct aws_linked_list_node head;
    struct aws_linked_list_node tail;
};

AWS_EXTERN_C_BEGIN

/**
 * Set node's next and prev pointers to NULL.
 */
AWS_STATIC_IMPL void aws_linked_list_node_reset(struct aws_linked_list_node *node);

/**
 * These functions need to be defined first as they are used in pre
 * and post conditions.
 */

/**
 * Tests if the list is empty.
 */
AWS_STATIC_IMPL bool aws_linked_list_empty(const struct aws_linked_list *list);

/**
 * Checks that a linked list is valid.
 */
AWS_STATIC_IMPL bool aws_linked_list_is_valid(const struct aws_linked_list *list);
/**
 * Checks that the prev of the next pointer of a node points to the
 * node. As this checks whether the [next] connection of a node is
 * bidirectional, it returns false if used for the list tail.
 */
AWS_STATIC_IMPL bool aws_linked_list_node_next_is_valid(const struct aws_linked_list_node *node);

/**
 * Checks that the next of the prev pointer of a node points to the
 * node. Similarly to the above, this returns false if used for the
 * head of a list.
 */
AWS_STATIC_IMPL bool aws_linked_list_node_prev_is_valid(const struct aws_linked_list_node *node);
/**
 * Checks that a linked list satisfies double linked list connectivity
 * constraints. This check is O(n) as it traverses the whole linked
 * list to ensure that tail is reachable from head (and vice versa)
 * and that every connection is bidirectional.
 *
 * Note: This check *cannot* go into an infinite loop, because we
 * ensure that the connection to the next node is
 * bidirectional. Therefore, if a node's [a] a.next is a previous node
 * [b] in the list, b.prev != &a and so this check would fail, thus
 * terminating the loop.
 */
AWS_STATIC_IMPL bool aws_linked_list_is_valid_deep(const struct aws_linked_list *list);

/**
 * Initializes the list. List will be empty after this call.
 */
AWS_STATIC_IMPL void aws_linked_list_init(struct aws_linked_list *list);

/**
 * Returns an iteration pointer for the first element in the list.
 */
AWS_STATIC_IMPL struct aws_linked_list_node *aws_linked_list_begin(const struct aws_linked_list *list);

/**
 * Returns an iteration pointer for one past the last element in the list.
 */
AWS_STATIC_IMPL const struct aws_linked_list_node *aws_linked_list_end(const struct aws_linked_list *list);

/**
 * Returns a pointer for the last element in the list.
 * Used to begin iterating the list in reverse. Ex:
 *   for (i = aws_linked_list_rbegin(list); i != aws_linked_list_rend(list); i = aws_linked_list_prev(i)) {...}
 */
AWS_STATIC_IMPL struct aws_linked_list_node *aws_linked_list_rbegin(const struct aws_linked_list *list);

/**
 * Returns the pointer to one before the first element in the list.
 * Used to end iterating the list in reverse.
 */
AWS_STATIC_IMPL const struct aws_linked_list_node *aws_linked_list_rend(const struct aws_linked_list *list);

/**
 * Returns the next element in the list.
 */
AWS_STATIC_IMPL struct aws_linked_list_node *aws_linked_list_next(const struct aws_linked_list_node *node);

/**
 * Returns the previous element in the list.
 */
AWS_STATIC_IMPL struct aws_linked_list_node *aws_linked_list_prev(const struct aws_linked_list_node *node);

/**
 * Inserts to_add immediately after after.
 */
AWS_STATIC_IMPL void aws_linked_list_insert_after(
    struct aws_linked_list_node *after,
    struct aws_linked_list_node *to_add);
/**
 * Swaps the order two nodes in the linked list.
 */
AWS_STATIC_IMPL void aws_linked_list_swap_nodes(struct aws_linked_list_node *a, struct aws_linked_list_node *b);

/**
 * Inserts to_add immediately before before.
 */
AWS_STATIC_IMPL void aws_linked_list_insert_before(
    struct aws_linked_list_node *before,
    struct aws_linked_list_node *to_add);

/**
 * Removes the specified node from the list (prev/next point to each other) and
 * returns the next node in the list.
 */
AWS_STATIC_IMPL void aws_linked_list_remove(struct aws_linked_list_node *node);

/**
 * Append new_node.
 */
AWS_STATIC_IMPL void aws_linked_list_push_back(struct aws_linked_list *list, struct aws_linked_list_node *node);

/**
 * Returns the element in the back of the list.
 */
AWS_STATIC_IMPL struct aws_linked_list_node *aws_linked_list_back(const struct aws_linked_list *list);

/**
 * Returns the element in the back of the list and removes it
 */
AWS_STATIC_IMPL struct aws_linked_list_node *aws_linked_list_pop_back(struct aws_linked_list *list);

/**
 * Prepend new_node.
 */
AWS_STATIC_IMPL void aws_linked_list_push_front(struct aws_linked_list *list, struct aws_linked_list_node *node);
/**
 * Returns the element in the front of the list.
 */
AWS_STATIC_IMPL struct aws_linked_list_node *aws_linked_list_front(const struct aws_linked_list *list);
/**
 * Returns the element in the front of the list and removes it
 */
AWS_STATIC_IMPL struct aws_linked_list_node *aws_linked_list_pop_front(struct aws_linked_list *list);

AWS_STATIC_IMPL void aws_linked_list_swap_contents(struct aws_linked_list *a, struct aws_linked_list *b);

#ifndef AWS_NO_STATIC_IMPL
#    include <aws/common/linked_list.inl>
#endif /* AWS_NO_STATIC_IMPL */
AWS_EXTERN_C_END

#endif /* AWS_COMMON_LINKED_LIST_H */
