/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include<core/storage/sframe_data/sarray_sorted_buffer.hpp>
#include<core/storage/sframe_data/sarray_reader_buffer.hpp>
#include<unordered_set>
#include<queue>
#include <core/util/cityhash_tc.hpp>

namespace turi {

static size_t BUFFER_ARRAY_SIZE = 16;

template<typename T>
sarray_sorted_buffer<T>::sarray_sorted_buffer(
    size_t buffer_size_,
    comparator_type comparator_,
    bool deduplicate_)
  : buffer_size(buffer_size_ / BUFFER_ARRAY_SIZE),
    comparator(comparator_),
    deduplicate(deduplicate_) {

    sink = std::make_shared<sink_type>();
    sink->open_for_write(1);
    out_iter = sink->get_output_iterator(0);

    buffer_array.resize(BUFFER_ARRAY_SIZE);
    buffer_mutex_array.resize(BUFFER_ARRAY_SIZE);
    for (size_t i = 0; i < BUFFER_ARRAY_SIZE; ++i) {
      buffer_array[i].reserve(buffer_size);
    }
  }

template<typename T>
void sarray_sorted_buffer<T>::add(value_type&& val, size_t thread_id) {
  auto hash = hash64(thread_id) % BUFFER_ARRAY_SIZE;
  buffer_mutex_array[hash].lock();
  buffer_array[hash].push_back(val);
  if (buffer_array[hash].size() == buffer_size) {
    auto swap_buffer = std::make_shared<std::vector<value_type>>();
    swap_buffer->swap(buffer_array[hash]);
    buffer_mutex_array[hash].unlock();
    save_buffer(swap_buffer);
  } else {
    buffer_mutex_array[hash].unlock();
  }
}

template<typename T>
void sarray_sorted_buffer<T>::add(const value_type& val, size_t thread_id) {
  auto hash = hash64(thread_id) % BUFFER_ARRAY_SIZE;
  buffer_mutex_array[hash].lock();
  buffer_array[hash].push_back(val);
  if (buffer_array[hash].size() == buffer_size) {
    auto swap_buffer = std::make_shared<std::vector<value_type>>();
    swap_buffer->swap(buffer_array[hash]);
    buffer_mutex_array[hash].unlock();
    save_buffer(swap_buffer);
  } else {
    buffer_mutex_array[hash].unlock();
  }
}

template<typename T>
void sarray_sorted_buffer<T>::close() {
  if (sink->is_opened_for_write()){
    for (size_t i = 0; i < BUFFER_ARRAY_SIZE; ++i) {
      if (buffer_array[i].size() > 0) {
        auto swap_buffer = std::make_shared<std::vector<value_type>>();
        swap_buffer->swap(buffer_array[i]);
        save_buffer(swap_buffer);
        buffer_array[i].clear();
        buffer_array[i].shrink_to_fit();
      }
    }
    sink->close();
  }
}


/// Save the buffer into the sarray backend.
template<typename T>
void sarray_sorted_buffer<T>::save_buffer(std::shared_ptr<std::vector<value_type>> swap_buffer) {
  std::sort(swap_buffer->begin(), swap_buffer->end(), comparator);
  if (deduplicate) {
    auto iter = std::unique(swap_buffer->begin(), swap_buffer->end());
    swap_buffer->resize(std::distance(swap_buffer->begin(), iter));
  }
  sink_mutex.lock();
  auto iter = swap_buffer->begin();
  while(iter != swap_buffer->end()) {
    *out_iter = *iter;
    ++iter;
    ++out_iter;
  }
  chunk_size.push_back(swap_buffer->size());
  sink_mutex.unlock();
}

// Explicit template class instantiation
template class sarray_sorted_buffer<flexible_type>;
} // end of turicreate
