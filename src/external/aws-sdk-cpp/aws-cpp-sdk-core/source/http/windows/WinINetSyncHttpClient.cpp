/*
  * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/core/http/windows/WinINetSyncHttpClient.h>

#include <aws/core/Http/HttpRequest.h>
#include <aws/core/http/standard/StandardHttpResponse.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/windows/WinINetConnectionPoolMgr.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/core/utils/ratelimiter/RateLimiterInterface.h>

#include <Windows.h>
#include <WinInet.h>
#include <sstream>
#include <iostream>

using namespace Aws::Client;
using namespace Aws::Http;
using namespace Aws::Http::Standard;
using namespace Aws::Utils;
using namespace Aws::Utils::Logging;

static const uint32_t HTTP_REQUEST_WRITE_BUFFER_LENGTH = 8192;

WinINetSyncHttpClient::WinINetSyncHttpClient(const ClientConfiguration& config) :
    Base()
{
    AWS_LOG_INFO(GetLogTag(), "Creating http client with user agent %s with max connections %d, request timeout %d, "
        "and connect timeout %d",
        config.userAgent.c_str(), config.maxConnections, config.requestTimeoutMs, config.connectTimeoutMs);

    m_allowRedirects = config.followRedirects;

    DWORD inetFlags = INTERNET_OPEN_TYPE_DIRECT;
    const char* proxyHosts = nullptr;
    Aws::String strProxyHosts;

    bool isUsingProxy = !config.proxyHost.empty();
    //setup initial proxy config.
    if (isUsingProxy)
    {
        AWS_LOG_INFO(GetLogTag(), "Http Client is using a proxy. Setting up proxy with settings host %s, port %d, username %s.",
            config.proxyHost, config.proxyPort, config.proxyUserName);

        inetFlags = INTERNET_OPEN_TYPE_PROXY;
        Aws::StringStream ss;
        const char* schemeString = Aws::Http::SchemeMapper::ToString(config.scheme);
        ss << StringUtils::ToUpper(schemeString) << "=" << schemeString << "://" << config.proxyHost << ":" << config.proxyPort;
        strProxyHosts.assign(ss.str());
        proxyHosts = strProxyHosts.c_str();

        AWS_LOG_DEBUG("Adding proxy host string to wininet %s", proxyHosts);
    }

    SetOpenHandle(InternetOpenA(config.userAgent.c_str(), inetFlags, proxyHosts, nullptr, 0));

    //override offline mode.
    InternetSetOptionA(GetOpenHandle(), INTERNET_OPTION_IGNORE_OFFLINE, nullptr, 0);
    //add proxy auth credentials to everything using this handle.
    if (isUsingProxy)
    {
        if (!config.proxyUserName.empty() && !InternetSetOptionA(GetOpenHandle(), INTERNET_OPTION_PROXY_USERNAME, (LPVOID)config.proxyUserName.c_str(), (DWORD)config.proxyUserName.length()))
            AWS_LOG_FATAL(GetLogTag(), "Failed setting username for proxy with error code: %d", GetLastError());
        if (!config.proxyPassword.empty() && !InternetSetOptionA(GetOpenHandle(), INTERNET_OPTION_PROXY_PASSWORD, (LPVOID)config.proxyPassword.c_str(), (DWORD)config.proxyPassword.length()))
            AWS_LOG_FATAL(GetLogTag(), "Failed setting password for proxy with error code: %d", GetLastError());
    }

    if (!config.verifySSL)
    {
        AWS_LOG_WARN(GetLogTag(), "Turning ssl unknown ca verification off.");
        DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;

        if (!InternetSetOptionA(GetOpenHandle(), INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags)))
            AWS_LOG_FATAL(GetLogTag(), "Failed to turn ssl cert ca verification off.");
    }

    AWS_LOG_DEBUG(GetLogTag(), "API handle %p.", GetOpenHandle());
    SetConnectionPoolManager(Aws::New<WinINetConnectionPoolMgr>(GetLogTag(),
        GetOpenHandle(), config.maxConnections, config.requestTimeoutMs, config.connectTimeoutMs));
}


WinINetSyncHttpClient::~WinINetSyncHttpClient()
{

}

void* WinINetSyncHttpClient::OpenRequest(const Aws::Http::HttpRequest& request, void* connection, const Aws::StringStream& ss) const
{
    DWORD requestFlags =
        INTERNET_FLAG_NO_AUTH | 
        INTERNET_FLAG_RELOAD | 
        INTERNET_FLAG_KEEP_CONNECTION | 
        (request.GetUri().GetScheme() == Scheme::HTTPS ? INTERNET_FLAG_SECURE : 0) |
        INTERNET_FLAG_NO_CACHE_WRITE |
        (m_allowRedirects ? 0 : INTERNET_FLAG_NO_AUTO_REDIRECT);

    static LPCSTR accept[2] = { "*/*", nullptr };
    HINTERNET hHttpRequest = HttpOpenRequestA(connection, HttpMethodMapper::GetNameForHttpMethod(request.GetMethod()),
        ss.str().c_str(), nullptr, nullptr, accept, requestFlags, 0);
    AWS_LOG_DEBUG(GetLogTag(), "HttpOpenRequestA returned handle %p", hHttpRequest);

    return hHttpRequest;
}

void WinINetSyncHttpClient::DoAddHeaders(void* hHttpRequest, Aws::String& headerStr) const
{
    HttpAddRequestHeadersA(hHttpRequest, headerStr.c_str(), (DWORD)headerStr.length(), HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
}

uint64_t WinINetSyncHttpClient::DoWriteData(void* hHttpRequest, char* streamBuffer, uint64_t bytesRead) const
{
    DWORD bytesWritten = 0;
    if(!InternetWriteFile(hHttpRequest, streamBuffer, (DWORD)bytesRead, &bytesWritten))
    {
        return 0;
    }

    return bytesWritten;
}

bool WinINetSyncHttpClient::DoReceiveResponse(void* hHttpRequest) const
{
    return (HttpEndRequest(hHttpRequest, nullptr, 0, 0) != 0);
}
   
bool WinINetSyncHttpClient::DoQueryHeaders(void* hHttpRequest, std::shared_ptr<StandardHttpResponse>& response, Aws::StringStream& ss, uint64_t& read) const
{

    char dwStatusCode[256];
    DWORD dwSize = sizeof(dwStatusCode);

    HttpQueryInfoA(hHttpRequest, HTTP_QUERY_STATUS_CODE, &dwStatusCode, &dwSize, 0);
    response->SetResponseCode((HttpResponseCode)atoi(dwStatusCode));
    AWS_LOG_DEBUG(GetLogTag(), "Received response code %s.", dwStatusCode);

    char contentTypeStr[1024];
    dwSize = sizeof(contentTypeStr);
    HttpQueryInfoA(hHttpRequest, HTTP_QUERY_CONTENT_TYPE, &contentTypeStr, &dwSize, 0);
    if (contentTypeStr[0] != NULL)
    {
        response->SetContentType(contentTypeStr);
        AWS_LOGSTREAM_DEBUG(GetLogTag(), "Received content type " << contentTypeStr);
    }

    char headerStr[1024];
    dwSize = sizeof(headerStr);
    AWS_LOG_DEBUG(GetLogTag(), "Received headers:");
    while (HttpQueryInfoA(hHttpRequest, HTTP_QUERY_RAW_HEADERS_CRLF, headerStr, &dwSize, (LPDWORD)&read) && dwSize > 0)
    {
        AWS_LOGSTREAM_DEBUG(GetLogTag(), headerStr);
        ss << headerStr;
    }
    return (read != 0);
}

bool WinINetSyncHttpClient::DoSendRequest(void* hHttpRequest) const
{
    return (HttpSendRequestEx(hHttpRequest, NULL, NULL, 0, 0) != 0);
}

bool WinINetSyncHttpClient::DoReadData(void* hHttpRequest, char* body, uint64_t size, uint64_t& read) const
{
    return (InternetReadFile(hHttpRequest, body, (DWORD)size, (LPDWORD)&read) != 0);
}

void* WinINetSyncHttpClient::GetClientModule() const
{
    return GetModuleHandle(TEXT("wininet.dll"));
}
