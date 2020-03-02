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
#include <aws/common/lru_cache.h>

struct cache_node {
    struct aws_linked_list_node node;
    struct aws_lru_cache *cache;
    const void *key;
    void *value;
};

static void s_element_destroy(void *value) {
    struct cache_node *cache_node = value;

    if (cache_node->cache->user_on_value_destroy) {
        cache_node->cache->user_on_value_destroy(cache_node->value);
    }

    aws_linked_list_remove(&cache_node->node);
    aws_mem_release(cache_node->cache->allocator, cache_node);
}

int aws_lru_cache_init(
    struct aws_lru_cache *cache,
    struct aws_allocator *allocator,
    aws_hash_fn *hash_fn,
    aws_hash_callback_eq_fn *equals_fn,
    aws_hash_callback_destroy_fn *destroy_key_fn,
    aws_hash_callback_destroy_fn *destroy_value_fn,
    size_t max_items) {
    AWS_ASSERT(allocator);
    AWS_ASSERT(max_items);

    cache->allocator = allocator;
    cache->max_items = max_items;
    cache->user_on_value_destroy = destroy_value_fn;

    aws_linked_list_init(&cache->list);
    return aws_hash_table_init(
        &cache->table, allocator, max_items, hash_fn, equals_fn, destroy_key_fn, s_element_destroy);
}

void aws_lru_cache_clean_up(struct aws_lru_cache *cache) {
    /* clearing the table will remove all elements. That will also deallocate
     * any cache entries we currently have. */
    aws_hash_table_clean_up(&cache->table);
    AWS_ZERO_STRUCT(*cache);
}

int aws_lru_cache_find(struct aws_lru_cache *cache, const void *key, void **p_value) {

    struct aws_hash_element *cache_element = NULL;
    int err_val = aws_hash_table_find(&cache->table, key, &cache_element);

    if (err_val || !cache_element) {
        *p_value = NULL;
        return err_val;
    }

    struct cache_node *cache_node = cache_element->value;
    *p_value = cache_node->value;

    /* on access, remove from current place in list and move it to the head. */
    aws_linked_list_remove(&cache_node->node);
    aws_linked_list_push_front(&cache->list, &cache_node->node);

    return AWS_OP_SUCCESS;
}

int aws_lru_cache_put(struct aws_lru_cache *cache, const void *key, void *p_value) {

    struct cache_node *cache_node = aws_mem_acquire(cache->allocator, sizeof(struct cache_node));

    if (!cache_node) {
        return AWS_OP_ERR;
    }

    struct aws_hash_element *element = NULL;
    int was_added = 0;
    int err_val = aws_hash_table_create(&cache->table, key, &element, &was_added);

    if (err_val) {
        aws_mem_release(cache->allocator, cache_node);
        return err_val;
    }

    if (element->value) {
        s_element_destroy(element->value);
    }

    cache_node->value = p_value;
    cache_node->key = key;
    cache_node->cache = cache;
    element->value = cache_node;

    aws_linked_list_push_front(&cache->list, &cache_node->node);

    /* we only want to manage the space if we actually added a new element. */
    if (was_added && aws_hash_table_get_entry_count(&cache->table) > cache->max_items) {

        /* we're over the cache size limit. Remove whatever is in the back of
         * the list. */
        struct aws_linked_list_node *node_to_remove = aws_linked_list_back(&cache->list);
        AWS_ASSUME(node_to_remove);
        struct cache_node *entry_to_remove = AWS_CONTAINER_OF(node_to_remove, struct cache_node, node);
        /*the callback will unlink and deallocate the node */
        aws_hash_table_remove(&cache->table, entry_to_remove->key, NULL, NULL);
    }

    return AWS_OP_SUCCESS;
}

int aws_lru_cache_remove(struct aws_lru_cache *cache, const void *key) {
    /* allocated cache memory and the linked list entry will be removed in the
     * callback. */
    return aws_hash_table_remove(&cache->table, key, NULL, NULL);
}

void aws_lru_cache_clear(struct aws_lru_cache *cache) {
    /* clearing the table will remove all elements. That will also deallocate
     * any cache entries we currently have. */
    aws_hash_table_clear(&cache->table);
}

void *aws_lru_cache_use_lru_element(struct aws_lru_cache *cache) {
    if (aws_linked_list_empty(&cache->list)) {
        return NULL;
    }

    struct aws_linked_list_node *lru_node = aws_linked_list_back(&cache->list);

    aws_linked_list_remove(lru_node);
    aws_linked_list_push_front(&cache->list, lru_node);
    struct cache_node *lru_element = AWS_CONTAINER_OF(lru_node, struct cache_node, node);
    return lru_element->value;
}

void *aws_lru_cache_get_mru_element(const struct aws_lru_cache *cache) {
    if (aws_linked_list_empty(&cache->list)) {
        return NULL;
    }

    struct aws_linked_list_node *mru_node = aws_linked_list_front(&cache->list);

    struct cache_node *mru_element = AWS_CONTAINER_OF(mru_node, struct cache_node, node);
    return mru_element->value;
}

size_t aws_lru_cache_get_element_count(const struct aws_lru_cache *cache) {
    return aws_hash_table_get_entry_count(&cache->table);
}
