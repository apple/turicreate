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

#include <aws/common/statistics.h>

void aws_crt_statistics_handler_process_statistics(
    struct aws_crt_statistics_handler *handler,
    struct aws_crt_statistics_sample_interval *interval,
    struct aws_array_list *stats,
    void *context) {
    handler->vtable->process_statistics(handler, interval, stats, context);
}

uint64_t aws_crt_statistics_handler_get_report_interval_ms(struct aws_crt_statistics_handler *handler) {
    return handler->vtable->get_report_interval_ms(handler);
}

void aws_crt_statistics_handler_destroy(struct aws_crt_statistics_handler *handler) {
    if (handler == NULL) {
        return;
    }

    handler->vtable->destroy(handler);
}
