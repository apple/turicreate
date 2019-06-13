/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_HASH_MAP_CONTAINER_H_
#define TURI_HASH_MAP_CONTAINER_H_

#include <core/parallel/pthread_tools.hpp>
#include <core/util/cityhash_tc.hpp>

namespace turi {

template <typename K, typename V>
class EXPORT hash_map;

// A two level hash_map
template <typename K, typename V>
class EXPORT hash_map_container {
 public:
  typedef hash_map<K, V> hash_map_type;

 public:
  hash_map_container() {
    num_segments = thread::cpu_count() * std::max<size_t>(1, log2(thread::cpu_count()));
    maps.resize(num_segments, hash_map_type());
  }

  explicit hash_map_container(size_t num_seg) {
    num_segments = std::min(num_seg, thread::cpu_count()
                            * std::max<size_t>(1, log2(thread::cpu_count())));
    maps.resize(num_segments, hash_map_type());
  }

  hash_map_container(const hash_map_container<K, V>&) = default;
  hash_map_container& operator=(const hash_map_container<K, V>&) = default;

  // update the value 'v' of key 'k' using function 'func'.
  void update(const K& k, const std::function<void (V&)>& func) {
    size_t seg_id = get_segment_id(k);
    maps[seg_id].update(k, func);
  }

  const V& get(const K& k) const {
    size_t seg_id = get_segment_id(k);
    return maps[seg_id].get(k);
  }

  inline size_t get_segment_id(const K& k) const {
    return hash64(k) % num_segments;
  }

  void clear() {
    for (auto& m: maps) {
      m.clear();
    }
  }

  void save(turi::oarchive& oarc) const {
    oarc << num_segments << maps;
  }

  void load(turi::iarchive& iarc) {
    iarc >> num_segments >> maps;
  }
 private:
  size_t num_segments;
  std::vector<hash_map_type> maps;
};

template <typename K, typename V>
class EXPORT hash_map {
 public:
  hash_map() = default;

  hash_map(const hash_map& oth) {
    umap = oth.umap;
  }

  hash_map& operator=(const hash_map& oth) {
    umap = oth.umap;
    return *this;
  }

  void update(const K& k, const std::function<void (V&)>& func) {
    lock.lock();
    auto fit = umap.find(k);
    if (fit == umap.end()) {
      fit = umap.insert(std::make_pair(k, default_value)).first;
    }
    func(fit->second);
    lock.unlock();
  }

  const V& get(const K& k) const {
    auto fit = umap.find(k);
    if (fit == umap.end()) {
      return default_value;
    }
    return fit->second;
  }

  void clear() {
    lock.lock();
    umap.clear();
    lock.unlock();
  }

  void save(turi::oarchive& oarc) const {
    oarc << umap;
  }

  void load(turi::iarchive& iarc) {
    iarc >> umap;
  }

 private:
  turi::simple_spinlock lock;
  std::unordered_map<K, V> umap;
  V default_value = V();
};

} // namespace turi

#endif // TURI_HASH_MAP_CONTAINER_H_
