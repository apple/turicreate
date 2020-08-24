
#ifndef AWS_COMMON_LOG_CHANNEL_H
#define AWS_COMMON_LOG_CHANNEL_H

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

#include <aws/common/common.h>

#include <aws/common/logging.h>

struct aws_string;
struct aws_log_writer;

/*
 * Log channel interface and default implementations
 *
 * A log channel is an abstraction for the transfer of formatted log data between a source (formatter)
 * and a sink (writer).
 */
struct aws_log_channel;

typedef int(aws_log_channel_send_fn)(struct aws_log_channel *channel, struct aws_string *output);
typedef void(aws_log_channel_clean_up_fn)(struct aws_log_channel *channel);

struct aws_log_channel_vtable {
    aws_log_channel_send_fn *send;
    aws_log_channel_clean_up_fn *clean_up;
};

struct aws_log_channel {
    struct aws_log_channel_vtable *vtable;
    struct aws_allocator *allocator;
    struct aws_log_writer *writer;
    void *impl;
};

AWS_EXTERN_C_BEGIN

/*
 * Simple channel that results in log lines being written in the same thread they were generated in.
 *
 * The passed in log writer is not an ownership transfer.  The log channel does not clean up the writer.
 */
AWS_COMMON_API
int aws_log_channel_init_foreground(
    struct aws_log_channel *channel,
    struct aws_allocator *allocator,
    struct aws_log_writer *writer);

/*
 * Simple channel that sends log lines to a background thread.
 *
 * The passed in log writer is not an ownership transfer.  The log channel does not clean up the writer.
 */
AWS_COMMON_API
int aws_log_channel_init_background(
    struct aws_log_channel *channel,
    struct aws_allocator *allocator,
    struct aws_log_writer *writer);

/*
 * Channel cleanup function
 */
AWS_COMMON_API
void aws_log_channel_clean_up(struct aws_log_channel *channel);

AWS_EXTERN_C_END

#endif /* AWS_COMMON_LOG_CHANNEL_H */
