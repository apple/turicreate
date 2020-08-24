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

#include <aws/core/utils/UUID.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/crypto/Factories.h>
#include <aws/core/utils/crypto/SecureRandom.h>
#include <iomanip>

namespace Aws
{
    namespace Utils
    {
        static const size_t UUID_STR_SIZE = 0x24u; // 36 characters
        static const size_t VERSION_LOCATION = 0x06u;
        static const size_t VARIANT_LOCATION = 0x08u;
        static const unsigned char VERSION = 0x40u;
        static const unsigned char VERSION_MASK = 0x0Fu;
        static const unsigned char VARIANT = 0x80u;
        static const unsigned char VARIANT_MASK = 0x3Fu;

        static void hexify(Aws::String& ss, const unsigned char* toWrite, size_t min, size_t max)
        {
            for (size_t i = min; i < max; ++i)
            {
                ss.push_back("0123456789ABCDEF"[toWrite[i] >> 4]);
                ss.push_back("0123456789ABCDEF"[toWrite[i] & 0x0F]);
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

        UUID::operator Aws::String() const
        {
            Aws::String ss;
            ss.reserve(UUID_STR_SIZE);
            hexify(ss, m_uuid, 0, 4);
            ss.push_back('-');

            hexify(ss, m_uuid, 4, 6);
            ss.push_back('-');

            hexify(ss, m_uuid, 6, 8);
            ss.push_back('-');

            hexify(ss, m_uuid, 8, 10);
            ss.push_back('-');

            hexify(ss, m_uuid, 10, 16);

            return ss;
        }

        UUID UUID::RandomUUID()
        {
            auto secureRandom = Crypto::CreateSecureRandomBytesImplementation();
            assert(secureRandom);

            unsigned char randomBytes[UUID_BINARY_SIZE];
            memset(randomBytes, 0, UUID_BINARY_SIZE);
            secureRandom->GetBytes(randomBytes, UUID_BINARY_SIZE);
            //Set version bits to 0100
            //https://tools.ietf.org/html/rfc4122#section-4.1.3
            randomBytes[VERSION_LOCATION] = (randomBytes[VERSION_LOCATION] & VERSION_MASK) | VERSION;
            //set variant bits to 10
            //https://tools.ietf.org/html/rfc4122#section-4.1.1
            randomBytes[VARIANT_LOCATION] = (randomBytes[VARIANT_LOCATION] & VARIANT_MASK) | VARIANT;

            return UUID(randomBytes);
        }
    }
}
