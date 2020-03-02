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
#include <aws/core/utils/memory/stl/AWSMap.h>
#include <chrono>

namespace Aws
{
    namespace Utils
    {
        /**
         * In-memory fixed-size cache utility.
         */
        template <typename TKey, typename TValue>
        class Cache
        {
        public:
            /**
             * Initialize the cache with a particular size.
             * The size of the cache is fixed and does not grow over time.
             */
            explicit Cache(size_t initialSize = 1000) : m_maxSize(initialSize)
            {
            }

            /**
             * Retrieves the value associated with the given key if exists and returns true. Otherwise, returns false.
             * @param key The of key of the entry to retrieve.
             * @param value The retrieved value in case the key exists in the cache
             */
            bool Get(const TKey& key, TValue& value) const
            {
                auto it = m_entries.find(key);
                if (it == m_entries.end())
                {
                    return false;
                }

                if (DateTime::Now() > it->second.expiration)
                {
                    return false;
                }

                value = it->second.val;
                return true;
            }

            /**
             * Add or update a cache entry.
             * When the number of items in the cache reaches the maximum, newly added items will evict expired items.
             * If the cache size is at its maximum and none of the existing items are expired, the entry that is closest
             * to expiration will be evicted.
             *
             * Note: Expired entries are not evicted upon expiration, but rather when space is needed for new items.
             *
             * @param key The of key of the entry that will be used to retrieve it.
             * @param val The value of the entry to associate with the given key.
             * @param duration The duration after which the cache entry will expire and become a candidate for eviction.
             */
            template<typename UValue>
            void Put(TKey&& key, UValue&& val, std::chrono::milliseconds duration)
            {
                auto it = m_entries.find(key);
                const DateTime expiration = DateTime::Now() + duration;
                if (it != m_entries.end())
                {
                    it->second.val = std::forward<UValue>(val);
                    it->second.expiration = expiration;
                    return;
                }

                if (m_entries.size() >= m_maxSize)
                {
                    Prune(); // removes expired/expiring elements
                }

                m_entries.emplace(std::move(key), Value { expiration, std::forward<UValue>(val) });
            }

            template<typename UValue>
            void Put(const TKey& key, UValue&& val, std::chrono::milliseconds duration)
            {
                auto it = m_entries.find(key);
                const DateTime expiration = DateTime::Now() + duration;
                if (it != m_entries.end())
                {
                    it->second.val = std::forward<UValue>(val);
                    it->second.expiration = expiration;
                    return;
                }

                if (m_entries.size() >= m_maxSize)
                {
                    Prune(); // removes expired/expiring elements
                }

                m_entries.emplace(key, Value { expiration, std::forward<UValue>(val) });
            }

        private:

            void Prune()
            {
                auto mostExpiring = m_entries.begin();
                // remove the expired ones. If none expired, remove the one that's closest to expiring.
                for (auto it = m_entries.begin(); it != m_entries.end();)
                {
                    if (DateTime::Now() > it->second.expiration)
                    {
                        it = m_entries.erase(it);
                    }
                    else
                    {
                        if (it->second.expiration < mostExpiring->second.expiration)
                        {
                            mostExpiring = it;
                        }
                        ++it;
                    }
                }

                // if nothing was erased. Remove the most expiring element.
                if (m_entries.size() >= m_maxSize)
                {
                    m_entries.erase(mostExpiring);
                }
            }

            struct Value
            {
                DateTime expiration;
                TValue val;
            };

            Aws::Map<TKey, Value> m_entries;
            const size_t m_maxSize;
        };
    }
}
