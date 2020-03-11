#ifndef AWS_COMMON_UUID_H
#define AWS_COMMON_UUID_H

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

struct aws_byte_cursor;
struct aws_byte_buf;

struct aws_uuid {
    uint8_t uuid_data[16];
};

/* 36 bytes for the UUID plus one more for the null terminator. */
#define AWS_UUID_STR_LEN 37

AWS_EXTERN_C_BEGIN

AWS_COMMON_API int aws_uuid_init(struct aws_uuid *uuid);
AWS_COMMON_API int aws_uuid_init_from_str(struct aws_uuid *uuid, const struct aws_byte_cursor *uuid_str);
AWS_COMMON_API int aws_uuid_to_str(const struct aws_uuid *uuid, struct aws_byte_buf *output);
AWS_COMMON_API bool aws_uuid_equals(const struct aws_uuid *a, const struct aws_uuid *b);

AWS_EXTERN_C_END

#endif /* AWS_COMMON_UUID_H */
