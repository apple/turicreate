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
#include <aws/core/http/windows/WinSyncHttpClient.h>
#include <aws/core/Http/HttpRequest.h>
#include <aws/core/http/standard/StandardHttpResponse.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/windows/WinConnectionPoolMgr.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/core/utils/ratelimiter/RateLimiterInterface.h>

#include <Windows.h>
#include <sstream>
#include <iostream>

using namespace Aws::Client;
using namespace Aws::Http;
using namespace Aws::Http::Standard;
using namespace Aws::Utils;
using namespace Aws::Utils::Logging;

static const uint32_t HTTP_REQUEST_WRITE_BUFFER_LENGTH = 8192;
static const char CLASS_TAG[] = "WinSyncHttpClient";

WinSyncHttpClient::~WinSyncHttpClient()
{
    AWS_LOGSTREAM_DEBUG(GetLogTag(), "Cleaning up client with handle " << m_openHandle);
    if (GetConnectionPoolManager() && GetOpenHandle())
    {
        GetConnectionPoolManager()->DoCloseHandle(GetOpenHandle());
    }
    Aws::Delete(GetConnectionPoolManager());
    SetConnectionPoolManager(nullptr);
}

void WinSyncHttpClient::SetOpenHandle(void* handle)
{
    m_openHandle = handle;
}

void WinSyncHttpClient::SetConnectionPoolManager(WinConnectionPoolMgr* connectionMgr)
{
    m_connectionPoolMgr = connectionMgr;
}

void* WinSyncHttpClient::AllocateWindowsHttpRequest(const Aws::Http::HttpRequest& request, void* connection) const
{
    Aws::StringStream ss;
    ss << request.GetUri().GetPath();

    if (request.GetUri().GetQueryStringParameters().size() > 0)
    {
        ss << request.GetUri().GetQueryString();
    }

    void* hHttpRequest = OpenRequest(request, connection, ss);
    AWS_LOGSTREAM_DEBUG(GetLogTag(), "AllocateWindowsHttpRequest returned handle " << hHttpRequest);

    return hHttpRequest;
}

void WinSyncHttpClient::AddHeadersToRequest(const HttpRequest& request, void* hHttpRequest) const
{
    if(request.GetHeaders().size() > 0)
    {
        Aws::StringStream ss;
        AWS_LOGSTREAM_DEBUG(GetLogTag(), "with headers:");
        for (auto& header : request.GetHeaders())
        {
            ss << header.first << ": " << header.second << "\r\n";
        }

        Aws::String headerString = ss.str();
        AWS_LOGSTREAM_DEBUG(GetLogTag(), headerString);

        DoAddHeaders(hHttpRequest, headerString);
    }
    else
    {
        AWS_LOGSTREAM_DEBUG(GetLogTag(), "with no headers");
    }
}

bool WinSyncHttpClient::StreamPayloadToRequest(const HttpRequest& request, void* hHttpRequest, Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter) const
{
    bool success = true;
    bool isChunked = request.HasTransferEncoding() && request.GetTransferEncoding() == Aws::Http::CHUNKED_VALUE;
    auto payloadStream = request.GetContentBody();
    if(payloadStream)
    {
        uint64_t bytesWritten;
        auto startingPos = payloadStream->tellg();
        char streamBuffer[ HTTP_REQUEST_WRITE_BUFFER_LENGTH ];
        bool done = false;
        while(success && !done)
        {
            payloadStream->read(streamBuffer, HTTP_REQUEST_WRITE_BUFFER_LENGTH);
            std::streamsize bytesRead = payloadStream->gcount();
            success = !payloadStream->bad();

            bytesWritten = 0;
            if (bytesRead > 0)
            {
                bytesWritten = DoWriteData(hHttpRequest, streamBuffer, bytesRead, isChunked);
                if (!bytesWritten)
                {
                    success = false;
                }
                else if(writeLimiter)
                {
                    writeLimiter->ApplyAndPayForCost(bytesWritten);
                }
            }

            auto& sentHandler = request.GetDataSentEventHandler();
            if (sentHandler)
            {
                sentHandler(&request, (long long)bytesWritten);
            }

            if(!payloadStream->good())
            {
                done = true;
            }

            success = success && ContinueRequest(request) && IsRequestProcessingEnabled();
        }

        if (success && isChunked)
        {
            bytesWritten = FinalizeWriteData(hHttpRequest);
            if (!bytesWritten)
            {
                success = false;
            }
            else if (writeLimiter)
            {
                writeLimiter->ApplyAndPayForCost(bytesWritten);
            }
        }

        payloadStream->clear();
        payloadStream->seekg(startingPos, payloadStream->beg);
    }

    if(success)
    {
        success = DoReceiveResponse(hHttpRequest);
    }
    return success;
}

void WinSyncHttpClient::LogRequestInternalFailure() const
{
    static const uint32_t WINDOWS_ERROR_MESSAGE_BUFFER_SIZE = 2048;

    DWORD error = GetLastError();

    char messageBuffer[WINDOWS_ERROR_MESSAGE_BUFFER_SIZE];

    FormatMessageA(
        FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        GetClientModule(),
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        messageBuffer,
        WINDOWS_ERROR_MESSAGE_BUFFER_SIZE,
        nullptr);
    AWS_LOGSTREAM_WARN(GetLogTag(), "Send request failed: " << messageBuffer);

}

bool WinSyncHttpClient::BuildSuccessResponse(const Aws::Http::HttpRequest& request, std::shared_ptr<HttpResponse>& response, void* hHttpRequest, Aws::Utils::RateLimits::RateLimiterInterface* readLimiter) const
{
    Aws::StringStream ss;
    uint64_t read = 0;

    DoQueryHeaders(hHttpRequest, response, ss, read);

    if(readLimiter != nullptr && read > 0)
    {
        readLimiter->ApplyAndPayForCost(read);
    }

    Aws::Vector<Aws::String> rawHeaders = StringUtils::SplitOnLine(ss.str());

    for (auto& header : rawHeaders)
    {
        Aws::Vector<Aws::String> keyValuePair = StringUtils::Split(header, ':', 2);
        if (keyValuePair.size() == 2)
        {
            response->AddHeader(StringUtils::Trim(keyValuePair[0].c_str()), StringUtils::Trim(keyValuePair[1].c_str()));
        }
    }

    if (request.GetMethod() != HttpMethod::HTTP_HEAD)
    {
        char body[1024];
        uint64_t bodySize = sizeof(body);
        int64_t numBytesResponseReceived = 0;
        read = 0;

        bool success = ContinueRequest(request);

        while (DoReadData(hHttpRequest, body, bodySize, read) && read > 0 && success)
        {
            response->GetResponseBody().write(body, read);
            if (read > 0)
            {
                numBytesResponseReceived += read;
                if (readLimiter != nullptr)
                {
                    readLimiter->ApplyAndPayForCost(read);
                }
                auto& receivedHandler = request.GetDataReceivedEventHandler();
                if (receivedHandler)
                {
                    receivedHandler(&request, response.get(), (long long)read);
                }
            }

            success = success && ContinueRequest(request) && IsRequestProcessingEnabled();
        }

        if (success && response->HasHeader(Aws::Http::CONTENT_LENGTH_HEADER))
        {
            const Aws::String& contentLength = response->GetHeader(Aws::Http::CONTENT_LENGTH_HEADER);
            AWS_LOGSTREAM_TRACE(GetLogTag(), "Response content-length header: " << contentLength);
            AWS_LOGSTREAM_TRACE(GetLogTag(), "Response body length: " << numBytesResponseReceived);
            if (StringUtils::ConvertToInt64(contentLength.c_str()) != numBytesResponseReceived)
            {
                success = false;
                response->SetClientErrorType(CoreErrors::NETWORK_CONNECTION);
                response->SetClientErrorMessage("Response body length doesn't match the content-length header.");
                AWS_LOGSTREAM_ERROR(GetLogTag(), "Response body length doesn't match the content-length header.");
            }
        }

        if(!success)
        {
            return false;
        }
    }

    //go ahead and flush the response body.
    response->GetResponseBody().flush();

    return true;
}

std::shared_ptr<HttpResponse> WinSyncHttpClient::MakeRequest(HttpRequest& request,
	Aws::Utils::RateLimits::RateLimiterInterface* readLimiter,
	Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter) const
{
	std::shared_ptr<HttpResponse> response = Aws::MakeShared<Standard::StandardHttpResponse>(CLASS_TAG, request);
	MakeRequestInternal(request, response, readLimiter, writeLimiter);
	return response;
}

std::shared_ptr<HttpResponse> WinSyncHttpClient::MakeRequest(const std::shared_ptr<HttpRequest>& request,
	Aws::Utils::RateLimits::RateLimiterInterface* readLimiter,
	Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter) const
{
	std::shared_ptr<HttpResponse> response = Aws::MakeShared<Standard::StandardHttpResponse>(CLASS_TAG, request);
	MakeRequestInternal(*request, response, readLimiter, writeLimiter);
	return response;
}

void WinSyncHttpClient::MakeRequestInternal(HttpRequest& request,
        std::shared_ptr<HttpResponse>& response,
        Aws::Utils::RateLimits::RateLimiterInterface* readLimiter,
        Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter) const
{
	//we URL encode right before going over the wire to avoid double encoding problems with the signer.
	URI& uriRef = request.GetUri();
	uriRef.SetPath(URI::URLEncodePathRFC3986(uriRef.GetPath()));

    AWS_LOGSTREAM_TRACE(GetLogTag(), "Making " << HttpMethodMapper::GetNameForHttpMethod(request.GetMethod()) <<
			" request to uri " << uriRef.GetURIString(true));

    bool success = false;
    void* connection = nullptr;
    void* hHttpRequest = nullptr;

    if(IsRequestProcessingEnabled())
    {
        if (writeLimiter != nullptr)
        {
            writeLimiter->ApplyAndPayForCost(request.GetSize());
        }

        connection = m_connectionPoolMgr->AcquireConnectionForHost(uriRef.GetAuthority(), uriRef.GetPort());
        AWS_LOGSTREAM_DEBUG(GetLogTag(), "Acquired connection " << connection);

        hHttpRequest = AllocateWindowsHttpRequest(request, connection);

        AddHeadersToRequest(request, hHttpRequest);
        if (DoSendRequest(hHttpRequest) && StreamPayloadToRequest(request, hHttpRequest, writeLimiter))
        {
            success = BuildSuccessResponse(request, response, hHttpRequest, readLimiter);
        }
        else
        {
            response->SetClientErrorType(CoreErrors::NETWORK_CONNECTION);
            response->SetClientErrorMessage("Encountered network error when sending http request");
        }
    }

    if (!success && !IsRequestProcessingEnabled() || !ContinueRequest(request))
    {
        response->SetClientErrorType(CoreErrors::USER_CANCELLED);
        response->SetClientErrorMessage("Request processing disabled or continuation cancelled by user's continuation handler.");
        response->SetResponseCode(Aws::Http::HttpResponseCode::NO_RESPONSE);
    }
    else if(!success)
    {
        LogRequestInternalFailure();
    }

    if (hHttpRequest)
    {
        AWS_LOGSTREAM_DEBUG(GetLogTag(), "Closing http request handle " << hHttpRequest);
        GetConnectionPoolManager()->DoCloseHandle(hHttpRequest);
    }

    AWS_LOGSTREAM_DEBUG(GetLogTag(), "Releasing connection handle " << connection);
    GetConnectionPoolManager()->ReleaseConnectionForHost(request.GetUri().GetAuthority(), request.GetUri().GetPort(), connection);
}
