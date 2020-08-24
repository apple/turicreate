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
#include <aws/core/utils/crypto/CryptoStream.h>

namespace Aws
{
    namespace Utils
    {
        namespace Crypto
        {
            static const char* CLASS_TAG = "Aws::Utils::Crypto::SymmetricCryptoStream";

            SymmetricCryptoStream::SymmetricCryptoStream(Aws::IStream& src, CipherMode mode, SymmetricCipher& cipher, size_t bufSize) :
                Aws::IOStream(m_cryptoBuf = Aws::New<SymmetricCryptoBufSrc>(CLASS_TAG, src, cipher, mode, bufSize)), m_hasOwnership(true)
            {
            }

            SymmetricCryptoStream::SymmetricCryptoStream(Aws::OStream& sink, CipherMode mode, SymmetricCipher& cipher, size_t bufSize, int16_t blockOffset) :
                    Aws::IOStream(m_cryptoBuf = Aws::New<SymmetricCryptoBufSink>(CLASS_TAG, sink, cipher, mode, bufSize, blockOffset)), m_hasOwnership(true)
            {
            }

            SymmetricCryptoStream::SymmetricCryptoStream(Aws::Utils::Crypto::SymmetricCryptoBufSrc& bufSrc) :
                    Aws::IOStream(&bufSrc), m_cryptoBuf(&bufSrc), m_hasOwnership(false)
            {
            }

            SymmetricCryptoStream::SymmetricCryptoStream(Aws::Utils::Crypto::SymmetricCryptoBufSink& bufSink) :
                    Aws::IOStream(&bufSink), m_cryptoBuf(&bufSink), m_hasOwnership(false)
            {
            }

            SymmetricCryptoStream::~SymmetricCryptoStream()
            {
                Finalize();

                if(m_hasOwnership && m_cryptoBuf)
                {
                    Aws::Delete(m_cryptoBuf);
                }
            }

            void SymmetricCryptoStream::Finalize()
            {
                assert(m_cryptoBuf);
                m_cryptoBuf->Finalize();
            }
        }
    }
}