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

#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpRequest.h>

using namespace Aws;
using namespace Aws::Http;

HttpClient::HttpClient() :
    m_disableRequestProcessing( false ),
    m_requestProcessingSignalLock(),
    m_requestProcessingSignal()
{
}

void HttpClient::DisableRequestProcessing() 
{ 
    m_disableRequestProcessing = true;
    m_requestProcessingSignal.notify_all();
}

void HttpClient::EnableRequestProcessing() 
{ 
    m_disableRequestProcessing = false; 
}

bool HttpClient::IsRequestProcessingEnabled() const 
{ 
    return m_disableRequestProcessing.load() == false; 
}

void HttpClient::RetryRequestSleep(std::chrono::milliseconds sleepTime) 
{
    std::unique_lock< std::mutex > signalLocker(m_requestProcessingSignalLock);
    m_requestProcessingSignal.wait_for(signalLocker, sleepTime, [this](){ return m_disableRequestProcessing.load() == true; });
}

bool HttpClient::ContinueRequest(const Aws::Http::HttpRequest& request) const
{
    if (request.GetContinueRequestHandler())
    {
        return request.GetContinueRequestHandler()(&request);
    }

    return true;
}
