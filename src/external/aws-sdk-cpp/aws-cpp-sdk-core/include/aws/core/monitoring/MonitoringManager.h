
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
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/core/client/AWSClient.h>
#include <aws/core/monitoring/CoreMetrics.h>

namespace Aws
{
    namespace Monitoring
    {
        class MonitoringFactory;
        /**
         * Wrapper function of OnRequestStarted defined by all monitoring instances
         */
        Aws::Vector<void*> OnRequestStarted(const Aws::String& serviceName, const Aws::String& requestName,
            const std::shared_ptr<const Aws::Http::HttpRequest>& request);

        /**
         * Wrapper function of OnRequestSucceeded defined by all monitoring instances
         */
        void OnRequestSucceeded(const Aws::String& serviceName, const Aws::String& requestName, const std::shared_ptr<const Aws::Http::HttpRequest>& request,
            const Aws::Client::HttpResponseOutcome& outcome, const CoreMetricsCollection& metricsFromCore, const Aws::Vector<void*>& contexts);

        /**
         * Wrapper function of OnRequestFailed defined by all monitoring instances
         */
        void OnRequestFailed(const Aws::String& serviceName, const Aws::String& requestName, const std::shared_ptr<const Aws::Http::HttpRequest>& request,
            const Aws::Client::HttpResponseOutcome& outcome, const CoreMetricsCollection& metricsFromCore, const Aws::Vector<void*>& contexts);

        /**
         * Wrapper function of OnRequestRetry defined by all monitoring instances
         */
        void OnRequestRetry(const Aws::String& serviceName, const Aws::String& requestName, 
            const std::shared_ptr<const Aws::Http::HttpRequest>& request, const Aws::Vector<void*>& contexts);

        /**
         * Wrapper function of OnFinish defined by all monitoring instances
         */
        void OnFinish(const Aws::String& serviceName, const Aws::String& requestName, 
            const std::shared_ptr<const Aws::Http::HttpRequest>& request, const Aws::Vector<void*>& contexts);

        typedef std::function<Aws::UniquePtr<MonitoringFactory>()> MonitoringFactoryCreateFunction;

        /**
         * Init monitoring using supplied factories, monitoring can support multipe instances.
         * We will try to (based on config resolution result) create a default client side monitoring listener instance defined in AWS SDK Core module.
         * and create other instances from these factories.
         * This function will be called during Aws::InitAPI call, argument is acquired from Aws::SDKOptions->MonitoringOptions
         */
        void AWS_CORE_API InitMonitoring(const std::vector<MonitoringFactoryCreateFunction>& monitoringFactoryCreateFunctions);

        /**
         * Clean up monitoring related global variables
         */
        void AWS_CORE_API CleanupMonitoring();
    }
}
