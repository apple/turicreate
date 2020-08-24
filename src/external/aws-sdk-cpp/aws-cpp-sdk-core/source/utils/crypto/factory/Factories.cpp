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


#include <aws/core/utils/crypto/Factories.h>
#include <aws/core/utils/crypto/Hash.h>
#include <aws/core/utils/crypto/HMAC.h>

#if ENABLE_BCRYPT_ENCRYPTION
    #include <aws/core/utils/crypto/bcrypt/CryptoImpl.h>
#elif ENABLE_OPENSSL_ENCRYPTION
    #include <aws/core/utils/crypto/openssl/CryptoImpl.h>
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
    #include <aws/core/utils/crypto/commoncrypto/CryptoImpl.h>
    #include <aws/core/utils/logging/LogMacros.h>
#else
    // if you don't have any encryption you still need to pull in the interface definitions
    #include <aws/core/utils/crypto/Hash.h>
    #include <aws/core/utils/crypto/HMAC.h>
    #include <aws/core/utils/crypto/Cipher.h>
    #include <aws/core/utils/crypto/SecureRandom.h>
    #define NO_ENCRYPTION
#endif

using namespace Aws::Utils;
using namespace Aws::Utils::Crypto;

static const char *s_allocationTag = "CryptoFactory";

static std::shared_ptr<HashFactory> s_MD5Factory(nullptr);
static std::shared_ptr<HashFactory> s_Sha256Factory(nullptr);
static std::shared_ptr<HMACFactory> s_Sha256HMACFactory(nullptr);

static std::shared_ptr<SymmetricCipherFactory> s_AES_CBCFactory(nullptr);
static std::shared_ptr<SymmetricCipherFactory> s_AES_CTRFactory(nullptr);
static std::shared_ptr<SymmetricCipherFactory> s_AES_GCMFactory(nullptr);
static std::shared_ptr<SymmetricCipherFactory> s_AES_KeyWrapFactory(nullptr);

static std::shared_ptr<SecureRandomFactory> s_SecureRandomFactory(nullptr);
static std::shared_ptr<SecureRandomBytes> s_SecureRandom(nullptr);

static bool s_InitCleanupOpenSSLFlag(false);

class DefaultMD5Factory : public HashFactory
{
public:
    std::shared_ptr<Hash> CreateImplementation() const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<MD5BcryptImpl>(s_allocationTag);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<MD5OpenSSLImpl>(s_allocationTag);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        return Aws::MakeShared<MD5CommonCryptoImpl>(s_allocationTag);
#else
    return nullptr;
#endif
    }

    /**
     * Opportunity to make any static initialization calls you need to make.
     * Will only be called once.
     */
    void InitStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.EnterRoom(&OpenSSL::init_static_state);
        }
#endif
    }

    /**
     * Opportunity to make any static cleanup calls you need to make.
     * will only be called at the end of the application.
     */
    void CleanupStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.LeaveRoom(&OpenSSL::cleanup_static_state);
        }
#endif
    }
};

class DefaultSHA256Factory : public HashFactory
{
public:
    std::shared_ptr<Hash> CreateImplementation() const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<Sha256BcryptImpl>(s_allocationTag);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<Sha256OpenSSLImpl>(s_allocationTag);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        return Aws::MakeShared<Sha256CommonCryptoImpl>(s_allocationTag);
#else
    return nullptr;
#endif
    }

    /**
     * Opportunity to make any static initialization calls you need to make.
     * Will only be called once.
     */
    void InitStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.EnterRoom(&OpenSSL::init_static_state);
        }
#endif
    }

    /**
     * Opportunity to make any static cleanup calls you need to make.
     * will only be called at the end of the application.
     */
    void CleanupStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.LeaveRoom(&OpenSSL::cleanup_static_state);
        }
#endif
    }
};

class DefaultSHA256HmacFactory : public HMACFactory
{
public:
    std::shared_ptr<Aws::Utils::Crypto::HMAC> CreateImplementation() const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<Sha256HMACBcryptImpl>(s_allocationTag);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<Sha256HMACOpenSSLImpl>(s_allocationTag);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        return Aws::MakeShared<Sha256HMACCommonCryptoImpl>(s_allocationTag);
#else
    return nullptr;
#endif
    }

    /**
     * Opportunity to make any static initialization calls you need to make.
     * Will only be called once.
     */
    void InitStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.EnterRoom(&OpenSSL::init_static_state);
        }
#endif
    }

    /**
     * Opportunity to make any static cleanup calls you need to make.
     * will only be called at the end of the application.
     */
    void CleanupStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.LeaveRoom(&OpenSSL::cleanup_static_state);
        }
#endif
    }
};


class DefaultAES_CBCFactory : public SymmetricCipherFactory
{
public:
    std::shared_ptr<SymmetricCipher> CreateImplementation(const CryptoBuffer& key) const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<AES_CBC_Cipher_BCrypt>(s_allocationTag, key);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<AES_CBC_Cipher_OpenSSL>(s_allocationTag, key);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        return Aws::MakeShared<AES_CBC_Cipher_CommonCrypto>(s_allocationTag, key);
#else
        AWS_UNREFERENCED_PARAM(key);
        return nullptr;
#endif
    }
    /**
     * Factory method. Returns cipher implementation. See the SymmetricCipher class for more details.
     */
    std::shared_ptr<SymmetricCipher> CreateImplementation(const CryptoBuffer& key, const CryptoBuffer& iv, const CryptoBuffer&) const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<AES_CBC_Cipher_BCrypt>(s_allocationTag, key, iv);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<AES_CBC_Cipher_OpenSSL>(s_allocationTag, key, iv);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        return Aws::MakeShared<AES_CBC_Cipher_CommonCrypto>(s_allocationTag, key, iv);
#else
        AWS_UNREFERENCED_PARAM(key);
        AWS_UNREFERENCED_PARAM(iv);

        return nullptr;
#endif
    }
    /**
     * Factory method. Returns cipher implementation. See the SymmetricCipher class for more details.
     */
    std::shared_ptr<SymmetricCipher> CreateImplementation(CryptoBuffer&& key, CryptoBuffer&& iv, CryptoBuffer&&) const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<AES_CBC_Cipher_BCrypt>(s_allocationTag, key, iv);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<AES_CBC_Cipher_OpenSSL>(s_allocationTag, key, iv);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        return Aws::MakeShared<AES_CBC_Cipher_CommonCrypto>(s_allocationTag, key, iv);
#else
        AWS_UNREFERENCED_PARAM(key);
        AWS_UNREFERENCED_PARAM(iv);

        return nullptr;
#endif
    }

    /**
     * Opportunity to make any static initialization calls you need to make.
     * Will only be called once.
     */
    void InitStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.EnterRoom(&OpenSSL::init_static_state);
        }
#endif
    }

    /**
     * Opportunity to make any static cleanup calls you need to make.
     * will only be called at the end of the application.
     */
    void CleanupStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.LeaveRoom(&OpenSSL::cleanup_static_state);
        }
#endif
    }
};

class DefaultAES_CTRFactory : public SymmetricCipherFactory
{
public:
    std::shared_ptr<SymmetricCipher> CreateImplementation(const CryptoBuffer& key) const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
       return Aws::MakeShared<AES_CTR_Cipher_BCrypt>(s_allocationTag, key);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<AES_CTR_Cipher_OpenSSL>(s_allocationTag, key);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        return Aws::MakeShared<AES_CTR_Cipher_CommonCrypto>(s_allocationTag, key);
#else
        AWS_UNREFERENCED_PARAM(key);
        return nullptr;
#endif
    }
    /**
     * Factory method. Returns cipher implementation. See the SymmetricCipher class for more details.
     */
    std::shared_ptr<SymmetricCipher> CreateImplementation(const CryptoBuffer& key, const CryptoBuffer& iv, const CryptoBuffer&) const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<AES_CTR_Cipher_BCrypt>(s_allocationTag, key, iv);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<AES_CTR_Cipher_OpenSSL>(s_allocationTag, key, iv);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        return Aws::MakeShared<AES_CTR_Cipher_CommonCrypto>(s_allocationTag, key, iv);
#else
        AWS_UNREFERENCED_PARAM(key);
        AWS_UNREFERENCED_PARAM(iv);

        return nullptr;
#endif
    }
    /**
     * Factory method. Returns cipher implementation. See the SymmetricCipher class for more details.
     */
    std::shared_ptr<SymmetricCipher> CreateImplementation(CryptoBuffer&& key, CryptoBuffer&& iv, CryptoBuffer&&) const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<AES_CTR_Cipher_BCrypt>(s_allocationTag, key, iv);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<AES_CTR_Cipher_OpenSSL>(s_allocationTag, key, iv);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        return Aws::MakeShared<AES_CTR_Cipher_CommonCrypto>(s_allocationTag, key, iv);
#else
        AWS_UNREFERENCED_PARAM(key);
        AWS_UNREFERENCED_PARAM(iv);

        return nullptr;
#endif
    }

    /**
     * Opportunity to make any static initialization calls you need to make.
     * Will only be called once.
     */
    void InitStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.EnterRoom(&OpenSSL::init_static_state);
        }
#endif
    }

    /**
     * Opportunity to make any static cleanup calls you need to make.
     * will only be called at the end of the application.
     */
    void CleanupStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.LeaveRoom(&OpenSSL::cleanup_static_state);
        }
#endif
    }
};

class DefaultAES_GCMFactory : public SymmetricCipherFactory
{
public:
    std::shared_ptr<SymmetricCipher> CreateImplementation(const CryptoBuffer& key) const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<AES_GCM_Cipher_BCrypt>(s_allocationTag, key);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<AES_GCM_Cipher_OpenSSL>(s_allocationTag, key);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        AWS_UNREFERENCED_PARAM(key);
        AWS_LOGSTREAM_ERROR(s_allocationTag, "AES GCM is not implemented on this platform, returning null.");
        assert(0);
        return nullptr;
#else
        AWS_UNREFERENCED_PARAM(key);

        return nullptr;
#endif
    }
    /**
     * Factory method. Returns cipher implementation. See the SymmetricCipher class for more details.
     */
    std::shared_ptr<SymmetricCipher> CreateImplementation(const CryptoBuffer& key, const CryptoBuffer& iv, const CryptoBuffer& tag) const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<AES_GCM_Cipher_BCrypt>(s_allocationTag, key, iv, tag);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<AES_GCM_Cipher_OpenSSL>(s_allocationTag, key, iv, tag);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        AWS_UNREFERENCED_PARAM(key);
        AWS_UNREFERENCED_PARAM(iv);
        AWS_UNREFERENCED_PARAM(tag);
        AWS_LOGSTREAM_ERROR(s_allocationTag, "AES GCM is not implemented on this platform, returning null.");
        assert(0);
        return nullptr;
#else
        AWS_UNREFERENCED_PARAM(key);
        AWS_UNREFERENCED_PARAM(iv);
        AWS_UNREFERENCED_PARAM(tag);
        return nullptr;
#endif
    }
    /**
     * Factory method. Returns cipher implementation. See the SymmetricCipher class for more details.
     */
    std::shared_ptr<SymmetricCipher> CreateImplementation(CryptoBuffer&& key, CryptoBuffer&& iv, CryptoBuffer&& tag) const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<AES_GCM_Cipher_BCrypt>(s_allocationTag, std::move(key), std::move(iv), std::move(tag));
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<AES_GCM_Cipher_OpenSSL>(s_allocationTag, std::move(key), std::move(iv), std::move(tag));
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        AWS_UNREFERENCED_PARAM(key);
        AWS_UNREFERENCED_PARAM(iv);
        AWS_UNREFERENCED_PARAM(tag);
        AWS_LOGSTREAM_ERROR(s_allocationTag, "AES GCM is not implemented on this platform, returning null.");
        assert(0);
        return nullptr;
#else
        AWS_UNREFERENCED_PARAM(key);
        AWS_UNREFERENCED_PARAM(iv);
        AWS_UNREFERENCED_PARAM(tag);
        return nullptr;
#endif
    }

    /**
     * Opportunity to make any static initialization calls you need to make.
     * Will only be called once.
     */
    void InitStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.EnterRoom(&OpenSSL::init_static_state);
        }
#endif
    }

    /**
     * Opportunity to make any static cleanup calls you need to make.
     * will only be called at the end of the application.
     */
    void CleanupStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.LeaveRoom(&OpenSSL::cleanup_static_state);
        }
#endif
    }
};

class DefaultAES_KeyWrapFactory : public SymmetricCipherFactory
{
public:
    std::shared_ptr<SymmetricCipher> CreateImplementation(const CryptoBuffer& key) const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<AES_KeyWrap_Cipher_BCrypt>(s_allocationTag, key);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<AES_KeyWrap_Cipher_OpenSSL>(s_allocationTag, key);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        return Aws::MakeShared<AES_KeyWrap_Cipher_CommonCrypto>(s_allocationTag, key);
#else
        AWS_UNREFERENCED_PARAM(key);
        return nullptr;
#endif
    }
    /**
    * Factory method. Returns cipher implementation. See the SymmetricCipher class for more details.
    */
    std::shared_ptr<SymmetricCipher> CreateImplementation(const CryptoBuffer& key, const CryptoBuffer& iv, const CryptoBuffer& tag) const override
    {
        AWS_UNREFERENCED_PARAM(key);
        AWS_UNREFERENCED_PARAM(iv);
        AWS_UNREFERENCED_PARAM(tag);
        return nullptr;
    }
    /**
    * Factory method. Returns cipher implementation. See the SymmetricCipher class for more details.
    */
    std::shared_ptr<SymmetricCipher> CreateImplementation(CryptoBuffer&& key, CryptoBuffer&& iv, CryptoBuffer&& tag) const override
    {
        AWS_UNREFERENCED_PARAM(key);
        AWS_UNREFERENCED_PARAM(iv);
        AWS_UNREFERENCED_PARAM(tag);
        return nullptr;
    }

    /**
    * Opportunity to make any static initialization calls you need to make.
    * Will only be called once.
    */
    void InitStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if (s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.EnterRoom(&OpenSSL::init_static_state);
        }
#endif
    }

    /**
    * Opportunity to make any static cleanup calls you need to make.
    * will only be called at the end of the application.
    */
    void CleanupStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if (s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.LeaveRoom(&OpenSSL::cleanup_static_state);
        }
#endif
    }
};

class DefaultSecureRandFactory : public SecureRandomFactory
{
    /**
     * Factory method. Returns SecureRandom implementation.
     */
    std::shared_ptr<SecureRandomBytes> CreateImplementation() const override
    {
#if ENABLE_BCRYPT_ENCRYPTION
        return Aws::MakeShared<SecureRandomBytes_BCrypt>(s_allocationTag);
#elif ENABLE_OPENSSL_ENCRYPTION
        return Aws::MakeShared<SecureRandomBytes_OpenSSLImpl>(s_allocationTag);
#elif ENABLE_COMMONCRYPTO_ENCRYPTION
        return Aws::MakeShared<SecureRandomBytes_CommonCrypto>(s_allocationTag);
#else
        return nullptr;
#endif
    }

    /**
 * Opportunity to make any static initialization calls you need to make.
 * Will only be called once.
 */
    void InitStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.EnterRoom(&OpenSSL::init_static_state);
        }
#endif
    }

    /**
     * Opportunity to make any static cleanup calls you need to make.
     * will only be called at the end of the application.
     */
    void CleanupStaticState() override
    {
#if ENABLE_OPENSSL_ENCRYPTION
        if(s_InitCleanupOpenSSLFlag)
        {
            OpenSSL::getTheLights.LeaveRoom(&OpenSSL::cleanup_static_state);
        }
#endif
    }
};

void Aws::Utils::Crypto::SetInitCleanupOpenSSLFlag(bool initCleanupFlag)
{
    s_InitCleanupOpenSSLFlag = initCleanupFlag;
}

void Aws::Utils::Crypto::InitCrypto()
{
    if(s_MD5Factory)
    {
        s_MD5Factory->InitStaticState();
    }
    else
    {
        s_MD5Factory = Aws::MakeShared<DefaultMD5Factory>(s_allocationTag);
        s_MD5Factory->InitStaticState();
    }

    if(s_Sha256Factory)
    {
        s_Sha256Factory->InitStaticState();
    }
    else
    {
        s_Sha256Factory = Aws::MakeShared<DefaultSHA256Factory>(s_allocationTag);
        s_Sha256Factory->InitStaticState();
    }

    if(s_Sha256HMACFactory)
    {
        s_Sha256HMACFactory->InitStaticState();
    }
    else
    {
        s_Sha256HMACFactory = Aws::MakeShared<DefaultSHA256HmacFactory>(s_allocationTag);
        s_Sha256HMACFactory->InitStaticState();
    }

    if(s_AES_CBCFactory)
    {
        s_AES_CBCFactory->InitStaticState();
    }
    else
    {
        s_AES_CBCFactory = Aws::MakeShared<DefaultAES_CBCFactory>(s_allocationTag);
        s_AES_CBCFactory->InitStaticState();
    }

    if(s_AES_CTRFactory)
    {
        s_AES_CTRFactory->InitStaticState();
    }
    else
    {
        s_AES_CTRFactory = Aws::MakeShared<DefaultAES_CTRFactory>(s_allocationTag);
        s_AES_CTRFactory->InitStaticState();
    }

    if(s_AES_GCMFactory)
    {
        s_AES_GCMFactory->InitStaticState();
    }
    else
    {
        s_AES_GCMFactory = Aws::MakeShared<DefaultAES_GCMFactory>(s_allocationTag);
        s_AES_GCMFactory->InitStaticState();
    }

    if (!s_AES_KeyWrapFactory)
    {
        s_AES_KeyWrapFactory = Aws::MakeShared<DefaultAES_KeyWrapFactory>(s_allocationTag);
    }
    s_AES_KeyWrapFactory->InitStaticState();

    if(s_SecureRandomFactory)
    {
        s_SecureRandomFactory->InitStaticState();       
    }
    else
    {
        s_SecureRandomFactory = Aws::MakeShared<DefaultSecureRandFactory>(s_allocationTag);
        s_SecureRandomFactory->InitStaticState();
    }  
    
    s_SecureRandom = s_SecureRandomFactory->CreateImplementation();
}

void Aws::Utils::Crypto::CleanupCrypto()
{
    if(s_MD5Factory)
    {
        s_MD5Factory->CleanupStaticState();
        s_MD5Factory = nullptr;
    }

    if(s_Sha256Factory)
    {
        s_Sha256Factory->CleanupStaticState();
        s_Sha256Factory = nullptr;
    }

    if(s_Sha256HMACFactory)
    {
        s_Sha256HMACFactory->CleanupStaticState();
        s_Sha256HMACFactory =  nullptr;
    }

    if(s_AES_CBCFactory)
    {
        s_AES_CBCFactory->CleanupStaticState();
        s_AES_CBCFactory = nullptr;
    }

    if(s_AES_CTRFactory)
    {
        s_AES_CTRFactory->CleanupStaticState();
        s_AES_CTRFactory = nullptr;
    }

    if(s_AES_GCMFactory)
    {
        s_AES_GCMFactory->CleanupStaticState();
        s_AES_GCMFactory = nullptr;
    }

    if(s_AES_KeyWrapFactory)
    {
        s_AES_KeyWrapFactory->CleanupStaticState();
        s_AES_KeyWrapFactory = nullptr;
    }

    if(s_SecureRandomFactory)
    {
        s_SecureRandom = nullptr;
        s_SecureRandomFactory->CleanupStaticState();
        s_SecureRandomFactory = nullptr;
    }   
}

void Aws::Utils::Crypto::SetMD5Factory(const std::shared_ptr<HashFactory>& factory)
{
    s_MD5Factory = factory;
}

void Aws::Utils::Crypto::SetSha256Factory(const std::shared_ptr<HashFactory>& factory)
{
    s_Sha256Factory = factory;
}

void Aws::Utils::Crypto::SetSha256HMACFactory(const std::shared_ptr<HMACFactory>& factory)
{
    s_Sha256HMACFactory = factory;
}

void Aws::Utils::Crypto::SetAES_CBCFactory(const std::shared_ptr<SymmetricCipherFactory>& factory)
{
    s_AES_CBCFactory = factory;
}

void Aws::Utils::Crypto::SetAES_CTRFactory(const std::shared_ptr<SymmetricCipherFactory>& factory)
{
    s_AES_CTRFactory = factory;
}

void Aws::Utils::Crypto::SetAES_GCMFactory(const std::shared_ptr<SymmetricCipherFactory>& factory)
{
    s_AES_GCMFactory = factory;
}

void Aws::Utils::Crypto::SetAES_KeyWrapFactory(const std::shared_ptr<SymmetricCipherFactory>& factory)
{
    s_AES_KeyWrapFactory = factory;
}

void Aws::Utils::Crypto::SetSecureRandomFactory(const std::shared_ptr<SecureRandomFactory>& factory)
{
    s_SecureRandomFactory = factory;
}

std::shared_ptr<Hash> Aws::Utils::Crypto::CreateMD5Implementation()
{
    return s_MD5Factory->CreateImplementation();
}

std::shared_ptr<Hash> Aws::Utils::Crypto::CreateSha256Implementation()
{
    return s_Sha256Factory->CreateImplementation();
}

std::shared_ptr<Aws::Utils::Crypto::HMAC> Aws::Utils::Crypto::CreateSha256HMACImplementation()
{
    return s_Sha256HMACFactory->CreateImplementation();
}

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4702 )
#endif

std::shared_ptr<SymmetricCipher> Aws::Utils::Crypto::CreateAES_CBCImplementation(const CryptoBuffer& key)
{
#ifdef NO_SYMMETRIC_ENCRYPTION
    return nullptr;
#endif
    return s_AES_CBCFactory->CreateImplementation(key);
}

std::shared_ptr<SymmetricCipher> Aws::Utils::Crypto::CreateAES_CBCImplementation(const CryptoBuffer& key, const CryptoBuffer& iv)
{
#ifdef NO_SYMMETRIC_ENCRYPTION
    return nullptr;
#endif
    return s_AES_CBCFactory->CreateImplementation(key, iv);
}

std::shared_ptr<SymmetricCipher> Aws::Utils::Crypto::CreateAES_CBCImplementation(CryptoBuffer&& key, CryptoBuffer&& iv)
{
#ifdef NO_SYMMETRIC_ENCRYPTION
    return nullptr;
#endif
    return s_AES_CBCFactory->CreateImplementation(std::move(key), std::move(iv));
}

std::shared_ptr<SymmetricCipher> Aws::Utils::Crypto::CreateAES_CTRImplementation(const CryptoBuffer& key)
{
#ifdef NO_SYMMETRIC_ENCRYPTION
    return nullptr;
#endif
    return s_AES_CTRFactory->CreateImplementation(key);
}

std::shared_ptr<SymmetricCipher> Aws::Utils::Crypto::CreateAES_CTRImplementation(const CryptoBuffer& key, const CryptoBuffer& iv)
{
#ifdef NO_SYMMETRIC_ENCRYPTION
    return nullptr;
#endif
    return s_AES_CTRFactory->CreateImplementation(key, iv);
}

std::shared_ptr<SymmetricCipher> Aws::Utils::Crypto::CreateAES_CTRImplementation(CryptoBuffer&& key, CryptoBuffer&& iv)
{
#ifdef NO_SYMMETRIC_ENCRYPTION
    return nullptr;
#endif
    return s_AES_CTRFactory->CreateImplementation(std::move(key), std::move(iv));
}

std::shared_ptr<SymmetricCipher> Aws::Utils::Crypto::CreateAES_GCMImplementation(const CryptoBuffer& key)
{
#ifdef NO_SYMMETRIC_ENCRYPTION
    return nullptr;
#endif
    return s_AES_GCMFactory->CreateImplementation(key);
}

std::shared_ptr<SymmetricCipher> Aws::Utils::Crypto::CreateAES_GCMImplementation(const CryptoBuffer& key, const CryptoBuffer& iv, const CryptoBuffer& tag)
{
#ifdef NO_SYMMETRIC_ENCRYPTION
    return nullptr;
#endif
    return s_AES_GCMFactory->CreateImplementation(key, iv, tag);
}

std::shared_ptr<SymmetricCipher> Aws::Utils::Crypto::CreateAES_GCMImplementation(CryptoBuffer&& key, CryptoBuffer&& iv, CryptoBuffer&& tag)
{
#ifdef NO_SYMMETRIC_ENCRYPTION
    return nullptr;
#endif
    return s_AES_GCMFactory->CreateImplementation(std::move(key), std::move(iv), std::move(tag));
}

std::shared_ptr<SymmetricCipher> Aws::Utils::Crypto::CreateAES_KeyWrapImplementation(const CryptoBuffer& key)
{
#ifdef NO_SYMMETRIC_ENCRYPTION
    return nullptr;
#endif
    return s_AES_KeyWrapFactory->CreateImplementation(key);
}

#ifdef _WIN32
#pragma warning(pop)
#endif

std::shared_ptr<SecureRandomBytes> Aws::Utils::Crypto::CreateSecureRandomBytesImplementation()
{
    return s_SecureRandom;
}
