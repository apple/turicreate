#ifndef AWS_TESTING_AWS_TEST_ALLOCATORS_H
#define AWS_TESTING_AWS_TEST_ALLOCATORS_H
/*
 * Copyright 2010-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/testing/aws_test_harness.h>

/** \file
 * Alternate allocators for use in testing.
 */

/**
 * Timebomb allocator fakes running out of memory after then Nth allocation.
 * Once this allocator starts failing, it never succeeds, even if memory is released.
 * Wraps an existing allocator.
 */
struct aws_timebomb_impl {
    size_t fail_after_n_allocations;
    size_t allocation_tally;
    struct aws_mutex mutex;
    struct aws_allocator *wrapped_allocator;
};

static void *s_timebomb_mem_acquire(struct aws_allocator *timebomb_alloc, size_t size) {
    struct aws_timebomb_impl *timebomb_impl = (struct aws_timebomb_impl *)timebomb_alloc->impl;
    void *ptr = NULL;

    aws_mutex_lock(&timebomb_impl->mutex);

    if (timebomb_impl->allocation_tally < timebomb_impl->fail_after_n_allocations) {
        timebomb_impl->allocation_tally++;
        ptr = timebomb_impl->wrapped_allocator->mem_acquire(timebomb_impl->wrapped_allocator, size);
    }

    aws_mutex_unlock(&timebomb_impl->mutex);

    return ptr;
}

static void s_timebomb_mem_release(struct aws_allocator *timebomb_alloc, void *ptr) {
    struct aws_timebomb_impl *timebomb_impl = (struct aws_timebomb_impl *)timebomb_alloc->impl;

    aws_mutex_lock(&timebomb_impl->mutex);
    timebomb_impl->wrapped_allocator->mem_release(timebomb_impl->wrapped_allocator, ptr);
    aws_mutex_unlock(&timebomb_impl->mutex);
}

static int aws_timebomb_allocator_init(
    struct aws_allocator *timebomb_allocator,
    struct aws_allocator *wrapped_allocator,
    size_t fail_after_n_allocations) {

    AWS_ZERO_STRUCT(*timebomb_allocator);

    struct aws_timebomb_impl *timebomb_impl =
        (struct aws_timebomb_impl *)aws_mem_calloc(wrapped_allocator, 1, sizeof(struct aws_timebomb_impl));
    ASSERT_NOT_NULL(timebomb_impl);

    timebomb_allocator->mem_acquire = s_timebomb_mem_acquire;
    timebomb_allocator->mem_release = s_timebomb_mem_release;
    /* Not defining calloc/realloc, all allocation will be piped through the one mem_acquire fn */

    timebomb_allocator->impl = timebomb_impl;
    timebomb_impl->wrapped_allocator = wrapped_allocator;
    timebomb_impl->fail_after_n_allocations = fail_after_n_allocations;
    ASSERT_SUCCESS(aws_mutex_init(&timebomb_impl->mutex));

    return AWS_OP_SUCCESS;
}

static void aws_timebomb_allocator_clean_up(struct aws_allocator *timebomb_alloc) {
    struct aws_timebomb_impl *timebomb_impl = (struct aws_timebomb_impl *)timebomb_alloc->impl;
    if (timebomb_impl) {
        aws_mutex_clean_up(&timebomb_impl->mutex);
        aws_mem_release(timebomb_impl->wrapped_allocator, timebomb_impl);
    }
    AWS_ZERO_STRUCT(*timebomb_alloc);
}

static void aws_timebomb_allocator_reset_countdown(
    struct aws_allocator *timebomb_alloc,
    size_t fail_after_n_allocations) {

    struct aws_timebomb_impl *timebomb_impl = (struct aws_timebomb_impl *)timebomb_alloc->impl;
    aws_mutex_lock(&timebomb_impl->mutex);
    timebomb_impl->allocation_tally = 0;
    timebomb_impl->fail_after_n_allocations = fail_after_n_allocations;
    aws_mutex_unlock(&timebomb_impl->mutex);
}

#endif /* AWS_TESTING_AWS_TEST_ALLOCATORS_H */
