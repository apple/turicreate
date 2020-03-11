
#ifndef AWS_COMMON_LOG_WRITER_H
#define AWS_COMMON_LOG_WRITER_H

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

struct aws_allocator;
struct aws_string;

/*
 * Log writer interface and default implementation(s)
 *
 * A log writer functions as a sink for formatted log lines.  We provide
 * default implementations that go to stdout, stderr, and a specified file.
 */
struct aws_log_writer;

typedef int(aws_log_writer_write_fn)(struct aws_log_writer *writer, const struct aws_string *output);
typedef void(aws_log_writer_clean_up_fn)(struct aws_log_writer *writer);

struct aws_log_writer_vtable {
    aws_log_writer_write_fn *write;
    aws_log_writer_clean_up_fn *clean_up;
};

struct aws_log_writer {
    struct aws_log_writer_vtable *vtable;
    struct aws_allocator *allocator;
    void *impl;
};

struct aws_log_writer_file_options {
    const char *filename;
    FILE *file;
};

AWS_EXTERN_C_BEGIN

/*
 * Initialize a log writer that sends log lines to stdout.  Uses C library IO.
 */
AWS_COMMON_API
int aws_log_writer_init_stdout(struct aws_log_writer *writer, struct aws_allocator *allocator);

/*
 * Initialize a log writer that sends log lines to stderr.  Uses C library IO.
 */
AWS_COMMON_API
int aws_log_writer_init_stderr(struct aws_log_writer *writer, struct aws_allocator *allocator);

/*
 * Initialize a log writer that sends log lines to a file.  Uses C library IO.
 */
AWS_COMMON_API
int aws_log_writer_init_file(
    struct aws_log_writer *writer,
    struct aws_allocator *allocator,
    struct aws_log_writer_file_options *options);

/*
 * Frees all resources used by a log writer with the exception of the base structure memory
 */
AWS_COMMON_API
void aws_log_writer_clean_up(struct aws_log_writer *writer);

AWS_EXTERN_C_END

#endif /* AWS_COMMON_LOG_WRITER_H */
