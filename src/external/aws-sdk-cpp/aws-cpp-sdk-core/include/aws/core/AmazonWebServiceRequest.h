/*
  * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/core/utils/memory/stl/AWSFunction.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/UnreferencedParam.h>
#include <aws/core/http/HttpTypes.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/core/utils/stream/ResponseStream.h>

namespace Aws
{
    namespace Http
    {
        class URI;
    } // namespace Http

    /**
     * Base level abstraction for all modeled AWS requests
     */
    class AWS_CORE_API AmazonWebServiceRequest
    {
    public:
        /**
         * Sets up default response stream factory. initializes other pointers to nullptr.
         */
        AmazonWebServiceRequest();
        virtual ~AmazonWebServiceRequest() = default;

        /**
         * Get the payload for the request
         */
        virtual std::shared_ptr<Aws::IOStream> GetBody() const = 0;
        /**
         * Get the headers for the request
         */
        virtual Aws::Http::HeaderValueCollection GetHeaders() const = 0;
        /**
         * Do nothing virtual, override this to add query strings to the request
         */
        virtual void AddQueryStringParameters(Aws::Http::URI& uri) const { AWS_UNREFERENCED_PARAM(uri); }
        /**
         * Retrieves the factory for creating response streams.
         */
        const Aws::IOStreamFactory& GetResponseStreamFactory() const { return m_responseStreamFactory; }
        /**
         * Set the response stream factory.
         */
        void SetResponseStreamFactory(const Aws::IOStreamFactory& factory) { m_responseStreamFactory = AWS_BUILD_FUNCTION(factory); }
        /**
         * Register closure for data recieved event.
         */
        inline virtual void SetDataReceivedEventHandler(const Aws::Http::DataReceivedEventHandler& dataReceivedEventHandler) { m_onDataReceived = dataReceivedEventHandler; }
        /**
         * register closure for data sent event
         */
        inline virtual void SetDataSentEventHandler(const Aws::Http::DataSentEventHandler& dataSentEventHandler) { m_onDataSent = dataSentEventHandler; }
        /**
        * Register closure for data recieved event.
        */
        inline virtual void SetDataReceivedEventHandler(Aws::Http::DataReceivedEventHandler&& dataReceivedEventHandler) { m_onDataReceived = dataReceivedEventHandler; }
        /**
        * register closure for data sent event
        */
        inline virtual void SetDataSentEventHandler(Aws::Http::DataSentEventHandler&& dataSentEventHandler) { m_onDataSent = dataSentEventHandler; }
        /**
        * get closure for data recieved event.
        */
        inline virtual const Aws::Http::DataReceivedEventHandler& GetDataReceivedEventHandler() const { return m_onDataReceived; }
        /**
        * get closure for data sent event
        */
        inline virtual const Aws::Http::DataSentEventHandler& GetDataSentEventHandler() const { return m_onDataSent; }
        /**
         * If this is set to true, content-md5 needs to be computed and set on the request
         */
        inline virtual bool ShouldComputeContentMd5() const { return false; }


    private:

        Aws::IOStreamFactory m_responseStreamFactory;

        Aws::Http::DataReceivedEventHandler m_onDataReceived;
        Aws::Http::DataSentEventHandler m_onDataSent;
    };

} // namespace Aws

