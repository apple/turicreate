#ifndef AWS_COMMON_LRU_CACHE_H
#define AWS_COMMON_LRU_CACHE_H
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

#include <aws/common/hash_table.h>
#include <aws/common/linked_list.h>

/**
 * Simple Least-recently-used cache using the standard lazy linked hash table
 * implementation. (Yes the one that was the answer to that interview question
 * that one time).
 */
struct aws_lru_cache {
    struct aws_allocator *allocator;
    struct aws_linked_list list;
    struct aws_hash_table table;
    aws_hash_callback_destroy_fn *user_on_value_destroy;
    size_t max_items;
};

AWS_EXTERN_C_BEGIN

/**
 * Initializes the cache. Sets up the underlying hash table and linked list.
 * Once `max_items` elements have been added, the least recently used item will
 * be removed. For the other parameters, see aws/common/hash_table.h. Hash table
 * semantics of these arguments are preserved.
 */
AWS_COMMON_API
int aws_lru_cache_init(
    struct aws_lru_cache *cache,
    struct aws_allocator *allocator,
    aws_hash_fn *hash_fn,
    aws_hash_callback_eq_fn *equals_fn,
    aws_hash_callback_destroy_fn *destroy_key_fn,
    aws_hash_callback_destroy_fn *destroy_value_fn,
    size_t max_items);

/**
 * Cleans up the cache. Elements in the cache will be evicted and cleanup
 * callbacks will be invoked.
 */
AWS_COMMON_API
void aws_lru_cache_clean_up(struct aws_lru_cache *cache);

/**
 * Finds element in the cache by key. If found, it will become most-recently
 * used, *p_value will hold the stored value, and AWS_OP_SUCCESS will be
 * returned. If not found, AWS_OP_SUCCESS will be returned and *p_value will be
 * NULL.
 *
 * If any errors occur AWS_OP_ERR will be returned.
 */
AWS_COMMON_API
int aws_lru_cache_find(struct aws_lru_cache *cache, const void *key, void **p_value);

/**
 * Puts `p_value` at `key`. If an element is already stored at `key` it will be replaced. Added item becomes
 * most-recently used. If the cache is already full, the least-recently-used item will be removed.
 */
AWS_COMMON_API
int aws_lru_cache_put(struct aws_lru_cache *cache, const void *key, void *p_value);

/**
 * Removes item at `key` from the cache.
 */
AWS_COMMON_API
int aws_lru_cache_remove(struct aws_lru_cache *cache, const void *key);

/**
 * Clears all items from the cache.
 */
AWS_COMMON_API
void aws_lru_cache_clear(struct aws_lru_cache *cache);

/**
 * Accesses the least-recently-used element, sets it to most-recently-used
 * element, and returns the value.
 */
AWS_COMMON_API
void *aws_lru_cache_use_lru_element(struct aws_lru_cache *cache);

/**
 * Accesses the most-recently-used element and returns its value.
 */
AWS_COMMON_API
void *aws_lru_cache_get_mru_element(const struct aws_lru_cache *cache);

/**
 * Returns the number of elements in the cache.
 */
AWS_COMMON_API
size_t aws_lru_cache_get_element_count(const struct aws_lru_cache *cache);

AWS_EXTERN_C_END

#endif /* AWS_COMMON_LRU_CACHE_H */
