/*
  * Copyright 2010-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <aws/core/utils/DateTime.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/internal/AWSHttpResourceClient.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <memory>

namespace Aws
{
    namespace Auth
    {
        /**
         * To support retrieving credentials of STS AssumeRole with web identity.
         * Note that STS accepts request with protocol of queryxml. Calling GetAWSCredentials() will trigger (if expired)
         * a query request using AWSHttpResourceClient under the hood.
         */
        class AWS_CORE_API STSAssumeRoleWebIdentityCredentialsProvider : public AWSCredentialsProvider
        {
        public:
            STSAssumeRoleWebIdentityCredentialsProvider();

            /**
             * Retrieves the credentials if found, otherwise returns empty credential set.
             */
            AWSCredentials GetAWSCredentials() override;

        protected:
            void Reload() override;

        private:
            void RefreshIfExpired();
            Aws::String CalculateQueryString() const;

            Aws::UniquePtr<Aws::Internal::STSCredentialsClient> m_client;
            Aws::Auth::AWSCredentials m_credentials;
            Aws::String m_roleArn;
            Aws::String m_tokenFile;
            Aws::String m_sessionName;
            Aws::String m_token;
            bool m_initialized;
        };
    } // namespace Auth
} // namespace Aws
