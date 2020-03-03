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

#include <WinSock2.h>
#include <cassert>
#include <aws/core/utils/logging/LogMacros.h>

namespace Aws
{
    namespace Net
    {
        static bool s_globalNetworkInitiated = false;

        bool IsNetworkInitiated() 
        {
            return s_globalNetworkInitiated;
        }

        void InitNetwork()
        {
            if (IsNetworkInitiated())
            {
                return;
            }
            // Initialize Winsock( requires winsock version 2.2)
            WSADATA wsaData;
            int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
            assert(result == NO_ERROR);
            if (result != NO_ERROR)
            {
                AWS_LOGSTREAM_ERROR("WinSock2", "Failed to Initate WinSock2.2");
                s_globalNetworkInitiated = false;
            }
            else
            {
                s_globalNetworkInitiated = true;
            }
        }

        void CleanupNetwork()
        {
            WSACleanup();
            s_globalNetworkInitiated = false;
        }
    }
}
