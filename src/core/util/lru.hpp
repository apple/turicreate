/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <utility>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>


namespace turi {

/**
 * \ingroup util
 * A simple general purpose LRU cache implementation.
 */
template <typename Key, typename Value>
struct lru_cache {
  typedef std::pair<Key, Value> value_type;

  typedef boost::multi_index_container <value_type,
      boost::multi_index::indexed_by <
          boost::multi_index::hashed_unique<BOOST_MULTI_INDEX_MEMBER(value_type, Key, first)>,
          boost::multi_index::sequenced<>
          >
      > cache_type;

  typedef typename cache_type::template nth_index<1>::type::const_iterator iterator;
  typedef iterator const_iterator;

  typedef typename cache_type::template nth_index<1>::type::const_reverse_iterator reverse_iterator;
  typedef reverse_iterator const_reverse_iterator;

  lru_cache() = default;
  lru_cache(const lru_cache&) = default;
  lru_cache(lru_cache&&) = default;
  lru_cache& operator=(const lru_cache&) = default;
  lru_cache& operator=(lru_cache&&) = default;

  /**
   * queries for a particular key. If it does not exist {false, Value()} is
   * returned. If it exists {true, value} is returned and the key is bumped
   * to the front of the cache.
   */
  std::pair<bool, Value> query(const Key& key) {
    auto& hash_container = m_cache.template get<0>();
    auto& seq_container = m_cache.template get<1>();
    auto iter = hash_container.find(key);
    if (iter == hash_container.end()) {
      ++m_misses;
      return {false, Value()};
    } else {
      ++m_hits;
      seq_container.relocate(seq_container.begin(), m_cache.template project<1>(iter));
      return {true, iter->second};
    }
  }

  /**
   * Inserts a key into the cache. If the key already exists, it is overwritten.
   * If the size of the cache is larger than the limit, the least recently
   * used items are erased.
   */
  void insert(const Key& key, const Value& value) {
    auto& hash_container = m_cache.template get<0>();
    auto& seq_container = m_cache.template get<1>();
    auto iter = hash_container.find(key);
    if (iter == hash_container.end()) {
      seq_container.push_front(value_type{key, value});
      if (size() > get_size_limit()) {
        seq_container.pop_back();
      }
    } else {
      hash_container.replace(iter, value_type{key, value});
      seq_container.relocate(seq_container.begin(), m_cache.template project<1>(iter));
    }
  }

  void erase(const Key& key) {
    auto& hash_container = m_cache.template get<0>();
    auto iter = hash_container.find(key);
    if (iter != hash_container.end()) {
      hash_container.erase(iter);
    }
  }

  /**
   * Retuns an iterator to the data
   */
  const_iterator begin() const {
    auto& seq_container = m_cache.template get<1>();
    return seq_container.begin();
  }

  /**
   * Retuns an iterator to the data
   */
  const_iterator end() const {
    auto& seq_container = m_cache.template get<1>();
    return seq_container.end();
  }

  /** Iterator to cache in reverse */
  const_reverse_iterator rbegin() const {
    auto& seq_container = m_cache.template get<1>();
    return seq_container.rbegin();
  }

  /** Iterator to cache in reverse */
  const_reverse_iterator rend() const {
    auto& seq_container = m_cache.template get<1>();
    return seq_container.rend();
  }


  /// Returns the current size of the cache
  size_t size() const {
    return m_cache.size();
  }

  /// Sets the upper limit on the size of the cache
  void set_size_limit(size_t limit) {
    m_limit = limit;
  }

  /// Gets the upper limit of the size of the cache
  size_t get_size_limit() const {
    return m_limit;
  }

  /// Returns the number of query hits
  size_t hits() const {
    return m_hits;
  }


  /// Returns the number of query misses
  size_t misses() const {
    return m_misses;
  }

 private:
  cache_type m_cache;
  size_t m_limit = (size_t)(-1);
  size_t m_hits = 0;
  size_t m_misses = 0;
};
} // namespace turi
