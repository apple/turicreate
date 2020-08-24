/*
  * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <aws/core/utils/memory/stl/AWSString.h>

namespace Aws
{
    namespace Utils
    {
        /**
         * ARN (Amazon Resource Name) is used to identify an unique resource on AWS.
         * A full qualified ARN has two forms:
         * 1. arn:partition:service:region:account-id:resource-type:resource-id:qualifier
         * 2. arn:partition:service:region:account-id:resource-type/resource-id/qualifier
         * Different services have different resource definition, here we treat anything
         * after "[account-id]:" as resource. Service should have their own resource parser.
         */
        class AWS_CORE_API ARN
        {
        public:
            ARN(const Aws::String& arnString);
            /**
             * return if the ARN is valid after construction.
             */
            explicit operator bool() const { return m_valid; }

            /**
             * Get the originating arn string.
             */
            const Aws::String& GetARNString() const { return m_arnString; }

            const Aws::String& GetPartition() const { return m_partition; }

            const Aws::String& GetService() const { return m_service; }

            const Aws::String& GetRegion() const { return m_region; }

            const Aws::String& GetAccountId() const { return m_accountId; }

            const Aws::String& GetResource() const { return m_resource; }

        private:
            Aws::String m_arnString;
            Aws::String m_partition;
            Aws::String m_service;
            Aws::String m_region;
            Aws::String m_accountId;
            Aws::String m_resource;

            bool m_valid;
        };
    }
}