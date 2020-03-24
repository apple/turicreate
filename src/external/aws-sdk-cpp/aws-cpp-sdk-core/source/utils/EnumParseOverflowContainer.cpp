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

#include <aws/core/utils/EnumParseOverflowContainer.h>
#include <aws/core/utils/logging/LogMacros.h>

using namespace Aws::Utils;
using namespace Aws::Utils::Threading;

static const char LOG_TAG[] = "EnumParseOverflowContainer";

const Aws::String& EnumParseOverflowContainer::RetrieveOverflow(int hashCode) const
{
    ReaderLockGuard guard(m_overflowLock);
    auto foundIter = m_overflowMap.find(hashCode);
    if (foundIter != m_overflowMap.end())
    {
        AWS_LOGSTREAM_DEBUG(LOG_TAG, "Found value " << foundIter->second << " for hash " << hashCode << " from enum overflow container.");
        return foundIter->second;
    }

    AWS_LOGSTREAM_ERROR(LOG_TAG, "Could not find a previously stored overflow value for hash " << hashCode << ". This will likely break some requests.");
    return m_emptyString;
}

void EnumParseOverflowContainer::StoreOverflow(int hashCode, const Aws::String& value)
{
    WriterLockGuard guard(m_overflowLock);
    AWS_LOGSTREAM_WARN(LOG_TAG, "Encountered enum member " << value << " which is not modeled in your clients. You should update your clients when you get a chance.");
    m_overflowMap[hashCode] = value;
}
