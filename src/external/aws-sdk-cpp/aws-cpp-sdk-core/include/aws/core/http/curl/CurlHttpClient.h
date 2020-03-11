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
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/curl/CurlHandleContainer.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <atomic>

namespace Aws
{
namespace Http
{
namespace Standard
{
    class StandardHttpResponse;
}

//Curl implementation of an http client. Right now it is only synchronous.
class AWS_CORE_API CurlHttpClient: public HttpClient
{
public:

    using Base = HttpClient;

    //Creates client, initializes curl handle if it hasn't been created already.
    CurlHttpClient(const Aws::Client::ClientConfiguration& clientConfig);
    //Makes request and receives response synchronously
    AWS_DEPRECATED("This funciton in base class has been deprecated")
    std::shared_ptr<HttpResponse> MakeRequest(HttpRequest& request, Aws::Utils::RateLimits::RateLimiterInterface* readLimiter = nullptr,
            Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter = nullptr) const override;

    //Makes request with shared_ptr typed request and receives response synchronously
    std::shared_ptr<HttpResponse> MakeRequest(const std::shared_ptr<HttpRequest>& request, Aws::Utils::RateLimits::RateLimiterInterface* readLimiter = nullptr,
            Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter = nullptr) const override;

    static void InitGlobalState();
    static void CleanupGlobalState();

private:
    mutable CurlHandleContainer m_curlHandleContainer;
    bool m_isUsingProxy;
    Aws::String m_proxyUserName;
    Aws::String m_proxyPassword;
    Aws::String m_proxyScheme;
    Aws::String m_proxyHost;
    Aws::String m_proxySSLCertPath;
    Aws::String m_proxySSLCertType;
    Aws::String m_proxySSLKeyPath;
    Aws::String m_proxySSLKeyType;
    Aws::String m_proxyKeyPasswd;
    unsigned m_proxyPort;
    bool m_verifySSL;
    Aws::String m_caPath;
    Aws::String m_caFile;
    bool m_disableExpectHeader;
    bool m_allowRedirects;
    static std::atomic<bool> isInit;

    void MakeRequestInternal(HttpRequest& request, std::shared_ptr<Standard::StandardHttpResponse>& response,
        Aws::Utils::RateLimits::RateLimiterInterface* readLimiter,
        Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter) const;


};

using PlatformHttpClient = CurlHttpClient;

} // namespace Http
} // namespace Aws

