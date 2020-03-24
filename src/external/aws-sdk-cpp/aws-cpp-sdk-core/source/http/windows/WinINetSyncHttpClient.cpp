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
#define AWS_DISABLE_DEPRECATION

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
#include <aws/core/utils/UnreferencedParam.h>

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

static void WinINetEnableHttp2(void* handle)
{
#ifdef WININET_HAS_H2
    DWORD http2 = HTTP_PROTOCOL_FLAG_HTTP2;
    if (!InternetSetOptionA(handle, INTERNET_OPTION_ENABLE_HTTP_PROTOCOL, &http2, sizeof(http2))) 
    {
        AWS_LOGSTREAM_ERROR("WinINetHttp2", "Failed to enable HTTP/2 on WinInet handle: " << handle << ". Falling back to HTTP/1.1.");
    }
    else
    {
        AWS_LOGSTREAM_DEBUG("WinINetHttp2", "HTTP/2 enabled on WinInet handle: " << handle << ".");
    }
#else
    AWS_UNREFERENCED_PARAM(handle);
#endif
}

WinINetSyncHttpClient::WinINetSyncHttpClient(const ClientConfiguration& config) :
    Base()
{
    AWS_LOGSTREAM_INFO(GetLogTag(), "Creating http client with user agent " << config.userAgent << " with max connections " <<
         config.maxConnections << ", request timeout " << config.requestTimeoutMs << ",and connect timeout " << config.connectTimeoutMs);       

    m_allowRedirects = config.followRedirects;

    DWORD inetFlags = INTERNET_OPEN_TYPE_DIRECT;
    const char* proxyHosts = nullptr;
    Aws::String strProxyHosts;

    m_usingProxy = !config.proxyHost.empty();
    //setup initial proxy config.
    if (m_usingProxy)
    {
        const char* const proxySchemeString = Aws::Http::SchemeMapper::ToString(config.proxyScheme);
        AWS_LOGSTREAM_INFO(GetLogTag(), "Http Client is using a proxy. Setting up proxy with settings scheme " << proxySchemeString
            << ", host " << config.proxyHost << ", port " << config.proxyPort << ", username " << config.proxyUserName << ".");

        inetFlags = INTERNET_OPEN_TYPE_PROXY;
        Aws::StringStream ss;
        const char* schemeString = Aws::Http::SchemeMapper::ToString(config.scheme);
        ss << StringUtils::ToUpper(schemeString) << "=" << proxySchemeString << "://" << config.proxyHost << ":" << config.proxyPort;
        strProxyHosts.assign(ss.str());
        proxyHosts = strProxyHosts.c_str();

        AWS_LOGSTREAM_DEBUG(GetLogTag(), "Adding proxy host string to wininet " << proxyHosts);

        m_proxyUserName = config.proxyUserName;
        m_proxyPassword = config.proxyPassword;
    }

    SetOpenHandle(InternetOpenA(config.userAgent.c_str(), inetFlags, proxyHosts, nullptr, 0));

    //override offline mode.
    InternetSetOptionA(GetOpenHandle(), INTERNET_OPTION_IGNORE_OFFLINE, nullptr, 0);
    WinINetEnableHttp2(GetOpenHandle());
    if (!config.verifySSL)
    {
        AWS_LOGSTREAM_WARN(GetLogTag(), "Turning ssl unknown ca verification off.");
        DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | INTERNET_FLAG_IGNORE_CERT_CN_INVALID;

        if (!InternetSetOptionA(GetOpenHandle(), INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags)))
            AWS_LOGSTREAM_FATAL(GetLogTag(), "Failed to turn ssl cert ca verification off.");
    }

    AWS_LOGSTREAM_DEBUG(GetLogTag(), "API handle " << GetOpenHandle());
    SetConnectionPoolManager(Aws::New<WinINetConnectionPoolMgr>(GetLogTag(),
        GetOpenHandle(), config.maxConnections, config.requestTimeoutMs, config.connectTimeoutMs, config.enableTcpKeepAlive, config.tcpKeepAliveIntervalMs));
}


WinINetSyncHttpClient::~WinINetSyncHttpClient()
{
    InternetCloseHandle(GetOpenHandle());
    SetOpenHandle(nullptr);  // the handle is already closed, annul it to avoid double-closing the handle (in the base class' destructor)
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

    LPCSTR accept[2] = { nullptr, nullptr };

    Aws::String acceptHeader("*/*");

    if (request.HasHeader(Aws::Http::ACCEPT_HEADER))
    {
        acceptHeader = request.GetHeaderValue(Aws::Http::ACCEPT_HEADER);
    }

    accept[0] = acceptHeader.c_str();

    HINTERNET hHttpRequest = HttpOpenRequestA(connection, HttpMethodMapper::GetNameForHttpMethod(request.GetMethod()),
        ss.str().c_str(), nullptr, nullptr, accept, requestFlags, 0);
    AWS_LOGSTREAM_DEBUG(GetLogTag(), "HttpOpenRequestA returned handle " << hHttpRequest);

    //add proxy auth credentials to everything using this handle.
    if (m_usingProxy)
    {
        if (!m_proxyUserName.empty() && !InternetSetOptionA(hHttpRequest, INTERNET_OPTION_PROXY_USERNAME, (LPVOID)m_proxyUserName.c_str(), (DWORD)m_proxyUserName.length()))
            AWS_LOGSTREAM_FATAL(GetLogTag(), "Failed setting username for proxy with error code: " << GetLastError());
        if (!m_proxyPassword.empty() && !InternetSetOptionA(hHttpRequest, INTERNET_OPTION_PROXY_PASSWORD, (LPVOID)m_proxyPassword.c_str(), (DWORD)m_proxyPassword.length()))
            AWS_LOGSTREAM_FATAL(GetLogTag(), "Failed setting password for proxy with error code: " << GetLastError());
    }
    WinINetEnableHttp2(hHttpRequest);

    return hHttpRequest;
}

void WinINetSyncHttpClient::DoAddHeaders(void* hHttpRequest, Aws::String& headerStr) const
{
    if (!HttpAddRequestHeadersA(hHttpRequest, headerStr.c_str(), (DWORD)headerStr.length(), HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD))
        AWS_LOGSTREAM_ERROR(GetLogTag(), "Failed to add HTTP request headers with error code: " << GetLastError());
}

uint64_t WinINetSyncHttpClient::DoWriteData(void* hHttpRequest, char* streamBuffer, uint64_t bytesRead, bool isChunked) const
{
    DWORD bytesWritten = 0;
    uint64_t totalBytesWritten = 0;
    const char CRLF[] = "\r\n";

    if (isChunked)
    {
        Aws::String chunkSizeHexString = StringUtils::ToHexString(bytesRead) + CRLF;

        if (!InternetWriteFile(hHttpRequest, chunkSizeHexString.c_str(), (DWORD)chunkSizeHexString.size(), &bytesWritten))
        {
            return totalBytesWritten;
        }
        totalBytesWritten += bytesWritten;
        if (!InternetWriteFile(hHttpRequest, streamBuffer, (DWORD)bytesRead, &bytesWritten))
        {
            return totalBytesWritten;
        }
        totalBytesWritten += bytesWritten;
        if (!InternetWriteFile(hHttpRequest, CRLF, (DWORD)(sizeof(CRLF) - 1), &bytesWritten))
        {
            return totalBytesWritten;
        }
        totalBytesWritten += bytesWritten;
    }
    else
    {
        if (!InternetWriteFile(hHttpRequest, streamBuffer, (DWORD)bytesRead, &bytesWritten))
        {
            return totalBytesWritten;
        }
        totalBytesWritten += bytesWritten;
    }

    return totalBytesWritten;
}

uint64_t WinINetSyncHttpClient::FinalizeWriteData(void* hHttpRequest) const
{
    DWORD bytesWritten = 0;
    const char trailingCRLF[] = "0\r\n\r\n";
    if (!InternetWriteFile(hHttpRequest, trailingCRLF, (DWORD)(sizeof(trailingCRLF) - 1), &bytesWritten))
    {
        return 0;
    }

    return bytesWritten;
}

bool WinINetSyncHttpClient::DoReceiveResponse(void* hHttpRequest) const
{
    return (HttpEndRequest(hHttpRequest, nullptr, 0, 0) != 0);
}
   
bool WinINetSyncHttpClient::DoQueryHeaders(void* hHttpRequest, std::shared_ptr<HttpResponse>& response, Aws::StringStream& ss, uint64_t& read) const
{

    char dwStatusCode[256];
    DWORD dwSize = sizeof(dwStatusCode);

    HttpQueryInfoA(hHttpRequest, HTTP_QUERY_STATUS_CODE, &dwStatusCode, &dwSize, 0);
    response->SetResponseCode((HttpResponseCode)atoi(dwStatusCode));
    AWS_LOGSTREAM_DEBUG(GetLogTag(), "Received response code " << dwStatusCode);

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
    AWS_LOGSTREAM_DEBUG(GetLogTag(), "Received headers:");
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
