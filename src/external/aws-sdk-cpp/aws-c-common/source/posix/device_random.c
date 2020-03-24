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
#include <aws/common/device_random.h>

#include <aws/common/byte_buf.h>
#include <aws/common/thread.h>

#include <fcntl.h>
#include <unistd.h>

static int s_rand_fd = -1;
static aws_thread_once s_rand_init = AWS_THREAD_ONCE_STATIC_INIT;

#ifdef O_CLOEXEC
#    define OPEN_FLAGS (O_RDONLY | O_CLOEXEC)
#else
#    define OPEN_FLAGS (O_RDONLY)
#endif
static void s_init_rand(void *user_data) {
    (void)user_data;
    s_rand_fd = open("/dev/urandom", OPEN_FLAGS);

    if (s_rand_fd == -1) {
        s_rand_fd = open("/dev/urandom", O_RDONLY);

        if (s_rand_fd == -1) {
            abort();
        }
    }

    if (-1 == fcntl(s_rand_fd, F_SETFD, FD_CLOEXEC)) {
        abort();
    }
}

static int s_fallback_device_random_buffer(struct aws_byte_buf *output) {

    aws_thread_call_once(&s_rand_init, s_init_rand, NULL);

    size_t diff = output->capacity - output->len;

    ssize_t amount_read = read(s_rand_fd, output->buffer + output->len, diff);

    if (amount_read != diff) {
        return aws_raise_error(AWS_ERROR_RANDOM_GEN_FAILED);
    }

    output->len += diff;

    return AWS_OP_SUCCESS;
}

int aws_device_random_buffer(struct aws_byte_buf *output) {
    return s_fallback_device_random_buffer(output);
}
