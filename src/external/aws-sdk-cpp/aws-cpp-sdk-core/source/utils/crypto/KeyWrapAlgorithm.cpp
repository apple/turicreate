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
#include <aws/core/utils/crypto/KeyWrapAlgorithm.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/EnumParseOverflowContainer.h>
#include <aws/core/Globals.h>

using namespace Aws::Utils;

namespace Aws
{
    namespace Utils
    {
        namespace Crypto
        {
            namespace KeyWrapAlgorithmMapper
            {
                static const int keyWrapAlgorithm_KMS_HASH = HashingUtils::HashString("kms");
                static const int keyWrapAlgorithm_KeyWrap_HASH = HashingUtils::HashString("AESWrap");

                KeyWrapAlgorithm GetKeyWrapAlgorithmForName(const Aws::String& name)
                {
                    int hashcode = HashingUtils::HashString(name.c_str());
                    if (hashcode == keyWrapAlgorithm_KMS_HASH)
                    {
                        return KeyWrapAlgorithm::KMS;
                    }
                    else if (hashcode == keyWrapAlgorithm_KeyWrap_HASH)
                    {
                        return KeyWrapAlgorithm::AES_KEY_WRAP;
                    }
                    assert(0);
                    return KeyWrapAlgorithm::NONE;
                }

                Aws::String GetNameForKeyWrapAlgorithm(KeyWrapAlgorithm enumValue)
                {
                    switch (enumValue)
                    {
                    case KeyWrapAlgorithm::KMS:
                        return "kms";
                    case KeyWrapAlgorithm::AES_KEY_WRAP:
                        return "AESWrap";
                    default:
                        assert(0);
                    }
                    return "";
                }
            }//namespace KeyWrapAlgorithmMapper
        }//namespace Crypto
    }//namespace Utils
}//namespace Aws