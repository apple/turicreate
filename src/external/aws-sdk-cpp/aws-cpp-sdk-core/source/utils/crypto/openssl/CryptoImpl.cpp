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

#include <cstring>

#include <aws/core/utils/crypto/openssl/CryptoImpl.h>
#include <aws/core/utils/Outcome.h>
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/err.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <thread>

using namespace Aws::Utils;
using namespace Aws::Utils::Crypto;

namespace Aws
{
    namespace Utils
    {
        namespace Crypto
        {
            namespace OpenSSL
            {
                static const char* OPENSSL_INTERNALS_TAG = "OpenSSLCallbackState";
                static std::mutex* locks(nullptr);

                GetTheLights getTheLights;

                void init_static_state()
                {
                    ERR_load_CRYPTO_strings();
                    OPENSSL_add_all_algorithms_noconf();

                    if (!CRYPTO_get_locking_callback())
                    {
                        locks = Aws::NewArray<std::mutex>(static_cast<size_t>(CRYPTO_num_locks()),
                                                          OPENSSL_INTERNALS_TAG);
                        CRYPTO_set_locking_callback(&locking_fn);
                    }

                    if (!CRYPTO_get_id_callback())
                    {
                        CRYPTO_set_id_callback(&id_fn);
                    }

                    RAND_poll();
                }

                void cleanup_static_state()
                {
                    if (CRYPTO_get_locking_callback() == &locking_fn)
                    {
                        CRYPTO_set_id_callback(nullptr);
                        assert(locks);
                        Aws::DeleteArray(locks);
                        locks = nullptr;
                    }

                    if (CRYPTO_get_id_callback() == &id_fn)
                    {
                        CRYPTO_set_locking_callback(nullptr);
                    }
                }

                void locking_fn(int mode, int n, const char*, int)
                {
                    if (mode & CRYPTO_LOCK)
                    {
                        locks[n].lock();
                    }
                    else
                    {
                        locks[n].unlock();
                    }
                }

                unsigned long id_fn()
                {
                    return static_cast<unsigned long>(std::hash<std::thread::id>()(std::this_thread::get_id()));
                }
            }

            void SecureRandomBytes_OpenSSLImpl::GetBytes(unsigned char* buffer, size_t bufferSize)
            {
                assert(buffer);

                int success = RAND_bytes(buffer, static_cast<int>(bufferSize));
                if (success != 1)
                {
                    m_failure = true;
                }
            }

            HashResult MD5OpenSSLImpl::Calculate(const Aws::String& str)
            {
                MD5_CTX md5;
                MD5_Init(&md5);
                MD5_Update(&md5, str.c_str(), str.size());

                ByteBuffer hash(MD5_DIGEST_LENGTH);
                MD5_Final(hash.GetUnderlyingData(), &md5);

                return HashResult(std::move(hash));
            }

            HashResult MD5OpenSSLImpl::Calculate(Aws::IStream& stream)
            {
                MD5_CTX md5;
                MD5_Init(&md5);

                auto currentPos = stream.tellg();
                if (currentPos == -1)
                {
                    currentPos = 0;
                    stream.clear();
                }
                stream.seekg(0, stream.beg);

                char streamBuffer[Aws::Utils::Crypto::Hash::INTERNAL_HASH_STREAM_BUFFER_SIZE];
                while (stream.good())
                {
                    stream.read(streamBuffer, Aws::Utils::Crypto::Hash::INTERNAL_HASH_STREAM_BUFFER_SIZE);
                    auto bytesRead = stream.gcount();

                    if (bytesRead > 0)
                    {
                        MD5_Update(&md5, streamBuffer, static_cast<size_t>(bytesRead));
                    }
                }

                stream.clear();
                stream.seekg(currentPos, stream.beg);

                ByteBuffer hash(MD5_DIGEST_LENGTH);
                MD5_Final(hash.GetUnderlyingData(), &md5);

                return HashResult(std::move(hash));
            }

            HashResult Sha256OpenSSLImpl::Calculate(const Aws::String& str)
            {
                SHA256_CTX sha256;
                SHA256_Init(&sha256);
                SHA256_Update(&sha256, str.c_str(), str.size());

                ByteBuffer hash(SHA256_DIGEST_LENGTH);
                SHA256_Final(hash.GetUnderlyingData(), &sha256);

                return HashResult(std::move(hash));
            }

            HashResult Sha256OpenSSLImpl::Calculate(Aws::IStream& stream)
            {
                SHA256_CTX sha256;
                SHA256_Init(&sha256);

                auto currentPos = stream.tellg();
                if (currentPos == -1)
                {
                    currentPos = 0;
                    stream.clear();
                }

                stream.seekg(0, stream.beg);

                char streamBuffer[Aws::Utils::Crypto::Hash::INTERNAL_HASH_STREAM_BUFFER_SIZE];
                while (stream.good())
                {
                    stream.read(streamBuffer, Aws::Utils::Crypto::Hash::INTERNAL_HASH_STREAM_BUFFER_SIZE);
                    auto bytesRead = stream.gcount();

                    if (bytesRead > 0)
                    {
                        SHA256_Update(&sha256, streamBuffer, static_cast<size_t>(bytesRead));
                    }
                }

                stream.clear();
                stream.seekg(currentPos, stream.beg);

                ByteBuffer hash(SHA256_DIGEST_LENGTH);
                SHA256_Final(hash.GetUnderlyingData(), &sha256);

                return HashResult(std::move(hash));
            }

            HashResult Sha256HMACOpenSSLImpl::Calculate(const ByteBuffer& toSign, const ByteBuffer& secret)
            {
                unsigned int length = SHA256_DIGEST_LENGTH;
                ByteBuffer digest(length);
                memset(digest.GetUnderlyingData(), 0, length);

                HMAC_CTX ctx;
                HMAC_CTX_init(&ctx);

                HMAC_Init_ex(&ctx, secret.GetUnderlyingData(), static_cast<int>(secret.GetLength()), EVP_sha256(),
                             NULL);
                HMAC_Update(&ctx, toSign.GetUnderlyingData(), toSign.GetLength());
                HMAC_Final(&ctx, digest.GetUnderlyingData(), &length);
                HMAC_CTX_cleanup(&ctx);

                return HashResult(std::move(digest));
            }

            static const char* OPENSSL_LOG_TAG = "OpenSSLCipher";

            void LogErrors(const char* logTag = OPENSSL_LOG_TAG)
            {
                unsigned long errorCode = ERR_get_error();
                char errStr[256];
                ERR_error_string_n(errorCode, errStr, 256);

                AWS_LOGSTREAM_ERROR(logTag, errStr);
            }


            OpenSSLCipher::OpenSSLCipher(const CryptoBuffer& key, size_t blockSizeBytes, bool ctrMode) :
                    SymmetricCipher(key, blockSizeBytes, ctrMode), m_encDecInitialized(false), m_encryptionMode(false),
                    m_decryptionMode(false)
            {
                Init();
            }

            OpenSSLCipher::OpenSSLCipher(OpenSSLCipher&& toMove) : SymmetricCipher(std::move(toMove)),
                                                                   m_encDecInitialized(false)
            {
                m_ctx = toMove.m_ctx;
                toMove.m_ctx.cipher = nullptr;
                toMove.m_ctx.cipher_data = nullptr;
                toMove.m_ctx.engine = nullptr;

                m_encDecInitialized = toMove.m_encDecInitialized;
                m_encryptionMode = toMove.m_encryptionMode;
                m_decryptionMode = toMove.m_decryptionMode;
            }

            OpenSSLCipher::OpenSSLCipher(CryptoBuffer&& key, CryptoBuffer&& initializationVector, CryptoBuffer&& tag) :
                    SymmetricCipher(std::move(key), std::move(initializationVector), std::move(tag)),
                    m_encDecInitialized(false),
                    m_encryptionMode(false), m_decryptionMode(false)
            {
                Init();
            }

            OpenSSLCipher::OpenSSLCipher(const CryptoBuffer& key, const CryptoBuffer& initializationVector,
                                         const CryptoBuffer& tag) :
                    SymmetricCipher(key, initializationVector, tag), m_encDecInitialized(false),
                    m_encryptionMode(false), m_decryptionMode(false)
            {
                Init();
            }

            OpenSSLCipher::~OpenSSLCipher()
            {
                Cleanup();
            }

            void OpenSSLCipher::Init()
            {
                EVP_CIPHER_CTX_init(&m_ctx);
            }

            void OpenSSLCipher::CheckInitEncryptor()
            {
                assert(!m_failure);
                assert(!m_decryptionMode);

                if (!m_encDecInitialized)
                {
                    InitEncryptor_Internal();
                    m_encryptionMode = true;
                    m_encDecInitialized = true;
                }
            }

            void OpenSSLCipher::CheckInitDecryptor()
            {
                assert(!m_failure);
                assert(!m_encryptionMode);

                if (!m_encDecInitialized)
                {
                    InitDecryptor_Internal();
                    m_decryptionMode = true;
                    m_encDecInitialized = true;
                }
            }

            CryptoBuffer OpenSSLCipher::EncryptBuffer(const CryptoBuffer& unEncryptedData)
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(OPENSSL_LOG_TAG, "Cipher not properly initialized for encryption. Aborting");
                    return CryptoBuffer();
                }

                CheckInitEncryptor();
                int lengthWritten = static_cast<int>(unEncryptedData.GetLength() + (GetBlockSizeBytes() - 1));
                CryptoBuffer encryptedText(static_cast<size_t>( lengthWritten + (GetBlockSizeBytes() - 1)));

                if (!EVP_EncryptUpdate(&m_ctx, encryptedText.GetUnderlyingData(), &lengthWritten,
                                       unEncryptedData.GetUnderlyingData(),
                                       static_cast<int>(unEncryptedData.GetLength())))
                {
                    m_failure = true;
                    LogErrors();
                    return CryptoBuffer();
                }

                if (static_cast<size_t>(lengthWritten) < encryptedText.GetLength())
                {
                    return CryptoBuffer(encryptedText.GetUnderlyingData(), static_cast<size_t>(lengthWritten));
                }

                return encryptedText;
            }

            CryptoBuffer OpenSSLCipher::FinalizeEncryption()
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(OPENSSL_LOG_TAG,
                                        "Cipher not properly initialized for encryption finalization. Aborting");
                    return CryptoBuffer();
                }

                CryptoBuffer finalBlock(GetBlockSizeBytes());
                int writtenSize = 0;
                if (!EVP_EncryptFinal_ex(&m_ctx, finalBlock.GetUnderlyingData(), &writtenSize))
                {
                    m_failure = true;
                    LogErrors();
                    return CryptoBuffer();
                }
                return CryptoBuffer(finalBlock.GetUnderlyingData(), static_cast<size_t>(writtenSize));
            }

            CryptoBuffer OpenSSLCipher::DecryptBuffer(const CryptoBuffer& encryptedData)
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(OPENSSL_LOG_TAG, "Cipher not properly initialized for decryption. Aborting");
                    return CryptoBuffer();
                }

                CheckInitDecryptor();
                int lengthWritten = static_cast<int>(encryptedData.GetLength() + (GetBlockSizeBytes() - 1));
                CryptoBuffer decryptedText(static_cast<size_t>(lengthWritten));

                if (!EVP_DecryptUpdate(&m_ctx, decryptedText.GetUnderlyingData(), &lengthWritten,
                                       encryptedData.GetUnderlyingData(),
                                       static_cast<int>(encryptedData.GetLength())))
                {
                    m_failure = true;
                    LogErrors();
                    return CryptoBuffer();
                }

                if (static_cast<size_t>(lengthWritten) < decryptedText.GetLength())
                {
                    return CryptoBuffer(decryptedText.GetUnderlyingData(), static_cast<size_t>(lengthWritten));
                }

                return decryptedText;
            }

            CryptoBuffer OpenSSLCipher::FinalizeDecryption()
            {
                if (m_failure)
                {
                    AWS_LOGSTREAM_FATAL(OPENSSL_LOG_TAG,
                                        "Cipher not properly initialized for decryption finalization. Aborting");
                    return CryptoBuffer();
                }

                CryptoBuffer finalBlock(GetBlockSizeBytes());
                int writtenSize = static_cast<int>(finalBlock.GetLength());
                if (!EVP_DecryptFinal_ex(&m_ctx, finalBlock.GetUnderlyingData(), &writtenSize))
                {
                    m_failure = true;
                    LogErrors();
                    return CryptoBuffer();
                }
                return CryptoBuffer(finalBlock.GetUnderlyingData(), static_cast<size_t>(writtenSize));
            }

            void OpenSSLCipher::Reset()
            {
                Cleanup();
                Init();
            }

            void OpenSSLCipher::Cleanup()
            {
                m_failure = false;
                m_encDecInitialized = false;
                m_encryptionMode = false;
                m_decryptionMode = false;

                if (m_ctx.cipher || m_ctx.cipher_data || m_ctx.engine)
                {
                    EVP_CIPHER_CTX_cleanup(&m_ctx);
                }

                m_ctx.cipher = nullptr;
                m_ctx.cipher_data = nullptr;
                m_ctx.engine = nullptr;
            }

            size_t AES_CBC_Cipher_OpenSSL::BlockSizeBytes = 16;
            size_t AES_CBC_Cipher_OpenSSL::KeyLengthBits = 256;
            static const char* CBC_LOG_TAG = "AES_CBC_Cipher_OpenSSL";

            AES_CBC_Cipher_OpenSSL::AES_CBC_Cipher_OpenSSL(const CryptoBuffer& key) : OpenSSLCipher(key, BlockSizeBytes)
            { }

            AES_CBC_Cipher_OpenSSL::AES_CBC_Cipher_OpenSSL(CryptoBuffer&& key, CryptoBuffer&& initializationVector) :
                    OpenSSLCipher(std::move(key), std::move(initializationVector))
            { }

            AES_CBC_Cipher_OpenSSL::AES_CBC_Cipher_OpenSSL(const CryptoBuffer& key,
                                                           const CryptoBuffer& initializationVector) :
                    OpenSSLCipher(key, initializationVector)
            { }

            void AES_CBC_Cipher_OpenSSL::InitEncryptor_Internal()
            {
                if (!EVP_EncryptInit_ex(&m_ctx, EVP_aes_256_cbc(), nullptr, m_key.GetUnderlyingData(),
                                        m_initializationVector.GetUnderlyingData()))
                {
                    m_failure = true;
                    LogErrors(CBC_LOG_TAG);
                }
            }

            void AES_CBC_Cipher_OpenSSL::InitDecryptor_Internal()
            {
                if (!EVP_DecryptInit_ex(&m_ctx, EVP_aes_256_cbc(), nullptr, m_key.GetUnderlyingData(),
                                        m_initializationVector.GetUnderlyingData()))
                {
                    m_failure = true;
                    LogErrors(CBC_LOG_TAG);
                }
            }

            size_t AES_CBC_Cipher_OpenSSL::GetBlockSizeBytes() const
            {
                return BlockSizeBytes;
            }

            size_t AES_CBC_Cipher_OpenSSL::GetKeyLengthBits() const
            {
                return KeyLengthBits;
            }

            size_t AES_CTR_Cipher_OpenSSL::BlockSizeBytes = 16;
            size_t AES_CTR_Cipher_OpenSSL::KeyLengthBits = 256;
            static const char* CTR_LOG_TAG = "AES_CTR_Cipher_OpenSSL";

            AES_CTR_Cipher_OpenSSL::AES_CTR_Cipher_OpenSSL(const CryptoBuffer& key) : OpenSSLCipher(key, BlockSizeBytes,
                                                                                                    true)
            { }

            AES_CTR_Cipher_OpenSSL::AES_CTR_Cipher_OpenSSL(CryptoBuffer&& key, CryptoBuffer&& initializationVector) :
                    OpenSSLCipher(std::move(key), std::move(initializationVector))
            { }

            AES_CTR_Cipher_OpenSSL::AES_CTR_Cipher_OpenSSL(const CryptoBuffer& key,
                                                           const CryptoBuffer& initializationVector) :
                    OpenSSLCipher(key, initializationVector)
            { }

            void AES_CTR_Cipher_OpenSSL::InitEncryptor_Internal()
            {
                if (!(EVP_EncryptInit_ex(&m_ctx, EVP_aes_256_ctr(), nullptr, m_key.GetUnderlyingData(),
                                         m_initializationVector.GetUnderlyingData())
                        && EVP_CIPHER_CTX_set_padding(&m_ctx, 0)))
                {
                    m_failure = true;
                    LogErrors(CTR_LOG_TAG);
                }
            }

            void AES_CTR_Cipher_OpenSSL::InitDecryptor_Internal()
            {
                if (!(EVP_DecryptInit_ex(&m_ctx, EVP_aes_256_ctr(), nullptr, m_key.GetUnderlyingData(),
                                         m_initializationVector.GetUnderlyingData())
                        && EVP_CIPHER_CTX_set_padding(&m_ctx, 0)))
                {
                    m_failure = true;
                    LogErrors(CTR_LOG_TAG);
                }
            }

            size_t AES_CTR_Cipher_OpenSSL::GetBlockSizeBytes() const
            {
                return BlockSizeBytes;
            }

            size_t AES_CTR_Cipher_OpenSSL::GetKeyLengthBits() const
            {
                return KeyLengthBits;
            }

            size_t AES_GCM_Cipher_OpenSSL::BlockSizeBytes = 16;
            size_t AES_GCM_Cipher_OpenSSL::KeyLengthBits = 256;
            size_t AES_GCM_Cipher_OpenSSL::IVLengthBytes = 12;
            size_t AES_GCM_Cipher_OpenSSL::TagLengthBytes = 16;

            static const char* GCM_LOG_TAG = "AES_GCM_Cipher_OpenSSL";

            AES_GCM_Cipher_OpenSSL::AES_GCM_Cipher_OpenSSL(const CryptoBuffer& key) : OpenSSLCipher(key, IVLengthBytes)
            { }

            AES_GCM_Cipher_OpenSSL::AES_GCM_Cipher_OpenSSL(CryptoBuffer&& key, CryptoBuffer&& initializationVector,
                                                           CryptoBuffer&& tag) :
                    OpenSSLCipher(std::move(key), std::move(initializationVector), std::move(tag))
            { }

            AES_GCM_Cipher_OpenSSL::AES_GCM_Cipher_OpenSSL(const CryptoBuffer& key,
                                                           const CryptoBuffer& initializationVector,
                                                           const CryptoBuffer& tag) :
                    OpenSSLCipher(key, initializationVector, tag)
            { }

            CryptoBuffer AES_GCM_Cipher_OpenSSL::FinalizeEncryption()
            {
                CryptoBuffer&& finalBuffer = OpenSSLCipher::FinalizeEncryption();
                m_tag = CryptoBuffer(TagLengthBytes);
                if (!EVP_CIPHER_CTX_ctrl(&m_ctx, EVP_CTRL_CCM_GET_TAG, static_cast<int>(m_tag.GetLength()),
                                         m_tag.GetUnderlyingData()))
                {
                    m_failure = true;
                    LogErrors(GCM_LOG_TAG);
                    return CryptoBuffer();
                }

                return finalBuffer;
            }

            void AES_GCM_Cipher_OpenSSL::InitEncryptor_Internal()
            {
                if (!(EVP_EncryptInit_ex(&m_ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) &&
                        EVP_EncryptInit_ex(&m_ctx, nullptr, nullptr, m_key.GetUnderlyingData(),
                                           m_initializationVector.GetUnderlyingData()) &&
                        EVP_CIPHER_CTX_set_padding(&m_ctx, 0)))
                {
                    m_failure = true;
                    LogErrors(GCM_LOG_TAG);
                }
            }

            void AES_GCM_Cipher_OpenSSL::InitDecryptor_Internal()
            {
                if (!(EVP_DecryptInit_ex(&m_ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) &&
                        EVP_DecryptInit_ex(&m_ctx, nullptr, nullptr, m_key.GetUnderlyingData(),
                                           m_initializationVector.GetUnderlyingData()) &&
                        EVP_CIPHER_CTX_set_padding(&m_ctx, 0)))
                {
                    m_failure = true;
                    LogErrors(GCM_LOG_TAG);
                    return;
                }

                //tag should always be set in GCM decrypt mode
                assert(m_tag.GetLength() > 0);

                if (m_tag.GetLength() < TagLengthBytes)
                {
                    AWS_LOGSTREAM_ERROR(GCM_LOG_TAG,
                                        "Illegal attempt to decrypt an AES GCM payload without a valid tag set: tag length=" <<
                                                m_tag.GetLength());
                    m_failure = true;
                    return;
                }

                if (!EVP_CIPHER_CTX_ctrl(&m_ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(m_tag.GetLength()),
                                         m_tag.GetUnderlyingData()))
                {
                    m_failure = true;
                    LogErrors(GCM_LOG_TAG);
                }
            }

            size_t AES_GCM_Cipher_OpenSSL::GetBlockSizeBytes() const
            {
                return BlockSizeBytes;
            }

            size_t AES_GCM_Cipher_OpenSSL::GetKeyLengthBits() const
            {
                return KeyLengthBits;
            }

            size_t AES_GCM_Cipher_OpenSSL::GetTagLengthBytes() const
            {
                return TagLengthBytes;
            }

            size_t AES_KeyWrap_Cipher_OpenSSL::KeyLengthBits = 256;
            size_t AES_KeyWrap_Cipher_OpenSSL::BlockSizeBytes = 8;
            static const unsigned char INTEGRITY_VALUE = 0xA6;
            static const size_t MIN_CEK_LENGTH_BYTES = 128 / 8;

            static const char* KEY_WRAP_TAG = "AES_KeyWrap_Cipher_OpenSSL";

            AES_KeyWrap_Cipher_OpenSSL::AES_KeyWrap_Cipher_OpenSSL(const CryptoBuffer& key) : OpenSSLCipher(key, 0)
            {
            }

            CryptoBuffer AES_KeyWrap_Cipher_OpenSSL::EncryptBuffer(const CryptoBuffer& plainText)
            {
                CheckInitEncryptor();
                m_workingKeyBuffer = CryptoBuffer({&m_workingKeyBuffer, (CryptoBuffer*) &plainText});
                return CryptoBuffer();
            }

            CryptoBuffer AES_KeyWrap_Cipher_OpenSSL::FinalizeEncryption()
            {
                CheckInitEncryptor();
                if (m_workingKeyBuffer.GetLength() < MIN_CEK_LENGTH_BYTES)
                {
                    AWS_LOGSTREAM_ERROR(KEY_WRAP_TAG, "Incorrect input length of " << m_workingKeyBuffer.GetLength());
                    m_failure = true;
                    return CryptoBuffer();
                }

                //the following is an in place implementation of
                //RFC 3394 using the alternate in-place implementation.
                //we use one in-place buffer instead of the copy at the end.
                //the one letter variable names are meant to directly reflect the variables in the RFC
                CryptoBuffer cipherText(m_workingKeyBuffer.GetLength() + BlockSizeBytes);

                //put the integrity check register in the first 8 bytes of the final buffer.
                memset(cipherText.GetUnderlyingData(), INTEGRITY_VALUE, BlockSizeBytes);
                unsigned char* a = cipherText.GetUnderlyingData();

                //put the register buffer after the integrity check register
                memcpy(cipherText.GetUnderlyingData() + BlockSizeBytes, m_workingKeyBuffer.GetUnderlyingData(),
                       m_workingKeyBuffer.GetLength());
                unsigned char* r = cipherText.GetUnderlyingData() + BlockSizeBytes;

                int n = static_cast<int>(m_workingKeyBuffer.GetLength() / BlockSizeBytes);

                //temporary encryption buffer
                CryptoBuffer b(BlockSizeBytes * 2);
                int outLen = static_cast<int>(b.GetLength());

                //concatenation buffer
                CryptoBuffer tempInput(BlockSizeBytes * 2);

                for (int j = 0; j <= 5; ++j)
                {
                    for (int i = 1; i <= n; ++i)
                    {
                        //concat A and R[i], A should be most significant and then R[i] should be least significant.
                        memcpy(tempInput.GetUnderlyingData(), a, BlockSizeBytes);
                        memcpy(tempInput.GetUnderlyingData() + BlockSizeBytes, r, BlockSizeBytes);

                        //encrypt the concatenated A and R[I] and store it in B
                        if (!EVP_EncryptUpdate(&m_ctx, b.GetUnderlyingData(), &outLen,
                                               tempInput.GetUnderlyingData(), static_cast<int>(tempInput.GetLength())))
                        {
                            LogErrors(KEY_WRAP_TAG);
                            m_failure = true;
                            return CryptoBuffer();
                        }

                        unsigned char t = static_cast<unsigned char>((n * j) + i);
                        //put the 64 MSB ^ T into A
                        memcpy(a, b.GetUnderlyingData(), BlockSizeBytes);
                        a[7] ^= t;
                        //put the 64 LSB into R[i]
                        memcpy(r, b.GetUnderlyingData() + BlockSizeBytes, BlockSizeBytes);
                        //increment i -> R[i]
                        r += BlockSizeBytes;
                    }
                    //reset R
                    r = cipherText.GetUnderlyingData() + BlockSizeBytes;
                }

                return cipherText;
            }

            CryptoBuffer AES_KeyWrap_Cipher_OpenSSL::DecryptBuffer(const CryptoBuffer& cipherText)
            {
                CheckInitDecryptor();
                m_workingKeyBuffer = CryptoBuffer({&m_workingKeyBuffer, (CryptoBuffer*)&cipherText});

                return CryptoBuffer();
            }

            CryptoBuffer AES_KeyWrap_Cipher_OpenSSL::FinalizeDecryption()
            {
                CheckInitDecryptor();
                if (m_workingKeyBuffer.GetLength() < MIN_CEK_LENGTH_BYTES + BlockSizeBytes)
                {
                    AWS_LOGSTREAM_ERROR(KEY_WRAP_TAG, "Incorrect input length of " << m_workingKeyBuffer.GetLength());
                    m_failure = true;
                    return CryptoBuffer();
                }

                //the following is an in place implementation of
                //RFC 3394 using the alternate in-place implementation.
                //we use one in-place buffer instead of the copy at the end.
                //the one letter variable names are meant to directly reflect the variables in the RFC
                CryptoBuffer plainText(m_workingKeyBuffer.GetLength() - BlockSizeBytes);
                memcpy(plainText.GetUnderlyingData(), m_workingKeyBuffer.GetUnderlyingData() + BlockSizeBytes, plainText.GetLength());

                //integrity register should be the first 8 bytes of the cipher text
                unsigned char* a = m_workingKeyBuffer.GetUnderlyingData();

                //in-place register is the plaintext. For decryption, start at the last array position (8 bytes before the end);
                unsigned char* r = plainText.GetUnderlyingData() + plainText.GetLength() - BlockSizeBytes;

                int n = static_cast<int>(plainText.GetLength() / BlockSizeBytes);

                //temporary encryption buffer
                CryptoBuffer b(BlockSizeBytes * 10);
                int outLen = static_cast<int>(b.GetLength());

                //concatenation buffer
                CryptoBuffer tempInput(BlockSizeBytes * 2);

                for(int j = 5; j >= 0; --j)
                {
                    for(int i = n; i >= 1; --i)
                    {
                        //concat
                        //A ^ t
                        memcpy(tempInput.GetUnderlyingData(), a, BlockSizeBytes);
                        unsigned char t = static_cast<unsigned char>((n * j) + i);
                        tempInput[7] ^= t;
                        //R[i]
                        memcpy(tempInput.GetUnderlyingData() + BlockSizeBytes, r, BlockSizeBytes);

                        //Decrypt the concatenated buffer
                        if(!EVP_DecryptUpdate(&m_ctx, b.GetUnderlyingData(), &outLen,
                                              tempInput.GetUnderlyingData(), static_cast<int>(tempInput.GetLength())))
                        {
                            m_failure = true;
                            LogErrors(KEY_WRAP_TAG);
                            return CryptoBuffer();
                        }

                        //set A to MSB 64 bits of decrypted result
                        memcpy(a, b.GetUnderlyingData(), BlockSizeBytes);
                        //set R[i] to LSB 64 bits of decrypted result
                        memcpy(r, b.GetUnderlyingData() + BlockSizeBytes, BlockSizeBytes);
                        //decrement i -> R[i]
                        r -= BlockSizeBytes;
                    }

                    r = plainText.GetUnderlyingData() + plainText.GetLength() - BlockSizeBytes;
                }

                //here we perform the integrity check to make sure A == 0xA6A6A6A6A6A6A6A6
                for(size_t i = 0; i < BlockSizeBytes; ++i)
                {
                    if(a[i] != INTEGRITY_VALUE)
                    {
                        m_failure = true;
                        AWS_LOGSTREAM_ERROR(KEY_WRAP_TAG, "Integrity check failed for key wrap decryption.");
                        return CryptoBuffer();
                    }
                }

                return plainText;
            }

            void AES_KeyWrap_Cipher_OpenSSL::InitEncryptor_Internal()
            {
                if (!EVP_EncryptInit_ex(&m_ctx, EVP_aes_256_ecb(), nullptr, m_key.GetUnderlyingData(), nullptr) &&
                        EVP_CIPHER_CTX_set_padding(&m_ctx, 0))
                {
                    m_failure = true;
                    LogErrors(KEY_WRAP_TAG);
                }
            }

            void AES_KeyWrap_Cipher_OpenSSL::InitDecryptor_Internal()
            {
                if (!(EVP_DecryptInit_ex(&m_ctx, EVP_aes_256_ecb(), nullptr, m_key.GetUnderlyingData(), nullptr) &&
                        EVP_CIPHER_CTX_set_padding(&m_ctx, 0)))
                {
                    m_failure = true;
                    LogErrors(KEY_WRAP_TAG);
                    return;
                }
            }
        }
    }
}
