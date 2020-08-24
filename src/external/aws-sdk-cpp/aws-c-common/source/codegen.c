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

/*
 * This file generates exportable implementations for inlineable functions.
 */

#define AWS_STATIC_IMPL AWS_COMMON_API

#include <aws/common/array_list.inl>
#include <aws/common/atomics.inl>
#include <aws/common/byte_order.inl>
#include <aws/common/clock.inl>
#include <aws/common/encoding.inl>
#include <aws/common/error.inl>
#include <aws/common/linked_list.inl>
#include <aws/common/math.inl>
#include <aws/common/ring_buffer.inl>
#include <aws/common/string.inl>
#include <aws/common/zero.inl>
