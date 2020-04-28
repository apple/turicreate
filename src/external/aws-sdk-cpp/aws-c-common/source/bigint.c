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
#include <aws/common/bigint.h>

#define BASE_BITS 32
#define NIBBLES_PER_DIGIT ((BASE_BITS) / 4)
#define LOWER_32_BIT_MASK 0xFFFFFFFF
#define INT64_MIN_AS_HEX 0x8000000000000000

/*
 * A basic big integer implementation using 2^32 as the base.  Algorithms used are formalizations of the basic
 * grade school operations everyone knows and loves (as formalized in AoCP Vol 2, 4.3.1).  Current use case
 * targets do not yet involve a domain large enough that its worth exploring more complex algorithms.
 */
struct aws_bigint {
    struct aws_allocator *allocator;

    /*
     * A sequence of base 2^32 digits starting from the least significant
     */
    struct aws_array_list digits;

    /*
     * 1 = positive, -1 = negative
     */
    int sign;
};

/*
 * Working set of invariants:
 *
 * (1) Negative zero is illegal
 * (2) The only time leading (trailing) 0-value digits are allowed is a single instance to represent actual zero.
 * (3) Functionally immutable API (no visible side-affects unless a successful operation with aliased parameters)
 *
 */

bool aws_bigint_is_valid(const struct aws_bigint *bigint) {
    if (bigint == NULL) {
        return false;
    }

    if (bigint->allocator == NULL) {
        return false;
    }

    if (!aws_array_list_is_valid(&bigint->digits)) {
        return false;
    }

    if (bigint->sign != 1 && bigint->sign != -1) {
        return false;
    }

    return true;
}

void aws_bigint_destroy(struct aws_bigint *bigint) {
    if (bigint == NULL) {
        return;
    }

    aws_array_list_clean_up(&bigint->digits);
    AWS_ZERO_STRUCT(bigint->digits);

    aws_mem_release(bigint->allocator, bigint);
}

static void s_advance_cursor_past_hex_prefix(struct aws_byte_cursor *hex_cursor) {
    AWS_PRECONDITION(aws_byte_cursor_is_valid(hex_cursor));

    if (hex_cursor->len >= 2) {
        const char *raw_ptr = (char *)hex_cursor->ptr;
        if (raw_ptr[0] == '0' && (raw_ptr[1] == 'x' || raw_ptr[1] == 'X')) {
            aws_byte_cursor_advance(hex_cursor, 2);
        }
    }
}

static void s_advance_cursor_to_non_zero(struct aws_byte_cursor *hex_cursor) {
    AWS_PRECONDITION(aws_byte_cursor_is_valid(hex_cursor));

    while (hex_cursor->len > 0 && *hex_cursor->ptr == '0') {
        aws_byte_cursor_advance(hex_cursor, 1);
    }
}

static int s_uint32_from_hex(struct aws_byte_cursor digit_cursor, uint32_t *output_value) {
    AWS_PRECONDITION(aws_byte_cursor_is_valid(&digit_cursor));
    AWS_PRECONDITION(output_value);

    AWS_FATAL_ASSERT(digit_cursor.len <= NIBBLES_PER_DIGIT);

    *output_value = 0;

    while (digit_cursor.len > 0) {
        char hex_digit = *digit_cursor.ptr;
        uint32_t hex_value = 0;
        if (hex_digit <= '9' && hex_digit >= '0') {
            hex_value = hex_digit - '0';
        } else if (hex_digit <= 'f' && hex_digit >= 'a') {
            hex_value = hex_digit - 'a' + 10;
        } else if (hex_digit <= 'F' && hex_digit >= 'A') {
            hex_value = hex_digit - 'A' + 10;
        } else {
            return AWS_OP_ERR;
        }

        *output_value <<= 4;
        *output_value += hex_value;

        aws_byte_cursor_advance(&digit_cursor, 1);
    }

    return AWS_OP_SUCCESS;
}

struct aws_bigint *aws_bigint_new_from_hex(struct aws_allocator *allocator, struct aws_byte_cursor hex_digits) {
    AWS_PRECONDITION(allocator);
    AWS_PRECONDITION(aws_byte_cursor_is_valid(&hex_digits));

    /* skip past the optional "0x" prefix */
    s_advance_cursor_past_hex_prefix(&hex_digits);
    if (hex_digits.len == 0) {
        aws_raise_error(AWS_ERROR_INVALID_ARGUMENT);
        return NULL;
    }

    /* skip past leading zeros */
    s_advance_cursor_to_non_zero(&hex_digits);
    if (hex_digits.len == 0) {
        return aws_bigint_new_from_uint64(allocator, 0);
    }

    struct aws_bigint *bigint = aws_mem_calloc(allocator, 1, sizeof(struct aws_bigint));
    if (bigint == NULL) {
        return NULL;
    }

    bigint->allocator = allocator;

    size_t digit_count = (hex_digits.len - 1) / NIBBLES_PER_DIGIT + 1;
    if (aws_array_list_init_dynamic(&bigint->digits, allocator, digit_count, sizeof(uint32_t))) {
        goto on_error;
    }

    while (hex_digits.len > 0) {
        struct aws_byte_cursor digit_cursor = hex_digits;
        if (digit_cursor.len > NIBBLES_PER_DIGIT) {
            digit_cursor.ptr += (digit_cursor.len - NIBBLES_PER_DIGIT);
            digit_cursor.len = NIBBLES_PER_DIGIT;
        }

        uint32_t digit_value = 0;
        if (s_uint32_from_hex(digit_cursor, &digit_value)) {
            aws_raise_error(AWS_ERROR_INVALID_ARGUMENT);
            goto on_error;
        }

        aws_array_list_push_back(&bigint->digits, &digit_value);

        hex_digits.len -= digit_cursor.len;
    }

    bigint->sign = 1;

    AWS_POSTCONDITION(aws_bigint_is_valid(bigint));

    return bigint;

on_error:

    aws_bigint_destroy(bigint);

    return NULL;
}

struct aws_bigint *aws_bigint_new_from_int64(struct aws_allocator *allocator, int64_t value) {
    AWS_PRECONDITION(allocator);

    if (value >= 0) {
        return aws_bigint_new_from_uint64(allocator, (uint64_t)value);
    }

    struct aws_bigint *bigint = NULL;
    if (value == INT64_MIN) {
        /* We can't just negate and cast, so just submit a constant */
        bigint = aws_bigint_new_from_uint64(allocator, INT64_MIN_AS_HEX);
    } else {
        /* The value is negative but can be safely negated to a positive value before casting to uint64 */
        bigint = aws_bigint_new_from_uint64(allocator, (uint64_t)(-value));
    }

    if (bigint == NULL) {
        return NULL;
    }

    bigint->sign = -1;

    AWS_POSTCONDITION(aws_bigint_is_valid(bigint));

    return bigint;
}

struct aws_bigint *aws_bigint_new_from_uint64(struct aws_allocator *allocator, uint64_t value) {
    AWS_PRECONDITION(allocator);

    struct aws_bigint *bigint = aws_mem_calloc(allocator, 1, sizeof(struct aws_bigint));
    if (bigint == NULL) {
        return NULL;
    }

    bigint->allocator = allocator;

    uint32_t lower_digit = (uint32_t)(value & LOWER_32_BIT_MASK);
    uint32_t upper_digit = (uint32_t)((value >> 32) & LOWER_32_BIT_MASK);
    size_t digit_count = upper_digit > 0 ? 2 : 1;
    if (aws_array_list_init_dynamic(&bigint->digits, allocator, digit_count, sizeof(uint32_t))) {
        goto on_error;
    }

    aws_array_list_push_back(&bigint->digits, &lower_digit);

    if (upper_digit > 0) {
        aws_array_list_push_back(&bigint->digits, &upper_digit);
    }

    bigint->sign = 1;

    AWS_POSTCONDITION(aws_bigint_is_valid(bigint));

    return bigint;

on_error:

    aws_bigint_destroy(bigint);

    return NULL;
}

struct aws_bigint *aws_bigint_new_from_copy(const struct aws_bigint *source) {
    AWS_PRECONDITION(aws_bigint_is_valid(source));

    struct aws_bigint *bigint = aws_mem_calloc(source->allocator, 1, sizeof(struct aws_bigint));
    if (bigint == NULL) {
        return NULL;
    }

    bigint->allocator = source->allocator;
    bigint->sign = source->sign;

    size_t source_length = aws_array_list_length(&source->digits);
    if (aws_array_list_init_dynamic(&bigint->digits, source->digits.alloc, source_length, sizeof(uint32_t))) {
        goto on_error;
    }

    for (size_t i = 0; i < source_length; ++i) {
        uint32_t digit = 0;
        aws_array_list_get_at(&source->digits, &digit, i);
        aws_array_list_push_back(&bigint->digits, &digit);
    }

    AWS_POSTCONDITION(aws_bigint_is_valid(bigint));

    return bigint;

on_error:

    aws_bigint_destroy(bigint);

    return NULL;
}

static const uint8_t *HEX_CHARS = (const uint8_t *)"0123456789abcdef";

static void s_append_uint32_as_hex(struct aws_byte_buf *buffer, uint32_t value, bool prepend_zeros) {

    bool have_seen_non_zero_nibble = false;
    size_t write_index = buffer->len;
    for (size_t i = 0; i < NIBBLES_PER_DIGIT; ++i) {
        uint8_t high_nibble = (uint8_t)(value >> 28);
        AWS_FATAL_ASSERT(high_nibble < 16);

        if (high_nibble > 0) {
            have_seen_non_zero_nibble = true;
        }

        if (have_seen_non_zero_nibble || prepend_zeros || i + 1 == NIBBLES_PER_DIGIT) {
            AWS_FATAL_ASSERT(write_index < buffer->capacity);
            buffer->buffer[write_index++] = HEX_CHARS[high_nibble];
        }

        value <<= 4;
    }

    buffer->len = write_index;
}

int aws_bigint_bytebuf_debug_output(const struct aws_bigint *bigint, struct aws_byte_buf *buffer) {
    AWS_PRECONDITION(aws_bigint_is_valid(bigint));
    AWS_PRECONDITION(aws_byte_buf_is_valid(buffer));

    size_t digit_count = aws_array_list_length(&bigint->digits);
    size_t max_hex_digits = aws_array_list_length(&bigint->digits) * NIBBLES_PER_DIGIT;
    size_t total_characters = max_hex_digits + ((bigint->sign < 0) ? 1 : 0);
    if (aws_byte_buf_reserve_relative(buffer, total_characters)) {
        return AWS_OP_ERR;
    }

    /*
     * We don't support negative hex numbers from an initialization standpoint, but we still
     * need to indicate the number's sign on output
     */
    if (bigint->sign < 0) {
        buffer->buffer[buffer->len++] = '-';
    }

    for (size_t i = 0; i < digit_count; i++) {
        size_t digit_index = digit_count - i - 1;
        uint32_t digit = 0;
        if (aws_array_list_get_at(&bigint->digits, &digit, digit_index)) {
            return AWS_OP_ERR;
        }

        bool prepend_zeros = i != 0;
        s_append_uint32_as_hex(buffer, digit, prepend_zeros);
    }

    AWS_POSTCONDITION(aws_bigint_is_valid(bigint));
    AWS_POSTCONDITION(aws_byte_buf_is_valid(buffer));

    return AWS_OP_SUCCESS;
}

bool aws_bigint_is_negative(const struct aws_bigint *bigint) {
    AWS_PRECONDITION(aws_bigint_is_valid(bigint));

    return bigint->sign < 0;
}

bool aws_bigint_is_positive(const struct aws_bigint *bigint) {
    AWS_PRECONDITION(aws_bigint_is_valid(bigint));

    return bigint->sign > 0 && !aws_bigint_is_zero(bigint);
}

bool aws_bigint_is_zero(const struct aws_bigint *bigint) {
    AWS_PRECONDITION(aws_bigint_is_valid(bigint));

    if (bigint->sign < 0) {
        return false;
    }

    size_t digit_count = aws_array_list_length(&bigint->digits);
    if (digit_count != 1) {
        return false;
    }

    uint32_t digit = 0;
    aws_array_list_get_at(&bigint->digits, &digit, 0);

    return digit == 0;
}

enum aws_bigint_ordering {
    AWS_BI_LESS_THAN,
    AWS_BI_EQUAL_TO,
    AWS_BI_GREATER_THAN,
};

static enum aws_bigint_ordering s_aws_bigint_get_magnitude_ordering(
    const struct aws_bigint *lhs,
    const struct aws_bigint *rhs) {

    size_t lhs_digit_count = aws_array_list_length(&lhs->digits);
    size_t rhs_digit_count = aws_array_list_length(&rhs->digits);

    if (lhs_digit_count < rhs_digit_count) {
        return AWS_BI_LESS_THAN;
    } else if (lhs_digit_count > rhs_digit_count) {
        return AWS_BI_GREATER_THAN;
    }

    for (size_t i = 0; i < lhs_digit_count; ++i) {
        uint32_t lhs_digit = 0;
        uint32_t rhs_digit = 0;

        aws_array_list_get_at(&lhs->digits, &lhs_digit, i);
        aws_array_list_get_at(&rhs->digits, &rhs_digit, i);

        if (lhs_digit < rhs_digit) {
            return AWS_BI_LESS_THAN;
        } else if (lhs_digit > rhs_digit) {
            return AWS_BI_GREATER_THAN;
        }
    }

    return AWS_BI_EQUAL_TO;
}

bool aws_bigint_equals(const struct aws_bigint *lhs, const struct aws_bigint *rhs) {
    AWS_PRECONDITION(aws_bigint_is_valid(lhs));
    AWS_PRECONDITION(aws_bigint_is_valid(rhs));

    return s_aws_bigint_get_magnitude_ordering(lhs, rhs) == AWS_BI_EQUAL_TO && lhs->sign == rhs->sign;
}

bool aws_bigint_not_equals(const struct aws_bigint *lhs, const struct aws_bigint *rhs) {
    AWS_PRECONDITION(aws_bigint_is_valid(lhs));
    AWS_PRECONDITION(aws_bigint_is_valid(rhs));

    return !aws_bigint_equals(lhs, rhs);
}

bool aws_bigint_less_than(const struct aws_bigint *lhs, const struct aws_bigint *rhs) {
    AWS_PRECONDITION(aws_bigint_is_valid(lhs));
    AWS_PRECONDITION(aws_bigint_is_valid(rhs));

    if (lhs->sign < 0) {
        if (rhs->sign < 0) {
            return s_aws_bigint_get_magnitude_ordering(lhs, rhs) == AWS_BI_GREATER_THAN;
        } else {
            return true;
        }
    } else {
        if (rhs->sign < 0) {
            return false;
        } else {
            return s_aws_bigint_get_magnitude_ordering(lhs, rhs) == AWS_BI_LESS_THAN;
        }
    }
}

bool aws_bigint_greater_than(const struct aws_bigint *lhs, const struct aws_bigint *rhs) {
    AWS_PRECONDITION(aws_bigint_is_valid(lhs));
    AWS_PRECONDITION(aws_bigint_is_valid(rhs));

    if (lhs->sign < 0) {
        if (rhs->sign < 0) {
            return s_aws_bigint_get_magnitude_ordering(lhs, rhs) == AWS_BI_LESS_THAN;
        } else {
            return false;
        }
    } else {
        if (rhs->sign < 0) {
            return true;
        } else {
            return s_aws_bigint_get_magnitude_ordering(lhs, rhs) == AWS_BI_GREATER_THAN;
        }
    }
}

bool aws_bigint_less_than_or_equals(const struct aws_bigint *lhs, const struct aws_bigint *rhs) {
    AWS_PRECONDITION(aws_bigint_is_valid(lhs));
    AWS_PRECONDITION(aws_bigint_is_valid(rhs));

    return !aws_bigint_greater_than(lhs, rhs);
}

bool aws_bigint_greater_than_or_equals(const struct aws_bigint *lhs, const struct aws_bigint *rhs) {
    AWS_PRECONDITION(aws_bigint_is_valid(lhs));
    AWS_PRECONDITION(aws_bigint_is_valid(rhs));

    return !aws_bigint_less_than(lhs, rhs);
}

void aws_bigint_negate(struct aws_bigint *bigint) {
    AWS_PRECONDITION(aws_bigint_is_valid(bigint));

    if (!aws_bigint_is_zero(bigint)) {
        bigint->sign *= -1;
    }
}
