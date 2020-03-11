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
#include <aws/core/utils/GetTheLights.h>
#include <cassert>

namespace Aws
{
    namespace Utils
    {
        GetTheLights::GetTheLights() : m_value(0)
        {
        }

        void GetTheLights::EnterRoom(std::function<void()> &&callable)
        {
            int cpy = ++m_value;
            assert(cpy > 0);
            if(cpy == 1)
            {
                callable();
            }
        }

        void GetTheLights::LeaveRoom(std::function<void()> &&callable)
        {
            int cpy = --m_value;
            assert(cpy >= 0);
            if(cpy == 0)
            {
                callable();
            }
        }
    }
}