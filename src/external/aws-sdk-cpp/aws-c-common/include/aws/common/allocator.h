#ifndef AWS_COMMON_ALLOCATOR_H
#define AWS_COMMON_ALLOCATOR_H
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

#include <aws/common/macros.h>
#include <aws/common/stdbool.h>
#include <aws/common/stdint.h>

AWS_EXTERN_C_BEGIN

/* Allocator structure. An instance of this will be passed around for anything needing memory allocation */
struct aws_allocator {
    void *(*mem_acquire)(struct aws_allocator *allocator, size_t size);
    void (*mem_release)(struct aws_allocator *allocator, void *ptr);
    /* Optional method; if not supported, this pointer must be NULL */
    void *(*mem_realloc)(struct aws_allocator *allocator, void *oldptr, size_t oldsize, size_t newsize);
    /* Optional method; if not supported, this pointer must be NULL */
    void *(*mem_calloc)(struct aws_allocator *allocator, size_t num, size_t size);
    void *impl;
};

/**
 * Inexpensive (constant time) check of data-structure invariants.
 */
AWS_COMMON_API
bool aws_allocator_is_valid(const struct aws_allocator *alloc);

AWS_COMMON_API
struct aws_allocator *aws_default_allocator(void);

#ifdef __MACH__
/* Avoid pulling in CoreFoundation headers in a header file. */
struct __CFAllocator;
typedef const struct __CFAllocator *CFAllocatorRef;

/**
 * Wraps a CFAllocator around aws_allocator. For Mac only. Use this anytime you need a CFAllocatorRef for interacting
 * with Apple Frameworks. Unfortunately, it allocates memory so we can't make it static file scope, be sure to call
 * aws_wrapped_cf_allocator_destroy when finished.
 */
AWS_COMMON_API
CFAllocatorRef aws_wrapped_cf_allocator_new(struct aws_allocator *allocator);

/**
 * Cleans up any resources alloced in aws_wrapped_cf_allocator_new.
 */
AWS_COMMON_API
void aws_wrapped_cf_allocator_destroy(CFAllocatorRef allocator);
#endif

/**
 * Returns at least `size` of memory ready for usage or returns NULL on failure.
 */
AWS_COMMON_API
void *aws_mem_acquire(struct aws_allocator *allocator, size_t size);

/**
 * Allocates a block of memory for an array of num elements, each of them size bytes long, and initializes all its bits
 * to zero. Returns null on failure.
 */
AWS_COMMON_API
void *aws_mem_calloc(struct aws_allocator *allocator, size_t num, size_t size);

/**
 * Allocates many chunks of bytes into a single block. Expects to be called with alternating void ** (dest), size_t
 * (size). The first void ** will be set to the root of the allocation. Alignment is assumed to be sizeof(intmax_t).
 *
 * This is useful for allocating structs using the pimpl pattern, as you may allocate the public object and impl object
 * in the same contiguous block of memory.
 *
 * Returns a pointer to the allocation.
 */
AWS_COMMON_API
void *aws_mem_acquire_many(struct aws_allocator *allocator, size_t count, ...);

/**
 * Releases ptr back to whatever allocated it.
 * Nothing happens if ptr is NULL.
 */
AWS_COMMON_API
void aws_mem_release(struct aws_allocator *allocator, void *ptr);

/*
 * Attempts to adjust the size of the pointed-to memory buffer from oldsize to
 * newsize. The pointer (*ptr) may be changed if the memory needs to be
 * reallocated.
 *
 * If reallocation fails, *ptr is unchanged, and this method raises an
 * AWS_ERROR_OOM error.
 */
AWS_COMMON_API
int aws_mem_realloc(struct aws_allocator *allocator, void **ptr, size_t oldsize, size_t newsize);
/*
 * Maintainer note: The above function doesn't return the pointer (as with
 * standard C realloc) as this pattern becomes error-prone when OOMs occur.
 * In particular, we want to avoid losing the old pointer when an OOM condition
 * occurs, so we prefer to take the old pointer as an in/out reference argument
 * that we can leave unchanged on failure.
 */

enum aws_mem_trace_level {
    AWS_MEMTRACE_NONE = 0,   /* no tracing */
    AWS_MEMTRACE_BYTES = 1,  /* just track allocation sizes and total allocated */
    AWS_MEMTRACE_STACKS = 2, /* capture callstacks for each allocation */
};

/*
 * Wraps an allocator and tracks all external allocations. If aws_mem_trace_dump() is called
 * and there are still allocations active, they will be reported to the aws_logger at TRACE level.
 * allocator - The allocator to wrap
 * deprecated - Deprecated arg, ignored.
 * level - The level to track allocations at
 * frames_per_stack is how many frames to store per callstack if AWS_MEMTRACE_STACKS is in use,
 * otherwise it is ignored. 8 tends to be a pretty good number balancing storage space vs useful stacks.
 * Returns the tracer allocator, which should be used for all allocations that should be tracked.
 */
AWS_COMMON_API
struct aws_allocator *aws_mem_tracer_new(
    struct aws_allocator *allocator,
    struct aws_allocator *deprecated,
    enum aws_mem_trace_level level,
    size_t frames_per_stack);

/*
 * Unwraps the traced allocator and cleans up the tracer.
 * Returns the original allocator
 */
AWS_COMMON_API
struct aws_allocator *aws_mem_tracer_destroy(struct aws_allocator *trace_allocator);

/*
 * If there are outstanding allocations, dumps them to log, along with any information gathered
 * based on the trace level set when aws_mem_trace() was called.
 * Should be passed the tracer allocator returned from aws_mem_trace().
 */
AWS_COMMON_API
void aws_mem_tracer_dump(struct aws_allocator *trace_allocator);

/*
 * Returns the current number of bytes in outstanding allocations
 */
AWS_COMMON_API
size_t aws_mem_tracer_bytes(struct aws_allocator *trace_allocator);

/*
 * Returns the current number of outstanding allocations
 */
AWS_COMMON_API
size_t aws_mem_tracer_count(struct aws_allocator *trace_allocator);

AWS_EXTERN_C_END

#endif /* AWS_COMMON_ALLOCATOR_H */
