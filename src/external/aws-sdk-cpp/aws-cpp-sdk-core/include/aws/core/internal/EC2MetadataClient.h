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
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <memory>

namespace Aws
{
    namespace Http
    {
        class HttpClient;
        class HttpClientFactory;
    } // namespace Http

    namespace Internal
    {
        /**
         * Simple client for accessing the Amazon EC2 Instance Metadata Service.
         */
        class AWS_CORE_API EC2MetadataClient
        {
        public:
            /**
             * Builds an instance using "http://169.254.169.254" as the endpoint if not specified otherwise,
             * and the default http stack if httpClientFactory is not specified.
             */
            EC2MetadataClient(const char* endpoint = "http://169.254.169.254");

            virtual ~EC2MetadataClient();

            /**
             * Connects to the Amazon EC2 Instance Metadata Service to retrieve the
             * default credential information (if any).
             */
            virtual Aws::String GetDefaultCredentials() const;

            /**
             * connects to the Amazon EC2 Instance metadata Service to retrieve the region
             * the current EC2 instance is running in.
             */
            virtual Aws::String GetCurrentRegion() const;

            /**
             * Connects to the metadata service to read the specified resource and
             * returns the text contents.
             */
            virtual Aws::String GetResource(const char* resource) const;

        private:

            EC2MetadataClient &operator =(const EC2MetadataClient &rhs);

            std::shared_ptr<Http::HttpClient> m_httpClient;
            std::shared_ptr<Http::HttpClientFactory const> m_httpClientFactory;
            Aws::String m_endpoint;
        };

    } // namespace Internal
} // namespace Aws
