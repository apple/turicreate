/*
 * Copyright 2010 - 2018 Amazon.com, Inc. or its affiliates.All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file.This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.See the License for the specific language governing
 * permissions and limitations under the License.
 */
#include <aws/common/thread.h>

#include <aws/common/clock.h>

#include <Windows.h>

static struct aws_thread_options s_default_options = {
    /* zero will make sure whatever the default for that version of windows is used. */
    .stack_size = 0,
};

struct thread_atexit_callback {
    aws_thread_atexit_fn *callback;
    void *user_data;
    struct thread_atexit_callback *next;
};

struct thread_wrapper {
    struct aws_allocator *allocator;
    void (*func)(void *arg);
    void *arg;
    struct thread_atexit_callback *atexit;
};

static AWS_THREAD_LOCAL struct thread_wrapper *tl_wrapper = NULL;

static DWORD WINAPI thread_wrapper_fn(LPVOID arg) {
    struct thread_wrapper thread_wrapper = *(struct thread_wrapper *)arg;
    struct aws_allocator *allocator = thread_wrapper.allocator;
    tl_wrapper = &thread_wrapper;
    thread_wrapper.func(thread_wrapper.arg);

    struct thread_atexit_callback *exit_callback_data = thread_wrapper.atexit;
    aws_mem_release(allocator, arg);

    while (exit_callback_data) {
        aws_thread_atexit_fn *exit_callback = exit_callback_data->callback;
        void *exit_callback_user_data = exit_callback_data->user_data;
        struct thread_atexit_callback *next_exit_callback_data = exit_callback_data->next;

        aws_mem_release(allocator, exit_callback_data);

        exit_callback(exit_callback_user_data);
        exit_callback_data = next_exit_callback_data;
    }
    tl_wrapper = NULL;

    return 0;
}

const struct aws_thread_options *aws_default_thread_options(void) {
    return &s_default_options;
}

struct callback_fn_wrapper {
    void (*call_once)(void *);
    void *user_data;
};

BOOL WINAPI s_init_once_wrapper(PINIT_ONCE init_once, void *param, void **context) {
    (void)context;
    (void)init_once;

    struct callback_fn_wrapper *callback_fn_wrapper = param;
    callback_fn_wrapper->call_once(callback_fn_wrapper->user_data);
    return TRUE;
}

void aws_thread_call_once(aws_thread_once *flag, void (*call_once)(void *), void *user_data) {
    struct callback_fn_wrapper wrapper;
    wrapper.call_once = call_once;
    wrapper.user_data = user_data;
    InitOnceExecuteOnce((PINIT_ONCE)flag, s_init_once_wrapper, &wrapper, NULL);
}

int aws_thread_init(struct aws_thread *thread, struct aws_allocator *allocator) {
    thread->thread_handle = 0;
    thread->thread_id = 0;
    thread->allocator = allocator;
    thread->detach_state = AWS_THREAD_NOT_CREATED;

    return AWS_OP_SUCCESS;
}

int aws_thread_launch(
    struct aws_thread *thread,
    void (*func)(void *arg),
    void *arg,
    const struct aws_thread_options *options) {

    SIZE_T stack_size = 0;

    if (options && options->stack_size > 0) {
        stack_size = (SIZE_T)options->stack_size;
    }

    struct thread_wrapper *thread_wrapper =
        (struct thread_wrapper *)aws_mem_calloc(thread->allocator, 1, sizeof(struct thread_wrapper));
    thread_wrapper->allocator = thread->allocator;
    thread_wrapper->arg = arg;
    thread_wrapper->func = func;

    thread->thread_handle =
        CreateThread(0, stack_size, thread_wrapper_fn, (LPVOID)thread_wrapper, 0, &thread->thread_id);

    if (!thread->thread_handle) {
        return aws_raise_error(AWS_ERROR_THREAD_INSUFFICIENT_RESOURCE);
    }

    thread->detach_state = AWS_THREAD_JOINABLE;
    return AWS_OP_SUCCESS;
}

aws_thread_id_t aws_thread_get_id(struct aws_thread *thread) {
    return thread->thread_id;
}

enum aws_thread_detach_state aws_thread_get_detach_state(struct aws_thread *thread) {
    return thread->detach_state;
}

int aws_thread_join(struct aws_thread *thread) {
    if (thread->detach_state == AWS_THREAD_JOINABLE) {
        WaitForSingleObject(thread->thread_handle, INFINITE);
        thread->detach_state = AWS_THREAD_JOIN_COMPLETED;
    }

    return AWS_OP_SUCCESS;
}

void aws_thread_clean_up(struct aws_thread *thread) {
    CloseHandle(thread->thread_handle);
    thread->thread_handle = 0;
}

aws_thread_id_t aws_thread_current_thread_id(void) {
    return GetCurrentThreadId();
}

bool aws_thread_thread_id_equal(aws_thread_id_t t1, aws_thread_id_t t2) {
    return t1 == t2;
}

void aws_thread_current_sleep(uint64_t nanos) {
    /* We don't really have a better option here for windows that isn't super
     * complex AND we don't have a use case yet where we should have sleeps
     * anywhere other than for context switches and testing. When that time
     * arises put the effort in here. */
    Sleep((DWORD)aws_timestamp_convert(nanos, AWS_TIMESTAMP_NANOS, AWS_TIMESTAMP_MILLIS, NULL));
}

int aws_thread_current_at_exit(aws_thread_atexit_fn *callback, void *user_data) {
    if (!tl_wrapper) {
        return aws_raise_error(AWS_ERROR_THREAD_NOT_JOINABLE);
    }

    struct thread_atexit_callback *cb = aws_mem_calloc(tl_wrapper->allocator, 1, sizeof(struct thread_atexit_callback));
    if (!cb) {
        return AWS_OP_ERR;
    }
    cb->callback = callback;
    cb->user_data = user_data;
    cb->next = tl_wrapper->atexit;
    tl_wrapper->atexit = cb;
    return AWS_OP_SUCCESS;
}
