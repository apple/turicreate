#ifndef AWS_COMMON_THREAD_H
#define AWS_COMMON_THREAD_H

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

#ifndef _WIN32
#    include <pthread.h>
#endif

enum aws_thread_detach_state {
    AWS_THREAD_NOT_CREATED = 1,
    AWS_THREAD_JOINABLE,
    AWS_THREAD_JOIN_COMPLETED,
};

struct aws_thread_options {
    size_t stack_size;
};

#ifdef _WIN32
typedef union {
    void *ptr;
} aws_thread_once;
#    define AWS_THREAD_ONCE_STATIC_INIT                                                                                \
        { NULL }
typedef unsigned long aws_thread_id_t;
#else
typedef pthread_once_t aws_thread_once;
#    define AWS_THREAD_ONCE_STATIC_INIT PTHREAD_ONCE_INIT
typedef pthread_t aws_thread_id_t;
#endif

/*
 * Buffer size needed to represent aws_thread_id_t as a string (2 hex chars per byte
 * plus '\0' terminator). Needed for portable printing because pthread_t is
 * opaque.
 */
#define AWS_THREAD_ID_T_REPR_BUFSZ (sizeof(aws_thread_id_t) * 2 + 1)

struct aws_thread {
    struct aws_allocator *allocator;
    enum aws_thread_detach_state detach_state;
#ifdef _WIN32
    void *thread_handle;
#endif
    aws_thread_id_t thread_id;
};

AWS_EXTERN_C_BEGIN

/**
 * Returns an instance of system default thread options.
 */
AWS_COMMON_API
const struct aws_thread_options *aws_default_thread_options(void);

AWS_COMMON_API void aws_thread_call_once(aws_thread_once *flag, void (*call_once)(void *), void *user_data);

/**
 * Initializes a new platform specific thread object struct (not the os-level
 * thread itself).
 */
AWS_COMMON_API
int aws_thread_init(struct aws_thread *thread, struct aws_allocator *allocator);

/**
 * Creates an OS level thread and associates it with func. context will be passed to func when it is executed.
 * options will be applied to the thread if they are applicable for the platform.
 * You must either call join or detach after creating the thread and before calling clean_up.
 */
AWS_COMMON_API
int aws_thread_launch(
    struct aws_thread *thread,
    void (*func)(void *arg),
    void *arg,
    const struct aws_thread_options *options);

/**
 * Gets the id of thread
 */
AWS_COMMON_API
aws_thread_id_t aws_thread_get_id(struct aws_thread *thread);

/**
 * Gets the detach state of the thread. For example, is it safe to call join on
 * this thread? Has it been detached()?
 */
AWS_COMMON_API
enum aws_thread_detach_state aws_thread_get_detach_state(struct aws_thread *thread);

/**
 * Joins the calling thread to a thread instance. Returns when thread is
 * finished.
 */
AWS_COMMON_API
int aws_thread_join(struct aws_thread *thread);

/**
 * Cleans up the thread handle. Either detach or join must be called
 * before calling this function.
 */
AWS_COMMON_API
void aws_thread_clean_up(struct aws_thread *thread);

/**
 * Returns the thread id of the calling thread.
 */
AWS_COMMON_API
aws_thread_id_t aws_thread_current_thread_id(void);

/**
 * Compare thread ids.
 */
AWS_COMMON_API
bool aws_thread_thread_id_equal(aws_thread_id_t t1, aws_thread_id_t t2);

/**
 * Sleeps the current thread by nanos.
 */
AWS_COMMON_API
void aws_thread_current_sleep(uint64_t nanos);

typedef void(aws_thread_atexit_fn)(void *user_data);

/**
 * Adds a callback to the chain to be called when the current thread joins.
 * Callbacks are called from the current thread, in the reverse order they
 * were added, after the thread function returns.
 * If not called from within an aws_thread, has no effect.
 */
AWS_COMMON_API
int aws_thread_current_at_exit(aws_thread_atexit_fn *callback, void *user_data);

AWS_EXTERN_C_END

#endif /* AWS_COMMON_THREAD_H */
