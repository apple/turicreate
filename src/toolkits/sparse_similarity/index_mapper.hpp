/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNITY_VECTOR_INDEX_MAPPER_H_
#define TURI_UNITY_VECTOR_INDEX_MAPPER_H_

#include <vector>
#include <core/util/dense_bitset.hpp>

namespace turi {

/** Index Mapping.  Just a simple utility to allow mapping indices to
 *  a subset of them based on a bitmask.  Upon construction, the index
 *  mapping is the identity; after an index mapping is applied,
 *  vectors of the original indices can be remapped to other
 */
class vector_index_mapper {
 private:

  bool _index_mapping_enabled = false;
  std::vector<size_t> data_to_internal_index_mapping;
  std::vector<size_t> internal_to_data_index_mapping;

 public:

  /** Is the current mapping the identity?
   */
  inline bool is_identity() const {
    return !_index_mapping_enabled;
  }

  /** Applies a mapping to the vertices so that only a subset of them
   *  are active, and each of these are mapped to a contiguous set of
   *  indices, 0, ..., n_active-1.
   */
  size_t set_index_mapping_from_mask(const dense_bitset& is_active_entry) {

    size_t num_items = is_active_entry.size();
    size_t new_size = is_active_entry.popcount();

    // Nothing to do if they are all active still
    if(new_size == num_items) {
      _index_mapping_enabled = false;
      internal_to_data_index_mapping.clear();
      data_to_internal_index_mapping.clear();
      return new_size;
    }

    // Build the index mappings
    internal_to_data_index_mapping.resize(new_size);
    data_to_internal_index_mapping.resize(num_items);

    size_t map_idx = 0;
    for(size_t src_idx = 0; src_idx < num_items; ++src_idx) {
      bool entry_active = is_active_entry.get(src_idx);

      if(entry_active) {
        internal_to_data_index_mapping[map_idx] = src_idx;
        data_to_internal_index_mapping[src_idx] = map_idx;
        ++map_idx;
      } else {
        data_to_internal_index_mapping[src_idx] = size_t(-1);
      }
    }

    _index_mapping_enabled = true;
    return new_size;
  }

  /**  Remaps a vector inplace such that only active indices are kept,
   *   and the rest are discarded.  Effectively, this applies the mask
   *   given to set_index_mapping_from_mask to this vector.  In the
   *   new vector, entry i in the original will then be entry
   *   map_data_index_to_internal_index(i) in the remapped vector.
   *
   *   The vector is unchanged if is_identity() is true.
   */
  template <typename T>
  GL_HOT_FLATTEN
  void remap_vector(std::vector<T>& data_vect) const {

    if(!_index_mapping_enabled) {
      return;
    }

    DASSERT_EQ(data_vect.size(), data_to_internal_index_mapping.size());

    // Go through and filter all the index data to be the internal
    // indices.
    for(size_t i = 0; i < internal_to_data_index_mapping.size(); ++i) {
      size_t src_idx = internal_to_data_index_mapping[i];
      size_t dest_idx = i;

      data_vect[dest_idx] = std::move(data_vect[src_idx]);
    }

    data_vect.resize(internal_to_data_index_mapping.size());
  }

  /**  Remaps a sparse vector of (index, value) pairs inplace such
   *   that only active indices are kept and the rest are discarded.
   *   Active indices are remapped.
   *
   *   The vector is unchanged if is_identity() is true.
   */
  template <typename T>
  GL_HOT_FLATTEN
  void remap_sparse_vector(std::vector<std::pair<size_t, T> >& data_vect) const {

    if(!_index_mapping_enabled) {
      return;
    }

    DASSERT_TRUE(std::is_sorted(data_vect.begin(), data_vect.end(),
                                [](const std::pair<size_t, T>& p1,
                                   const std::pair<size_t, T>& p2) {
                                  DASSERT_NE(p1.first, p2.first);
                                  return p1.first < p2.first; }));

    size_t dest_idx = 0;
    for(size_t src_idx = 0; src_idx < data_vect.size(); ++src_idx) {
      auto& p = data_vect[src_idx];
      size_t mapped_index = map_data_index_to_internal_index(p.first);
      if(mapped_index != size_t(-1)) {
        data_vect[dest_idx] = {mapped_index, std::move(p.second)};
        ++dest_idx;
      }
    }
    data_vect.resize(dest_idx);

    DASSERT_TRUE(std::is_sorted(data_vect.begin(), data_vect.end(),
                                [](const std::pair<size_t, T>& p1,
                                   const std::pair<size_t, T>& p2) {
                                  DASSERT_NE(p1.first, p2.first);
                                  return p1.first < p2.first; }));
  }

  /** Is a given entry still active?
   */
  inline bool is_active(size_t data_idx) const GL_HOT_INLINE_FLATTEN {
    return _index_mapping_enabled || (data_to_internal_index_mapping[data_idx] != size_t(-1));
  }

  /** What's the internal mapped index for the given entry?
   */
  inline size_t map_data_index_to_internal_index(size_t data_idx) const GL_HOT_INLINE_FLATTEN {
    DASSERT_LT(data_idx, data_to_internal_index_mapping.size());
    if(_index_mapping_enabled) {
      return data_to_internal_index_mapping[data_idx];
    } else {
      return data_idx;
    }
  }

  /** What's the external index here?
   */
  inline size_t map_internal_index_to_data_index(size_t internal_idx) const GL_HOT_INLINE_FLATTEN {
    if(_index_mapping_enabled) {
      DASSERT_LT(internal_idx, internal_to_data_index_mapping.size());
      return internal_to_data_index_mapping[internal_idx];
    } else {
      return internal_idx;
    }
  }

};

}

#endif
