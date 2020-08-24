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


#include <aws/core/utils/crypto/Sha256HMAC.h>
#include <aws/core/utils/crypto/Factories.h>
#include <aws/core/utils/Outcome.h>

namespace Aws
{
namespace Utils
{
namespace Crypto
{

Sha256HMAC::Sha256HMAC() : 
    m_hmacImpl(CreateSha256HMACImplementation())
{
}

Sha256HMAC::~Sha256HMAC()
{
}

HashResult Sha256HMAC::Calculate(const Aws::Utils::ByteBuffer& toSign, const Aws::Utils::ByteBuffer& secret)
{
    return m_hmacImpl->Calculate(toSign, secret);
}

} // namespace Crypto
} // namespace Utils
} // namespace Aws