/*
* Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/core/net/Net.h>

namespace Aws
{
    namespace Net
    {
        // For Posix system, currently we don't need to do anything for network stack initialization.
        // But we need to do initialization for WinSock on Windows and call them in Aws.cpp. So these functions
        // also exist for Posix systems.
        bool IsNetworkInitiated() 
        {
            return true;
        }

        void InitNetwork()
        {
        }

        void CleanupNetwork()
        {
        }
    }
}
