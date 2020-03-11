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

#include <aws/core/platform/Security.h>

#include <aws/core/utils/UnreferencedParam.h>

#include <cstring>

namespace Aws
{
namespace Security
{

void SecureMemClear(unsigned char *data, size_t length)
{
    memset(data, 0, length);
    if(length > 0)
    {
        volatile unsigned char* volData = (volatile unsigned char *)data;

        auto val = *volData;
        AWS_UNREFERENCED_PARAM(val);
    }
}

} // namespace Security
} // namespace Aws
