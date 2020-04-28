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

#pragma once

#include <aws/core/utils/DateTime.h>
#include <aws/core/utils/Cache.h>
#include <aws/core/utils/threading/ReaderWriterLock.h>

namespace Aws
{
    namespace Utils
    {
        template <typename TKey, typename TValue>
        class ConcurrentCache
        {
        public:
            explicit ConcurrentCache(size_t size = 1000) : m_cache(size) { }

            bool Get(const TKey& key, TValue& value) const
            {
                Aws::Utils::Threading::ReaderLockGuard g(m_rwlock);
                return m_cache.Get(key, value);
            }

            template<typename UValue>
            void Put(const TKey& key, UValue&& val, std::chrono::milliseconds duration)
            {
                Aws::Utils::Threading::WriterLockGuard g(m_rwlock);
                m_cache.Put(key, std::forward<UValue>(val), duration);
            }

            template<typename UValue>
            void Put(TKey&& key, UValue&& val, std::chrono::milliseconds duration)
            {
                Aws::Utils::Threading::WriterLockGuard g(m_rwlock);
                m_cache.Put(std::move(key), std::forward<UValue>(val), duration);
            }

        private:
            Aws::Utils::Cache<TKey, TValue> m_cache;
            mutable Aws::Utils::Threading::ReaderWriterLock m_rwlock;
        };
    }
}
