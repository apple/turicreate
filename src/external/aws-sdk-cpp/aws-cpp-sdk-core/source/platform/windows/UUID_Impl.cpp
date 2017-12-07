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

#include <aws/core/utils/UUID.h>
#include <rpc.h>

namespace Aws
{
    namespace Utils
    {
        Aws::Utils::UUID Aws::Utils::UUID::RandomUUID()
        {
            ::UUID uuidStruct;
            UuidCreate(&uuidStruct);

            unsigned char newUuid[UUID_BINARY_SIZE];
            memcpy(newUuid, &uuidStruct.Data1, sizeof(uuidStruct.Data1));
            memcpy(newUuid + 4, &uuidStruct.Data2, sizeof(uuidStruct.Data2));
            memcpy(newUuid + 6, &uuidStruct.Data3, sizeof(uuidStruct.Data3));
            memcpy(newUuid + 8, &uuidStruct.Data4, sizeof(uuidStruct.Data4));
            return Aws::Utils::UUID(newUuid);
        }
    }
}