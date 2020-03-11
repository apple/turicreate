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
#include <aws/core/utils/crypto/ContentCryptoMaterial.h>
#include <aws/core/utils/crypto/Cipher.h>

using namespace Aws::Utils::Crypto;

namespace Aws
{
    namespace Utils
    {
        namespace Crypto
        {
            ContentCryptoMaterial::ContentCryptoMaterial() :
                m_cryptoTagLength(0), m_keyWrapAlgorithm(KeyWrapAlgorithm::NONE), m_contentCryptoScheme(ContentCryptoScheme::NONE)
            {
            }

            ContentCryptoMaterial::ContentCryptoMaterial(ContentCryptoScheme contentCryptoScheme) :
                m_contentEncryptionKey(SymmetricCipher::GenerateKey()), m_cryptoTagLength(0), m_keyWrapAlgorithm(KeyWrapAlgorithm::NONE), m_contentCryptoScheme(contentCryptoScheme)
            {

            }

            ContentCryptoMaterial::ContentCryptoMaterial(const Aws::Utils::CryptoBuffer & cek, ContentCryptoScheme contentCryptoScheme) :
                m_contentEncryptionKey(cek), m_cryptoTagLength(0), m_keyWrapAlgorithm(KeyWrapAlgorithm::NONE), m_contentCryptoScheme(contentCryptoScheme)
            {

            }
        }
    }
}
