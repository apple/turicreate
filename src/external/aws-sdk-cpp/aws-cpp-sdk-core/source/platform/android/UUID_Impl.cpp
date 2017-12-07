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

namespace Aws
{
    namespace Utils
    {
        UUID UUID::RandomUUID()
        {
            char uuid[UUID_STR_SIZE];
            memset(uuid, 0, sizeof(uuid));

            int fd = fopen("/proc/sys/kernel/random/uuid", "r");

            if(fd)
            {
                fread(uuid, sizeof(uuid), sizeof(uuid), fd);
                fclose(fd);
            }
            Aws::String uuidStr(uuid);
            return UUID(uuidStr);
        }
    }
}