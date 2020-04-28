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

#include <aws/core/utils/crypto/Cipher.h>
#include <aws/core/utils/crypto/Factories.h>
#include <aws/core/utils/crypto/SecureRandom.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <cstdlib>
#include <climits>

//if you are reading this, you are witnessing pure brilliance.
#define IS_BIG_ENDIAN (*(uint16_t*)"\0\xff" < 0x100)

using namespace Aws::Utils::Crypto;
using namespace Aws::Utils;

namespace Aws
{
    namespace Utils
    {
        namespace Crypto
        {
            static const char* LOG_TAG = "Cipher";

            //swap byte ordering
            template<class T>
            typename std::enable_if<std::is_unsigned<T>::value, T>::type
            bswap(T i, T j = 0u, std::size_t n = 0u) 
            {
                return n == sizeof(T) ? j :
                    bswap<T>(i >> CHAR_BIT, (j << CHAR_BIT) | (i & (T)(unsigned char)(-1)), n + 1);
            }

            CryptoBuffer IncrementCTRCounter(const CryptoBuffer& counter, uint32_t numberOfBlocks)
            {               
                // minium counter size is 12 bytes. This isn't a variable because some compilers
                // are stupid and thing that variable is unused.
                assert(counter.GetLength() >= 12);

                CryptoBuffer incrementedCounter(counter);               

                //get the last 4 bytes and manipulate them as an integer.
                uint32_t* ctrPtr = (uint32_t*)(incrementedCounter.GetUnderlyingData() + incrementedCounter.GetLength() - sizeof(int32_t));                
                if(IS_BIG_ENDIAN)
                {
                    //you likely are not Big Endian, but
                    //if it's big endian, just go ahead and increment it... done
                    *ctrPtr += numberOfBlocks; 
                }
                else
                {
                    //otherwise, swap the byte ordering of the integer we loaded from the buffer (because it is backwards). However, the number of blocks is already properly 
                    //aligned. Once we compute the new value, swap it back so that the mirroring operation goes back to the actual buffer.
                    *ctrPtr = bswap<uint32_t>(bswap<uint32_t>(*ctrPtr) + numberOfBlocks);
                }

                return incrementedCounter;
            }

            CryptoBuffer GenerateXRandomBytes(size_t lengthBytes, bool ctrMode)
            {
                std::shared_ptr<SecureRandomBytes> rng = CreateSecureRandomBytesImplementation();

                CryptoBuffer bytes(lengthBytes);
                size_t lengthToGenerate = ctrMode ? (3 * bytes.GetLength())  / 4 : bytes.GetLength();
                
                rng->GetBytes(bytes.GetUnderlyingData(), lengthToGenerate);

                if(!*rng)
                {
                    AWS_LOGSTREAM_FATAL(LOG_TAG, "Random Number generation failed. Abort all crypto operations.");
                    assert(false);
                    abort();                    
                }

                return bytes;
            }

            void SymmetricCipher::Validate()
            {
                assert(m_key.GetLength() >= SYMMETRIC_KEY_LENGTH);
                assert(m_initializationVector.GetLength() == 0 || m_initializationVector.GetLength() >= MIN_IV_LENGTH);

                if(m_key.GetLength() < SYMMETRIC_KEY_LENGTH || 
                    (m_initializationVector.GetLength() > 0 && m_initializationVector.GetLength() < MIN_IV_LENGTH))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_FATAL(LOG_TAG, "Invalid state for symmetric cipher, key length is " << m_key.GetLength() <<
                                            " iv length is " << m_initializationVector.GetLength());
                }
            }

            /**
             * Generate random number per 4 bytes and use each byte for the byte in the iv
             */
            CryptoBuffer SymmetricCipher::GenerateIV(size_t ivLengthBytes, bool ctrMode)
            {
                CryptoBuffer iv(GenerateXRandomBytes(ivLengthBytes, ctrMode));

                if(iv.GetLength() == 0)
                {
                    AWS_LOGSTREAM_ERROR(LOG_TAG, "Unable to generate iv of length " << ivLengthBytes);
                    return iv;
                }

                if(ctrMode)
                {
                    //init the counter
                    size_t length = iv.GetLength();
                    //[ nonce 1/4] [ iv 1/2 ] [ ctr 1/4 ]
                    size_t ctrStart = (length / 2) + (length / 4);
                    for(; ctrStart < iv.GetLength() - 1; ++ ctrStart)
                    {
                        iv[ctrStart] = 0;
                    }
                    iv[length - 1] = 1;
                }

                return iv;
            }

            CryptoBuffer SymmetricCipher::GenerateKey(size_t keyLengthBytes)
            {
                CryptoBuffer const& key = GenerateXRandomBytes(keyLengthBytes, false);

                if(key.GetLength() == 0)
                {
                    AWS_LOGSTREAM_ERROR(LOG_TAG, "Unable to generate key of length " << keyLengthBytes);
                }

                return key;
            }
        }
    }
}

