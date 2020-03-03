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


#include <aws/core/utils/crypto/bcrypt/CryptoImpl.h>

#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/crypto/Hash.h>
#include <aws/core/utils/HashingUtils.h>
#include <atomic>
#include <bcrypt.h> 
#include <winternl.h> 
#include <winerror.h> 

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif // NT_SUCCESS

using namespace Aws::Utils;
using namespace Aws::Utils::Crypto;

namespace Aws
{
    namespace Utils
    {
        namespace Crypto
        {
            SecureRandomBytes_BCrypt::SecureRandomBytes_BCrypt()
            {
                NTSTATUS status = BCryptOpenAlgorithmProvider(&m_algHandle, BCRYPT_RNG_ALGORITHM, nullptr, 0);
                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_FATAL(SecureRandom_BCrypt_Tag, "Failed to initialize decryptor chaining mode with status code " << status);
                }
            }

            SecureRandomBytes_BCrypt::~SecureRandomBytes_BCrypt()
            {
                if (m_algHandle)
                {
                    BCryptCloseAlgorithmProvider(m_algHandle, 0);
                }
            }

            void SecureRandomBytes_BCrypt::GetBytes(unsigned char* buffer, size_t bufferSize)
            {
                assert(m_algHandle);
                if(bufferSize)
                {
                    assert(buffer);
                    if (m_algHandle)
                    {
                        NTSTATUS status = BCryptGenRandom(m_algHandle, buffer, static_cast<ULONG>(bufferSize), 0);

                        if (!NT_SUCCESS(status))
                        {
                            m_failure = true;
                            AWS_LOGSTREAM_FATAL(SecureRandom_BCrypt_Tag, "Failed to generate random number with status " << status);
                        }
                    }
                    else
                    {
                        m_failure = true;
                        AWS_LOGSTREAM_FATAL(SecureRandom_BCrypt_Tag, "Algorithm handle not initialized ");
                    }
                }
            }


            static const char* logTag = "CryptoHash";

            // RAII class for one-use-per-hash-call data used in Windows cryptographic hash implementations
            // Useful so we don't have to call a Cleanup function for every failure point
            class BCryptHashContext
            {
            public:

                BCryptHashContext(void* algorithmHandle, PBYTE hashObject, DWORD hashObjectLength) :
                    m_hashHandle(nullptr),
                    m_isValid(false)
                {
                    NTSTATUS status = BCryptCreateHash(algorithmHandle, &m_hashHandle, hashObject, hashObjectLength, nullptr, 0, 0);
                    m_isValid = NT_SUCCESS(status);
                }

                BCryptHashContext(void* algorithmHandle, PBYTE hashObject, DWORD hashObjectLength, const ByteBuffer& secret) :
                    m_hashHandle(nullptr),
                    m_isValid(false)
                {
                    NTSTATUS status = BCryptCreateHash(algorithmHandle, &m_hashHandle, hashObject, hashObjectLength, secret.GetUnderlyingData(), (ULONG)secret.GetLength(), 0);
                    m_isValid = NT_SUCCESS(status);
                }

                ~BCryptHashContext()
                {
                    if (m_hashHandle)
                    {
                        BCryptDestroyHash(m_hashHandle);
                    }
                }

                bool IsValid() const { return m_isValid; }

                BCRYPT_HASH_HANDLE m_hashHandle;
                bool m_isValid;
            };



            BCryptHashImpl::BCryptHashImpl(LPCWSTR algorithmName, bool isHMAC) :
                m_algorithmHandle(nullptr),
                m_hashBufferLength(0),
                m_hashBuffer(nullptr),
                m_hashObjectLength(0),
                m_hashObject(nullptr),
                m_algorithmMutex()
            {
                NTSTATUS status = BCryptOpenAlgorithmProvider(&m_algorithmHandle, algorithmName, MS_PRIMITIVE_PROVIDER, isHMAC ? BCRYPT_ALG_HANDLE_HMAC_FLAG : 0);
                if (!NT_SUCCESS(status))
                {
                    AWS_LOGSTREAM_ERROR(logTag, "Failed initializing BCryptOpenAlgorithmProvider for " << algorithmName);
                    return;
                }

                DWORD resultLength = 0;
                status = BCryptGetProperty(m_algorithmHandle, BCRYPT_HASH_LENGTH, (PBYTE)&m_hashBufferLength, sizeof(m_hashBufferLength), &resultLength, 0);
                if (!NT_SUCCESS(status) || m_hashBufferLength <= 0)
                {
                    AWS_LOGSTREAM_ERROR(logTag, "Error computing hash buffer length.");
                    return;
                }

                m_hashBuffer = Aws::NewArray<BYTE>(m_hashBufferLength, logTag);
                if (!m_hashBuffer)
                {
                    AWS_LOGSTREAM_ERROR(logTag, "Error allocating hash buffer.");
                    return;
                }

                resultLength = 0;
                status = BCryptGetProperty(m_algorithmHandle, BCRYPT_OBJECT_LENGTH, (PBYTE)&m_hashObjectLength, sizeof(m_hashObjectLength), &resultLength, 0);
                if (!NT_SUCCESS(status) || m_hashObjectLength <= 0)
                {
                    AWS_LOGSTREAM_ERROR(logTag, "Error computing hash object length.");
                    return;
                }

                m_hashObject = Aws::NewArray<BYTE>(m_hashObjectLength, logTag);
                if (!m_hashObject)
                {
                    AWS_LOGSTREAM_ERROR(logTag, "Error allocating hash object.");
                    return;
                }
            }

            BCryptHashImpl::~BCryptHashImpl()
            {
                Aws::DeleteArray(m_hashObject);
                Aws::DeleteArray(m_hashBuffer);

                if (m_algorithmHandle)
                {
                    BCryptCloseAlgorithmProvider(m_algorithmHandle, 0);
                }
            }

            HashResult BCryptHashImpl::HashData(const BCryptHashContext& context, PBYTE data, ULONG dataLength)
            {
                NTSTATUS status = BCryptHashData(context.m_hashHandle, data, dataLength, 0);
                if (!NT_SUCCESS(status))
                {
                    AWS_LOGSTREAM_ERROR(logTag, "Error computing hash.");
                    return HashResult();
                }

                status = BCryptFinishHash(context.m_hashHandle, m_hashBuffer, m_hashBufferLength, 0);
                if (!NT_SUCCESS(status))
                {
                    AWS_LOGSTREAM_ERROR(logTag, "Error obtaining computed hash");
                    return HashResult();
                }

                return HashResult(ByteBuffer(m_hashBuffer, m_hashBufferLength));
            }

            HashResult BCryptHashImpl::Calculate(const Aws::String& str)
            {
                if (!IsValid())
                {
                    return HashResult();
                }

                std::lock_guard<std::mutex> locker(m_algorithmMutex);

                BCryptHashContext context(m_algorithmHandle, m_hashObject, m_hashObjectLength);
                if (!context.IsValid())
                {
                    AWS_LOGSTREAM_ERROR(logTag, "Error creating hash handle.");
                    return HashResult();
                }

                return HashData(context, (PBYTE)str.c_str(), static_cast<ULONG>(str.length()));
            }

            HashResult BCryptHashImpl::Calculate(const ByteBuffer& toHash, const ByteBuffer& secret)
            {
                if (!IsValid())
                {
                    return HashResult();
                }

                std::lock_guard<std::mutex> locker(m_algorithmMutex);

                BCryptHashContext context(m_algorithmHandle, m_hashObject, m_hashObjectLength, secret);
                if (!context.IsValid())
                {
                    AWS_LOGSTREAM_ERROR(logTag, "Error creating hash handle.");
                    return HashResult();
                }

                return HashData(context, static_cast<PBYTE>(toHash.GetUnderlyingData()), static_cast<ULONG>(toHash.GetLength()));
            }

            bool BCryptHashImpl::IsValid() const
            {
                return m_hashBuffer != nullptr && m_hashBufferLength > 0 && m_hashObject != nullptr && m_hashObjectLength > 0;
            }

            bool BCryptHashImpl::HashStream(Aws::IStream& stream)
            {
                BCryptHashContext context(m_algorithmHandle, m_hashObject, m_hashObjectLength);
                if (!context.IsValid())
                {
                    AWS_LOGSTREAM_ERROR(logTag, "Error creating hash handle.");
                    return false;
                }

                char streamBuffer[Aws::Utils::Crypto::Hash::INTERNAL_HASH_STREAM_BUFFER_SIZE];
                NTSTATUS status = 0;
                stream.seekg(0, stream.beg);
                while (stream.good())
                {
                    stream.read(streamBuffer, Aws::Utils::Crypto::Hash::INTERNAL_HASH_STREAM_BUFFER_SIZE);
                    std::streamsize bytesRead = stream.gcount();
                    if (bytesRead > 0)
                    {
                        status = BCryptHashData(context.m_hashHandle, (PBYTE)streamBuffer, (ULONG)bytesRead, 0);
                        if (!NT_SUCCESS(status))
                        {
                            AWS_LOGSTREAM_ERROR(logTag, "Error computing hash.");
                            return false;
                        }
                    }
                }

                if (!stream.eof())
                {
                    return false;
                }

                status = BCryptFinishHash(context.m_hashHandle, m_hashBuffer, m_hashBufferLength, 0);
                if (!NT_SUCCESS(status))
                {
                    AWS_LOGSTREAM_ERROR(logTag, "Error obtaining computed hash");
                    return false;
                }

                return true;
            }

            HashResult BCryptHashImpl::Calculate(Aws::IStream& stream)
            {
                if (!IsValid())
                {
                    return HashResult();
                }

                std::lock_guard<std::mutex> locker(m_algorithmMutex);

                auto startingPos = stream.tellg();

                bool success = HashStream(stream);
                if (success)
                {
                    stream.clear();
                }

                stream.seekg(startingPos, stream.beg);

                if (!success)
                {
                    return HashResult();
                }

                return HashResult(ByteBuffer(m_hashBuffer, m_hashBufferLength));
            }

            MD5BcryptImpl::MD5BcryptImpl() :
                m_impl(BCRYPT_MD5_ALGORITHM, false)
            {
            }

            HashResult MD5BcryptImpl::Calculate(const Aws::String& str)
            {
                return m_impl.Calculate(str);
            }

            HashResult MD5BcryptImpl::Calculate(Aws::IStream& stream)
            {
                return m_impl.Calculate(stream);
            }

            Sha256BcryptImpl::Sha256BcryptImpl() :
                m_impl(BCRYPT_SHA256_ALGORITHM, false)
            {
            }

            HashResult Sha256BcryptImpl::Calculate(const Aws::String& str)
            {
                return m_impl.Calculate(str);
            }

            HashResult Sha256BcryptImpl::Calculate(Aws::IStream& stream)
            {
                return m_impl.Calculate(stream);
            }

            Sha256HMACBcryptImpl::Sha256HMACBcryptImpl() :
                m_impl(BCRYPT_SHA256_ALGORITHM, true)
            {
            }

            HashResult Sha256HMACBcryptImpl::Calculate(const ByteBuffer& toSign, const ByteBuffer& secret)
            {
                return m_impl.Calculate(toSign, secret);
            }

            static const char* SYM_CIPHER_TAG = "BCryptSymmetricCipherImpl";

            BCryptSymmetricCipher::BCryptSymmetricCipher(const CryptoBuffer& key, size_t ivSizeBytes, bool ctrMode) :
                SymmetricCipher(key, ivSizeBytes, ctrMode),
                m_algHandle(nullptr), m_keyHandle(nullptr), m_authInfoPtr(nullptr)
            {
                Init();
            }

            BCryptSymmetricCipher::BCryptSymmetricCipher(BCryptSymmetricCipher&& toMove) : SymmetricCipher(std::move(toMove)),
                m_authInfoPtr(nullptr)
            {
                m_algHandle = toMove.m_algHandle;
                m_keyHandle = toMove.m_keyHandle;
                toMove.m_algHandle = nullptr;
                toMove.m_keyHandle = nullptr;
            }

            BCryptSymmetricCipher::BCryptSymmetricCipher(CryptoBuffer&& key, CryptoBuffer&& initializationVector, CryptoBuffer&& tag) :
                SymmetricCipher(std::move(key), std::move(initializationVector), std::move(tag)),
                m_algHandle(nullptr), m_keyHandle(nullptr), m_authInfoPtr(nullptr)
            {
                Init();
            }

            BCryptSymmetricCipher::BCryptSymmetricCipher(const CryptoBuffer& key, const CryptoBuffer& initializationVector,
                const CryptoBuffer& tag) :
                SymmetricCipher(key, initializationVector, tag),
                m_algHandle(nullptr), m_keyHandle(nullptr), m_authInfoPtr(nullptr)
            {
                Init();
            }

            BCryptSymmetricCipher::~BCryptSymmetricCipher()
            {
                Cleanup();
            }

            void BCryptSymmetricCipher::Init()
            {
                m_workingIv = m_initializationVector;
            }

            BCRYPT_KEY_HANDLE BCryptSymmetricCipher::ImportKeyBlob(BCRYPT_ALG_HANDLE algHandle, CryptoBuffer& key)
            {
                NTSTATUS status = 0;

                BCRYPT_KEY_DATA_BLOB_HEADER keyData;
                keyData.dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
                keyData.dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
                keyData.cbKeyData = static_cast<ULONG>(key.GetLength());

                CryptoBuffer pbInputBuffer(sizeof(keyData) + key.GetLength());
                memcpy(pbInputBuffer.GetUnderlyingData(), &keyData, sizeof(keyData));
                memcpy(pbInputBuffer.GetUnderlyingData() + sizeof(keyData), key.GetUnderlyingData(), key.GetLength());

                BCRYPT_KEY_HANDLE keyHandle;
                status = BCryptImportKey(algHandle, nullptr, BCRYPT_KEY_DATA_BLOB, &keyHandle, nullptr, 0, pbInputBuffer.GetUnderlyingData(), static_cast<ULONG>(pbInputBuffer.GetLength()), 0);
                if (!NT_SUCCESS(status))
                {
                    AWS_LOGSTREAM_ERROR(SYM_CIPHER_TAG, "Failed to set symmetric key with status code " << status);
                    return nullptr;
                }

                return keyHandle;
            }

            void BCryptSymmetricCipher::InitKey()
            {
                if (m_algHandle)
                {
                    m_keyHandle = ImportKeyBlob(m_algHandle, m_key);
                    if (!m_keyHandle)
                    {
                        m_failure = true;                        
                        return;
                    }

                    if(!m_authInfoPtr && m_initializationVector.GetLength() > 0)
                    {              
                        NTSTATUS status = BCryptSetProperty(m_keyHandle, BCRYPT_INITIALIZATION_VECTOR, m_initializationVector.GetUnderlyingData(), static_cast<ULONG>(m_initializationVector.GetLength()), 0);

                        if (!NT_SUCCESS(status))
                        {
                            m_failure = true;
                            AWS_LOGSTREAM_ERROR(SYM_CIPHER_TAG, "Failed to set symetric key initialization vector with status code " << status);
                            return;
                        }
                    }
                }
            }

            CryptoBuffer BCryptSymmetricCipher::EncryptBuffer(const CryptoBuffer& unEncryptedData)
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(SYM_CIPHER_TAG, "Cipher not properly initialized for encryption. Aborting");
                    return CryptoBuffer();
                }

                if (unEncryptedData.GetLength() == 0)
                {
                    return CryptoBuffer();
                }

                size_t predictedWriteLengths = m_flags & BCRYPT_BLOCK_PADDING ? unEncryptedData.GetLength() + (GetBlockSizeBytes() - unEncryptedData.GetLength() % GetBlockSizeBytes())
                                                             : unEncryptedData.GetLength();

                ULONG lengthWritten = static_cast<ULONG>(predictedWriteLengths);
                CryptoBuffer encryptedText(static_cast<size_t>(predictedWriteLengths));

                PUCHAR iv = nullptr;
                ULONG ivSize = 0;

                if (m_authInfoPtr)
                {
                    iv = m_workingIv.GetUnderlyingData();
                    ivSize = static_cast<ULONG>(m_workingIv.GetLength());
                }

                //iv was set on the key itself, so we don't need to pass it here.
                NTSTATUS status = BCryptEncrypt(m_keyHandle, unEncryptedData.GetUnderlyingData(), (ULONG)unEncryptedData.GetLength(),
                    m_authInfoPtr, iv, ivSize, encryptedText.GetUnderlyingData(), (ULONG)encryptedText.GetLength(), &lengthWritten, m_flags);

                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(SYM_CIPHER_TAG, "Failed to compute encrypted output with error code " << status);
                    return CryptoBuffer();
                }

                if (static_cast<size_t>(lengthWritten) < encryptedText.GetLength())
                {
                    return CryptoBuffer(encryptedText.GetUnderlyingData(), static_cast<size_t>(lengthWritten));
                }

                return encryptedText;
            }

            CryptoBuffer BCryptSymmetricCipher::FinalizeEncryption()
            {
                return CryptoBuffer();
            }

            CryptoBuffer BCryptSymmetricCipher::DecryptBuffer(const CryptoBuffer& encryptedData)
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(SYM_CIPHER_TAG, "Cipher not properly initialized for decryption. Aborting");
                    return CryptoBuffer();
                }

                if (encryptedData.GetLength() == 0)
                {
                    return CryptoBuffer();
                }

                PUCHAR iv = nullptr;
                ULONG ivSize = 0;

                if (m_authInfoPtr)
                {
                    iv = m_workingIv.GetUnderlyingData();
                    ivSize = static_cast<ULONG>(m_workingIv.GetLength());
                }

                size_t predictedWriteLengths = encryptedData.GetLength();
                ULONG lengthWritten = static_cast<ULONG>(predictedWriteLengths);
                CryptoBuffer decryptedText(static_cast<size_t>(predictedWriteLengths));

                //iv was set on the key itself, so we don't need to pass it here.
                NTSTATUS status = BCryptDecrypt(m_keyHandle, encryptedData.GetUnderlyingData(), (ULONG)encryptedData.GetLength(),
                    m_authInfoPtr, iv, ivSize, decryptedText.GetUnderlyingData(), (ULONG)decryptedText.GetLength(), &lengthWritten, m_flags);

                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(SYM_CIPHER_TAG, "Failed to compute encrypted output with error code " << status);
                    return CryptoBuffer();
                }

                if (static_cast<size_t>(lengthWritten) < decryptedText.GetLength())
                {
                    return CryptoBuffer(decryptedText.GetUnderlyingData(), static_cast<size_t>(lengthWritten));
                }

                return decryptedText;
            }

            CryptoBuffer BCryptSymmetricCipher::FinalizeDecryption()
            {
                return CryptoBuffer();
            }

            void BCryptSymmetricCipher::Reset()
            {
                Cleanup();
                Init();
            }

            void BCryptSymmetricCipher::Cleanup()
            {
                if (m_keyHandle)
                {
                    BCryptDestroyKey(m_keyHandle);
                    m_keyHandle = nullptr;
                }

                if (m_algHandle)
                {
                    BCryptCloseAlgorithmProvider(m_algHandle, 0);
                    m_algHandle = nullptr;
                }

                m_flags = 0;
                m_authInfoPtr = nullptr;
                m_failure = false;
            }

            size_t AES_CBC_Cipher_BCrypt::BlockSizeBytes = 16;
            size_t AES_CBC_Cipher_BCrypt::KeyLengthBits = 256;

            AES_CBC_Cipher_BCrypt::AES_CBC_Cipher_BCrypt(const CryptoBuffer& key) : BCryptSymmetricCipher(key, BlockSizeBytes)
            {
                InitCipher();
                InitKey();
            }

            AES_CBC_Cipher_BCrypt::AES_CBC_Cipher_BCrypt(CryptoBuffer&& key, CryptoBuffer&& initializationVector) : BCryptSymmetricCipher(key, initializationVector)
            {
                InitCipher();
                InitKey();
            }

            AES_CBC_Cipher_BCrypt::AES_CBC_Cipher_BCrypt(const CryptoBuffer& key, const CryptoBuffer& initializationVector) : BCryptSymmetricCipher(key, initializationVector)
            {
                InitCipher();
                InitKey();
            }

            static const char* CBC_LOG_TAG = "BCrypt_AES_CBC_Cipher";

            void AES_CBC_Cipher_BCrypt::InitCipher()
            {
                //due to odd BCrypt api behavior, we have to manually handle the padding, however we are producing padded output.
                m_flags = 0;
                NTSTATUS status = BCryptOpenAlgorithmProvider(&m_algHandle, BCRYPT_AES_ALGORITHM, nullptr, 0);

                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CBC_LOG_TAG, "Failed to initialize encryptor/decryptor with status code " << status);
                }

                status = BCryptSetProperty(m_algHandle, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, static_cast<ULONG>(wcslen(BCRYPT_CHAIN_MODE_CBC) + 1), 0);
                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CBC_LOG_TAG, "Failed to initialize encryptor/decryptor chaining mode with status code " << status);
                }
            }

            /**
             * This is needlessly complicated due to the way BCrypt handles CBC mode. It assumes that you will only make one call to BCryptEncrypt and as a result
             * appends the padding to the output of every call. The simplest way around this is to have an extra 32 byte block sitting around. During EncryptBuffer calls
             * we don't use padding at all, we enforce that we only pass multiples of 32 bytes to BCryptEncrypt. Anything extra goes into either the next EncryptBuffer call
             * or is handled in the Finalize call.  On the very last call, we add the padding back. This is what the other Crypto APIs such as OpenSSL and CommonCrypto do under the hood anyways.
             */
            CryptoBuffer AES_CBC_Cipher_BCrypt::FillInOverflow(const CryptoBuffer& buffer)
            {
                static const size_t RESERVE_SIZE = BlockSizeBytes * 2;
                m_flags = 0;

                CryptoBuffer finalBuffer;

                if (m_blockOverflow.GetLength() > 0)
                {
                    finalBuffer = CryptoBuffer({ (ByteBuffer*)&m_blockOverflow, (ByteBuffer*)&buffer });
                    m_blockOverflow = CryptoBuffer();
                }
                else
                {
                    finalBuffer = buffer;
                }

                auto overflow = finalBuffer.GetLength() % RESERVE_SIZE;

                if (finalBuffer.GetLength() > RESERVE_SIZE)
                {
                    auto offset = overflow == 0 ? RESERVE_SIZE : overflow;
                    m_blockOverflow = CryptoBuffer(finalBuffer.GetUnderlyingData() + finalBuffer.GetLength() - offset, offset);
                    finalBuffer = CryptoBuffer(finalBuffer.GetUnderlyingData(), finalBuffer.GetLength() - offset);
                    return finalBuffer;
                }
                else
                {
                    m_blockOverflow = finalBuffer;
                    return CryptoBuffer();
                }
            }
            
            CryptoBuffer AES_CBC_Cipher_BCrypt::EncryptBuffer(const CryptoBuffer& unEncryptedData)
            {                    
                return BCryptSymmetricCipher::EncryptBuffer(FillInOverflow(unEncryptedData));              
            }

            /**
             * If we had actual data that overflowed a block left over from the packing, then let BCrypt handle the padding.
             * Otherwise, we have to manally encrypt the padding indicating that a full block is in the previous block.
             */
            CryptoBuffer AES_CBC_Cipher_BCrypt::FinalizeEncryption()
            {
                if (m_blockOverflow.GetLength() > 0)
                {
                    m_flags = BCRYPT_BLOCK_PADDING;
                    return BCryptSymmetricCipher::EncryptBuffer(m_blockOverflow);
                }               

                return CryptoBuffer();
            }
            
            CryptoBuffer AES_CBC_Cipher_BCrypt::DecryptBuffer(const CryptoBuffer& encryptedData)
            {
                return BCryptSymmetricCipher::DecryptBuffer(FillInOverflow(encryptedData));
            }

            CryptoBuffer AES_CBC_Cipher_BCrypt::FinalizeDecryption()
            {
                if ( m_blockOverflow.GetLength() > 0)
                {                   
                    m_flags = BCRYPT_BLOCK_PADDING;
                    return BCryptSymmetricCipher::DecryptBuffer(m_blockOverflow);
                }
                return CryptoBuffer();
            }

            void AES_CBC_Cipher_BCrypt::Reset()
            {
                BCryptSymmetricCipher::Reset();
                m_blockOverflow = CryptoBuffer();
                InitCipher();
                InitKey();
            }

            size_t AES_CBC_Cipher_BCrypt::GetBlockSizeBytes() const
            {
                return BlockSizeBytes;
            }

            size_t AES_CBC_Cipher_BCrypt::GetKeyLengthBits() const
            {
                return KeyLengthBits;
            }

            static const char* CTR_LOG_TAG = "BCrypt_AES_CTR_Cipher";
            size_t AES_CTR_Cipher_BCrypt::BlockSizeBytes = 16;
            size_t AES_CTR_Cipher_BCrypt::KeyLengthBits = 256;

            AES_CTR_Cipher_BCrypt::AES_CTR_Cipher_BCrypt(const CryptoBuffer& key) : BCryptSymmetricCipher(key, BlockSizeBytes, true)
            {
                InitCipher();
                InitKey();
            }

            AES_CTR_Cipher_BCrypt::AES_CTR_Cipher_BCrypt(CryptoBuffer&& key, CryptoBuffer&& initializationVector) : BCryptSymmetricCipher(key, initializationVector)
            {
                InitCipher();
                InitKey();
            }

            AES_CTR_Cipher_BCrypt::AES_CTR_Cipher_BCrypt(const CryptoBuffer& key, const CryptoBuffer& initializationVector) : BCryptSymmetricCipher(key, initializationVector)
            {
                InitCipher();
                InitKey();
            }

            CryptoBuffer AES_CTR_Cipher_BCrypt::EncryptBuffer(const CryptoBuffer& unEncryptedData)
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(CTR_LOG_TAG, "Cipher not properly initialized for encryption. Aborting");
                    return CryptoBuffer();
                }

                return EncryptWithCtr(unEncryptedData);
            }

            /**
             * In case we didn't have an even 16 byte multiple for the message, send the last
             * remaining data.
             */
            CryptoBuffer AES_CTR_Cipher_BCrypt::FinalizeEncryption()
            {
                if (m_blockOverflow.GetLength())
                {
                    CryptoBuffer const& returnBuffer = EncryptBuffer(m_blockOverflow);
                    m_blockOverflow = CryptoBuffer();
                    return returnBuffer;
                }

                return CryptoBuffer();
            }

            CryptoBuffer AES_CTR_Cipher_BCrypt::DecryptBuffer(const CryptoBuffer& encryptedData)
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(CTR_LOG_TAG, "Cipher not properly initialized for encryption. Aborting");
                    return CryptoBuffer();
                }

                //Encryption and decryption are identical in CTR mode.
                return EncryptWithCtr(encryptedData);
            }

            /**
            * In case we didn't have an even 16 byte multiple for the message, send the last
            * remaining data.
            */
            CryptoBuffer AES_CTR_Cipher_BCrypt::FinalizeDecryption()
            {
                if (m_blockOverflow.GetLength())
                {
                    CryptoBuffer const& returnBuffer = DecryptBuffer(m_blockOverflow);
                    m_blockOverflow = CryptoBuffer();
                    return returnBuffer;
                }

                return CryptoBuffer();
            }

            void AES_CTR_Cipher_BCrypt::InitCipher()
            {
                m_flags = 0;
                NTSTATUS status = BCryptOpenAlgorithmProvider(&m_algHandle, BCRYPT_AES_ALGORITHM, nullptr, 0);

                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CTR_LOG_TAG, "Failed to initialize encryptor/decryptor with status code " << status);
                }

                status = BCryptSetProperty(m_algHandle, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_ECB, static_cast<ULONG>(wcslen(BCRYPT_CHAIN_MODE_ECB) + 1), 0);
                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(CTR_LOG_TAG, "Failed to initialize encryptor/decryptor chaining mode with status code " << status);
                }
            }

            /**
            * Windows doesn't expose CTR mode. We can however, build it manually from ECB. Here, split each
            * buffer into 16 byte chunks, for each complete buffer encrypt the counter and xor it against the unencrypted
            * text. Save anything left over for the next run.
            */
            CryptoBuffer AES_CTR_Cipher_BCrypt::EncryptWithCtr(const CryptoBuffer& buffer)
            {
                size_t bytesWritten = 0;
                Aws::Vector<ByteBuffer*> finalBufferSet(0);

                CryptoBuffer bufferToEncrypt;

                if (m_blockOverflow.GetLength() > 0 && &m_blockOverflow != &buffer)
                {
                    bufferToEncrypt = CryptoBuffer({ (ByteBuffer*)&m_blockOverflow, (ByteBuffer*)&buffer });
                    m_blockOverflow = CryptoBuffer();
                }
                else
                {
                    bufferToEncrypt = buffer;
                }

                Aws::Utils::Array<CryptoBuffer> slicedBuffers;

                if (bufferToEncrypt.GetLength() > BlockSizeBytes)
                {
                    slicedBuffers = bufferToEncrypt.Slice(BlockSizeBytes);
                }
                else
                {
                    slicedBuffers = Aws::Utils::Array<CryptoBuffer>(1u);
                    slicedBuffers[0] = bufferToEncrypt;
                }

                finalBufferSet = Aws::Vector<ByteBuffer*>(slicedBuffers.GetLength());
                InitBuffersToNull(finalBufferSet);

                for (size_t i = 0; i < slicedBuffers.GetLength(); ++i)
                {
                    if (slicedBuffers[i].GetLength() == BlockSizeBytes || (m_blockOverflow.GetLength() > 0 && slicedBuffers.GetLength() == 1))
                    {                       
                        ULONG lengthWritten = static_cast<ULONG>(BlockSizeBytes);
                        CryptoBuffer encryptedText(BlockSizeBytes);

                        NTSTATUS status = BCryptEncrypt(m_keyHandle, m_workingIv.GetUnderlyingData(), (ULONG)m_workingIv.GetLength(),
                            nullptr, nullptr, 0, encryptedText.GetUnderlyingData(), (ULONG)encryptedText.GetLength(), &lengthWritten, m_flags);

                        if (!NT_SUCCESS(status))
                        {
                            m_failure = true;
                            AWS_LOGSTREAM_ERROR(CTR_LOG_TAG, "Failed to compute encrypted output with error code " << status);
                            CleanupBuffers(finalBufferSet);
                            return CryptoBuffer();
                        }

                        CryptoBuffer* newBuffer = Aws::New<CryptoBuffer>(CTR_LOG_TAG, BlockSizeBytes);
                        *newBuffer = slicedBuffers[i] ^ encryptedText;
                        finalBufferSet[i] = newBuffer;
                        m_workingIv = IncrementCTRCounter(m_workingIv, 1);
                        bytesWritten += static_cast<size_t>(lengthWritten);
                    }
                    else
                    {
                        m_blockOverflow = slicedBuffers[i];
                        CryptoBuffer* newBuffer = Aws::New<CryptoBuffer>(CTR_LOG_TAG, 0);
                        finalBufferSet[i] = newBuffer;
                    }
                }

                CryptoBuffer returnBuffer(std::move(finalBufferSet));
                CleanupBuffers(finalBufferSet);

                return returnBuffer;
            }

            void AES_CTR_Cipher_BCrypt::Reset()
            {
                BCryptSymmetricCipher::Reset();
                m_blockOverflow = CryptoBuffer();
                InitCipher();
                InitKey();
            }

            size_t AES_CTR_Cipher_BCrypt::GetBlockSizeBytes() const
            {
                return BlockSizeBytes;
            }

            size_t AES_CTR_Cipher_BCrypt::GetKeyLengthBits() const
            {
                return KeyLengthBits;
            }           

            void AES_CTR_Cipher_BCrypt::InitBuffersToNull(Aws::Vector<ByteBuffer*>& initBuffers)
            {
                for (ByteBuffer*& buffer : initBuffers)
                {
                    buffer = nullptr;
                }
            }

            void AES_CTR_Cipher_BCrypt::CleanupBuffers(Aws::Vector<ByteBuffer*>& cleanupBuffers)
            {
                for (ByteBuffer* buffer : cleanupBuffers)
                {
                    if (buffer)
                    {
                        Aws::Delete(buffer);
                    }
                }
            }

            static const char* GCM_LOG_TAG = "BCrypt_AES_GCM_Cipher";
            size_t AES_GCM_Cipher_BCrypt::BlockSizeBytes = 16;
            size_t AES_GCM_Cipher_BCrypt::NonceSizeBytes = 12;
            size_t AES_GCM_Cipher_BCrypt::KeyLengthBits = 256;
            size_t AES_GCM_Cipher_BCrypt::TagLengthBytes = 16;

            AES_GCM_Cipher_BCrypt::AES_GCM_Cipher_BCrypt(const CryptoBuffer& key) :
                    BCryptSymmetricCipher(key, NonceSizeBytes), m_macBuffer(TagLengthBytes)
            {
                m_tag = CryptoBuffer(TagLengthBytes);
                InitCipher();
                InitKey();
            }

            AES_GCM_Cipher_BCrypt::AES_GCM_Cipher_BCrypt(CryptoBuffer&& key, CryptoBuffer&& initializationVector, CryptoBuffer&& tag) :
                    BCryptSymmetricCipher(std::move(key), std::move(initializationVector), std::move(tag)), m_macBuffer(TagLengthBytes)
            {
                if (m_tag.GetLength() == 0)
                {
                    m_tag = CryptoBuffer(TagLengthBytes);
                }
                InitCipher();
                InitKey();
            }

            AES_GCM_Cipher_BCrypt::AES_GCM_Cipher_BCrypt(const CryptoBuffer& key, const CryptoBuffer& initializationVector, const CryptoBuffer& tag) :
                    BCryptSymmetricCipher(key, initializationVector, tag), m_macBuffer(TagLengthBytes)
            {
                if (m_tag.GetLength() == 0)
                {
                    m_tag = CryptoBuffer(TagLengthBytes);
                }
                InitCipher();
                InitKey();
            }

            /**
             * This will always return a buffer due to the way the windows api is written.
             * The chain flag has to be explicitly turned off and a buffer has to be passed in
             * in order for the tag to compute properly. As a result, we have to hold a buffer until
             * the end to make sure the cipher computes the auth tag correctly.
             */
            CryptoBuffer AES_GCM_Cipher_BCrypt::FinalizeEncryption()
            {
                m_authInfo.dwFlags &= ~BCRYPT_AUTH_MODE_CHAIN_CALLS_FLAG;
                return BCryptSymmetricCipher::EncryptBuffer(m_finalBuffer);
            }

            /**
             * Since we have to assume these calls are being chained, and due to the way the windows
             * api works, we have to make sure we hold a final buffer until the end so we can tell
             * windows to compute the auth tag. Also, prior to the last call, we have to pass the data
             * in multiples of 16 byte blocks. So, here we keep a buffer of the % 16 + 16 bytes.
             * That gets saved until the end where we will encrypt the last buffer and compute the tag.
             */
            CryptoBuffer AES_GCM_Cipher_BCrypt::EncryptBuffer(const CryptoBuffer& toEncrypt)
            {
                assert(!m_failure);

                CryptoBuffer workingBuffer;

                if (m_finalBuffer.GetLength() > 0)
                {
                    workingBuffer = CryptoBuffer({(ByteBuffer*)&m_finalBuffer, (ByteBuffer*)&toEncrypt});
                    m_finalBuffer = CryptoBuffer();
                }
                else
                {
                    workingBuffer = toEncrypt;
                }

                if (workingBuffer.GetLength() > TagLengthBytes)
                {
                    auto offset = workingBuffer.GetLength() % TagLengthBytes;

                    m_finalBuffer = CryptoBuffer(workingBuffer.GetUnderlyingData() + workingBuffer.GetLength() - (TagLengthBytes +  offset), TagLengthBytes + offset);
                    workingBuffer = CryptoBuffer(workingBuffer.GetUnderlyingData(), workingBuffer.GetLength() - (TagLengthBytes + offset));
                    return BCryptSymmetricCipher::EncryptBuffer(workingBuffer);
                }
                else
                {
                    m_finalBuffer = workingBuffer;
                    return CryptoBuffer();
                }
            }

            /**
             * Since we have to assume these calls are being chained, and due to the way the windows
             * api works, we have to make sure we hold a final buffer until the end so we can tell
             * windows to compute the auth tag. Also, prior to the last call, we have to pass the data
             * in multiples of 16 byte blocks. So, here we keep a buffer of the % 16 + 16 bytes.
             * That gets saved until the end where we will decrypt the last buffer and compute the tag.
             */
            CryptoBuffer AES_GCM_Cipher_BCrypt::DecryptBuffer(const CryptoBuffer& toDecrypt)
            {
                assert(!m_failure);

                CryptoBuffer workingBuffer;

                if (m_finalBuffer.GetLength() > 0)
                {
                    workingBuffer = CryptoBuffer({ (ByteBuffer*)&m_finalBuffer, (ByteBuffer*)&toDecrypt });
                    m_finalBuffer = CryptoBuffer();
                }
                else
                {
                    workingBuffer = toDecrypt;
                }

                if (workingBuffer.GetLength() > TagLengthBytes)
                {
                    auto offset = workingBuffer.GetLength() % TagLengthBytes;
                    m_finalBuffer = CryptoBuffer(workingBuffer.GetUnderlyingData() + workingBuffer.GetLength() - (TagLengthBytes + offset), TagLengthBytes + offset);
                    workingBuffer = CryptoBuffer(workingBuffer.GetUnderlyingData(), workingBuffer.GetLength() - (TagLengthBytes + offset));
                    return BCryptSymmetricCipher::DecryptBuffer(workingBuffer);
                }
                else
                {
                    m_finalBuffer = workingBuffer;
                    return CryptoBuffer();
                }
            }

            /**
             * This will always return a buffer due to the way the windows api is written.
             * The chain flag has to be explicitly turned off and a buffer has to be passed in
             * in order for the tag to compute properly. As a result, we have to hold a buffer until
             * the end to make sure the cipher computes the auth tag correctly.
             */
            CryptoBuffer AES_GCM_Cipher_BCrypt::FinalizeDecryption()
            {
                m_authInfo.dwFlags &= ~BCRYPT_AUTH_MODE_CHAIN_CALLS_FLAG;

                return BCryptSymmetricCipher::DecryptBuffer(m_finalBuffer);
            }

            /**
             * Encrypt and decrypt do the same exact thing here.
             * Summary:
             * No Padding, open AES alg, Set GCM as chain mode, create the auth struct, turn on chaining,
             *   initialize a buffer for bcrypt to use while running.
             */
            void AES_GCM_Cipher_BCrypt::InitCipher()
            {
                m_flags = 0;
                NTSTATUS status = BCryptOpenAlgorithmProvider(&m_algHandle, BCRYPT_AES_ALGORITHM, nullptr, 0);

                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(GCM_LOG_TAG, "Failed to initialize encryptor/decryptor with status code " << status);
                }

                status = BCryptSetProperty(m_algHandle, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_GCM, static_cast<ULONG>(wcslen(BCRYPT_CHAIN_MODE_GCM) + 1), 0);
                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(GCM_LOG_TAG, "Failed to initialize encryptor/decryptor chaining mode with status code " << status);
                }

                BCRYPT_INIT_AUTH_MODE_INFO(m_authInfo);
                m_authInfo.pbNonce = m_initializationVector.GetUnderlyingData();
                m_authInfo.cbNonce = static_cast<ULONG>(m_initializationVector.GetLength());
                m_authInfo.pbTag = m_tag.GetUnderlyingData();
                m_authInfo.cbTag = static_cast<ULONG>(m_tag.GetLength());
                m_authInfo.pbMacContext = m_macBuffer.GetUnderlyingData();
                m_authInfo.cbMacContext = static_cast<ULONG>(m_macBuffer.GetLength());
                m_authInfo.cbData = 0;
                m_authInfo.dwFlags = BCRYPT_AUTH_MODE_CHAIN_CALLS_FLAG;

                m_authInfoPtr = &m_authInfo;

                m_workingIv = CryptoBuffer(TagLengthBytes);
                m_workingIv.Zero();

            }

            void AES_GCM_Cipher_BCrypt::Reset()
            {
                m_macBuffer.Zero();
                m_finalBuffer = CryptoBuffer();
                BCryptSymmetricCipher::Reset();
                InitCipher();
                InitKey();
            }

            size_t AES_GCM_Cipher_BCrypt::GetBlockSizeBytes() const
            {
                return BlockSizeBytes;
            }

            size_t AES_GCM_Cipher_BCrypt::GetKeyLengthBits() const
            {
                return KeyLengthBits;
            }

            size_t AES_GCM_Cipher_BCrypt::GetTagLengthBytes() const
            {
                return TagLengthBytes;
            }

            static const char* KEYWRAP_LOG_TAG = "AES_KeyWrap_Cipher_BCrypt";
            size_t AES_KeyWrap_Cipher_BCrypt::BlockSizeBytes = 8;
            size_t AES_KeyWrap_Cipher_BCrypt::KeyLengthBits = 256;

            AES_KeyWrap_Cipher_BCrypt::AES_KeyWrap_Cipher_BCrypt(const CryptoBuffer& key)
                : BCryptSymmetricCipher(key, 0)
            {
                InitCipher();
                InitKey();
            }

            CryptoBuffer AES_KeyWrap_Cipher_BCrypt::EncryptBuffer(const CryptoBuffer& unEncryptedData)
            {
                assert(!m_failure);

                m_operatingKeyBuffer = CryptoBuffer({(ByteBuffer*)&m_operatingKeyBuffer, (ByteBuffer*)&unEncryptedData});

                return CryptoBuffer();
            }

            CryptoBuffer AES_KeyWrap_Cipher_BCrypt::DecryptBuffer(const CryptoBuffer& encryptedData)
            {
                assert(!m_failure);

                m_operatingKeyBuffer = CryptoBuffer({ (ByteBuffer*)&m_operatingKeyBuffer, (ByteBuffer*)&encryptedData });

                return CryptoBuffer();
            }


            void AES_KeyWrap_Cipher_BCrypt::InitCipher()
            {
                NTSTATUS status = BCryptOpenAlgorithmProvider(&m_algHandle, BCRYPT_AES_ALGORITHM, nullptr, 0);

                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(KEYWRAP_LOG_TAG, "Failed to initialize encryptor/decryptor with status code " << status);
                }
            }

            CryptoBuffer AES_KeyWrap_Cipher_BCrypt::FinalizeEncryption()
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(SYM_CIPHER_TAG, "Cipher not properly initialized for encryption. Aborting");
                    return CryptoBuffer();
                }

                BCRYPT_KEY_HANDLE keyHandleToEncrypt = ImportKeyBlob(m_algHandle, m_operatingKeyBuffer);
                
                NTSTATUS status = 0;

                ULONG sizeOfCipherText;
                status = BCryptExportKey(keyHandleToEncrypt, m_keyHandle, BCRYPT_AES_WRAP_KEY_BLOB, 
                    nullptr, 0, &sizeOfCipherText, 0);

                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(KEYWRAP_LOG_TAG, "Failed to export symmetric key size with status code " << status);
                    return CryptoBuffer();
                }
                
                CryptoBuffer cipherText(static_cast<size_t>(sizeOfCipherText));
                status = BCryptExportKey(keyHandleToEncrypt, m_keyHandle, BCRYPT_AES_WRAP_KEY_BLOB,
                    cipherText.GetUnderlyingData(), static_cast<ULONG>(cipherText.GetLength()), &sizeOfCipherText, 0);

                if (keyHandleToEncrypt)
                {
                    BCryptDestroyKey(keyHandleToEncrypt);
                }

                if (!NT_SUCCESS(status))
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(KEYWRAP_LOG_TAG, "Failed to export symmetric key with status code " << status);
                    return CryptoBuffer();
                }

                return cipherText;
            }

            CryptoBuffer AES_KeyWrap_Cipher_BCrypt::FinalizeDecryption()
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(SYM_CIPHER_TAG, "Cipher not properly initialized for decryption. Aborting");
                    return CryptoBuffer();
                }

                CryptoBuffer returnBuffer;    
                
                BCRYPT_KEY_HANDLE importKey(nullptr);
                NTSTATUS status = BCryptImportKey(m_algHandle, m_keyHandle, BCRYPT_AES_WRAP_KEY_BLOB, &importKey,
                    nullptr, 0,
                    m_operatingKeyBuffer.GetUnderlyingData(), static_cast<ULONG>(m_operatingKeyBuffer.GetLength()), 0);

                if (importKey)
                {
                    ULONG exportSize(0);
                    CryptoBuffer outputBuffer(sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + m_operatingKeyBuffer.GetLength());
                    status = BCryptExportKey(importKey, nullptr, BCRYPT_KEY_DATA_BLOB, 
                                outputBuffer.GetUnderlyingData(), static_cast<ULONG>(outputBuffer.GetLength()), &exportSize, 0);

                    if (NT_SUCCESS(status))
                    {
                        BCRYPT_KEY_DATA_BLOB_HEADER* streamHeader = (BCRYPT_KEY_DATA_BLOB_HEADER*)outputBuffer.GetUnderlyingData();
                        returnBuffer = CryptoBuffer(outputBuffer.GetUnderlyingData() + sizeof(BCRYPT_KEY_DATA_BLOB_HEADER), streamHeader->cbKeyData);
                    }
                    else
                    {
                        m_failure = true;
                        AWS_LOGSTREAM_ERROR(KEYWRAP_LOG_TAG, "Failed to re-export key with status code " << status);
                    }

                    BCryptDestroyKey(importKey);
                }
                else               
                {
                    m_failure = true;
                    AWS_LOGSTREAM_ERROR(KEYWRAP_LOG_TAG, "Failed to import symmetric key with status code " << status);
                }

                return returnBuffer;
            }

            void AES_KeyWrap_Cipher_BCrypt::Reset()
            {
                BCryptSymmetricCipher::Reset();
                m_operatingKeyBuffer = CryptoBuffer();
                InitCipher();
                InitKey();
            }

            size_t AES_KeyWrap_Cipher_BCrypt::GetBlockSizeBytes() const
            {
                return BlockSizeBytes;
            }

            size_t AES_KeyWrap_Cipher_BCrypt::GetKeyLengthBits() const
            {
                return KeyLengthBits;
            }

        } // namespace Crypto
    } // namespace Utils
} // namespace Amazon
