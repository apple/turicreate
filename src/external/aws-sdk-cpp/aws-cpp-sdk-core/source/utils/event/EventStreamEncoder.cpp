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

#include <aws/core/utils/event/EventHeader.h>
#include <aws/core/utils/event/EventMessage.h>
#include <aws/core/utils/event/EventStreamEncoder.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/auth/AWSAuthSigner.h>
#include <aws/common/byte_order.h>
#include <aws/core/utils/memory/AWSMemory.h>

#include <cassert>

namespace Aws
{
    namespace Utils
    {
        namespace Event
        {
            static const char TAG[] = "EventStreamEncoder";

            static void EncodeHeaders(const Aws::Utils::Event::Message& msg, aws_array_list* headers)
            {
                aws_array_list_init_dynamic(headers, get_aws_allocator(), msg.GetEventHeaders().size(), sizeof(aws_event_stream_header_value_pair));
                for (auto&& header : msg.GetEventHeaders())
                {
                    const uint8_t headerKeyLen = static_cast<uint8_t>(header.first.length());
                    switch(header.second.GetType())
                    {
                        case EventHeaderValue::EventHeaderType::BOOL_TRUE:
                        case EventHeaderValue::EventHeaderType::BOOL_FALSE:
                            aws_event_stream_add_bool_header(headers, header.first.c_str(), headerKeyLen, header.second.GetEventHeaderValueAsBoolean());
                            break;
                        case EventHeaderValue::EventHeaderType::BYTE:
                            aws_event_stream_add_bool_header(headers, header.first.c_str(), headerKeyLen, header.second.GetEventHeaderValueAsByte());
                            break;
                        case EventHeaderValue::EventHeaderType::INT16:
                            aws_event_stream_add_int16_header(headers, header.first.c_str(), headerKeyLen, header.second.GetEventHeaderValueAsInt16());
                            break;
                        case EventHeaderValue::EventHeaderType::INT32:
                            aws_event_stream_add_int32_header(headers, header.first.c_str(), headerKeyLen, header.second.GetEventHeaderValueAsInt32());
                            break;
                        case EventHeaderValue::EventHeaderType::INT64:
                            aws_event_stream_add_int64_header(headers, header.first.c_str(), headerKeyLen, header.second.GetEventHeaderValueAsInt64());
                            break;
                        case EventHeaderValue::EventHeaderType::BYTE_BUF:
                            {
                                const auto& bytes = header.second.GetEventHeaderValueAsBytebuf();
                                aws_event_stream_add_bytebuf_header(headers, header.first.c_str(), headerKeyLen, bytes.GetUnderlyingData(), static_cast<uint16_t>(bytes.GetLength()), 1 /*copy*/);
                            }
                            break;
                        case EventHeaderValue::EventHeaderType::STRING:
                            {
                                const auto& bytes = header.second.GetUnderlyingBuffer();
                                aws_event_stream_add_string_header(headers, header.first.c_str(), headerKeyLen, reinterpret_cast<char*>(bytes.GetUnderlyingData()), static_cast<uint16_t>(bytes.GetLength()), 0 /*copy*/);
                            }
                            break;
                        case EventHeaderValue::EventHeaderType::TIMESTAMP:
                            aws_event_stream_add_timestamp_header(headers, header.first.c_str(), headerKeyLen, header.second.GetEventHeaderValueAsTimestamp());
                            break;
                        case EventHeaderValue::EventHeaderType::UUID:
                            {
                                ByteBuffer uuidBytes = header.second.GetEventHeaderValueAsUuid();
                                aws_event_stream_add_uuid_header(headers, header.first.c_str(), headerKeyLen, uuidBytes.GetUnderlyingData());
                            }
                            break;
                        default:
                            AWS_LOG_ERROR(TAG, "Encountered unknown type of header.");
                            break;
                    }
                }
            }

            EventStreamEncoder::EventStreamEncoder(Client::AWSAuthSigner* signer) : m_signer(signer)
            {
            }


            Aws::Vector<unsigned char> EventStreamEncoder::EncodeAndSign(const Aws::Utils::Event::Message& msg)
            {
                aws_event_stream_message encoded = Encode(msg);
                aws_event_stream_message signedMessage = Sign(&encoded);

                const auto signedMessageLength = signedMessage.message_buffer ? aws_event_stream_message_total_length(&signedMessage) : 0;

                Aws::Vector<unsigned char> outputBits(signedMessage.message_buffer, signedMessage.message_buffer + signedMessageLength);
                aws_event_stream_message_clean_up(&encoded);
                aws_event_stream_message_clean_up(&signedMessage);
                return outputBits;
            }

            aws_event_stream_message EventStreamEncoder::Encode(const Aws::Utils::Event::Message& msg)
            {
                aws_array_list headers;
                EncodeHeaders(msg, &headers);

                aws_byte_buf payload;
                payload.len = msg.GetEventPayload().size();
                // this const_cast is OK because aws_byte_buf will only be "read from" by the following functions.
                payload.buffer = const_cast<uint8_t*>(msg.GetEventPayload().data());
                payload.capacity = 0;
                payload.allocator = nullptr;

                aws_event_stream_message encoded;
                if(aws_event_stream_message_init(&encoded, get_aws_allocator(), &headers, &payload) == AWS_OP_ERR)
                {
                    AWS_LOGSTREAM_ERROR(TAG, "Error creating event-stream message from paylaod.");
                    aws_event_stream_headers_list_cleanup(&headers);
                    // GCC 4.9.4 issues a warning with -Wextra if we simply do
                    // return {};
                    aws_event_stream_message empty{nullptr, nullptr, 0};
                    return empty;
                }
                aws_event_stream_headers_list_cleanup(&headers);
                return encoded;
            }

            aws_event_stream_message EventStreamEncoder::Sign(aws_event_stream_message* msg)
            {
                const auto msglen = msg->message_buffer ? aws_event_stream_message_total_length(msg) : 0;
                Event::Message signedMessage;
                signedMessage.WriteEventPayload(msg->message_buffer, msglen);

                assert(m_signer);
                if (!m_signer->SignEventMessage(signedMessage, m_signatureSeed))
                {
                    AWS_LOGSTREAM_ERROR(TAG, "Failed to sign event message frame.");
                    // GCC 4.9.4 issues a warning with -Wextra if we simply do
                    // return {};
                    aws_event_stream_message empty{nullptr, nullptr, 0};
                    return empty;
                }

                aws_array_list headers;
                EncodeHeaders(signedMessage, &headers);

                aws_byte_buf payload;
                payload.len = signedMessage.GetEventPayload().size();
                payload.buffer = signedMessage.GetEventPayload().data();
                payload.capacity = 0;
                payload.allocator = nullptr;

                aws_event_stream_message signedmsg;
                if(aws_event_stream_message_init(&signedmsg, get_aws_allocator(), &headers, &payload))
                {
                    AWS_LOGSTREAM_ERROR(TAG, "Error creating event-stream message from paylaod.");
                    aws_event_stream_headers_list_cleanup(&headers);
                    // GCC 4.9.4 issues a warning with -Wextra if we simply do
                    // return {};
                    aws_event_stream_message empty{nullptr, nullptr, 0};
                    return empty;
                }
                aws_event_stream_headers_list_cleanup(&headers);
                return signedmsg;
            }

        } // namespace Event
    } // namespace Utils
} // namespace Aws

