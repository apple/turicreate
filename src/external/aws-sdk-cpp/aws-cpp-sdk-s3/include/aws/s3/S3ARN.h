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

#pragma once

#include <aws/s3/S3_EXPORTS.h>

#include <aws/core/client/AWSError.h>
#include <aws/core/utils/ARN.h>
#include <aws/s3/S3Errors.h>

namespace Aws
{
    namespace Utils
    {
        template<typename R, typename E> class Outcome;
    }

    namespace S3
    {
        namespace ARNResourceType
        {
            static const char ACCESSPOINT[] = "accesspoint";
        }

        typedef Aws::Utils::Outcome<bool, Aws::Client::AWSError<S3Errors>> S3ARNOutcome;

        class AWS_S3_API S3ARN : public Aws::Utils::ARN
        {
        public:
            S3ARN(const Aws::String& arn);

            const Aws::String& GetResourceType() const { return m_resourceType; }
            const Aws::String& GetResourceId() const { return m_resourceId; }
            const Aws::String& GetResourceQualifier() const { return m_resourceQualifier; }

            // Check if S3ARN is valid.
            S3ARNOutcome Validate() const;
            // Check if S3ARN is valid, and especially, ARN region should match the region specified.
            S3ARNOutcome Validate(const char* region) const;

        private:
            void ParseARNResource();

            Aws::String m_resourceType;
            Aws::String m_resourceId;
            Aws::String m_resourceQualifier;
        };
    }
}