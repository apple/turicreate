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

#include <aws/core/utils/threading/Semaphore.h>
#include <algorithm>

using namespace Aws::Utils::Threading;

Semaphore::Semaphore(size_t initialCount, size_t maxCount)
    : m_count(initialCount), m_maxCount(maxCount)
{
}

void Semaphore::WaitOne()
{
    std::unique_lock<std::mutex> locker(m_mutex);
    if(0 == m_count)
    {
        m_syncPoint.wait(locker, [this] { return m_count > 0; });
    }
    --m_count;
}

void Semaphore::Release()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    m_count = (std::min)(m_maxCount, m_count + 1);
    m_syncPoint.notify_one();
}

void Semaphore::ReleaseAll()
{    
    std::lock_guard<std::mutex> locker(m_mutex);
    m_count = m_maxCount;
    m_syncPoint.notify_all();
}

