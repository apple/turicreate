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

#include <cassert>
#include <aws/core/utils/DNS.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/s3/S3ARN.h>

namespace Aws
{
    namespace S3
    {
        S3ARN::S3ARN(const Aws::String& arn) : Utils::ARN(arn)
        {
            ParseARNResource();
        }

        S3ARNOutcome S3ARN::Validate(const char* clientRegion) const
        {
            // Take pseudo region into consideration here.
            if (this->GetRegion() != clientRegion && "fips-" + this->GetRegion() != clientRegion && this->GetRegion() + "-fips" != clientRegion)
            {
                Aws::StringStream ss;
                ss << "Region mismatch between \"" << this->GetRegion() << "\" defined in ARN and \""
                    << clientRegion << "\" defined in client configuration. "
                    << "You can specify AWS_S3_USE_ARN_REGION to ignore region defined in client configuration.";
                return S3ARNOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::VALIDATION, "VALIDATION", ss.str(), false));
            }
            else
            {
                return Validate();
            }
        }

        S3ARNOutcome S3ARN::Validate() const
        {
            Aws::String errorMessage;
            bool success = false;
            Aws::StringStream ss;

            if (!*this)
            {
                errorMessage = "Invalid ARN.";
            }
            // Validation on partition.
            else if (this->GetPartition().find("aws") != 0)
            {
                ss.str("");
                ss << "Invalid partition in ARN: " << this->GetPartition() << ". Valid options: aws, aws-cn, and etc.";
            }
            // Validation on service.
            else if (this->GetService() != "s3")
            {
                ss.str("");
                ss << "Invalid service in ARN: " << this->GetService() << ". Valid options: s3";
                errorMessage = ss.str();
            }
            // Validation on region.
            // TODO: Failure on different partitions.
            else if (this->GetRegion().empty())
            {
                errorMessage = "Invalid ARN with empty region.";
            }
            else if (!Utils::IsValidDnsLabel(this->GetRegion()))
            {
                ss.str("");
                ss << "Invalid region in ARN: " << this->GetRegion() << ". Region should be a RFC 3986 Host label.";
                errorMessage = ss.str();
            }
            // Validation on account ID
            else if (!Utils::IsValidDnsLabel(this->GetAccountId()))
            {
                ss.str("");
                ss << "Invalid account ID in ARN: " << this->GetAccountId() << ". Account ID should be a RFC 3986 Host label.";
                errorMessage = ss.str();
            }
            // Validation on resource.
            else if (this->GetResourceType() != ARNResourceType::ACCESSPOINT)
            {
                ss.str("");
                ss << "Invalid resource type in ARN: " << this->GetResourceType() << ". Valid options: " << ARNResourceType::ACCESSPOINT;
                errorMessage = ss.str();
            }
            else if (this->GetResourceId().empty())
            {
                errorMessage = "Invalid Access Point ARN with empty resource ID.";
            }
            else if (!Utils::IsValidDnsLabel(this->GetResourceId()))
            {
                ss.str("");
                ss << "Invalid resource ID in Access Point ARN: " << this->GetResourceId() << ". Resource ID should be a RFC 3986 Host label.";
                errorMessage = ss.str();
            }
            else if (!this->GetResourceQualifier().empty())
            {
                errorMessage = "Invalid Access Point ARN with non empty resource qualifier.";
            }
            else
            {
                success = true;
            }

            if (success)
            {
                return S3ARNOutcome(success);
            }
            else
            {
                return S3ARNOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::VALIDATION, "VALIDATION", errorMessage, false));
            }
        }

        void S3ARN::ParseARNResource()
        {
            if (!*this) return;

            Aws::String resource = this->GetResource();
            Aws::Vector<Aws::String> resourceSegments;
            if (resource.find(':') != std::string::npos)
            {
                resourceSegments = Utils::StringUtils::Split(resource, ':', 3, Utils::StringUtils::SplitOptions::INCLUDE_EMPTY_ENTRIES);
            }
            else if (resource.find('/') != std::string::npos)
            {
                resourceSegments = Utils::StringUtils::Split(resource, '/', 3, Utils::StringUtils::SplitOptions::INCLUDE_EMPTY_ENTRIES);
            }
            else
            {
                resourceSegments.emplace_back(resource);
            }

            switch (resourceSegments.size())
            {
            case 1:
                m_resourceId = resourceSegments[0];
                break;
            case 2:
                m_resourceType = resourceSegments[0];
                m_resourceId = resourceSegments[1];
                break;
            case 3:
                m_resourceType = resourceSegments[0];
                m_resourceId = resourceSegments[1];
                m_resourceQualifier = resourceSegments[2];
                break;
            default:
                assert(false);
                break;
            }
        }
    }
}
