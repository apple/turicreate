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

#include <aws/common/ring_buffer.h>

#include <aws/common/byte_buf.h>

#ifdef CBMC
#    define AWS_ATOMIC_LOAD_PTR(ring_buf, dest_ptr, atomic_ptr, memory_order)                                          \
        dest_ptr = aws_atomic_load_ptr_explicit(atomic_ptr, memory_order);                                             \
        assert(__CPROVER_POINTER_OBJECT(dest_ptr) == __CPROVER_POINTER_OBJECT(ring_buf->allocation));                  \
        assert(aws_ring_buffer_check_atomic_ptr(ring_buf, dest_ptr));
#    define AWS_ATOMIC_STORE_PTR(ring_buf, atomic_ptr, src_ptr, memory_order)                                          \
        assert(aws_ring_buffer_check_atomic_ptr(ring_buf, src_ptr));                                                   \
        aws_atomic_store_ptr_explicit(atomic_ptr, src_ptr, memory_order);
#else
#    define AWS_ATOMIC_LOAD_PTR(ring_buf, dest_ptr, atomic_ptr, memory_order)                                          \
        dest_ptr = aws_atomic_load_ptr_explicit(atomic_ptr, memory_order);
#    define AWS_ATOMIC_STORE_PTR(ring_buf, atomic_ptr, src_ptr, memory_order)                                          \
        aws_atomic_store_ptr_explicit(atomic_ptr, src_ptr, memory_order);
#endif
#define AWS_ATOMIC_LOAD_TAIL_PTR(ring_buf, dest_ptr)                                                                   \
    AWS_ATOMIC_LOAD_PTR(ring_buf, dest_ptr, &(ring_buf)->tail, aws_memory_order_acquire);
#define AWS_ATOMIC_STORE_TAIL_PTR(ring_buf, src_ptr)                                                                   \
    AWS_ATOMIC_STORE_PTR(ring_buf, &(ring_buf)->tail, src_ptr, aws_memory_order_release);
#define AWS_ATOMIC_LOAD_HEAD_PTR(ring_buf, dest_ptr)                                                                   \
    AWS_ATOMIC_LOAD_PTR(ring_buf, dest_ptr, &(ring_buf)->head, aws_memory_order_relaxed);
#define AWS_ATOMIC_STORE_HEAD_PTR(ring_buf, src_ptr)                                                                   \
    AWS_ATOMIC_STORE_PTR(ring_buf, &(ring_buf)->head, src_ptr, aws_memory_order_relaxed);

int aws_ring_buffer_init(struct aws_ring_buffer *ring_buf, struct aws_allocator *allocator, size_t size) {
    AWS_PRECONDITION(ring_buf != NULL);
    AWS_PRECONDITION(allocator != NULL);
    AWS_PRECONDITION(size > 0);

    AWS_ZERO_STRUCT(*ring_buf);

    ring_buf->allocation = aws_mem_acquire(allocator, size);

    if (!ring_buf->allocation) {
        return AWS_OP_ERR;
    }

    ring_buf->allocator = allocator;
    aws_atomic_init_ptr(&ring_buf->head, ring_buf->allocation);
    aws_atomic_init_ptr(&ring_buf->tail, ring_buf->allocation);
    ring_buf->allocation_end = ring_buf->allocation + size;

    AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
    return AWS_OP_SUCCESS;
}

void aws_ring_buffer_clean_up(struct aws_ring_buffer *ring_buf) {
    AWS_PRECONDITION(aws_ring_buffer_is_valid(ring_buf));
    if (ring_buf->allocation) {
        aws_mem_release(ring_buf->allocator, ring_buf->allocation);
    }

    AWS_ZERO_STRUCT(*ring_buf);
}

int aws_ring_buffer_acquire(struct aws_ring_buffer *ring_buf, size_t requested_size, struct aws_byte_buf *dest) {
    AWS_PRECONDITION(aws_ring_buffer_is_valid(ring_buf));
    AWS_PRECONDITION(aws_byte_buf_is_valid(dest));
    AWS_ERROR_PRECONDITION(requested_size != 0);

    uint8_t *tail_cpy;
    uint8_t *head_cpy;
    AWS_ATOMIC_LOAD_TAIL_PTR(ring_buf, tail_cpy);
    AWS_ATOMIC_LOAD_HEAD_PTR(ring_buf, head_cpy);

    /* this branch is, we don't have any vended buffers. */
    if (head_cpy == tail_cpy) {
        size_t ring_space = ring_buf->allocation_end - ring_buf->allocation;

        if (requested_size > ring_space) {
            AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
            AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
            return aws_raise_error(AWS_ERROR_OOM);
        }
        AWS_ATOMIC_STORE_HEAD_PTR(ring_buf, ring_buf->allocation + requested_size);
        AWS_ATOMIC_STORE_TAIL_PTR(ring_buf, ring_buf->allocation);
        *dest = aws_byte_buf_from_empty_array(ring_buf->allocation, requested_size);
        AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
        AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
        return AWS_OP_SUCCESS;
    }

    /* you'll constantly bounce between the next two branches as the ring buffer is traversed. */
    /* after N + 1 wraps */
    if (tail_cpy > head_cpy) {
        size_t space = tail_cpy - head_cpy - 1;

        if (space >= requested_size) {
            AWS_ATOMIC_STORE_HEAD_PTR(ring_buf, head_cpy + requested_size);
            *dest = aws_byte_buf_from_empty_array(head_cpy, requested_size);
            AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
            AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
            return AWS_OP_SUCCESS;
        }
        /* After N wraps */
    } else if (tail_cpy < head_cpy) {
        /* prefer the head space for efficiency. */
        if ((size_t)(ring_buf->allocation_end - head_cpy) >= requested_size) {
            AWS_ATOMIC_STORE_HEAD_PTR(ring_buf, head_cpy + requested_size);
            *dest = aws_byte_buf_from_empty_array(head_cpy, requested_size);
            AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
            AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
            return AWS_OP_SUCCESS;
        }

        if ((size_t)(tail_cpy - ring_buf->allocation) > requested_size) {
            AWS_ATOMIC_STORE_HEAD_PTR(ring_buf, ring_buf->allocation + requested_size);
            *dest = aws_byte_buf_from_empty_array(ring_buf->allocation, requested_size);
            AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
            AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
            return AWS_OP_SUCCESS;
        }
    }

    AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
    AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
    return aws_raise_error(AWS_ERROR_OOM);
}

int aws_ring_buffer_acquire_up_to(
    struct aws_ring_buffer *ring_buf,
    size_t minimum_size,
    size_t requested_size,
    struct aws_byte_buf *dest) {
    AWS_PRECONDITION(requested_size >= minimum_size);
    AWS_PRECONDITION(aws_ring_buffer_is_valid(ring_buf));
    AWS_PRECONDITION(aws_byte_buf_is_valid(dest));

    if (requested_size == 0 || minimum_size == 0 || !ring_buf || !dest) {
        AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
        AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
        return aws_raise_error(AWS_ERROR_INVALID_ARGUMENT);
    }

    uint8_t *tail_cpy;
    uint8_t *head_cpy;
    AWS_ATOMIC_LOAD_TAIL_PTR(ring_buf, tail_cpy);
    AWS_ATOMIC_LOAD_HEAD_PTR(ring_buf, head_cpy);

    /* this branch is, we don't have any vended buffers. */
    if (head_cpy == tail_cpy) {
        size_t ring_space = ring_buf->allocation_end - ring_buf->allocation;

        size_t allocation_size = ring_space > requested_size ? requested_size : ring_space;

        if (allocation_size < minimum_size) {
            AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
            AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
            return aws_raise_error(AWS_ERROR_OOM);
        }

        /* go as big as we can. */
        /* we don't have any vended, so this should be safe. */
        AWS_ATOMIC_STORE_HEAD_PTR(ring_buf, ring_buf->allocation + allocation_size);
        AWS_ATOMIC_STORE_TAIL_PTR(ring_buf, ring_buf->allocation);
        *dest = aws_byte_buf_from_empty_array(ring_buf->allocation, allocation_size);
        AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
        AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
        return AWS_OP_SUCCESS;
    }
    /* you'll constantly bounce between the next two branches as the ring buffer is traversed. */
    /* after N + 1 wraps */
    if (tail_cpy > head_cpy) {
        size_t space = tail_cpy - head_cpy;
        /* this shouldn't be possible. */
        AWS_ASSERT(space);
        space -= 1;

        size_t returnable_size = space > requested_size ? requested_size : space;

        if (returnable_size >= minimum_size) {
            AWS_ATOMIC_STORE_HEAD_PTR(ring_buf, head_cpy + returnable_size);
            *dest = aws_byte_buf_from_empty_array(head_cpy, returnable_size);
            AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
            AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
            return AWS_OP_SUCCESS;
        }
        /* after N wraps */
    } else if (tail_cpy < head_cpy) {
        size_t head_space = ring_buf->allocation_end - head_cpy;
        size_t tail_space = tail_cpy - ring_buf->allocation;

        /* if you can vend the whole thing do it. Also prefer head space to tail space. */
        if (head_space >= requested_size) {
            AWS_ATOMIC_STORE_HEAD_PTR(ring_buf, head_cpy + requested_size);
            *dest = aws_byte_buf_from_empty_array(head_cpy, requested_size);
            AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
            AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
            return AWS_OP_SUCCESS;
        }

        if (tail_space > requested_size) {
            AWS_ATOMIC_STORE_HEAD_PTR(ring_buf, ring_buf->allocation + requested_size);
            *dest = aws_byte_buf_from_empty_array(ring_buf->allocation, requested_size);
            AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
            AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
            return AWS_OP_SUCCESS;
        }

        /* now vend as much as possible, once again preferring head space. */
        if (head_space >= minimum_size && head_space >= tail_space) {
            AWS_ATOMIC_STORE_HEAD_PTR(ring_buf, head_cpy + head_space);
            *dest = aws_byte_buf_from_empty_array(head_cpy, head_space);
            AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
            AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
            return AWS_OP_SUCCESS;
        }

        if (tail_space > minimum_size) {
            AWS_ATOMIC_STORE_HEAD_PTR(ring_buf, ring_buf->allocation + tail_space - 1);
            *dest = aws_byte_buf_from_empty_array(ring_buf->allocation, tail_space - 1);
            AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
            AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
            return AWS_OP_SUCCESS;
        }
    }

    AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buf));
    AWS_POSTCONDITION(aws_byte_buf_is_valid(dest));
    return aws_raise_error(AWS_ERROR_OOM);
}

static inline bool s_buf_belongs_to_pool(const struct aws_ring_buffer *ring_buffer, const struct aws_byte_buf *buf) {
#ifdef CBMC
    /* only continue if buf points-into ring_buffer because comparison of pointers to different objects is undefined
     * (C11 6.5.8) */
    if ((__CPROVER_POINTER_OBJECT(buf->buffer) != __CPROVER_POINTER_OBJECT(ring_buffer->allocation)) ||
        (__CPROVER_POINTER_OBJECT(buf->buffer) != __CPROVER_POINTER_OBJECT(ring_buffer->allocation_end))) {
        return false;
    }
#endif
    return buf->buffer && ring_buffer->allocation && ring_buffer->allocation_end &&
           buf->buffer >= ring_buffer->allocation && buf->buffer + buf->capacity <= ring_buffer->allocation_end;
}

void aws_ring_buffer_release(struct aws_ring_buffer *ring_buffer, struct aws_byte_buf *buf) {
    AWS_PRECONDITION(aws_ring_buffer_is_valid(ring_buffer));
    AWS_PRECONDITION(aws_byte_buf_is_valid(buf));
    AWS_PRECONDITION(s_buf_belongs_to_pool(ring_buffer, buf));
    AWS_ATOMIC_STORE_TAIL_PTR(ring_buffer, buf->buffer + buf->capacity);
    AWS_ZERO_STRUCT(*buf);
    AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buffer));
}

bool aws_ring_buffer_buf_belongs_to_pool(const struct aws_ring_buffer *ring_buffer, const struct aws_byte_buf *buf) {
    AWS_PRECONDITION(aws_ring_buffer_is_valid(ring_buffer));
    AWS_PRECONDITION(aws_byte_buf_is_valid(buf));
    bool rval = s_buf_belongs_to_pool(ring_buffer, buf);
    AWS_POSTCONDITION(aws_ring_buffer_is_valid(ring_buffer));
    AWS_POSTCONDITION(aws_byte_buf_is_valid(buf));
    return rval;
}

/* Ring buffer allocator implementation */
static void *s_ring_buffer_mem_acquire(struct aws_allocator *allocator, size_t size) {
    struct aws_ring_buffer *buffer = allocator->impl;
    struct aws_byte_buf buf;
    AWS_ZERO_STRUCT(buf);
    /* allocate extra space for the size */
    if (aws_ring_buffer_acquire(buffer, size + sizeof(size_t), &buf)) {
        return NULL;
    }
    /* store the size ahead of the allocation */
    *((size_t *)buf.buffer) = buf.capacity;
    return buf.buffer + sizeof(size_t);
}

static void s_ring_buffer_mem_release(struct aws_allocator *allocator, void *ptr) {
    /* back up to where the size is stored */
    const void *addr = ((uint8_t *)ptr - sizeof(size_t));
    const size_t size = *((size_t *)addr);

    struct aws_byte_buf buf = aws_byte_buf_from_array(addr, size);
    buf.allocator = allocator;

    struct aws_ring_buffer *buffer = allocator->impl;
    aws_ring_buffer_release(buffer, &buf);
}

static void *s_ring_buffer_mem_calloc(struct aws_allocator *allocator, size_t num, size_t size) {
    void *mem = s_ring_buffer_mem_acquire(allocator, num * size);
    if (!mem) {
        return NULL;
    }
    memset(mem, 0, num * size);
    return mem;
}

static void *s_ring_buffer_mem_realloc(struct aws_allocator *allocator, void *ptr, size_t old_size, size_t new_size) {
    (void)allocator;
    (void)ptr;
    (void)old_size;
    (void)new_size;
    AWS_FATAL_ASSERT(!"ring_buffer_allocator does not support realloc, as it breaks allocation ordering");
    return NULL;
}

int aws_ring_buffer_allocator_init(struct aws_allocator *allocator, struct aws_ring_buffer *ring_buffer) {
    if (allocator == NULL || ring_buffer == NULL) {
        return aws_raise_error(AWS_ERROR_INVALID_ARGUMENT);
    }

    allocator->impl = ring_buffer;
    allocator->mem_acquire = s_ring_buffer_mem_acquire;
    allocator->mem_release = s_ring_buffer_mem_release;
    allocator->mem_calloc = s_ring_buffer_mem_calloc;
    allocator->mem_realloc = s_ring_buffer_mem_realloc;
    return AWS_OP_SUCCESS;
}

void aws_ring_buffer_allocator_clean_up(struct aws_allocator *allocator) {
    AWS_ZERO_STRUCT(*allocator);
}
