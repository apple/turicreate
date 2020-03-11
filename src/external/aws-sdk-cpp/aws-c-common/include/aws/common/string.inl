#ifndef AWS_COMMON_STRING_INL
#define AWS_COMMON_STRING_INL
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

#include <aws/common/string.h>

AWS_EXTERN_C_BEGIN
/**
 * Equivalent to str->bytes.
 */
AWS_STATIC_IMPL
const uint8_t *aws_string_bytes(const struct aws_string *str) {
    AWS_PRECONDITION(aws_string_is_valid(str));
    return str->bytes;
}

/**
 * Equivalent to `(const char *)str->bytes`.
 */
AWS_STATIC_IMPL
const char *aws_string_c_str(const struct aws_string *str) {
    AWS_PRECONDITION(aws_string_is_valid(str));
    return (const char *)str->bytes;
}

/**
 * Evaluates the set of properties that define the shape of all valid aws_string structures.
 * It is also a cheap check, in the sense it run in constant time (i.e., no loops or recursion).
 */
AWS_STATIC_IMPL
bool aws_string_is_valid(const struct aws_string *str) {
    return str && AWS_MEM_IS_READABLE(&str->bytes[0], str->len + 1) && str->bytes[str->len] == 0;
}

/**
 * Best-effort checks aws_string invariants, when the str->len is unknown
 */
AWS_STATIC_IMPL
bool aws_c_string_is_valid(const char *str) {
    /* Knowing the actual length to check would require strlen(), which is
     * a) linear time in the length of the string
     * b) could already cause a memory violation for a non-zero-terminated string.
     * But we know that a c-string must have at least one character, to store the null terminator
     */
    return str && AWS_MEM_IS_READABLE(str, 1);
}
AWS_EXTERN_C_END
#endif /* AWS_COMMON_STRING_INL */
