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
#include <aws/core/http/windows/WinHttpSyncHttpClient.h>

#include <aws/core/Http/HttpRequest.h>
#include <aws/core/http/standard/StandardHttpResponse.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/windows/WinHttpConnectionPoolMgr.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/Array.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/core/utils/ratelimiter/RateLimiterInterface.h>
#include <aws/core/utils/UnreferencedParam.h>

#include <Windows.h>
#include <winhttp.h>
#include <sstream>
#include <iostream>

using namespace Aws::Client;
using namespace Aws::Http;
using namespace Aws::Http::Standard;
using namespace Aws::Utils;
using namespace Aws::Utils::Logging;

static const uint32_t HTTP_REQUEST_WRITE_BUFFER_LENGTH = 8192;

static void WinHttpEnableHttp2(void* handle)
{
#ifdef WINHTTP_HAS_H2
    DWORD http2 = WINHTTP_PROTOCOL_FLAG_HTTP2;
    if (!WinHttpSetOption(handle, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &http2, sizeof(http2))) 
    {
        AWS_LOGSTREAM_ERROR("WinHttpHttp2", "Failed to enable HTTP/2 on WinHttp handle: " << handle << ". Falling back to HTTP/1.1.");
    }
    else
    {
        AWS_LOGSTREAM_DEBUG("WinHttpHttp2", "HTTP/2 enabled on WinHttp handle: " << handle << ".");
    }
#else
    AWS_UNREFERENCED_PARAM(handle);
#endif
}

WinHttpSyncHttpClient::WinHttpSyncHttpClient(const ClientConfiguration& config) :
    Base()
{
    AWS_LOGSTREAM_INFO(GetLogTag(), "Creating http client with user agent " << config.userAgent << " with max connections " << config.maxConnections 
        << " request timeout " << config.requestTimeoutMs << ",and connect timeout " << config.connectTimeoutMs);

    DWORD winhttpFlags = WINHTTP_ACCESS_TYPE_NO_PROXY;
    const char* proxyHosts = nullptr;
    Aws::String strProxyHosts;

    m_allowRedirects = config.followRedirects;

    m_usingProxy = !config.proxyHost.empty();
    //setup initial proxy config.

    Aws::WString proxyString;

    if (m_usingProxy)
    {
        const char* const proxySchemeString = Aws::Http::SchemeMapper::ToString(config.proxyScheme);
        AWS_LOGSTREAM_INFO(GetLogTag(), "Http Client is using a proxy. Setting up proxy with settings scheme " << proxySchemeString
             << ", host " << config.proxyHost << ", port " << config.proxyPort << ", username " << config.proxyUserName);

        winhttpFlags = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        Aws::StringStream ss;
        const char* schemeString = Aws::Http::SchemeMapper::ToString(config.scheme);
        ss << StringUtils::ToUpper(schemeString) << "=" << proxySchemeString << "://" << config.proxyHost << ":" << config.proxyPort;
        strProxyHosts.assign(ss.str());
        proxyHosts = strProxyHosts.c_str();

        proxyString = StringUtils::ToWString(proxyHosts);
        AWS_LOGSTREAM_DEBUG(GetLogTag(), "Adding proxy host string to winhttp " << proxyHosts);

        m_proxyUserName = StringUtils::ToWString(config.proxyUserName.c_str());
        m_proxyPassword = StringUtils::ToWString(config.proxyPassword.c_str());
    }

    Aws::WString openString = StringUtils::ToWString(config.userAgent.c_str());

    SetOpenHandle(WinHttpOpen(openString.c_str(), winhttpFlags, proxyString.c_str(), nullptr, 0));

    if (!WinHttpSetTimeouts(GetOpenHandle(), config.connectTimeoutMs, config.connectTimeoutMs, -1, config.requestTimeoutMs))
    {
        AWS_LOGSTREAM_WARN(GetLogTag(), "Error setting timeouts " << GetLastError());
    }
    WinHttpEnableHttp2(GetOpenHandle());
    m_verifySSL = config.verifySSL;
    if (m_verifySSL)
    {
        //disable insecure tls protocols, otherwise you might as well turn ssl verification off.
        DWORD flags = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
        if (!WinHttpSetOption(GetOpenHandle(), WINHTTP_OPTION_SECURE_PROTOCOLS, &flags, sizeof(flags)))
        {
            AWS_LOGSTREAM_FATAL(GetLogTag(), "Failed setting secure crypto protocols with error code: " << GetLastError());
        }
    }

    // WinHTTP doesn't have the option to turn off keep-alive, so we will only set the value if keep-alive is turned on.
    // see https://docs.microsoft.com/en-us/windows/desktop/winhttp/option-flags for more information on default values.
    if (config.enableTcpKeepAlive)
    {
        DWORD keepAliveIntervalMs = config.tcpKeepAliveIntervalMs;
        if (!WinHttpSetOption(GetOpenHandle(), WINHTTP_OPTION_WEB_SOCKET_KEEPALIVE_INTERVAL, &keepAliveIntervalMs, sizeof(keepAliveIntervalMs)))
        {
            AWS_LOGSTREAM_WARN(GetLogTag(), "Failed setting TCP keep-alive interval with error code: " << GetLastError());
        }
    }

    AWS_LOGSTREAM_DEBUG(GetLogTag(), "API handle " << GetOpenHandle());
    SetConnectionPoolManager(Aws::New<WinHttpConnectionPoolMgr>(GetLogTag(),
        GetOpenHandle(), config.maxConnections, config.requestTimeoutMs, config.connectTimeoutMs, config.enableTcpKeepAlive, config.tcpKeepAliveIntervalMs));
}


WinHttpSyncHttpClient::~WinHttpSyncHttpClient()
{
    WinHttpCloseHandle(GetOpenHandle());
    SetOpenHandle(nullptr);  // the handle is already closed, annul it to avoid double-closing the handle (in the base class' destructor)
}

void* WinHttpSyncHttpClient::OpenRequest(const Aws::Http::HttpRequest& request, void* connection, const Aws::StringStream& ss) const
{
    LPCWSTR accept[2] = { nullptr, nullptr };

    DWORD requestFlags = WINHTTP_FLAG_REFRESH |
        (request.GetUri().GetScheme() == Scheme::HTTPS ? WINHTTP_FLAG_SECURE : 0);

    Aws::WString acceptHeader(L"*/*");

    if (request.HasHeader(Aws::Http::ACCEPT_HEADER)) 
    {
        acceptHeader = Aws::Utils::StringUtils::ToWString(request.GetHeaderValue(Aws::Http::ACCEPT_HEADER).c_str());  
    }

    accept[0] = acceptHeader.c_str();

    Aws::WString wss = StringUtils::ToWString(ss.str().c_str());

    HINTERNET hHttpRequest = WinHttpOpenRequest(connection, StringUtils::ToWString(HttpMethodMapper::GetNameForHttpMethod(request.GetMethod())).c_str(),
        wss.c_str(), nullptr, nullptr, accept, requestFlags);

    //add proxy auth credentials to everything using this handle.
    if (m_usingProxy)
    {
        if (!m_proxyUserName.empty() && !WinHttpSetOption(hHttpRequest, WINHTTP_OPTION_PROXY_USERNAME, (LPVOID)m_proxyUserName.c_str(), (DWORD)m_proxyUserName.length()))
            AWS_LOGSTREAM_FATAL(GetLogTag(), "Failed setting username for proxy with error code: " << GetLastError());
        if (!m_proxyPassword.empty() && !WinHttpSetOption(hHttpRequest, WINHTTP_OPTION_PROXY_PASSWORD, (LPVOID)m_proxyPassword.c_str(), (DWORD)m_proxyPassword.length()))
            AWS_LOGSTREAM_FATAL(GetLogTag(), "Failed setting password for proxy with error code: " << GetLastError());
    }

    if (!m_verifySSL) // Turning ssl unknown ca verification off
    {
        DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        if (!WinHttpSetOption(hHttpRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags)))
            AWS_LOGSTREAM_FATAL(GetLogTag(), "Failed to turn ssl cert ca verification off.");
    }

    //DISABLE_FEATURE settings need to be made after OpenRequest but before SendRequest
    if (!m_allowRedirects)
    {
        requestFlags = WINHTTP_DISABLE_REDIRECTS;

        if (!WinHttpSetOption(hHttpRequest, WINHTTP_OPTION_DISABLE_FEATURE, &requestFlags, sizeof(requestFlags)))
            AWS_LOGSTREAM_FATAL(GetLogTag(), "Failed to turn off redirects!");
    }

    WinHttpEnableHttp2(hHttpRequest);
    return hHttpRequest;
}

void WinHttpSyncHttpClient::DoAddHeaders(void* hHttpRequest, Aws::String& headerStr) const
{

    Aws::WString wHeaderString = StringUtils::ToWString(headerStr.c_str());

    if (!WinHttpAddRequestHeaders(hHttpRequest, wHeaderString.c_str(), (DWORD)wHeaderString.length(), WINHTTP_ADDREQ_FLAG_REPLACE | WINHTTP_ADDREQ_FLAG_ADD))
        AWS_LOGSTREAM_ERROR(GetLogTag(), "Failed to add HTTP request headers with error code: " << GetLastError());
}

uint64_t WinHttpSyncHttpClient::DoWriteData(void* hHttpRequest, char* streamBuffer, uint64_t bytesRead, bool isChunked) const
{
    DWORD bytesWritten = 0;
    uint64_t totalBytesWritten = 0;
    const char CRLF[] = "\r\n";

    if (isChunked)
    {
        Aws::String chunkSizeHexString = StringUtils::ToHexString(bytesRead) + CRLF;

        if (!WinHttpWriteData(hHttpRequest, chunkSizeHexString.c_str(), (DWORD)chunkSizeHexString.size(), &bytesWritten))
        {
            return totalBytesWritten;
        }
        totalBytesWritten += bytesWritten;
        if (!WinHttpWriteData(hHttpRequest, streamBuffer, (DWORD)bytesRead, &bytesWritten))
        {
            return totalBytesWritten;
        }
        totalBytesWritten += bytesWritten;
        if (!WinHttpWriteData(hHttpRequest, CRLF, (DWORD)(sizeof(CRLF) - 1), &bytesWritten))
        {
            return totalBytesWritten;
        }
        totalBytesWritten += bytesWritten;
    }
    else
    {
        if (!WinHttpWriteData(hHttpRequest, streamBuffer, (DWORD)bytesRead, &bytesWritten))
        {
            return totalBytesWritten;
        }
        totalBytesWritten += bytesWritten;
    }

    return totalBytesWritten;
}

uint64_t WinHttpSyncHttpClient::FinalizeWriteData(void* hHttpRequest) const
{
    DWORD bytesWritten = 0;
    const char trailingCRLF[] = "0\r\n\r\n";
    if (!WinHttpWriteData(hHttpRequest, trailingCRLF, (DWORD)(sizeof(trailingCRLF) - 1), &bytesWritten))
    {
        return 0;
    }
        
    return bytesWritten;
}

bool WinHttpSyncHttpClient::DoReceiveResponse(void* httpRequest) const
{
    return (WinHttpReceiveResponse(httpRequest, nullptr) != 0);
}

bool WinHttpSyncHttpClient::DoQueryHeaders(void* hHttpRequest, std::shared_ptr<HttpResponse>& response, Aws::StringStream& ss, uint64_t& read) const
{
    wchar_t dwStatusCode[256];
    DWORD dwSize = sizeof(dwStatusCode);
    wmemset(dwStatusCode, 0, static_cast<size_t>(dwSize/sizeof(wchar_t)));

    WinHttpQueryHeaders(hHttpRequest, WINHTTP_QUERY_STATUS_CODE, nullptr, &dwStatusCode, &dwSize, 0);
    int responseCode = _wtoi(dwStatusCode);
    response->SetResponseCode((HttpResponseCode)responseCode);
    AWS_LOGSTREAM_DEBUG(GetLogTag(), "Received response code " << responseCode);

    wchar_t contentTypeStr[1024];
    dwSize = sizeof(contentTypeStr);
    wmemset(contentTypeStr, 0, static_cast<size_t>(dwSize / sizeof(wchar_t)));
    
    WinHttpQueryHeaders(hHttpRequest, WINHTTP_QUERY_CONTENT_TYPE, nullptr, &contentTypeStr, &dwSize, 0);
    if (contentTypeStr[0] != NULL)
    {
        Aws::String contentStr = StringUtils::FromWString(contentTypeStr);
        response->SetContentType(contentStr);
        AWS_LOGSTREAM_DEBUG(GetLogTag(), "Received content type " << contentStr);
    }
       
    BOOL queryResult = false;
    AWS_LOGSTREAM_DEBUG(GetLogTag(), "Received headers:");
    WinHttpQueryHeaders(hHttpRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, nullptr, &dwSize, WINHTTP_NO_HEADER_INDEX);
    
    //I know it's ugly, but this is how MSFT says to do it so....
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        Utils::Array<wchar_t> headerRawString(dwSize / sizeof(wchar_t));

        queryResult = WinHttpQueryHeaders(hHttpRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, headerRawString.GetUnderlyingData(), &dwSize, WINHTTP_NO_HEADER_INDEX);
        if (queryResult)
        {
            Aws::String headers(StringUtils::FromWString(headerRawString.GetUnderlyingData()));
            AWS_LOGSTREAM_DEBUG(GetLogTag(), headers);
            ss << std::move(headers);
            read = dwSize;
        }
    } 

    return queryResult == TRUE;
}

bool WinHttpSyncHttpClient::DoSendRequest(void* hHttpRequest) const
{
    return (WinHttpSendRequest(hHttpRequest, NULL, NULL, 0, 0, 0, NULL) != 0);
}

bool WinHttpSyncHttpClient::DoReadData(void* hHttpRequest, char* body, uint64_t size, uint64_t& read) const
{
    return (WinHttpReadData(hHttpRequest, body, (DWORD)size, (LPDWORD)&read) != 0);
}

void* WinHttpSyncHttpClient::GetClientModule() const
{
    return GetModuleHandle(TEXT("winhttp.dll"));
}
