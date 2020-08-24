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

#include <aws/core/utils/HashingUtils.h>
#include <aws/core/monitoring/HttpClientMetrics.h>

namespace Aws
{
    namespace Monitoring
    {
        static const char HTTP_CLIENT_METRICS_DESTINATION_IP[] = "DestinationIp";
        static const char HTTP_CLIENT_METRICS_ACQUIRE_CONNECTION_LATENCY[] = "AcquireConnectionLatency";
        static const char HTTP_CLIENT_METRICS_CONNECTION_REUSED[] = "ConnectionReused";
        static const char HTTP_CLIENT_METRICS_CONNECTION_LATENCY[] = "ConnectLatency";
        static const char HTTP_CLIENT_METRICS_REQUEST_LATENCY[] = "RequestLatency";
        static const char HTTP_CLIENT_METRICS_DNS_LATENCY[] = "DnsLatency";
        static const char HTTP_CLIENT_METRICS_TCP_LATENCY[] = "TcpLatency";
        static const char HTTP_CLIENT_METRICS_SSL_LATENCY[] = "SslLatency";
        static const char HTTP_CLIENT_METRICS_UNKNOWN[] = "Unknown";

        using namespace Aws::Utils;
        HttpClientMetricsType GetHttpClientMetricTypeByName(const Aws::String& name)
        {
            static std::map<int, HttpClientMetricsType> metricsNameHashToType =
            {
                std::pair<int, HttpClientMetricsType>(HashingUtils::HashString(HTTP_CLIENT_METRICS_DESTINATION_IP), HttpClientMetricsType::DestinationIp),
                std::pair<int, HttpClientMetricsType>(HashingUtils::HashString(HTTP_CLIENT_METRICS_ACQUIRE_CONNECTION_LATENCY), HttpClientMetricsType::AcquireConnectionLatency),
                std::pair<int, HttpClientMetricsType>(HashingUtils::HashString(HTTP_CLIENT_METRICS_CONNECTION_REUSED), HttpClientMetricsType::ConnectionReused),
                std::pair<int, HttpClientMetricsType>(HashingUtils::HashString(HTTP_CLIENT_METRICS_CONNECTION_LATENCY), HttpClientMetricsType::ConnectLatency),
                std::pair<int, HttpClientMetricsType>(HashingUtils::HashString(HTTP_CLIENT_METRICS_REQUEST_LATENCY), HttpClientMetricsType::RequestLatency),
                std::pair<int, HttpClientMetricsType>(HashingUtils::HashString(HTTP_CLIENT_METRICS_DNS_LATENCY), HttpClientMetricsType::DnsLatency),
                std::pair<int, HttpClientMetricsType>(HashingUtils::HashString(HTTP_CLIENT_METRICS_TCP_LATENCY), HttpClientMetricsType::TcpLatency),
                std::pair<int, HttpClientMetricsType>(HashingUtils::HashString(HTTP_CLIENT_METRICS_SSL_LATENCY), HttpClientMetricsType::SslLatency)
            };

            int nameHash = HashingUtils::HashString(name.c_str());
            auto it = metricsNameHashToType.find(nameHash);
            if (it == metricsNameHashToType.end())
            {
                return HttpClientMetricsType::Unknown;
            }
            return it->second;
        }

        Aws::String GetHttpClientMetricNameByType(HttpClientMetricsType type)
        {
            static std::map<int, std::string> metricsTypeToName =
            {
                std::pair<int, std::string>(static_cast<int>(HttpClientMetricsType::DestinationIp), HTTP_CLIENT_METRICS_DESTINATION_IP),
                std::pair<int, std::string>(static_cast<int>(HttpClientMetricsType::AcquireConnectionLatency), HTTP_CLIENT_METRICS_ACQUIRE_CONNECTION_LATENCY),
                std::pair<int, std::string>(static_cast<int>(HttpClientMetricsType::ConnectionReused), HTTP_CLIENT_METRICS_CONNECTION_REUSED),
                std::pair<int, std::string>(static_cast<int>(HttpClientMetricsType::ConnectLatency), HTTP_CLIENT_METRICS_CONNECTION_LATENCY),
                std::pair<int, std::string>(static_cast<int>(HttpClientMetricsType::RequestLatency), HTTP_CLIENT_METRICS_REQUEST_LATENCY),
                std::pair<int, std::string>(static_cast<int>(HttpClientMetricsType::DnsLatency), HTTP_CLIENT_METRICS_DNS_LATENCY),
                std::pair<int, std::string>(static_cast<int>(HttpClientMetricsType::TcpLatency), HTTP_CLIENT_METRICS_TCP_LATENCY),
                std::pair<int, std::string>(static_cast<int>(HttpClientMetricsType::SslLatency), HTTP_CLIENT_METRICS_SSL_LATENCY),
                std::pair<int, std::string>(static_cast<int>(HttpClientMetricsType::Unknown), HTTP_CLIENT_METRICS_UNKNOWN)
            };

            auto it = metricsTypeToName.find(static_cast<int>(type));
            if (it == metricsTypeToName.end())
            {
                return HTTP_CLIENT_METRICS_UNKNOWN;
            }
            return Aws::String(it->second.c_str());
        }

    }
}
