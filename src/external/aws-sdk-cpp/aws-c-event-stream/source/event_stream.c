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

#include <aws/event-stream/event_stream.h>

#include <aws/checksums/crc.h>

#include <aws/common/encoding.h>

#include <inttypes.h>

/* max message size is 16MB */
#define MAX_MESSAGE_SIZE (16 * 1024 * 1024)

/* max header size is 128kb */
#define MAX_HEADERS_SIZE (128 * 1024)
#define LIB_NAME "libaws-c-event-stream"

#if _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4221) /* aggregate initializer using local variable addresses */
#    pragma warning(disable : 4204) /* non-constant aggregate initializer */
#    pragma warning(disable : 4306) /* msft doesn't trust us to do pointer arithmetic. */
#endif

static struct aws_error_info s_errors[] = {
    AWS_DEFINE_ERROR_INFO(AWS_ERROR_EVENT_STREAM_BUFFER_LENGTH_MISMATCH, "Buffer length mismatch", LIB_NAME),
    AWS_DEFINE_ERROR_INFO(AWS_ERROR_EVENT_STREAM_INSUFFICIENT_BUFFER_LEN, "insufficient buffer length", LIB_NAME),
    AWS_DEFINE_ERROR_INFO(
        AWS_ERROR_EVENT_STREAM_MESSAGE_FIELD_SIZE_EXCEEDED,
        "a field for the message was too large",
        LIB_NAME),
    AWS_DEFINE_ERROR_INFO(AWS_ERROR_EVENT_STREAM_PRELUDE_CHECKSUM_FAILURE, "prelude checksum was incorrect", LIB_NAME),
    AWS_DEFINE_ERROR_INFO(AWS_ERROR_EVENT_STREAM_MESSAGE_CHECKSUM_FAILURE, "message checksum was incorrect", LIB_NAME),
    AWS_DEFINE_ERROR_INFO(
        AWS_ERROR_EVENT_STREAM_MESSAGE_INVALID_HEADERS_LEN,
        "message headers length was incorrect",
        LIB_NAME),
    AWS_DEFINE_ERROR_INFO(
        AWS_ERROR_EVENT_STREAM_MESSAGE_UNKNOWN_HEADER_TYPE,
        "An unknown header type was encountered",
        LIB_NAME),
    AWS_DEFINE_ERROR_INFO(
        AWS_ERROR_EVENT_STREAM_MESSAGE_PARSER_ILLEGAL_STATE,
        "message parser encountered an illegal state",
        LIB_NAME),
};

static struct aws_error_info_list s_list = {
    .error_list = s_errors,
    .count = sizeof(s_errors) / sizeof(struct aws_error_info),
};

static bool s_event_stream_library_initialized = false;

void aws_event_stream_library_init(struct aws_allocator *allocator) {
    if (!s_event_stream_library_initialized) {
        s_event_stream_library_initialized = true;
        aws_common_library_init(allocator);
        aws_register_error_info(&s_list);
    }
}

void aws_event_stream_library_clean_up(void) {
    if (s_event_stream_library_initialized) {
        s_event_stream_library_initialized = false;
        aws_unregister_error_info(&s_list);
        aws_common_library_clean_up();
    }
}

#define TOTAL_LEN_OFFSET 0
#define PRELUDE_CRC_OFFSET (sizeof(uint32_t) + sizeof(uint32_t))
#define HEADER_LEN_OFFSET sizeof(uint32_t)

/* Computes the byte length necessary to store the headers represented in the headers list.
 * returns that length. */
uint32_t compute_headers_len(struct aws_array_list *headers) {
    if (!headers || !aws_array_list_length(headers)) {
        return 0;
    }

    size_t headers_count = aws_array_list_length(headers);
    size_t headers_len = 0;

    for (size_t i = 0; i < headers_count; ++i) {
        struct aws_event_stream_header_value_pair *header = NULL;

        aws_array_list_get_at_ptr(headers, (void **)&header, i);

        headers_len += sizeof(header->header_name_len) + header->header_name_len + 1;

        if (header->header_value_type == AWS_EVENT_STREAM_HEADER_STRING ||
            header->header_value_type == AWS_EVENT_STREAM_HEADER_BYTE_BUF) {
            headers_len += sizeof(header->header_value_len);
        }

        if (header->header_value_type != AWS_EVENT_STREAM_HEADER_BOOL_FALSE &&
            header->header_value_type != AWS_EVENT_STREAM_HEADER_BOOL_TRUE) {
            headers_len += header->header_value_len;
        }
    }

    return (uint32_t)headers_len;
}

/* adds the headers represented in the headers list to the buffer.
 returns the new buffer offset for use elsewhere. Assumes buffer length is at least the length of the return value
 from compute_headers_length() */
size_t add_headers_to_buffer(struct aws_array_list *headers, uint8_t *buffer) {
    if (!headers || !aws_array_list_length(headers)) {
        return 0;
    }

    size_t headers_count = aws_array_list_length(headers);
    uint8_t *buffer_alias = buffer;

    for (size_t i = 0; i < headers_count; ++i) {
        struct aws_event_stream_header_value_pair *header = NULL;

        aws_array_list_get_at_ptr(headers, (void **)&header, i);
        *buffer_alias = (uint8_t)header->header_name_len;
        buffer_alias++;
        memcpy(buffer_alias, header->header_name, (size_t)header->header_name_len);
        buffer_alias += header->header_name_len;
        *buffer_alias = (uint8_t)header->header_value_type;
        buffer_alias++;
        switch (header->header_value_type) {
            case AWS_EVENT_STREAM_HEADER_BOOL_FALSE:
            case AWS_EVENT_STREAM_HEADER_BOOL_TRUE:
                break;
            case AWS_EVENT_STREAM_HEADER_BYTE:
                *buffer_alias = header->header_value.static_val[0];
                buffer_alias++;
                break;
                /* additions of integers here assume the endianness conversion has already happened */
            case AWS_EVENT_STREAM_HEADER_INT16:
                memcpy(buffer_alias, header->header_value.static_val, sizeof(uint16_t));
                buffer_alias += sizeof(uint16_t);
                break;
            case AWS_EVENT_STREAM_HEADER_INT32:
                memcpy(buffer_alias, header->header_value.static_val, sizeof(uint32_t));
                buffer_alias += sizeof(uint32_t);
                break;
            case AWS_EVENT_STREAM_HEADER_INT64:
            case AWS_EVENT_STREAM_HEADER_TIMESTAMP:
                memcpy(buffer_alias, header->header_value.static_val, sizeof(uint64_t));
                buffer_alias += sizeof(uint64_t);
                break;
            case AWS_EVENT_STREAM_HEADER_BYTE_BUF:
            case AWS_EVENT_STREAM_HEADER_STRING:
                aws_write_u16(header->header_value_len, buffer_alias);
                buffer_alias += sizeof(uint16_t);
                memcpy(buffer_alias, header->header_value.variable_len_val, header->header_value_len);
                buffer_alias += header->header_value_len;
                break;
            case AWS_EVENT_STREAM_HEADER_UUID:
                memcpy(buffer_alias, header->header_value.static_val, 16);
                buffer_alias += header->header_value_len;
                break;
        }
    }

    return buffer_alias - buffer;
}

/* Get the headers from the buffer, store them in the headers list.
 * the user's reponsibility to cleanup the list when they are finished with it.
 * no buffer copies happen here, the lifetime of the buffer, must outlive the usage of the headers.
 * returns error codes defined in the public interface. */
int get_headers_from_buffer(struct aws_array_list *headers, const uint8_t *buffer, size_t headers_len) {

    if (AWS_UNLIKELY(headers_len > MAX_HEADERS_SIZE)) {
        return aws_raise_error(AWS_ERROR_EVENT_STREAM_MESSAGE_FIELD_SIZE_EXCEEDED);
    }

    /* iterate the buffer per header. */
    const uint8_t *buffer_start = buffer;
    while ((size_t)(buffer - buffer_start) < headers_len) {
        struct aws_event_stream_header_value_pair header;
        AWS_ZERO_STRUCT(header);

        /* get the header info from the buffer, make sure to increment buffer offset. */
        header.header_name_len = *buffer;
        buffer += sizeof(header.header_name_len);
        memcpy((void *)header.header_name, buffer, (size_t)header.header_name_len);
        buffer += header.header_name_len;
        header.header_value_type = (enum aws_event_stream_header_value_type) * buffer;
        buffer++;

        switch (header.header_value_type) {
            case AWS_EVENT_STREAM_HEADER_BOOL_FALSE:
                header.header_value_len = 0;
                header.header_value.static_val[0] = 0;
                break;
            case AWS_EVENT_STREAM_HEADER_BOOL_TRUE:
                header.header_value_len = 0;
                header.header_value.static_val[0] = 1;
                break;
            case AWS_EVENT_STREAM_HEADER_BYTE:
                header.header_value_len = sizeof(uint8_t);
                header.header_value.static_val[0] = *buffer;
                buffer++;
                break;
            case AWS_EVENT_STREAM_HEADER_INT16:
                header.header_value_len = sizeof(uint16_t);
                memcpy(header.header_value.static_val, buffer, sizeof(uint16_t));
                buffer += sizeof(uint16_t);
                break;
            case AWS_EVENT_STREAM_HEADER_INT32:
                header.header_value_len = sizeof(uint32_t);
                memcpy(header.header_value.static_val, buffer, sizeof(uint32_t));
                buffer += sizeof(uint32_t);
                break;
            case AWS_EVENT_STREAM_HEADER_INT64:
            case AWS_EVENT_STREAM_HEADER_TIMESTAMP:
                header.header_value_len = sizeof(uint64_t);
                memcpy(header.header_value.static_val, buffer, sizeof(uint64_t));
                buffer += sizeof(uint64_t);
                break;
            case AWS_EVENT_STREAM_HEADER_BYTE_BUF:
            case AWS_EVENT_STREAM_HEADER_STRING:
                header.header_value_len = aws_read_u16(buffer);
                buffer += sizeof(header.header_value_len);
                header.header_value.variable_len_val = (uint8_t *)buffer;
                buffer += header.header_value_len;
                break;
            case AWS_EVENT_STREAM_HEADER_UUID:
                header.header_value_len = 16;
                memcpy(header.header_value.static_val, buffer, 16);
                buffer += header.header_value_len;
                break;
        }

        if (aws_array_list_push_back(headers, (const void *)&header)) {
            return AWS_OP_ERR;
        }
    }

    return AWS_OP_SUCCESS;
}

/* initialize message with the arguments
 * the underlying buffer will be allocated and payload will be copied.
 * see specification, this code should simply add these fields according to that.*/
int aws_event_stream_message_init(
    struct aws_event_stream_message *message,
    struct aws_allocator *alloc,
    struct aws_array_list *headers,
    struct aws_byte_buf *payload) {

    size_t payload_len = payload ? payload->len : 0;

    uint32_t headers_length = compute_headers_len(headers);

    if (AWS_UNLIKELY(headers_length > MAX_HEADERS_SIZE)) {
        return aws_raise_error(AWS_ERROR_EVENT_STREAM_MESSAGE_FIELD_SIZE_EXCEEDED);
    }

    uint32_t total_length =
        (uint32_t)(AWS_EVENT_STREAM_PRELUDE_LENGTH + headers_length + payload_len + AWS_EVENT_STREAM_TRAILER_LENGTH);

    if (AWS_UNLIKELY(total_length < headers_length || total_length < payload_len)) {
        return aws_raise_error(AWS_ERROR_OVERFLOW_DETECTED);
    }

    if (AWS_UNLIKELY(total_length > MAX_MESSAGE_SIZE)) {
        return aws_raise_error(AWS_ERROR_EVENT_STREAM_MESSAGE_FIELD_SIZE_EXCEEDED);
    }

    message->alloc = alloc;
    message->message_buffer = aws_mem_acquire(message->alloc, total_length);

    if (message->message_buffer) {
        message->owns_buffer = 1;
        aws_write_u32(total_length, message->message_buffer);
        uint8_t *buffer_offset = message->message_buffer + sizeof(total_length);
        aws_write_u32(headers_length, buffer_offset);
        buffer_offset += sizeof(headers_length);

        uint32_t running_crc =
            aws_checksums_crc32(message->message_buffer, (int)(buffer_offset - message->message_buffer), 0);

        const uint8_t *message_crc_boundary_start = buffer_offset;
        aws_write_u32(running_crc, buffer_offset);
        buffer_offset += sizeof(running_crc);

        if (headers_length) {
            buffer_offset += add_headers_to_buffer(headers, buffer_offset);
        }

        if (payload) {
            memcpy(buffer_offset, payload->buffer, payload->len);
            buffer_offset += payload->len;
        }

        running_crc = aws_checksums_crc32(
            message_crc_boundary_start, (int)(buffer_offset - message_crc_boundary_start), running_crc);
        aws_write_u32(running_crc, buffer_offset);

        return AWS_OP_SUCCESS;
    }

    return aws_raise_error(AWS_ERROR_OOM);
}

/* add buffer to the message (non-owning). Verify buffer crcs and that length fields are reasonable. */
int aws_event_stream_message_from_buffer(
    struct aws_event_stream_message *message,
    struct aws_allocator *alloc,
    struct aws_byte_buf *buffer) {
    AWS_ASSERT(buffer);

    message->alloc = alloc;
    message->owns_buffer = 0;

    if (AWS_UNLIKELY(buffer->len < AWS_EVENT_STREAM_PRELUDE_LENGTH + AWS_EVENT_STREAM_TRAILER_LENGTH)) {
        return aws_raise_error(AWS_ERROR_EVENT_STREAM_BUFFER_LENGTH_MISMATCH);
    }

    uint32_t message_length = aws_read_u32(buffer->buffer + TOTAL_LEN_OFFSET);

    if (AWS_UNLIKELY(message_length != buffer->len)) {
        return aws_raise_error(AWS_ERROR_EVENT_STREAM_BUFFER_LENGTH_MISMATCH);
    }

    if (AWS_UNLIKELY(message_length > MAX_MESSAGE_SIZE)) {
        return aws_raise_error(AWS_ERROR_EVENT_STREAM_MESSAGE_FIELD_SIZE_EXCEEDED);
    }

    uint32_t running_crc = aws_checksums_crc32(buffer->buffer, (int)PRELUDE_CRC_OFFSET, 0);
    uint32_t prelude_crc = aws_read_u32(buffer->buffer + PRELUDE_CRC_OFFSET);

    if (running_crc != prelude_crc) {
        return aws_raise_error(AWS_ERROR_EVENT_STREAM_PRELUDE_CHECKSUM_FAILURE);
    }

    running_crc = aws_checksums_crc32(
        buffer->buffer + PRELUDE_CRC_OFFSET,
        (int)(message_length - PRELUDE_CRC_OFFSET - AWS_EVENT_STREAM_TRAILER_LENGTH),
        running_crc);
    uint32_t message_crc = aws_read_u32(buffer->buffer + message_length - AWS_EVENT_STREAM_TRAILER_LENGTH);

    if (running_crc != message_crc) {
        return aws_raise_error(AWS_ERROR_EVENT_STREAM_MESSAGE_CHECKSUM_FAILURE);
    }

    message->message_buffer = buffer->buffer;

    if (aws_event_stream_message_headers_len(message) >
        message_length - AWS_EVENT_STREAM_PRELUDE_LENGTH - AWS_EVENT_STREAM_TRAILER_LENGTH) {
        message->message_buffer = 0;
        return aws_raise_error(AWS_ERROR_EVENT_STREAM_MESSAGE_INVALID_HEADERS_LEN);
    }

    return AWS_OP_SUCCESS;
}

/* Verify buffer crcs and that length fields are reasonable. Once that is done, the buffer is copied to the message. */
int aws_event_stream_message_from_buffer_copy(
    struct aws_event_stream_message *message,
    struct aws_allocator *alloc,
    const struct aws_byte_buf *buffer) {
    int parse_value = aws_event_stream_message_from_buffer(message, alloc, (struct aws_byte_buf *)buffer);

    if (!parse_value) {
        message->message_buffer = aws_mem_acquire(alloc, buffer->len);

        if (message->message_buffer) {
            memcpy(message->message_buffer, buffer->buffer, buffer->len);
            message->alloc = alloc;
            message->owns_buffer = 1;

            return AWS_OP_SUCCESS;
        }

        return aws_raise_error(AWS_ERROR_OOM);
    }

    return parse_value;
}

/* if buffer is owned, release the memory. */
void aws_event_stream_message_clean_up(struct aws_event_stream_message *message) {
    if (message->message_buffer && message->owns_buffer) {
        aws_mem_release(message->alloc, message->message_buffer);
    }
}

uint32_t aws_event_stream_message_total_length(const struct aws_event_stream_message *message) {
    return aws_read_u32(message->message_buffer + TOTAL_LEN_OFFSET);
}

uint32_t aws_event_stream_message_headers_len(const struct aws_event_stream_message *message) {
    return aws_read_u32(message->message_buffer + HEADER_LEN_OFFSET);
}

uint32_t aws_event_stream_message_prelude_crc(const struct aws_event_stream_message *message) {
    return aws_read_u32(message->message_buffer + PRELUDE_CRC_OFFSET);
}

int aws_event_stream_message_headers(const struct aws_event_stream_message *message, struct aws_array_list *headers) {
    return get_headers_from_buffer(
        headers,
        message->message_buffer + AWS_EVENT_STREAM_PRELUDE_LENGTH,
        aws_event_stream_message_headers_len(message));
}

const uint8_t *aws_event_stream_message_payload(const struct aws_event_stream_message *message) {
    return message->message_buffer + AWS_EVENT_STREAM_PRELUDE_LENGTH + aws_event_stream_message_headers_len(message);
}

uint32_t aws_event_stream_message_payload_len(const struct aws_event_stream_message *message) {
    return aws_event_stream_message_total_length(message) -
           (AWS_EVENT_STREAM_PRELUDE_LENGTH + aws_event_stream_message_headers_len(message) +
            AWS_EVENT_STREAM_TRAILER_LENGTH);
}

uint32_t aws_event_stream_message_message_crc(const struct aws_event_stream_message *message) {
    return aws_read_u32(
        message->message_buffer + (aws_event_stream_message_total_length(message) - AWS_EVENT_STREAM_TRAILER_LENGTH));
}

const uint8_t *aws_event_stream_message_buffer(const struct aws_event_stream_message *message) {
    return message->message_buffer;
}

#define DEBUG_STR_PRELUDE_TOTAL_LEN "\"total_length\": "
#define DEBUG_STR_PRELUDE_HDRS_LEN "\"headers_length\": "
#define DEBUG_STR_PRELUDE_CRC "\"prelude_crc\": "
#define DEBUG_STR_MESSAGE_CRC "\"message_crc\": "
#define DEBUG_STR_HEADER_NAME "\"name\": "
#define DEBUG_STR_HEADER_VALUE "\"value\": "
#define DEBUG_STR_HEADER_TYPE "\"type\": "

int aws_event_stream_message_to_debug_str(FILE *fd, const struct aws_event_stream_message *message) {
    struct aws_array_list headers;
    aws_event_stream_headers_list_init(&headers, message->alloc);
    aws_event_stream_message_headers(message, &headers);

    fprintf(
        fd,
        "{\n  " DEBUG_STR_PRELUDE_TOTAL_LEN "%d,\n  " DEBUG_STR_PRELUDE_HDRS_LEN "%d,\n  " DEBUG_STR_PRELUDE_CRC
        "%d,\n",
        aws_event_stream_message_total_length(message),
        aws_event_stream_message_headers_len(message),
        aws_event_stream_message_prelude_crc(message));

    int count = 0;

    uint16_t headers_count = (uint16_t)aws_array_list_length(&headers);

    fprintf(fd, "  \"headers\": [");

    for (uint16_t i = 0; i < headers_count; ++i) {
        struct aws_event_stream_header_value_pair *header = NULL;

        aws_array_list_get_at_ptr(&headers, (void **)&header, i);

        fprintf(fd, "    {\n");

        fprintf(fd, "      " DEBUG_STR_HEADER_NAME "\"");
        fwrite(header->header_name, sizeof(char), (size_t)header->header_name_len, fd);
        fprintf(fd, "\",\n");

        fprintf(fd, "      " DEBUG_STR_HEADER_TYPE "%d,\n", header->header_value_type);

        if (header->header_value_type == AWS_EVENT_STREAM_HEADER_BOOL_FALSE) {
            fprintf(fd, "      " DEBUG_STR_HEADER_VALUE "false\n");
        } else if (header->header_value_type == AWS_EVENT_STREAM_HEADER_BOOL_TRUE) {
            fprintf(fd, "      " DEBUG_STR_HEADER_VALUE "true\n");
        } else if (header->header_value_type == AWS_EVENT_STREAM_HEADER_BYTE) {
            int8_t int_value = (int8_t)header->header_value.static_val[0];
            fprintf(fd, "      " DEBUG_STR_HEADER_VALUE "%d\n", (int)int_value);
        } else if (header->header_value_type == AWS_EVENT_STREAM_HEADER_INT16) {
            int16_t int_value = aws_read_u16(header->header_value.static_val);
            fprintf(fd, "      " DEBUG_STR_HEADER_VALUE "%d\n", (int)int_value);
        } else if (header->header_value_type == AWS_EVENT_STREAM_HEADER_INT32) {
            int32_t int_value = (int32_t)aws_read_u32(header->header_value.static_val);
            fprintf(fd, "      " DEBUG_STR_HEADER_VALUE "%d\n", (int)int_value);
        } else if (
            header->header_value_type == AWS_EVENT_STREAM_HEADER_INT64 ||
            header->header_value_type == AWS_EVENT_STREAM_HEADER_TIMESTAMP) {
            int64_t int_value = (int64_t)aws_read_u64(header->header_value.static_val);
            fprintf(fd, "      " DEBUG_STR_HEADER_VALUE "%lld\n", (long long)int_value);
        } else {
            size_t buffer_len = 0;
            aws_base64_compute_encoded_len(header->header_value_len, &buffer_len);
            char *encoded_buffer = (char *)aws_mem_acquire(message->alloc, buffer_len);
            if (!encoded_buffer) {
                return aws_raise_error(AWS_ERROR_OOM);
            }

            struct aws_byte_buf encode_output = aws_byte_buf_from_array((uint8_t *)encoded_buffer, buffer_len);

            if (header->header_value_type == AWS_EVENT_STREAM_HEADER_UUID) {
                struct aws_byte_cursor to_encode =
                    aws_byte_cursor_from_array(header->header_value.static_val, header->header_value_len);

                aws_base64_encode(&to_encode, &encode_output);
            } else {
                struct aws_byte_cursor to_encode =
                    aws_byte_cursor_from_array(header->header_value.variable_len_val, header->header_value_len);
                aws_base64_encode(&to_encode, &encode_output);
            }
            fprintf(fd, "      " DEBUG_STR_HEADER_VALUE "\"%s\"\n", encoded_buffer);
            aws_mem_release(message->alloc, encoded_buffer);
        }

        fprintf(fd, "    }");

        if (count < headers_count - 1) {
            fprintf(fd, ",");
        }
        fprintf(fd, "\n");

        count++;
    }
    aws_event_stream_headers_list_cleanup(&headers);
    fprintf(fd, "  ],\n");

    size_t payload_len = aws_event_stream_message_payload_len(message);
    const uint8_t *payload = aws_event_stream_message_payload(message);
    size_t encoded_len = 0;
    aws_base64_compute_encoded_len(payload_len, &encoded_len);
    char *encoded_payload = (char *)aws_mem_acquire(message->alloc, encoded_len);

    if (!encoded_payload) {
        return aws_raise_error(AWS_ERROR_OOM);
    }

    struct aws_byte_cursor payload_buffer = aws_byte_cursor_from_array(payload, payload_len);
    struct aws_byte_buf encoded_payload_buffer = aws_byte_buf_from_array((uint8_t *)encoded_payload, encoded_len);

    aws_base64_encode(&payload_buffer, &encoded_payload_buffer);
    fprintf(fd, "  \"payload\": \"%s\",\n", encoded_payload);
    fprintf(fd, "  " DEBUG_STR_MESSAGE_CRC "%d\n}\n", aws_event_stream_message_message_crc(message));

    return AWS_OP_SUCCESS;
}

int aws_event_stream_headers_list_init(struct aws_array_list *headers, struct aws_allocator *allocator) {
    AWS_ASSERT(headers);
    AWS_ASSERT(allocator);

    return aws_array_list_init_dynamic(headers, allocator, 4, sizeof(struct aws_event_stream_header_value_pair));
}

void aws_event_stream_headers_list_cleanup(struct aws_array_list *headers) {
    AWS_ASSERT(headers);

    for (size_t i = 0; i < aws_array_list_length(headers); ++i) {
        struct aws_event_stream_header_value_pair *header = NULL;
        aws_array_list_get_at_ptr(headers, (void **)&header, i);

        if (header->value_owned) {
            aws_mem_release(headers->alloc, (void *)header->header_value.variable_len_val);
        }
    }

    aws_array_list_clean_up(headers);
}

static int s_add_variable_len_header(
    struct aws_array_list *headers,
    struct aws_event_stream_header_value_pair *header,
    const char *name,
    uint8_t name_len,
    uint8_t *value,
    uint16_t value_len,
    int8_t copy) {

    memcpy((void *)header->header_name, (void *)name, (size_t)name_len);

    if (copy) {
        header->header_value.variable_len_val = aws_mem_acquire(headers->alloc, value_len);
        if (!header->header_value.variable_len_val) {
            return aws_raise_error(AWS_ERROR_OOM);
        }

        header->value_owned = 1;
        memcpy((void *)header->header_value.variable_len_val, (void *)value, value_len);
    } else {
        header->value_owned = 0;
        header->header_value.variable_len_val = value;
    }

    if (aws_array_list_push_back(headers, (void *)header)) {
        if (header->value_owned) {
            aws_mem_release(headers->alloc, (void *)header->header_value.variable_len_val);
        }
        return AWS_OP_ERR;
    }

    return AWS_OP_SUCCESS;
}

int aws_event_stream_add_string_header(
    struct aws_array_list *headers,
    const char *name,
    uint8_t name_len,
    const char *value,
    uint16_t value_len,
    int8_t copy) {
    struct aws_event_stream_header_value_pair header = {.header_name_len = name_len,
                                                        .header_value_len = value_len,
                                                        .value_owned = copy,
                                                        .header_value_type = AWS_EVENT_STREAM_HEADER_STRING};

    return s_add_variable_len_header(headers, &header, name, name_len, (uint8_t *)value, value_len, copy);
}

int aws_event_stream_add_byte_header(struct aws_array_list *headers, const char *name, uint8_t name_len, int8_t value) {
    struct aws_event_stream_header_value_pair header = {.header_name_len = name_len,
                                                        .header_value_len = 1,
                                                        .value_owned = 0,
                                                        .header_value_type = AWS_EVENT_STREAM_HEADER_BYTE,
                                                        .header_value.static_val[0] = (uint8_t)value};

    memcpy((void *)header.header_name, (void *)name, (size_t)name_len);

    return aws_array_list_push_back(headers, (void *)&header);
}

int aws_event_stream_add_bool_header(struct aws_array_list *headers, const char *name, uint8_t name_len, int8_t value) {
    struct aws_event_stream_header_value_pair header = {
        .header_name_len = name_len,
        .header_value_len = 0,
        .value_owned = 0,
        .header_value_type = value ? AWS_EVENT_STREAM_HEADER_BOOL_TRUE : AWS_EVENT_STREAM_HEADER_BOOL_FALSE,
    };

    memcpy((void *)header.header_name, (void *)name, (size_t)name_len);

    return aws_array_list_push_back(headers, (void *)&header);
}

int aws_event_stream_add_int16_header(
    struct aws_array_list *headers,
    const char *name,
    uint8_t name_len,
    int16_t value) {
    struct aws_event_stream_header_value_pair header = {
        .header_name_len = name_len,
        .header_value_len = sizeof(value),
        .value_owned = 0,
        .header_value_type = AWS_EVENT_STREAM_HEADER_INT16,
    };

    memcpy((void *)header.header_name, (void *)name, (size_t)name_len);
    aws_write_u16((uint16_t)value, header.header_value.static_val);

    return aws_array_list_push_back(headers, (void *)&header);
}

int aws_event_stream_add_int32_header(
    struct aws_array_list *headers,
    const char *name,
    uint8_t name_len,
    int32_t value) {
    struct aws_event_stream_header_value_pair header = {
        .header_name_len = name_len,
        .header_value_len = sizeof(value),
        .value_owned = 0,
        .header_value_type = AWS_EVENT_STREAM_HEADER_INT32,
    };

    memcpy((void *)header.header_name, (void *)name, (size_t)name_len);
    aws_write_u32((uint32_t)value, header.header_value.static_val);

    return aws_array_list_push_back(headers, (void *)&header);
}

int aws_event_stream_add_int64_header(
    struct aws_array_list *headers,
    const char *name,
    uint8_t name_len,
    int64_t value) {
    struct aws_event_stream_header_value_pair header = {
        .header_name_len = name_len,
        .header_value_len = sizeof(value),
        .value_owned = 0,
        .header_value_type = AWS_EVENT_STREAM_HEADER_INT64,
    };

    memcpy((void *)header.header_name, (void *)name, (size_t)name_len);
    aws_write_u64((uint64_t)value, header.header_value.static_val);

    return aws_array_list_push_back(headers, (void *)&header);
}

int aws_event_stream_add_bytebuf_header(
    struct aws_array_list *headers,
    const char *name,
    uint8_t name_len,
    uint8_t *value,
    uint16_t value_len,
    int8_t copy) {
    struct aws_event_stream_header_value_pair header = {.header_name_len = name_len,
                                                        .header_value_len = value_len,
                                                        .value_owned = copy,
                                                        .header_value_type = AWS_EVENT_STREAM_HEADER_BYTE_BUF};

    return s_add_variable_len_header(headers, &header, name, name_len, value, value_len, copy);
}

int aws_event_stream_add_timestamp_header(
    struct aws_array_list *headers,
    const char *name,
    uint8_t name_len,
    int64_t value) {
    struct aws_event_stream_header_value_pair header = {
        .header_name_len = name_len,
        .header_value_len = sizeof(uint64_t),
        .value_owned = 0,
        .header_value_type = AWS_EVENT_STREAM_HEADER_TIMESTAMP,
    };

    memcpy((void *)header.header_name, (void *)name, (size_t)name_len);
    aws_write_u64((uint64_t)value, header.header_value.static_val);

    return aws_array_list_push_back(headers, (void *)&header);
}

int aws_event_stream_add_uuid_header(
    struct aws_array_list *headers,
    const char *name,
    uint8_t name_len,
    const uint8_t *value) {
    struct aws_event_stream_header_value_pair header = {
        .header_name_len = name_len,
        .header_value_len = 16,
        .value_owned = 0,
        .header_value_type = AWS_EVENT_STREAM_HEADER_UUID,
    };

    memcpy((void *)header.header_name, (void *)name, (size_t)name_len);
    memcpy((void *)header.header_value.static_val, value, 16);

    return aws_array_list_push_back(headers, (void *)&header);
}

struct aws_byte_buf aws_event_stream_header_name(struct aws_event_stream_header_value_pair *header) {
    return aws_byte_buf_from_array((uint8_t *)header->header_name, header->header_name_len);
}

int8_t aws_event_stream_header_value_as_byte(struct aws_event_stream_header_value_pair *header) {
    return (int8_t)header->header_value.static_val[0];
}

struct aws_byte_buf aws_event_stream_header_value_as_string(struct aws_event_stream_header_value_pair *header) {
    return aws_event_stream_header_value_as_bytebuf(header);
}

int8_t aws_event_stream_header_value_as_bool(struct aws_event_stream_header_value_pair *header) {
    return header->header_value_type == AWS_EVENT_STREAM_HEADER_BOOL_TRUE ? (int8_t)1 : (int8_t)0;
}

int16_t aws_event_stream_header_value_as_int16(struct aws_event_stream_header_value_pair *header) {
    return (int16_t)aws_read_u16(header->header_value.static_val);
}

int32_t aws_event_stream_header_value_as_int32(struct aws_event_stream_header_value_pair *header) {
    return (int32_t)aws_read_u32(header->header_value.static_val);
}

int64_t aws_event_stream_header_value_as_int64(struct aws_event_stream_header_value_pair *header) {
    return (int64_t)aws_read_u64(header->header_value.static_val);
}

struct aws_byte_buf aws_event_stream_header_value_as_bytebuf(struct aws_event_stream_header_value_pair *header) {
    return aws_byte_buf_from_array(header->header_value.variable_len_val, header->header_value_len);
}

int64_t aws_event_stream_header_value_as_timestamp(struct aws_event_stream_header_value_pair *header) {
    return aws_event_stream_header_value_as_int64(header);
}

struct aws_byte_buf aws_event_stream_header_value_as_uuid(struct aws_event_stream_header_value_pair *header) {
    return aws_byte_buf_from_array(header->header_value.static_val, 16);
}

uint16_t aws_event_stream_header_value_length(struct aws_event_stream_header_value_pair *header) {
    return header->header_value_len;
}

static struct aws_event_stream_message_prelude s_empty_prelude = {.total_len = 0, .headers_len = 0, .prelude_crc = 0};

static void s_reset_header_state(struct aws_event_stream_streaming_decoder *decoder, uint8_t free_header_data) {

    if (free_header_data && decoder->current_header.value_owned) {
        aws_mem_release(decoder->alloc, (void *)decoder->current_header.header_value.variable_len_val);
    }

    memset((void *)&decoder->current_header, 0, sizeof(struct aws_event_stream_header_value_pair));
}

static void s_reset_state(struct aws_event_stream_streaming_decoder *decoder);

static int s_headers_state(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed);

static int s_read_header_value(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed) {

    size_t current_pos = decoder->message_pos;

    size_t length_read = current_pos - decoder->current_header_value_offset;
    struct aws_event_stream_header_value_pair *current_header = &decoder->current_header;

    if (!length_read) {
        /* save an allocation, this can only happen if the data we were handed is larger than the length of the header
         * value. we don't really need to handle offsets in this case. This expects the user is living by the contract
         * that they cannot act like they own this memory beyond the lifetime of their callback, and they should not
         * mutate it */
        if (len >= current_header->header_value_len) {
            /* this part works regardless of type since the layout of the union will line up. */
            current_header->header_value.variable_len_val = (uint8_t *)data;
            current_header->value_owned = 0;
            decoder->on_header(decoder, &decoder->prelude, &decoder->current_header, decoder->user_context);
            *processed += current_header->header_value_len;
            decoder->message_pos += current_header->header_value_len;
            decoder->running_crc =
                aws_checksums_crc32(data, (int)current_header->header_value_len, decoder->running_crc);

            s_reset_header_state(decoder, 1);
            decoder->state = s_headers_state;
            return AWS_OP_SUCCESS;
        }

        /* a possible optimization later would be to only allocate this once, and then keep reusing the same buffer. for
         * subsequent messages.*/
        if (current_header->header_value_type == AWS_EVENT_STREAM_HEADER_BYTE_BUF ||
            current_header->header_value_type == AWS_EVENT_STREAM_HEADER_STRING) {
            current_header->header_value.variable_len_val =
                aws_mem_acquire(decoder->alloc, decoder->current_header.header_value_len);

            if (!current_header->header_value.variable_len_val) {
                return aws_raise_error(AWS_ERROR_OOM);
            }

            current_header->value_owned = 1;
        }
    }

    size_t max_read =
        len >= current_header->header_value_len - length_read ? current_header->header_value_len - length_read : len;

    const uint8_t *header_value_alias = current_header->header_value_type == AWS_EVENT_STREAM_HEADER_BYTE_BUF ||
                                                current_header->header_value_type == AWS_EVENT_STREAM_HEADER_STRING
                                            ? current_header->header_value.variable_len_val
                                            : current_header->header_value.static_val;

    memcpy((void *)(header_value_alias + length_read), data, max_read);
    decoder->running_crc = aws_checksums_crc32(data, (int)max_read, decoder->running_crc);

    *processed += max_read;
    decoder->message_pos += max_read;
    length_read += max_read;

    if (length_read == current_header->header_value_len) {
        decoder->on_header(decoder, &decoder->prelude, current_header, decoder->user_context);
        s_reset_header_state(decoder, 1);
        decoder->state = s_headers_state;
    }

    return AWS_OP_SUCCESS;
}

static int s_read_header_value_len(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed) {
    size_t current_pos = decoder->message_pos;

    size_t length_portion_read = current_pos - decoder->current_header_value_offset;

    if (length_portion_read < sizeof(uint16_t)) {
        size_t max_to_read =
            len > sizeof(uint16_t) - length_portion_read ? sizeof(uint16_t) - length_portion_read : len;
        memcpy(decoder->working_buffer + length_portion_read, data, max_to_read);
        decoder->running_crc = aws_checksums_crc32(data, (int)max_to_read, decoder->running_crc);

        *processed += max_to_read;
        decoder->message_pos += max_to_read;

        length_portion_read = decoder->message_pos - decoder->current_header_value_offset;
    }

    if (length_portion_read == sizeof(uint16_t)) {
        decoder->current_header.header_value_len = aws_read_u16(decoder->working_buffer);
        decoder->current_header_value_offset = decoder->message_pos;
        decoder->state = s_read_header_value;
    }

    return AWS_OP_SUCCESS;
}

static int s_read_header_type(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed) {
    (void)len;
    uint8_t type = *data;
    decoder->running_crc = aws_checksums_crc32(data, 1, decoder->running_crc);
    *processed += 1;
    decoder->message_pos++;
    decoder->current_header_value_offset++;
    struct aws_event_stream_header_value_pair *current_header = &decoder->current_header;

    if (type >= AWS_EVENT_STREAM_HEADER_BOOL_FALSE && type <= AWS_EVENT_STREAM_HEADER_UUID) {
        current_header->header_value_type = (enum aws_event_stream_header_value_type)type;

        switch (type) {
            case AWS_EVENT_STREAM_HEADER_STRING:
            case AWS_EVENT_STREAM_HEADER_BYTE_BUF:
                decoder->state = s_read_header_value_len;
                break;
            case AWS_EVENT_STREAM_HEADER_BOOL_FALSE:
                current_header->header_value_len = 0;
                current_header->header_value.static_val[0] = 0;
                decoder->on_header(decoder, &decoder->prelude, current_header, decoder->user_context);
                s_reset_header_state(decoder, 1);
                break;
            case AWS_EVENT_STREAM_HEADER_BOOL_TRUE:
                current_header->header_value_len = 0;
                current_header->header_value.static_val[0] = 1;
                decoder->on_header(decoder, &decoder->prelude, current_header, decoder->user_context);
                s_reset_header_state(decoder, 1);
                break;
            case AWS_EVENT_STREAM_HEADER_BYTE:
                current_header->header_value_len = 1;
                decoder->state = s_read_header_value;
                break;
            case AWS_EVENT_STREAM_HEADER_INT16:
                current_header->header_value_len = sizeof(uint16_t);
                decoder->state = s_read_header_value;
                break;
            case AWS_EVENT_STREAM_HEADER_INT32:
                current_header->header_value_len = sizeof(uint32_t);
                decoder->state = s_read_header_value;
                break;
            case AWS_EVENT_STREAM_HEADER_INT64:
            case AWS_EVENT_STREAM_HEADER_TIMESTAMP:
                current_header->header_value_len = sizeof(uint64_t);
                decoder->state = s_read_header_value;
                break;
            case AWS_EVENT_STREAM_HEADER_UUID:
                current_header->header_value_len = 16;
                decoder->state = s_read_header_value;
                break;
            default:
                return aws_raise_error(AWS_ERROR_EVENT_STREAM_MESSAGE_UNKNOWN_HEADER_TYPE);
        }

        return AWS_OP_SUCCESS;
    }

    return aws_raise_error(AWS_ERROR_EVENT_STREAM_MESSAGE_UNKNOWN_HEADER_TYPE);
}

static int s_read_header_name(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed) {
    size_t current_pos = decoder->message_pos;

    size_t length_read = current_pos - decoder->current_header_name_offset;

    size_t max_read = len >= decoder->current_header.header_name_len - length_read
                          ? decoder->current_header.header_name_len - length_read
                          : len;
    memcpy((void *)(decoder->current_header.header_name + length_read), data, max_read);
    decoder->running_crc = aws_checksums_crc32(data, (int)max_read, decoder->running_crc);

    *processed += max_read;
    decoder->message_pos += max_read;
    length_read += max_read;

    if (length_read == decoder->current_header.header_name_len) {
        decoder->state = s_read_header_type;
        decoder->current_header_value_offset = decoder->message_pos;
    }

    return AWS_OP_SUCCESS;
}

static int s_read_header_name_len(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed) {
    (void)len;
    decoder->current_header.header_name_len = *data;
    decoder->message_pos++;
    decoder->current_header_name_offset++;
    *processed += 1;
    decoder->state = s_read_header_name;
    decoder->running_crc = aws_checksums_crc32(data, 1, decoder->running_crc);

    return AWS_OP_SUCCESS;
}

static int s_start_header(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed) /* NOLINT */ {
    (void)data;
    (void)len;
    (void)processed;
    decoder->state = s_read_header_name_len;
    decoder->current_header_name_offset = decoder->message_pos;

    return AWS_OP_SUCCESS;
}

static int s_payload_state(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed);

/*Handles the initial state for header parsing.
  will oscillate between multiple other states as well.
  after all headers have been handled, payload will be set as the next state. */
static int s_headers_state(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed) /* NOLINT */ {
    (void)data;
    (void)len;
    (void)processed;

    size_t current_pos = decoder->message_pos;

    size_t headers_boundary = decoder->prelude.headers_len + AWS_EVENT_STREAM_PRELUDE_LENGTH;

    if (current_pos < headers_boundary) {
        decoder->state = s_start_header;
        return AWS_OP_SUCCESS;
    }

    if (current_pos == headers_boundary) {
        decoder->state = s_payload_state;
        return AWS_OP_SUCCESS;
    }

    return aws_raise_error(AWS_ERROR_EVENT_STREAM_MESSAGE_PARSER_ILLEGAL_STATE);
}

/* handles reading the trailer. Once it has been read, it will be compared to the running checksum. If successful,
 * the state will be reset. */
static int s_read_trailer_state(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed) {

    size_t remaining_amount = decoder->prelude.total_len - decoder->message_pos;
    size_t segment_length = len > remaining_amount ? remaining_amount : len;
    size_t offset = sizeof(uint32_t) - remaining_amount;
    memcpy(decoder->working_buffer + offset, data, segment_length);
    decoder->message_pos += segment_length;
    *processed += segment_length;

    if (decoder->message_pos == decoder->prelude.total_len) {
        uint32_t message_crc = aws_read_u32(decoder->working_buffer);

        if (message_crc == decoder->running_crc) {
            s_reset_state(decoder);
        } else {
            char error_message[70];
            snprintf(
                error_message,
                sizeof(error_message),
                "CRC Mismatch. message_crc was 0x08%" PRIX32 ", but computed 0x08%" PRIX32,
                message_crc,
                decoder->running_crc);
            aws_raise_error(AWS_ERROR_EVENT_STREAM_MESSAGE_CHECKSUM_FAILURE);
            decoder->on_error(
                decoder,
                &decoder->prelude,
                AWS_ERROR_EVENT_STREAM_MESSAGE_CHECKSUM_FAILURE,
                error_message,
                decoder->user_context);
            return AWS_OP_ERR;
        }
    }

    return AWS_OP_SUCCESS;
}

/* handles the reading of the payload up to the final checksum. Sets read_trailer_state as the new state once
 * the payload has been processed. */
static int s_payload_state(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed) {

    if (decoder->message_pos < decoder->prelude.total_len - AWS_EVENT_STREAM_TRAILER_LENGTH) {
        size_t remaining_amount = decoder->prelude.total_len - decoder->message_pos - AWS_EVENT_STREAM_TRAILER_LENGTH;
        size_t segment_length = len > remaining_amount ? remaining_amount : len;
        int8_t final_segment =
            (segment_length + decoder->message_pos) == (decoder->prelude.total_len - AWS_EVENT_STREAM_TRAILER_LENGTH);
        struct aws_byte_buf payload_buf = aws_byte_buf_from_array(data, segment_length);
        decoder->on_payload(decoder, &payload_buf, final_segment, decoder->user_context);
        decoder->message_pos += segment_length;
        decoder->running_crc = aws_checksums_crc32(data, (int)segment_length, decoder->running_crc);
        *processed += segment_length;
    }

    if (decoder->message_pos == decoder->prelude.total_len - AWS_EVENT_STREAM_TRAILER_LENGTH) {
        decoder->state = s_read_trailer_state;
    }

    return AWS_OP_SUCCESS;
}

/* Parses the payload and verifies checksums. Sets the next state if successful. */
static int s_verify_prelude_state(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed) /* NOLINT */ {
    (void)data;
    (void)len;
    (void)processed;

    decoder->prelude.headers_len = aws_read_u32(decoder->working_buffer + HEADER_LEN_OFFSET);
    decoder->prelude.prelude_crc = aws_read_u32(decoder->working_buffer + PRELUDE_CRC_OFFSET);
    decoder->prelude.total_len = aws_read_u32(decoder->working_buffer + TOTAL_LEN_OFFSET);

    decoder->running_crc = aws_checksums_crc32(decoder->working_buffer, PRELUDE_CRC_OFFSET, 0);

    if (AWS_LIKELY(decoder->running_crc == decoder->prelude.prelude_crc)) {

        if (AWS_UNLIKELY(
                decoder->prelude.headers_len > MAX_HEADERS_SIZE || decoder->prelude.total_len > MAX_MESSAGE_SIZE)) {
            aws_raise_error(AWS_ERROR_EVENT_STREAM_MESSAGE_FIELD_SIZE_EXCEEDED);
            char error_message[] = "Maximum message field size exceeded";

            decoder->on_error(
                decoder,
                &decoder->prelude,
                AWS_ERROR_EVENT_STREAM_MESSAGE_FIELD_SIZE_EXCEEDED,
                error_message,
                decoder->user_context);
            return AWS_OP_ERR;
        }

        /* Should only call on_prelude() after passing crc check and limitation check, otherwise call on_prelude() with
         * incorrect prelude is error prune. */
        decoder->on_prelude(decoder, &decoder->prelude, decoder->user_context);

        decoder->running_crc = aws_checksums_crc32(
            decoder->working_buffer + PRELUDE_CRC_OFFSET,
            (int)sizeof(decoder->prelude.prelude_crc),
            decoder->running_crc);
        memset(decoder->working_buffer, 0, sizeof(decoder->working_buffer));
        decoder->state = decoder->prelude.headers_len > 0 ? s_headers_state : s_payload_state;
    } else {
        char error_message[70];
        snprintf(
            error_message,
            sizeof(error_message),
            "CRC Mismatch. prelude_crc was 0x08%" PRIX32 ", but computed 0x08%" PRIX32,
            decoder->prelude.prelude_crc,
            decoder->running_crc);

        aws_raise_error(AWS_ERROR_EVENT_STREAM_PRELUDE_CHECKSUM_FAILURE);
        decoder->on_error(
            decoder,
            &decoder->prelude,
            AWS_ERROR_EVENT_STREAM_PRELUDE_CHECKSUM_FAILURE,
            error_message,
            decoder->user_context);
        return AWS_OP_ERR;
    }

    return AWS_OP_SUCCESS;
}

/* initial state handles up to the reading of the prelude */
static int s_start_state(
    struct aws_event_stream_streaming_decoder *decoder,
    const uint8_t *data,
    size_t len,
    size_t *processed) {

    size_t previous_position = decoder->message_pos;
    if (decoder->message_pos < AWS_EVENT_STREAM_PRELUDE_LENGTH) {
        if (len >= AWS_EVENT_STREAM_PRELUDE_LENGTH - decoder->message_pos) {
            memcpy(
                decoder->working_buffer + decoder->message_pos,
                data,
                AWS_EVENT_STREAM_PRELUDE_LENGTH - decoder->message_pos);
            decoder->message_pos += AWS_EVENT_STREAM_PRELUDE_LENGTH - decoder->message_pos;
        } else {
            memcpy(decoder->working_buffer + decoder->message_pos, data, len);
            decoder->message_pos += len;
        }

        *processed += decoder->message_pos - previous_position;
    }

    if (decoder->message_pos == AWS_EVENT_STREAM_PRELUDE_LENGTH) {
        decoder->state = s_verify_prelude_state;
    }

    return AWS_OP_SUCCESS;
}

static void s_reset_state(struct aws_event_stream_streaming_decoder *decoder) {
    decoder->message_pos = 0;
    decoder->prelude = s_empty_prelude;
    decoder->running_crc = 0;
    memset(decoder->working_buffer, 0, sizeof(decoder->working_buffer));
    decoder->state = s_start_state;
}

void aws_event_stream_streaming_decoder_init(
    struct aws_event_stream_streaming_decoder *decoder,
    struct aws_allocator *alloc,
    aws_event_stream_process_on_payload_segment_fn *on_payload_segment,
    aws_event_stream_prelude_received_fn *on_prelude,
    aws_event_stream_header_received_fn *on_header,
    aws_event_stream_on_error_fn *on_error,
    void *user_data) {

    s_reset_state(decoder);
    decoder->alloc = alloc;
    decoder->on_error = on_error;
    decoder->on_header = on_header;
    decoder->on_payload = on_payload_segment;
    decoder->on_prelude = on_prelude;
    decoder->user_context = user_data;
}

void aws_event_stream_streaming_decoder_clean_up(struct aws_event_stream_streaming_decoder *decoder) {
    s_reset_state(decoder);
    decoder->on_error = 0;
    decoder->on_header = 0;
    decoder->on_payload = 0;
    decoder->on_prelude = 0;
    decoder->user_context = 0;
}

/* Simply sends the data to the state machine until all has been processed or an error is returned. */
int aws_event_stream_streaming_decoder_pump(
    struct aws_event_stream_streaming_decoder *decoder,
    const struct aws_byte_buf *data) {

    size_t processed = 0;
    int err_val = 0;
    while (!err_val && data->buffer && data->len && processed < data->len) {
        err_val = decoder->state(decoder, data->buffer + processed, data->len - processed, &processed);
    }

    return err_val;
}
#if _MSC_VER
#    pragma warning(pop)
#endif
