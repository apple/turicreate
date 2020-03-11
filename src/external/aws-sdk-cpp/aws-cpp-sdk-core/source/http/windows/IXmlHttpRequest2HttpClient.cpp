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

#include <aws/core/http/windows/IXmlHttpRequest2HttpClient.h>
#include <aws/core/http/standard/StandardHttpResponse.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/ratelimiter/RateLimiterInterface.h>
#include <aws/core/utils/logging/LogMacros.h>

#include <comdef.h>
#include <IXmlHttpRequest2Ref.h>

namespace Aws
{
    namespace Http
    {
        static const char* CLASS_TAG = "IXmlHttpRequest2HttpClient";

        using namespace Microsoft::WRL;

        /**
         * Wrapper around std::iostream. Also has rate limiters and callbacks plugged in.
         */
        class IOStreamSequentialStream : public RuntimeClass<RuntimeClassFlags<ClassicCom>, ISequentialStream, IDispatch>
        {
            public:
                /**
                 * Init an instance of this class with stream to read or write from, the http client instance, the request that contains this stream,
                 * the response (only for Write mode), and the ratelimiter to use. Ratelimiter can be nullptr.
                 */
                IOStreamSequentialStream(Aws::IOStream& stream, const HttpClient& client,  const HttpRequest& request,
                            HttpRequestComHandle requestHandle, HttpResponse* response, Aws::Utils::RateLimits::RateLimiterInterface* rateLimiter)
                    : m_stream(stream), m_client(client), m_request(request), m_requestHandle(requestHandle), m_rateLimiter(rateLimiter), m_response(response) {}

                HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead) override
                {
                    if (!m_client.ContinueRequest(m_request) || !m_client.IsRequestProcessingEnabled())
                    {
                        m_requestHandle->Abort();
                    }

                    ULONG read = 0;

                    if (pv && cb > 0 && m_stream)
                    {
                        m_stream.read((char*)pv, static_cast<std::streamsize>(cb));
                        read = static_cast<ULONG>(m_stream.gcount());

                        auto dataSentHandler = m_request.GetDataSentEventHandler();
                        if (dataSentHandler)
                        {
                            dataSentHandler(&m_request, static_cast<long long>(read));
                        }

                        if (m_rateLimiter)
                        {
                            m_rateLimiter->ApplyAndPayForCost(static_cast<int64_t>(read));
                        }

                        AWS_LOGSTREAM_TRACE(CLASS_TAG, "Read " << read << " bytes from the request stream.");
                    }

                    if (pcbRead)
                    {
                        *pcbRead = read;
                    }

                    if (read < cb)
                    {
                        AWS_LOGSTREAM_TRACE(CLASS_TAG, "Read " << read
                            << " bytes from the request stream. Since this is less than was requested, the stream will send a fail flag.");
                        return S_FALSE;
                    }

                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG* pcbWritten) override
                {
                    if (!m_client.ContinueRequest(m_request) || !m_client.IsRequestProcessingEnabled())
                    {
                        m_requestHandle->Abort();
                    }

                    ULONG written = 0;

                    if (pv && cb > 0 && m_stream)
                    {
                        m_stream.write((const char*)pv, static_cast<std::streamsize>(cb));
                        written = cb;

                        auto dataReceivedHandler = m_request.GetDataReceivedEventHandler();
                        if (dataReceivedHandler)
                        {
                            dataReceivedHandler(&m_request, m_response, static_cast<long long>(written));
                        }

                        if (m_rateLimiter)
                        {
                            m_rateLimiter->ApplyAndPayForCost(static_cast<int64_t>(written));
                        }

                        AWS_LOGSTREAM_TRACE(CLASS_TAG, "Wrote " << written << " bytes to the response stream.");
                    }

                    if (pcbWritten)
                    {
                        *pcbWritten = written;
                    }

                    if (written < cb)
                    {
                        AWS_LOGSTREAM_WARN(CLASS_TAG, "Wrote " << written
                            << " bytes to the response stream. Which was less than requested. Failing the stream.");
                        return STG_E_CANTSAVE;
                    }

                    return S_OK;
                }

                HRESULT STDMETHODCALLTYPE GetTypeInfoCount(unsigned int FAR*) override { return E_NOTIMPL; }
                HRESULT STDMETHODCALLTYPE GetTypeInfo(unsigned int, LCID, ITypeInfo FAR* FAR*) override { return E_NOTIMPL; }
                HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID, OLECHAR FAR* FAR*, unsigned int, LCID, DISPID FAR*) override { return DISP_E_UNKNOWNNAME; }
                HRESULT STDMETHODCALLTYPE Invoke(DISPID, REFIID, LCID, WORD, DISPPARAMS FAR*, VARIANT FAR*, EXCEPINFO FAR*, unsigned int FAR*) override { return S_OK; }

            private:
                Aws::IOStream& m_stream;
                const HttpClient& m_client;
                const HttpRequest& m_request;
                HttpRequestComHandle m_requestHandle;
                HttpResponse* m_response;
                Aws::Utils::RateLimits::RateLimiterInterface* m_rateLimiter;

                LONG m_refCount;
        };

        /**
         * Callbacks for the http client. Handles lifecycle management for the client.
         */
        class IXmlHttpRequest2HttpClientCallbacks : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IXMLHTTPRequest2Callback>
        {
            public:
                /**
                 * Initialize callbacks with a response to fill in. If allow redirects will be used when a redirect is detected.
                 */
                IXmlHttpRequest2HttpClientCallbacks(HttpResponse& response, bool allowRedirects) :
                    m_response(response), m_allowRedirects(allowRedirects), m_isFinished(false) {}

                /**
                 * We currently have this turned off so it should never be called.
                 */
                HRESULT STDMETHODCALLTYPE OnDataAvailable(IXMLHTTPRequest2*, ISequentialStream*) override
                {
                    return S_OK;
                }

                /**
                 * When an error happens at the request level. This does not fire when the server sends a 400 or 500 level response.
                 */
                HRESULT STDMETHODCALLTYPE OnError(IXMLHTTPRequest2*, HRESULT res) override
                {
                    //timeout
                    if (res == 0x800c000b)
                    {
                        m_response.SetResponseCode(HttpResponseCode::REQUEST_TIMEOUT);
                    }
                    else if (FAILED(res))
                    {
                        m_response.SetResponseCode(HttpResponseCode::REQUEST_NOT_MADE);
                    }
                    else
                    {
                        m_response.SetResponseCode(HttpResponseCode::CLIENT_CLOSED_TO_REQUEST);
                    }

#ifdef PLATFORM_WINDOWS
                    _com_error err(res);
                    LPCTSTR errMsg = err.ErrorMessage();
                    AWS_LOGSTREAM_ERROR(CLASS_TAG, "Error while making request with code: " << errMsg);
#else
                    _com_error err(res, nullptr);
                    const wchar_t* errMsg = err.ErrorMessage();
                    AWS_LOGSTREAM_ERROR(CLASS_TAG, "Error while making request with code: " << errMsg);
#endif // PLATFORM_WINDOWS

                    m_isFinished = true;
                    m_completionSignal.notify_all();
                    return S_OK;
                }

                /**
                 * The beginning of the response has been received from the server. This method will set the response code and fill in the
                 * response headers.
                 */
                HRESULT STDMETHODCALLTYPE OnHeadersAvailable(IXMLHTTPRequest2 *pXHR, DWORD dwStatus, const WCHAR*) override
                {
                    m_response.SetResponseCode(static_cast<HttpResponseCode>(dwStatus));
                    AWS_LOGSTREAM_DEBUG(CLASS_TAG, "Response received with code " << dwStatus);
                    if (pXHR)
                    {
                        wchar_t* headers = nullptr;
                        HRESULT hr = pXHR->GetAllResponseHeaders(&headers);
                        if (SUCCEEDED(hr))
                        {
                            AWS_LOGSTREAM_TRACE(CLASS_TAG, "Reading response headers:");
                            auto unparsedHeadersStr = Aws::Utils::StringUtils::FromWString(headers);
                            for (auto& headerPair : Aws::Utils::StringUtils::SplitOnLine(unparsedHeadersStr))
                            {
                                Aws::Vector<Aws::String>&& keyValue = Aws::Utils::StringUtils::Split(headerPair, ':', 2);
                                if (keyValue.size() == 2)
                                {
                                    AWS_LOGSTREAM_TRACE(CLASS_TAG, keyValue[0] << ": " << keyValue[1]);
                                    m_response.AddHeader(Aws::Utils::StringUtils::Trim(keyValue[0].c_str()), Aws::Utils::StringUtils::Trim(keyValue[1].c_str()));
                                }
                            }
                        }
                    }

                    return S_OK;
                }

                /**
                 * If allowRedirects was true, then this will allow the request to continue. Otherwise the request will be aborted.
                 */
                HRESULT STDMETHODCALLTYPE OnRedirect(IXMLHTTPRequest2* pXHR, const WCHAR* url) override
                {
                    AWS_LOGSTREAM_INFO(CLASS_TAG, "Redirect to url " << url << " detected");
                    if (pXHR && !m_allowRedirects)
                    {
                        pXHR->Abort();
                    }

                    return S_OK;
                }

                /**
                 * Request/Response cycle finished successfully.
                 */
                HRESULT STDMETHODCALLTYPE OnResponseReceived(IXMLHTTPRequest2*, ISequentialStream*) override
                {
                    AWS_LOGSTREAM_DEBUG(CLASS_TAG, "Response received.");
                    m_isFinished = true;
                    m_completionSignal.notify_all();
                    return S_OK;
                }

                /**
                 * This API is asynchronous, to use the APi synchronously, call this after calling Send().
                 */
                void WaitUntilFinished()
                {
                    AWS_LOGSTREAM_DEBUG(CLASS_TAG, "Waiting for request to finish.");
                    std::unique_lock<std::mutex> completionLock(m_completionMutex);
                    m_completionSignal.wait(completionLock, [this]() { return m_isFinished.load(); });
                    AWS_LOGSTREAM_DEBUG(CLASS_TAG, "Request completed, continueing thread.");
                }


            private:
                HttpResponse& m_response;
                bool m_allowRedirects;
                std::condition_variable m_completionSignal;
                std::mutex m_completionMutex;
                std::atomic<bool> m_isFinished;
        };

        void IXmlHttpRequest2HttpClient::InitCOM()
        {
            AWS_LOGSTREAM_INFO(CLASS_TAG, "Initializing COM with flag RO_INIT_MULTITHREADED");
            Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
        }

        void IXmlHttpRequest2HttpClient::ReturnHandleToResourceManager() const
        {
            HttpRequestComHandle handle;
#ifdef PLATFORM_WINDOWS
            CoCreateInstance(CLSID_FreeThreadedXMLHTTP60, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&handle));
#else
            CoCreateInstance(CLSID_FreeThreadedXMLHTTP60, nullptr, CLSCTX_SERVER, IID_PPV_ARGS(&handle));
#endif // PLATFORM_WINDOWS
            m_resourceManager.Release(handle);
        }

        IXmlHttpRequest2HttpClient::IXmlHttpRequest2HttpClient(const Aws::Client::ClientConfiguration& clientConfig) :
            m_proxyUserName(clientConfig.proxyUserName), m_proxyPassword(clientConfig.proxyPassword), m_poolSize(clientConfig.maxConnections),
            m_followRedirects(clientConfig.followRedirects), m_verifySSL(clientConfig.verifySSL), m_totalTimeoutMs(clientConfig.requestTimeoutMs + clientConfig.connectTimeoutMs)
        {
            //user defined proxy not supported on this interface, this has to come from the default settings.
            assert(clientConfig.proxyHost.empty());
            AWS_LOGSTREAM_INFO(CLASS_TAG, "Initializing client with pool size of " << clientConfig.maxConnections);

            for (unsigned int i = 0; i < m_poolSize; ++i)
            {
                HttpRequestComHandle handle;
                auto hrResult = CoCreateInstance(CLSID_FreeThreadedXMLHTTP60,
                                                    nullptr,
#ifdef PLATFORM_WINDOWS
                                                    CLSCTX_INPROC_SERVER,
#else
                                                    CLSCTX_SERVER,
#endif
                                                    IID_PPV_ARGS(&handle));
                if (!FAILED(hrResult))
                {
                    m_resourceManager.PutResource(handle);
                }
                else
                {
                    AWS_LOGSTREAM_FATAL(CLASS_TAG, "Unable to create IXmlHttpRequest2 instance with status code " << hrResult);
                    assert(0);
                }
            }
        }

        IXmlHttpRequest2HttpClient::~IXmlHttpRequest2HttpClient()
        {
            //we don't actually need to do anything with these.
            //just make sure they are dereferenced. The comptr destructor
            //will handle the release calls.
            m_resourceManager.ShutdownAndWait(m_poolSize);
        }

        std::shared_ptr<HttpResponse> IXmlHttpRequest2HttpClient::MakeRequest(HttpRequest& request,
                Aws::Utils::RateLimits::RateLimiterInterface* readLimiter,
                Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter) const
        {
            std::shared_ptr<HttpResponse> response = Aws::MakeShared<Standard::StandardHttpResponse>(CLASS_TAG, request);
            MakeRequestInternal(request, response, readLimiter, writeLimiter);
            return response;
        }

        std::shared_ptr<HttpResponse> IXmlHttpRequest2HttpClient::MakeRequest(const std::shared_ptr<HttpRequest>& request,
                                        Aws::Utils::RateLimits::RateLimiterInterface* readLimiter,
                                        Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter) const
        {
            std::shared_ptr<HttpResponse> response = Aws::MakeShared<Standard::StandardHttpResponse>(CLASS_TAG, request);
            MakeRequestInternal(*request, response, readLimiter, writeLimiter);
            return response;
        }

        void IXmlHttpRequest2HttpClient::MakeRequestInternal(HttpRequest& request,
                                        std::shared_ptr<HttpResponse>& response,
                                        Aws::Utils::RateLimits::RateLimiterInterface* readLimiter,
                                        Aws::Utils::RateLimits::RateLimiterInterface* writeLimiter) const
        {
            auto uri = request.GetUri();
            auto fullUriString = uri.GetURIString();
            AWS_LOGSTREAM_DEBUG(CLASS_TAG, "Making " << HttpMethodMapper::GetNameForHttpMethod(request.GetMethod())
                        << " request to url: " << fullUriString);

            auto url = Aws::Utils::StringUtils::ToWString(fullUriString.c_str());
            auto methodStr = Aws::Utils::StringUtils::ToWString(HttpMethodMapper::GetNameForHttpMethod(request.GetMethod()));
            auto proxyUserNameStr = Aws::Utils::StringUtils::ToWString(m_proxyUserName.c_str());
            auto proxyPasswordStr = Aws::Utils::StringUtils::ToWString(m_proxyPassword.c_str());

            auto requestHandle = m_resourceManager.Acquire();

            ComPtr<IXmlHttpRequest2HttpClientCallbacks> callbacks = Make<IXmlHttpRequest2HttpClientCallbacks>(*response, m_followRedirects);

            HRESULT hrResult = requestHandle->Open(methodStr.c_str(), url.c_str(), callbacks.Get(), nullptr, nullptr, proxyUserNameStr.c_str(), proxyPasswordStr.c_str());
            FillClientSettings(requestHandle);

            if (FAILED(hrResult))
            {
                Aws::StringStream ss;
                ss << "Error opening http request with status code " << hrResult;
                AWS_LOGSTREAM_ERROR(CLASS_TAG, ss.str());
                AWS_LOGSTREAM_DEBUG(CLASS_TAG, "The http request is: " << uri.GetURIString());
                response->SetClientErrorType(Aws::Client::CoreErrors::NETWORK_CONNECTION);
                response->SetClientErrorMessage(ss.str());
                ReturnHandleToResourceManager();
                return;
            }

            AWS_LOGSTREAM_TRACE(CLASS_TAG, "Setting http headers:");
            for (auto& header : request.GetHeaders())
            {
                AWS_LOGSTREAM_TRACE(CLASS_TAG, header.first << ": " << header.second);
                hrResult = requestHandle->SetRequestHeader(Aws::Utils::StringUtils::ToWString(header.first.c_str()).c_str(),
                                                                Aws::Utils::StringUtils::ToWString(header.second.c_str()).c_str());

                if (FAILED(hrResult))
                {
                    Aws::StringStream ss;
                    ss << "Error setting http header " << header.first << " With status code: " << hrResult;
                    AWS_LOGSTREAM_ERROR(CLASS_TAG, ss.str());
                    AWS_LOGSTREAM_DEBUG(CLASS_TAG, "Corresponding header's value is: " << header.second);
                    response->SetClientErrorType(Aws::Client::CoreErrors::NETWORK_CONNECTION);
                    response->SetClientErrorMessage(ss.str());
                    ReturnHandleToResourceManager();
                    return;
                }
            }

            if (writeLimiter)
            {
                writeLimiter->ApplyAndPayForCost(request.GetSize());
            }

            ComPtr<IOStreamSequentialStream> responseStream = Make<IOStreamSequentialStream>(response->GetResponseBody(), *this, request,
                            requestHandle, response.get(), writeLimiter);

            requestHandle->SetCustomResponseStream(responseStream.Get());

            ComPtr<IOStreamSequentialStream> requestStream(nullptr);
            ULONGLONG streamLength(0);

            if (request.GetContentBody() && !request.GetContentLength().empty())
            {
                AWS_LOGSTREAM_TRACE(CLASS_TAG, "Content detected, setting request stream.");
                requestStream = Make<IOStreamSequentialStream>(*request.GetContentBody(), *this, request, requestHandle, nullptr, readLimiter);
                streamLength = static_cast<ULONGLONG>(Aws::Utils::StringUtils::ConvertToInt64(request.GetContentLength().c_str()));
            }

            hrResult = requestHandle->Send(requestStream.Get(), streamLength);
            callbacks->WaitUntilFinished();
            if (FAILED(hrResult) || response->GetResponseCode() == Http::HttpResponseCode::REQUEST_NOT_MADE)
            {
                Aws::StringStream ss;
                ss << "Request finished with response code: " << static_cast<int>(response->GetResponseCode());
                AWS_LOGSTREAM_ERROR(CLASS_TAG, ss.str());
                response->SetClientErrorType(Aws::Client::CoreErrors::NETWORK_CONNECTION);
                response->SetClientErrorMessage(ss.str());
            }
            else
            {
                response->GetResponseBody().flush();
                AWS_LOGSTREAM_DEBUG(CLASS_TAG, "Request finished with response code: " << static_cast<int>(response->GetResponseCode()));
            }
            ReturnHandleToResourceManager();
        }

        void IXmlHttpRequest2HttpClient::FillClientSettings(const HttpRequestComHandle& handle) const
        {
            AWS_LOGSTREAM_TRACE(CLASS_TAG, "Setting up request handle with verifySSL = " << m_verifySSL
                            << " ,follow redirects = " << m_followRedirects << " and timeout = " << m_totalTimeoutMs);
            handle->SetProperty(XHR_PROP_NO_DEFAULT_HEADERS, TRUE);
            handle->SetProperty(XHR_PROP_REPORT_REDIRECT_STATUS, m_followRedirects);
            handle->SetProperty(XHR_PROP_NO_CRED_PROMPT, TRUE);
            handle->SetProperty(XHR_PROP_NO_CACHE, TRUE);
            handle->SetProperty(XHR_PROP_TIMEOUT, m_totalTimeoutMs);

            // These appear to not be defined for win8, but it should be safe to set them unconditionally, they just won't
            // have an affect
            // XHR_PROP_ONDATA_THRESHOLD = 0x9
            // XHR_PROP_ONDATA_NEVER = (uint64_t) -1
            handle->SetProperty(static_cast<XHR_PROPERTY>(0x9), static_cast<uint64_t>(-1));

#ifdef PLATFORM_WINDOWS
            handle->SetProperty(XHR_PROP_IGNORE_CERT_ERRORS, !m_verifySSL);
#endif // PLATFORM_WINDOWS
        }
    }
}
