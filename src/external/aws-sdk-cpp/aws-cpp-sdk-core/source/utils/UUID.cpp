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

#include <aws/core/utils/UUID.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/StringUtils.h>
#include <iomanip>

namespace Aws
{
    namespace Utils
    {
        void WriteRangeOutToStream(Aws::StringStream& ss, unsigned char* toWrite, size_t min, size_t max)
        {
            for (size_t i = min; i < max; ++i)
            {
                ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2)
                    << (unsigned int)toWrite[i];
            }
        }

        UUID::UUID(const Aws::String& uuidToConvert)
        {
            //GUID has 2 characters per byte + 4 dashes = 36 bytes
            assert(uuidToConvert.length() == UUID_STR_SIZE);
            memset(m_uuid, 0, sizeof(m_uuid));
            Aws::String escapedHexStr(uuidToConvert);
            StringUtils::Replace(escapedHexStr, "-", "");
            assert(escapedHexStr.length() == UUID_BINARY_SIZE * 2);
            ByteBuffer&& rawUuid = HashingUtils::HexDecode(escapedHexStr);
            memcpy(m_uuid, rawUuid.GetUnderlyingData(), rawUuid.GetLength());
        }

        UUID::UUID(const unsigned char toCopy[UUID_BINARY_SIZE])
        {
            memcpy(m_uuid, toCopy, sizeof(m_uuid));
        }

        UUID::operator Aws::String()
        {
            Aws::StringStream ss;
            WriteRangeOutToStream(ss, m_uuid, 0, 4);
            ss << "-";

            WriteRangeOutToStream(ss, m_uuid, 4, 6);
            ss << "-";

            WriteRangeOutToStream(ss, m_uuid, 6, 8);
            ss << "-";

            WriteRangeOutToStream(ss, m_uuid, 8, 10);
            ss << "-";

            WriteRangeOutToStream(ss, m_uuid, 10, 16);
            
            return ss.str();
        }        
    }
}