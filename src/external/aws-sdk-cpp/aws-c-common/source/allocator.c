/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/common/assert.h>
#include <aws/common/common.h>
#include <aws/common/logging.h>
#include <aws/common/math.h>

#include <stdarg.h>
#include <stdlib.h>

#ifdef _WIN32
#    include <Windows.h>
#endif

#ifdef __MACH__
#    include <CoreFoundation/CoreFoundation.h>
#endif

/* turn off unused named parameter warning on msvc.*/
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4100)
#endif

bool aws_allocator_is_valid(const struct aws_allocator *alloc) {
    /* An allocator must define mem_acquire and mem_release.  All other fields are optional */
    return alloc && AWS_OBJECT_PTR_IS_READABLE(alloc) && alloc->mem_acquire && alloc->mem_release;
}

static void *s_default_malloc(struct aws_allocator *allocator, size_t size) {
    (void)allocator;
    return malloc(size);
}

static void s_default_free(struct aws_allocator *allocator, void *ptr) {
    (void)allocator;
    free(ptr);
}

static void *s_default_realloc(struct aws_allocator *allocator, void *ptr, size_t oldsize, size_t newsize) {
    (void)allocator;
    (void)oldsize;
    return realloc(ptr, newsize);
}

static void *s_default_calloc(struct aws_allocator *allocator, size_t num, size_t size) {
    (void)allocator;
    return calloc(num, size);
}

static struct aws_allocator default_allocator = {
    .mem_acquire = s_default_malloc,
    .mem_release = s_default_free,
    .mem_realloc = s_default_realloc,
    .mem_calloc = s_default_calloc,
};

struct aws_allocator *aws_default_allocator(void) {
    return &default_allocator;
}

void *aws_mem_acquire(struct aws_allocator *allocator, size_t size) {
    AWS_FATAL_PRECONDITION(allocator != NULL);
    AWS_FATAL_PRECONDITION(allocator->mem_acquire != NULL);
    /* Protect against https://wiki.sei.cmu.edu/confluence/display/c/MEM04-C.+Beware+of+zero-length+allocations */
    AWS_FATAL_PRECONDITION(size != 0);

    void *mem = allocator->mem_acquire(allocator, size);
    if (!mem) {
        aws_raise_error(AWS_ERROR_OOM);
    }
    return mem;
}

void *aws_mem_calloc(struct aws_allocator *allocator, size_t num, size_t size) {
    AWS_FATAL_PRECONDITION(allocator != NULL);
    AWS_FATAL_PRECONDITION(allocator->mem_calloc || allocator->mem_acquire);
    /* Protect against https://wiki.sei.cmu.edu/confluence/display/c/MEM04-C.+Beware+of+zero-length+allocations */
    AWS_FATAL_PRECONDITION(num != 0 && size != 0);

    /* Defensive check: never use calloc with size * num that would overflow
     * https://wiki.sei.cmu.edu/confluence/display/c/MEM07-C.+Ensure+that+the+arguments+to+calloc%28%29%2C+when+multiplied%2C+do+not+wrap
     */
    size_t required_bytes;
    if (aws_mul_size_checked(num, size, &required_bytes)) {
        return NULL;
    }

    /* If there is a defined calloc, use it */
    if (allocator->mem_calloc) {
        void *mem = allocator->mem_calloc(allocator, num, size);
        if (!mem) {
            aws_raise_error(AWS_ERROR_OOM);
        }
        return mem;
    }

    /* Otherwise, emulate calloc */
    void *mem = allocator->mem_acquire(allocator, required_bytes);
    if (!mem) {
        aws_raise_error(AWS_ERROR_OOM);
        return NULL;
    }
    memset(mem, 0, required_bytes);
    AWS_POSTCONDITION(mem != NULL);
    return mem;
}

#define AWS_ALIGN_ROUND_UP(value, alignment) (((value) + ((alignment)-1)) & ~((alignment)-1))

void *aws_mem_acquire_many(struct aws_allocator *allocator, size_t count, ...) {

    enum { S_ALIGNMENT = sizeof(intmax_t) };

    va_list args_size;
    va_start(args_size, count);
    va_list args_allocs;
    va_copy(args_allocs, args_size);

    size_t total_size = 0;
    for (size_t i = 0; i < count; ++i) {

        /* Ignore the pointer argument for now */
        va_arg(args_size, void **);

        size_t alloc_size = va_arg(args_size, size_t);
        total_size += AWS_ALIGN_ROUND_UP(alloc_size, S_ALIGNMENT);
    }
    va_end(args_size);

    void *allocation = NULL;

    if (total_size > 0) {

        allocation = aws_mem_acquire(allocator, total_size);
        if (!allocation) {
            aws_raise_error(AWS_ERROR_OOM);
            goto cleanup;
        }

        uint8_t *current_ptr = allocation;

        for (size_t i = 0; i < count; ++i) {

            void **out_ptr = va_arg(args_allocs, void **);

            size_t alloc_size = va_arg(args_allocs, size_t);
            alloc_size = AWS_ALIGN_ROUND_UP(alloc_size, S_ALIGNMENT);

            *out_ptr = current_ptr;
            current_ptr += alloc_size;
        }
    }

cleanup:
    va_end(args_allocs);
    return allocation;
}

#undef AWS_ALIGN_ROUND_UP

void aws_mem_release(struct aws_allocator *allocator, void *ptr) {
    AWS_FATAL_PRECONDITION(allocator != NULL);
    AWS_FATAL_PRECONDITION(allocator->mem_release != NULL);

    if (ptr != NULL) {
        allocator->mem_release(allocator, ptr);
    }
}

int aws_mem_realloc(struct aws_allocator *allocator, void **ptr, size_t oldsize, size_t newsize) {
    AWS_FATAL_PRECONDITION(allocator != NULL);
    AWS_FATAL_PRECONDITION(allocator->mem_realloc || allocator->mem_acquire);
    AWS_FATAL_PRECONDITION(allocator->mem_release);

    /* Protect against https://wiki.sei.cmu.edu/confluence/display/c/MEM04-C.+Beware+of+zero-length+allocations */
    if (newsize == 0) {
        aws_mem_release(allocator, *ptr);
        *ptr = NULL;
        return AWS_OP_SUCCESS;
    }

    if (allocator->mem_realloc) {
        void *newptr = allocator->mem_realloc(allocator, *ptr, oldsize, newsize);
        if (!newptr) {
            return aws_raise_error(AWS_ERROR_OOM);
        }
        *ptr = newptr;
        return AWS_OP_SUCCESS;
    }

    /* Since the allocator doesn't support realloc, we'll need to emulate it (inefficiently). */
    if (oldsize >= newsize) {
        return AWS_OP_SUCCESS;
    }

    void *newptr = allocator->mem_acquire(allocator, newsize);
    if (!newptr) {
        return aws_raise_error(AWS_ERROR_OOM);
    }

    memcpy(newptr, *ptr, oldsize);
    memset((uint8_t *)newptr + oldsize, 0, newsize - oldsize);

    aws_mem_release(allocator, *ptr);

    *ptr = newptr;

    return AWS_OP_SUCCESS;
}

/* Wraps a CFAllocator around aws_allocator. For Mac only. */
#ifdef __MACH__

static CFStringRef s_cf_allocator_description = CFSTR("CFAllocator wrapping aws_allocator.");

/* note we don't have a standard specification stating sizeof(size_t) == sizeof(void *) so we have some extra casts */
static void *s_cf_allocator_allocate(CFIndex alloc_size, CFOptionFlags hint, void *info) {
    (void)hint;

    struct aws_allocator *allocator = info;

    void *mem = aws_mem_acquire(allocator, (size_t)alloc_size + sizeof(size_t));

    if (!mem) {
        return NULL;
    }

    size_t allocation_size = (size_t)alloc_size + sizeof(size_t);
    memcpy(mem, &allocation_size, sizeof(size_t));
    return (void *)((uint8_t *)mem + sizeof(size_t));
}

static void s_cf_allocator_deallocate(void *ptr, void *info) {
    struct aws_allocator *allocator = info;

    void *original_allocation = (uint8_t *)ptr - sizeof(size_t);

    aws_mem_release(allocator, original_allocation);
}

static void *s_cf_allocator_reallocate(void *ptr, CFIndex new_size, CFOptionFlags hint, void *info) {
    (void)hint;

    struct aws_allocator *allocator = info;
    AWS_ASSERT(allocator->mem_realloc);

    void *original_allocation = (uint8_t *)ptr - sizeof(size_t);
    size_t original_size = 0;
    memcpy(&original_size, original_allocation, sizeof(size_t));

    if (aws_mem_realloc(allocator, &original_allocation, original_size, (size_t)new_size)) {
        return NULL;
    }

    size_t new_allocation_size = (size_t)new_size;
    memcpy(original_allocation, &new_allocation_size, sizeof(size_t));

    return (void *)((uint8_t *)original_allocation + sizeof(size_t));
}

static CFStringRef s_cf_allocator_copy_description(const void *info) {
    (void)info;

    return s_cf_allocator_description;
}

static CFIndex s_cf_allocator_preferred_size(CFIndex size, CFOptionFlags hint, void *info) {
    (void)hint;
    (void)info;

    return size + sizeof(size_t);
}

CFAllocatorRef aws_wrapped_cf_allocator_new(struct aws_allocator *allocator) {
    CFAllocatorRef cf_allocator = NULL;

    CFAllocatorReallocateCallBack reallocate_callback = NULL;

    if (allocator->mem_realloc) {
        reallocate_callback = s_cf_allocator_reallocate;
    }

    CFAllocatorContext context = {
        .allocate = s_cf_allocator_allocate,
        .copyDescription = s_cf_allocator_copy_description,
        .deallocate = s_cf_allocator_deallocate,
        .reallocate = reallocate_callback,
        .info = allocator,
        .preferredSize = s_cf_allocator_preferred_size,
        .release = NULL,
        .retain = NULL,
        .version = 0,
    };

    cf_allocator = CFAllocatorCreate(NULL, &context);

    if (!cf_allocator) {
        aws_raise_error(AWS_ERROR_OOM);
    }

    return cf_allocator;
}

void aws_wrapped_cf_allocator_destroy(CFAllocatorRef allocator) {
    CFRelease(allocator);
}

#endif /*__MACH__ */
