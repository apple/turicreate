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

#pragma once
#include <aws/core/Core_EXPORTS.h>
#include <aws/core/monitoring/HttpClientMetrics.h>

namespace Aws
{
    namespace Monitoring
    {
        /**
         * Metrics collected from AWS SDK Core include Http Client Metrics and other types of metrics.
         */
        struct AWS_CORE_API CoreMetricsCollection
        {
            /**
             * Metrics collected from underlying http client during execution of a request
             */
            HttpClientMetricsCollection httpClientMetrics;

            // Add Other types of metrics here.
        };
    }
}
