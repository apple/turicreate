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

#include <aws/core/utils/crypto/commoncrypto/CryptoImpl.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/Outcome.h>

#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonHMAC.h>
#include <CommonCrypto/CommonCryptor.h>
#include <CommonCrypto/CommonSymmetricKeywrap.h>

//for OSX < 10.10 compatibility
typedef int32_t CCStatus;
typedef int32_t CCCryptorStatus;

namespace Aws
{
    namespace Utils
    {
        namespace Crypto
        {
            static const char* CC_LOG_TAG = "CommonCryptoImpl";

            SecureRandomBytes_CommonCrypto::SecureRandomBytes_CommonCrypto()
            {
                fp = fopen("/dev/random", "r");

                if(!fp)
                {
                    m_failure = true;

                }
            }

            SecureRandomBytes_CommonCrypto::~SecureRandomBytes_CommonCrypto()
            {
                if(fp)
                {
                    fclose(fp);
                }
            }

            /**
             * https://developer.apple.com/library/ios/documentation/Security/Conceptual/cryptoservices/RandomNumberGenerationAPIs/RandomNumberGenerationAPIs.html
             *  This is not thread safe. If you need thread safety, it is your responsibility.
             */
            void SecureRandomBytes_CommonCrypto::GetBytes(unsigned char* buffer, size_t bufferSize)
            {
                assert(buffer);

                if(!fp)
                {
                    m_failure = true;
                }

                size_t read = fread(buffer, sizeof(unsigned char), bufferSize, fp);

                if(read != bufferSize)
                {
                    m_failure = true;
                }
            }

            HashResult MD5CommonCryptoImpl::Calculate(const Aws::String& str)
            {
                ByteBuffer hash(CC_MD5_DIGEST_LENGTH);
                //CC_MD5 is deprecated by MacOS 10.15 due to cryptographically broken, but SDk only use it for digestion calculation
AWS_SUPPRESS_DEPRECATION(
                CC_MD5(str.c_str(), static_cast<CC_LONG>(str.length()), hash.GetUnderlyingData());
                )
                return HashResult(std::move(hash));
            }

            HashResult MD5CommonCryptoImpl::Calculate(Aws::IStream& stream)
            {
AWS_SUPPRESS_DEPRECATION(
                CC_MD5_CTX md5;
                CC_MD5_Init(&md5);
                )

                auto currentPos = stream.tellg();
                stream.seekg(0, stream.beg);

                char streamBuffer[Aws::Utils::Crypto::Hash::INTERNAL_HASH_STREAM_BUFFER_SIZE];
                while(stream.good())
                {
                    stream.read(streamBuffer, Aws::Utils::Crypto::Hash::INTERNAL_HASH_STREAM_BUFFER_SIZE);
                    auto bytesRead = stream.gcount();

                    if(bytesRead > 0)
                    {
AWS_SUPPRESS_DEPRECATION(
                        CC_MD5_Update(&md5, streamBuffer, static_cast<CC_LONG>(bytesRead));
                        )
                    }
                }

                stream.clear();
                stream.seekg(currentPos, stream.beg);

                ByteBuffer hash(CC_MD5_DIGEST_LENGTH);
AWS_SUPPRESS_DEPRECATION(
                CC_MD5_Final(hash.GetUnderlyingData(), &md5);
                )
                return HashResult(std::move(hash));
            }

            HashResult Sha256CommonCryptoImpl::Calculate(const Aws::String& str)
            {
                ByteBuffer hash(CC_SHA256_DIGEST_LENGTH);
                CC_SHA256(str.c_str(), static_cast<CC_LONG>(str.length()), hash.GetUnderlyingData());

                return HashResult(std::move(hash));
            }

            HashResult Sha256CommonCryptoImpl::Calculate(Aws::IStream& stream)
            {
                CC_SHA256_CTX sha256;
                CC_SHA256_Init(&sha256);

                auto currentPos = stream.tellg();
                stream.seekg(0, stream.beg);

                char streamBuffer[Aws::Utils::Crypto::Hash::INTERNAL_HASH_STREAM_BUFFER_SIZE];
                while(stream.good())
                {
                    stream.read(streamBuffer, Aws::Utils::Crypto::Hash::INTERNAL_HASH_STREAM_BUFFER_SIZE);
                    auto bytesRead = stream.gcount();

                    if(bytesRead > 0)
                    {
                        CC_SHA256_Update(&sha256, streamBuffer, static_cast<CC_LONG>(bytesRead));
                    }
                }

                stream.clear();
                stream.seekg(currentPos, stream.beg);

                ByteBuffer hash(CC_SHA256_DIGEST_LENGTH);
                CC_SHA256_Final(hash.GetUnderlyingData(), &sha256);

                return HashResult(std::move(hash));
            }

            HashResult Sha256HMACCommonCryptoImpl::Calculate(const ByteBuffer& toSign, const ByteBuffer& secret)
            {
                unsigned int length = CC_SHA256_DIGEST_LENGTH;
                ByteBuffer digest(length);
                std::memset(digest.GetUnderlyingData(), 0, length);

                CCHmac(kCCHmacAlgSHA256, secret.GetUnderlyingData(), secret.GetLength(), toSign.GetUnderlyingData(), toSign.GetLength(), digest.GetUnderlyingData());

                return HashResult(std::move(digest));
            }

            CommonCryptoCipher::CommonCryptoCipher(const CryptoBuffer& key, size_t ivSizeBytes, bool ctrMode) :
                    SymmetricCipher(key, ivSizeBytes, ctrMode), m_encryptorHandle(nullptr), m_decryptorHandle(nullptr)
            {
                Init();
            }

            CommonCryptoCipher::CommonCryptoCipher(CommonCryptoCipher&& toMove) : SymmetricCipher(std::move(toMove))
            {
                m_encryptorHandle = toMove.m_encryptorHandle;
                toMove.m_encryptorHandle = nullptr;
                m_decryptorHandle = toMove.m_decryptorHandle;
                toMove.m_decryptorHandle = nullptr;
            }

            CommonCryptoCipher::CommonCryptoCipher(CryptoBuffer&& key, CryptoBuffer&& initializationVector, CryptoBuffer&& tag) :
                    SymmetricCipher(std::move(key), std::move(initializationVector), std::move(tag)),
                    m_encryptorHandle(nullptr), m_decryptorHandle(nullptr)
            {
                Init();
            }

            CommonCryptoCipher::CommonCryptoCipher(const CryptoBuffer& key, const CryptoBuffer& initializationVector,
                                         const CryptoBuffer& tag) :
                    SymmetricCipher(key, initializationVector, tag),
                    m_encryptorHandle(nullptr), m_decryptorHandle(nullptr)
            {
                Init();
            }

            CommonCryptoCipher::~CommonCryptoCipher()
            {
                if (m_encryptorHandle)
                {
                    CCCryptorRelease(m_encryptorHandle);
                }
                if (m_decryptorHandle)
                {
                    CCCryptorRelease(m_decryptorHandle);
                }
            }

            void CommonCryptoCipher::Init()
            {

            }

            CryptoBuffer CommonCryptoCipher::EncryptBuffer(const CryptoBuffer& unEncryptedData)
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(CC_LOG_TAG, "Cipher not properly initialized for encryption. Aborting");
                    return CryptoBuffer();
                }

                size_t lengthWritten = unEncryptedData.GetLength() + (GetBlockSizeBytes() - 1);
                CryptoBuffer encryptedText(static_cast<size_t>( lengthWritten + (GetBlockSizeBytes() - 1)));

                CCStatus status = CCCryptorUpdate(m_encryptorHandle, unEncryptedData.GetUnderlyingData(), unEncryptedData.GetLength(),
                                                  encryptedText.GetUnderlyingData(), encryptedText.GetLength(), &lengthWritten);

                if (status != kCCSuccess)
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CC_LOG_TAG, "Encryption of buffer failed with status code: " << status);
                    return CryptoBuffer();
                }

                if (lengthWritten < encryptedText.GetLength())
                {
                    return CryptoBuffer(encryptedText.GetUnderlyingData(), lengthWritten);
                }

                return encryptedText;
            }

            CryptoBuffer CommonCryptoCipher::FinalizeEncryption()
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(CC_LOG_TAG,
                                        "Cipher not properly initialized for encryption finalization. Aborting");
                    return CryptoBuffer();
                }

                CryptoBuffer finalBlock(GetBlockSizeBytes());
                size_t writtenSize = 0;

                CCStatus status = CCCryptorFinal(m_encryptorHandle, finalBlock.GetUnderlyingData(), finalBlock.GetLength(), &writtenSize);
                if (status != kCCSuccess)
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CC_LOG_TAG, "Decryption of buffer failed with status code: " << status);
                    return CryptoBuffer();
                }

                return CryptoBuffer(finalBlock.GetUnderlyingData(), writtenSize);
            }

            CryptoBuffer CommonCryptoCipher::DecryptBuffer(const CryptoBuffer& encryptedData)
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(CC_LOG_TAG, "Cipher not properly initialized for decryption. Aborting");
                    return CryptoBuffer();
                }

                size_t lengthWritten = encryptedData.GetLength() + (GetBlockSizeBytes() - 1);
                CryptoBuffer decryptedText(static_cast<size_t>(lengthWritten));

                CCStatus status = CCCryptorUpdate(m_decryptorHandle, encryptedData.GetUnderlyingData(), encryptedData.GetLength(),
                    decryptedText.GetUnderlyingData(), decryptedText.GetLength(), &lengthWritten);

                if (status != kCCSuccess)
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CC_LOG_TAG, "Decryption of buffer failed with status code: " << status);
                    return CryptoBuffer();
                }

                if (lengthWritten < decryptedText.GetLength())
                {
                    return CryptoBuffer(decryptedText.GetUnderlyingData(), static_cast<size_t>(lengthWritten));
                }

                return decryptedText;
            }

            CryptoBuffer CommonCryptoCipher::FinalizeDecryption()
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(CC_LOG_TAG,
                                        "Cipher not properly initialized for decryption finalization. Aborting");
                    return CryptoBuffer();
                }

                CryptoBuffer finalBlock(GetBlockSizeBytes());
                size_t writtenSize = static_cast<int>(finalBlock.GetLength());
                CCStatus status = CCCryptorFinal(m_decryptorHandle, finalBlock.GetUnderlyingData(), finalBlock.GetLength(), &writtenSize);
                if (status != kCCSuccess)
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CC_LOG_TAG, "Decryption of buffer failed with status code: " << status);
                    return CryptoBuffer();
                }
                return CryptoBuffer(finalBlock.GetUnderlyingData(), writtenSize);
            }

            void CommonCryptoCipher::Reset()
            {
                m_failure = false;

                if (m_encryptorHandle)
                {
                    CCCryptorRelease(m_encryptorHandle);
                }
                if (m_decryptorHandle)
                {
                    CCCryptorRelease(m_decryptorHandle);
                }

                m_encryptorHandle = nullptr;
                m_decryptorHandle = nullptr;
                Init();
            }

            size_t AES_CBC_Cipher_CommonCrypto::BlockSizeBytes = 16;
            size_t AES_CBC_Cipher_CommonCrypto::KeyLengthBits = 256;
            static const char* CBC_CC_LOG_TAG = "AES_CBC_Cipher_CommonCrypto";

            AES_CBC_Cipher_CommonCrypto::AES_CBC_Cipher_CommonCrypto(const CryptoBuffer& key) : CommonCryptoCipher(key, BlockSizeBytes)
            {
                InitCipher();
            }

            AES_CBC_Cipher_CommonCrypto::AES_CBC_Cipher_CommonCrypto(CryptoBuffer&& key, CryptoBuffer&& initializationVector) :
                    CommonCryptoCipher(std::move(key), std::move(initializationVector))
            {
                InitCipher();
            }

            AES_CBC_Cipher_CommonCrypto::AES_CBC_Cipher_CommonCrypto(const CryptoBuffer& key,
                                                           const CryptoBuffer& initializationVector) :
                    CommonCryptoCipher(key, initializationVector)
            {
                InitCipher();
            }

            void AES_CBC_Cipher_CommonCrypto::InitCipher()
            {
                CCCryptorStatus status = CCCryptorCreateWithMode(kCCEncrypt, kCCModeCBC, kCCAlgorithmAES, ccPKCS7Padding,
                                                                 m_initializationVector.GetUnderlyingData(), m_key.GetUnderlyingData(), m_key.GetLength(),
                                                                 nullptr, 0, 0, 0, &m_encryptorHandle);
                if (status != kCCSuccess)
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CBC_CC_LOG_TAG, "Error while initializing AES 256 CBC encryptor. Status code: " << status);
                }

                status = CCCryptorCreateWithMode(kCCDecrypt, kCCModeCBC, kCCAlgorithmAES, ccPKCS7Padding,
                                                 m_initializationVector.GetUnderlyingData(), m_key.GetUnderlyingData(), m_key.GetLength(),
                                                 nullptr, 0, 0, 0, &m_decryptorHandle);
                if (status != kCCSuccess)
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CBC_CC_LOG_TAG, "Error while initializing AES 256 CBC decryptor. Status code: " << status);
                }

            }

            size_t AES_CBC_Cipher_CommonCrypto::GetBlockSizeBytes() const
            {
                return BlockSizeBytes;
            }

            size_t AES_CBC_Cipher_CommonCrypto::GetKeyLengthBits() const
            {
                return KeyLengthBits;
            }

            void AES_CBC_Cipher_CommonCrypto::Reset()
            {
                CommonCryptoCipher::Reset();
                InitCipher();
            }

            size_t AES_CTR_Cipher_CommonCrypto::BlockSizeBytes = 16;
            size_t AES_CTR_Cipher_CommonCrypto::KeyLengthBits = 256;
            static const char* CTR_CC_LOG_TAG = "AES_CTR_Cipher_CommonCrypto";

            AES_CTR_Cipher_CommonCrypto::AES_CTR_Cipher_CommonCrypto(const CryptoBuffer& key) :
                    CommonCryptoCipher(key, BlockSizeBytes, true)
            {
                InitCipher();
            }

            AES_CTR_Cipher_CommonCrypto::AES_CTR_Cipher_CommonCrypto(CryptoBuffer&& key, CryptoBuffer&& initializationVector) :
                    CommonCryptoCipher(std::move(key), std::move(initializationVector))
            {
                InitCipher();
            }

            AES_CTR_Cipher_CommonCrypto::AES_CTR_Cipher_CommonCrypto(const CryptoBuffer& key,
                                                           const CryptoBuffer& initializationVector) :
                    CommonCryptoCipher(key, initializationVector)
            {
                InitCipher();
            }

            void AES_CTR_Cipher_CommonCrypto::InitCipher()
            {
                CCCryptorStatus status = CCCryptorCreateWithMode(kCCEncrypt, kCCModeCTR, kCCAlgorithmAES, ccNoPadding,
                                                                 m_initializationVector.GetUnderlyingData(), m_key.GetUnderlyingData(), m_key.GetLength(),
                                                                 nullptr, 0, 0, kCCModeOptionCTR_BE, &m_encryptorHandle);
                if (status != kCCSuccess)
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CTR_CC_LOG_TAG, "Error while initializing AES 256 CTR encryptor. Status code: " << status);
                }

                status = CCCryptorCreateWithMode(kCCDecrypt, kCCModeCTR, kCCAlgorithmAES, ccNoPadding,
                                                 m_initializationVector.GetUnderlyingData(), m_key.GetUnderlyingData(), m_key.GetLength(),
                                                 nullptr, 0, 0, kCCModeOptionCTR_BE, &m_decryptorHandle);
                if (status != kCCSuccess)
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CTR_CC_LOG_TAG, "Error while initializing AES 256 CTR decryptor. Status code: " << status);
                }
            }

            size_t AES_CTR_Cipher_CommonCrypto::GetBlockSizeBytes() const
            {
                return BlockSizeBytes;
            }

            size_t AES_CTR_Cipher_CommonCrypto::GetKeyLengthBits() const
            {
                return KeyLengthBits;
            }

            void AES_CTR_Cipher_CommonCrypto::Reset()
            {
                CommonCryptoCipher::Reset();
                InitCipher();
            }

            static const char* const AES_KEY_WRAP_LOG_TAG = "AES_KeyWrap_Cipher_CommonCrypto";
            size_t AES_KeyWrap_Cipher_CommonCrypto::BlockSizeBytes = 8;
            size_t AES_KeyWrap_Cipher_CommonCrypto::KeyLengthBits = 256;

            AES_KeyWrap_Cipher_CommonCrypto::AES_KeyWrap_Cipher_CommonCrypto(const CryptoBuffer& key) : CommonCryptoCipher(key, 0)
            { }

            CryptoBuffer AES_KeyWrap_Cipher_CommonCrypto::EncryptBuffer(const CryptoBuffer& unEncryptedData)
            {
                assert(!m_failure);

                m_workingKeyBuffer = CryptoBuffer({&m_workingKeyBuffer, (CryptoBuffer*)&unEncryptedData});

                return CryptoBuffer();
            }

            CryptoBuffer AES_KeyWrap_Cipher_CommonCrypto::FinalizeEncryption()
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(CC_LOG_TAG,
                                        "Cipher not properly initialized for encryption finalization. Aborting");
                    return CryptoBuffer();
                }

                if (m_workingKeyBuffer.GetLength() == 0)
                {
                    m_failure = true;
                    return CryptoBuffer();
                }

                size_t outputBufferLength = GetBlockSizeBytes() + m_workingKeyBuffer.GetLength();
                CryptoBuffer outputBuffer(outputBufferLength);

                CCCryptorStatus status = CCSymmetricKeyWrap(kCCWRAPAES, CCrfc3394_iv, CCrfc3394_ivLen, m_key.GetUnderlyingData(), m_key.GetLength(),
                    m_workingKeyBuffer.GetUnderlyingData(), m_workingKeyBuffer.GetLength(), outputBuffer.GetUnderlyingData(), &outputBufferLength);

                if(status != kCCSuccess)
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(AES_KEY_WRAP_LOG_TAG, "Key wrap failed with status code " << status);
                    return CryptoBuffer();
                }

                return outputBuffer;
            }

            CryptoBuffer AES_KeyWrap_Cipher_CommonCrypto::DecryptBuffer(const CryptoBuffer& encryptedData)
            {
                assert(!m_failure);

                m_workingKeyBuffer = CryptoBuffer({&m_workingKeyBuffer, (CryptoBuffer*)&encryptedData});

                return CryptoBuffer();
            }

            CryptoBuffer AES_KeyWrap_Cipher_CommonCrypto::FinalizeDecryption()
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(CC_LOG_TAG,
                                        "Cipher not properly initialized for decryption finalization. Aborting");
                    return CryptoBuffer();
                }

                if (m_workingKeyBuffer.GetLength() == 0)
                {
                    m_failure = true;
                    return CryptoBuffer();
                }

                size_t outputBufferLength = m_workingKeyBuffer.GetLength() - GetBlockSizeBytes();
                CryptoBuffer outputBuffer(outputBufferLength);

                CCCryptorStatus status = CCSymmetricKeyUnwrap(kCCWRAPAES, CCrfc3394_iv, CCrfc3394_ivLen, m_key.GetUnderlyingData(), m_key.GetLength(),
                                                            m_workingKeyBuffer.GetUnderlyingData(), m_workingKeyBuffer.GetLength(), outputBuffer.GetUnderlyingData(), &outputBufferLength);

                if(status != kCCSuccess)
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(AES_KEY_WRAP_LOG_TAG, "Key unwrap failed with status code " << status);
                    return CryptoBuffer();
                }

                return outputBuffer;
            }

            void AES_KeyWrap_Cipher_CommonCrypto::Reset()
            {
                CommonCryptoCipher::Reset();
                m_workingKeyBuffer = CryptoBuffer();
            }

        }
    }
}
