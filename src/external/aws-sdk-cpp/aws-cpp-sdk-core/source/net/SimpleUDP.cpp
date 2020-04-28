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

#include <cstddef>
#include <aws/core/utils/UnreferencedParam.h>
#include <aws/core/net/SimpleUDP.h>

namespace Aws
{
    namespace Net
    {
        SimpleUDP::SimpleUDP(int, size_t, size_t, bool)
        {
        }

        SimpleUDP::SimpleUDP(bool, size_t, size_t, bool)
        {
        }

        SimpleUDP::SimpleUDP(const char*, unsigned short, size_t, size_t, bool)
        {
            //prevent compiler warning for unused private variables
            m_port = 0;
        }

        SimpleUDP::~SimpleUDP()
        {
        }

        void SimpleUDP::CreateSocket(int, size_t, size_t, bool)
        {
        }

        int SimpleUDP::Connect(const sockaddr*, size_t)
        {
            return -1;
        }

        int SimpleUDP::ConnectToHost(const char*, unsigned short) const
        {
            return -1;
        }

        int SimpleUDP::ConnectToLocalHost(unsigned short) const
        {
            return -1;
        }

        int SimpleUDP::Bind(const sockaddr*, size_t) const
        {
            return -1;
        }

        int SimpleUDP::BindToLocalHost(unsigned short) const
        {
            return -1;
        }

        int SimpleUDP::SendData(const uint8_t*, size_t) const
        {
            return -1;
        }

        int SimpleUDP::SendDataTo(const sockaddr*, size_t, const uint8_t*, size_t) const
        {
            return -1;
        }

        int SimpleUDP::SendDataToLocalHost(const uint8_t*, size_t, unsigned short) const
        {
            return -1;
        }

        int SimpleUDP::ReceiveData(uint8_t*, size_t) const
        {
            return -1;
        }

        int SimpleUDP::ReceiveDataFrom(sockaddr*, size_t*, uint8_t*, size_t) const
        {
            return -1;
        }
    }
}
