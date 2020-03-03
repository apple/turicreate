/*
  * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <aws/core/http/Scheme.h>
#include <aws/core/Region.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/http/HttpTypes.h>
#include <memory>

namespace Aws
{
    namespace Utils
    {
        namespace Threading
        {
            class Executor;
        } // namespace Threading

        namespace RateLimits
        {
            class RateLimiterInterface;
        } // namespace RateLimits
    } // namespace Utils
    namespace Client
    {
        class RetryStrategy; // forward declare

        /**
          * This mutable structure is used to configure any of the AWS clients.
          * Default values can only be overwritten prior to passing to the client constructors.
          */
        struct AWS_CORE_API ClientConfiguration
        {
            ClientConfiguration();

            /**
             * Create a configuration based on settings in the aws configuration file for the given profile name.
             * The configuration file location can be set via the environment variable AWS_CONFIG_FILE
             */
            ClientConfiguration(const char* profileName);

            /**
             * User Agent string user for http calls. This is filled in for you in the constructor. Don't override this unless you have a really good reason.
             */
            Aws::String userAgent;
            /**
             * Http scheme to use. E.g. Http or Https. Default HTTPS
             */
            Aws::Http::Scheme scheme;
            /**
             * AWS Region to use in signing requests. Default US_EAST_1
             */
            Aws::String region;
            /**
             * Use dual stack endpoint in the endpoint calculation. It is your responsibility to verify that the service supports ipv6 in the region you select.
             */
            bool useDualStack;
            /**
             * Max concurrent tcp connections for a single http client to use. Default 25.
             */
            unsigned maxConnections;
            /**
             * This is currently only applicable for Curl to set the http request level timeout, including possible dns lookup time, connection establish time, ssl handshake time and actual data transmission time.
             * the corresponding Curl option is CURLOPT_TIMEOUT_MS
             * defaults to 0, no http request level timeout.
             */
            long httpRequestTimeoutMs;
            /**
             * Socket read timeouts for HTTP clients on Windows. Default 3000 ms. This should be more than adequate for most services. However, if you are transfering large amounts of data
             * or are worried about higher latencies, you should set to something that makes more sense for your use case.
             * For Curl, it's the low speed time, which contains the time in number milliseconds that transfer speed should be below "lowSpeedLimit" for the library to consider it too slow and abort.
             * Note that for Curl this config is converted to seconds by rounding down to the nearest whole second except when the value is greater than 0 and less than 1000. In this case it is set to one second. When it's 0, low speed limit check will be disabled.
             * Note that for Windows when this config is 0, the behavior is not specified by Windows.
             */
            long requestTimeoutMs;
            /**
             * Socket connect timeout. Default 1000 ms. Unless you are very far away from your the data center you are talking to. 1000ms is more than sufficient.
             */
            long connectTimeoutMs;
            /**
             * Enable TCP keep-alive. Default true;
             * No-op for WinHTTP, WinINet and IXMLHTTPRequest2 client.
             */
            bool enableTcpKeepAlive;
            /**
             * Interval to send a keep-alive packet over the connection. Default 30 seconds. Minimum 15 seconds.
             * WinHTTP & libcurl support this option.
             * No-op for WinINet and IXMLHTTPRequest2 client.
             */
            unsigned long tcpKeepAliveIntervalMs;
            /**
             * Average transfer speed in bytes per second that the transfer should be below during the request timeout interval for it to be considered too slow and abort.
             * Default 1 byte/second. Only for CURL client currently.
             */
            unsigned long lowSpeedLimit;
            /**
             * Strategy to use in case of failed requests. Default is DefaultRetryStrategy (e.g. exponential backoff)
             */
            std::shared_ptr<RetryStrategy> retryStrategy;
            /**
             * Override the http endpoint used to talk to a service.
             */
            Aws::String endpointOverride;
            /**
             * If you have users going through a proxy, set the proxy scheme here. Default HTTP
             */
            Aws::Http::Scheme proxyScheme;
            /**
             * If you have users going through a proxy, set the host here.
             */
            Aws::String proxyHost;
            /**
             * If you have users going through a proxy, set the port here.
             */
            unsigned proxyPort;
            /**
             * If you have users going through a proxy, set the username here.
             */
            Aws::String proxyUserName;
            /**
            * If you have users going through a proxy, set the password here.
            */
            Aws::String proxyPassword;
            /**
            * SSL Certificate file to use for connecting to an HTTPS proxy.
            * Used to set CURLOPT_PROXY_SSLCERT in libcurl. Example: client.pem
            */
            Aws::String proxySSLCertPath;
            /**
            * Type of proxy client SSL certificate.
            * Used to set CURLOPT_PROXY_SSLCERTTYPE in libcurl. Example: PEM
            */
            Aws::String proxySSLCertType;
            /**
            * Private key file to use for connecting to an HTTPS proxy.
            * Used to set CURLOPT_PROXY_SSLKEY in libcurl. Example: key.pem
            */
            Aws::String proxySSLKeyPath;
            /**
            * Type of private key file used to connect to an HTTPS proxy.
            * Used to set CURLOPT_PROXY_SSLKEYTYPE in libcurl. Example: PEM
            */
            Aws::String proxySSLKeyType;
            /**
            * Passphrase to the private key file used to connect to an HTTPS proxy.
            * Used to set CURLOPT_PROXY_KEYPASSWD in libcurl. Example: password1
            */
            Aws::String proxySSLKeyPassword;
            /**
            * Threading Executor implementation. Default uses std::thread::detach()
            */
            std::shared_ptr<Aws::Utils::Threading::Executor> executor;
            /**
             * If you need to test and want to get around TLS validation errors, do that here.
             * you probably shouldn't use this flag in a production scenario.
             */
            bool verifySSL;
            /**
             * If your Certificate Authority path is different from the default, you can tell
             * clients that aren't using the default trust store where to find your CA trust store.
             * If you are on windows or apple, you likely don't want this.
             */
            Aws::String caPath;
            /**
             * If you certificate file is different from the default, you can tell clients that
             * aren't using the default trust store where to find your ca file.
             * If you are on windows or apple, you likely don't want this.
             */
             Aws::String caFile;
            /**
             * Rate Limiter implementation for outgoing bandwidth. Default is wide-open.
             */
            std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> writeRateLimiter;
            /**
            * Rate Limiter implementation for incoming bandwidth. Default is wide-open.
            */
            std::shared_ptr<Aws::Utils::RateLimits::RateLimiterInterface> readRateLimiter;
            /**
             * Override the http implementation the default factory returns.
             */
            Aws::Http::TransferLibType httpLibOverride;
            /**
             * If set to true the http stack will follow 300 redirect codes.
             */
            bool followRedirects;

            /**
             * Only works for Curl http client.
             * Curl will by default add "Expect: 100-Continue" header in a Http request so as to avoid sending http
             * payload to wire if server respond error immediately after receiving the header.
             * Set this option to true will tell Curl to send http request header and body together.
             * This can save one round-trip time and especially useful when the payload is small and network latency is more important.
             * But be careful when Http request has large payload such S3 PutObject. You don't want to spend long time sending a large payload just getting a error response for server.
             * The default value will be false.
             */
            bool disableExpectHeader;

            /**
             * If set to true clock skew will be adjusted after each http attempt, default to true.
             */
            bool enableClockSkewAdjustment;

            /**
             * Enable host prefix injection.
             * For services whose endpoint is injectable. e.g. servicediscovery, you can modify the http host's prefix so as to add "data-" prefix for DiscoverInstances request.
             * Default to true, enabled. You can disable it for testing purpose.
             */
            bool enableHostPrefixInjection;

            /**
             * Enable endpoint discovery
             * For some services to dynamically set up their endpoints for different requests.
             * Defaults to false, it's an opt-in feature.
             * If disabled, regional or overriden endpoint will be used instead.
             * If a request requires endpoint discovery but you disabled it. The request will never succeed.
             */
            bool enableEndpointDiscovery;

            /**
             * profileName in config file that will be used by this object to reslove more configurations.
             */
            Aws::String profileName;
        };

    } // namespace Client
} // namespace Aws


