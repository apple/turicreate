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

#include <aws/core/utils/ARN.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/logging/LogMacros.h>

namespace Aws
{
    namespace Utils
    {
        ARN::ARN(const Aws::String& arnString)
        {
            m_valid = false;

            // An ARN can be identified as any string starting with arn: with 6 defined segments each separated by a :
            const auto result = StringUtils::Split(arnString, ':', StringUtils::SplitOptions::INCLUDE_EMPTY_ENTRIES);

            if (result.size() < 6)
            {
                return;
            }

            if (result[0] != "arn")
            {
                return;
            }

            m_arnString = arnString;
            m_partition = result[1];
            m_service = result[2];
            m_region = result[3];
            m_accountId = result[4];
            m_resource = result[5];

            for (size_t i = 6; i < result.size(); i++)
            {
                m_resource += ":" + result[i];
            }

            m_valid = true;
        }
    }
}