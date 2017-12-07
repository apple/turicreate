/*
  * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/base64/Base64.h>
#include <aws/core/utils/crypto/Sha256.h>
#include <aws/core/utils/crypto/Sha256HMAC.h>
#include <aws/core/utils/crypto/MD5.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <iomanip>

using namespace Aws::Utils;
using namespace Aws::Utils::Base64;
using namespace Aws::Utils::Crypto;

// internal buffers are fixed-size arrays, so this is harmless memory-management wise
static Aws::Utils::Base64::Base64 s_base64;

Aws::String HashingUtils::Base64Encode(const ByteBuffer& message)
{
    return s_base64.Encode(message);
}

ByteBuffer HashingUtils::Base64Decode(const Aws::String& encodedMessage)
{
    return s_base64.Decode(encodedMessage);
}

ByteBuffer HashingUtils::CalculateSHA256HMAC(const ByteBuffer& toSign, const ByteBuffer& secret)
{
    Sha256HMAC hash;
    return hash.Calculate(toSign, secret).GetResult();
}

ByteBuffer HashingUtils::CalculateSHA256(const Aws::String& str)
{
    Sha256 hash;
    return hash.Calculate(str).GetResult();
}

ByteBuffer HashingUtils::CalculateSHA256(Aws::IOStream& stream)
{
    Sha256 hash;
    return hash.Calculate(stream).GetResult();
}

Aws::String HashingUtils::HexEncode(const ByteBuffer& message)
{
    Aws::StringStream ss;

    for (unsigned i = 0; i < message.GetLength(); ++i)
    {
        ss << std::hex << std::setw(2) << std::setfill('0')
            << (unsigned int) message[i];
    }

    return ss.str();
}

ByteBuffer HashingUtils::HexDecode(const Aws::String& str)
{
    //number of characters should be even
    assert(str.length() % 2 == 0);
    assert(str.length() >= 2);

    if(str.length() < 2 || str.length() % 2 != 0)
    {
        return ByteBuffer();
    }

    size_t strLength = str.length();
    size_t readIndex = 0;

    if(str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        strLength -= 2;
        readIndex = 2;
    }

    ByteBuffer hexBuffer(strLength / 2);
    size_t bufferIndex = 0;

    for (size_t i = readIndex; i < str.length(); i += 2)
    {
        if(!isalnum(str[i]) || !isalnum(str[i + 1]))
        {
            //contains non-hex characters
            assert(0);
        }

        char firstChar = str[i];
        uint8_t distance = firstChar - '0';

        if(isalpha(firstChar))
        {
            firstChar = static_cast<char>(toupper(firstChar));
            distance = firstChar - 'A' + 10;
        }

        unsigned char val = distance * 16;

        char secondChar = str[i + 1];
        distance = secondChar - '0';

        if(isalpha(secondChar))
        {
            secondChar = static_cast<char>(toupper(secondChar));
            distance = secondChar - 'A' + 10;
        }

        val += distance;
        hexBuffer[bufferIndex++] = val;
    }

    return hexBuffer;
}

ByteBuffer HashingUtils::CalculateMD5(const Aws::String& str)
{
    MD5 hash;
    return hash.Calculate(str).GetResult();
}

ByteBuffer HashingUtils::CalculateMD5(Aws::IOStream& stream)
{
    MD5 hash;
    return hash.Calculate(stream).GetResult();
}

int HashingUtils::HashString(const char* strToHash)
{
    if (!strToHash)
        return 0;

    int hash = 0;
    while (char charValue = *strToHash++)
    {
        hash = charValue + 31 * hash;    
    }

    return hash;
}