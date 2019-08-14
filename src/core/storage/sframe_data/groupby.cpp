/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include<core/storage/sframe_data/groupby.hpp>
#include<core/storage/sframe_data/sarray_reader_buffer.hpp>
#include<unordered_set>
#include<queue>
#include <core/util/cityhash_tc.hpp>

namespace turi {

sframe group(sframe sframe_in, std::string key_column) {
  sframe ret;
  // Comparator that compares rows based on the key column value.
  size_t key_column_id = sframe_in.column_index(key_column);
  auto comparator = [=](const std::vector<flexible_type>& a, const std::vector<flexible_type>& b) {
    auto atype = a[key_column_id].get_type();
    auto btype = b[key_column_id].get_type();
    if (atype < btype) {
      return true;
    } else if(atype > btype) {
      return false;
    } else if (atype == flex_type_enum::UNDEFINED && btype == flex_type_enum::UNDEFINED) {
      return false;
    } else {
      return a[key_column_id] < b[key_column_id];
    }
  };

  size_t input_nsegments = sframe_in.num_segments();

  size_t output_nsegments = std::max(input_nsegments,
                              thread::cpu_count() * std::max<size_t>(1, log2(thread::cpu_count())));
  hash_bucket_container<std::vector<flexible_type>> hash_container(output_nsegments, comparator);

  // shuffle the rows based on the value of the key column.
  auto input_reader = sframe_in.get_reader();
  parallel_for (0, input_nsegments, [&](size_t i) {
    auto iter = input_reader->begin(i);
    auto enditer = input_reader->end(i);
    while(iter != enditer) {
      auto& row = *iter;
      auto key = row[key_column_id];
      size_t hash = hash64(key.hash()) % output_nsegments;
      hash_container.add(row, hash);
      ++iter;
    }
  });

  ret.open_for_write(sframe_in.column_names(),
                     sframe_in.column_types(),
                     "",
                     output_nsegments);
  hash_container.sort_and_write(ret);
  return ret;
}

template<typename T>
hash_bucket<T>::hash_bucket(
    size_t buffer_size,
    std::shared_ptr<sink_type> sink,
    size_t segmentid,
    comparator_type comparator,
    bool deduplicate)
  : segmentid(segmentid),
    sink(sink),
    buffer_size(buffer_size),
    comparator(comparator),
    deduplicate(deduplicate) {

    buffer.reserve(buffer_size);
    out_iter = sink->get_output_iterator(segmentid);
  }

template<typename T>
void hash_bucket<T>::add(const value_type& val) {
  buffer_mutex.lock();
  buffer.push_back(val);
  if (buffer.size() == buffer_size) {
    std::vector<value_type> swap_buffer;
    swap_buffer.swap(buffer);
    buffer_mutex.unlock();
    save_buffer(swap_buffer);
  } else {
    buffer_mutex.unlock();
  }
}

template<typename T>
void hash_bucket<T>::add(value_type&& val) {
  buffer_mutex.lock();
  buffer.push_back(val);
  if (buffer.size() == buffer_size) {
    std::vector<value_type> swap_buffer;
    swap_buffer.swap(buffer);
    buffer_mutex.unlock();
    save_buffer(swap_buffer);
  } else {
    buffer_mutex.unlock();
  }
}

template<typename T>
void hash_bucket<T>::flush() {
  if (buffer.size() > 0) {
    save_buffer(buffer);
    buffer.clear();
  }
}

template<typename T>
template<typename OutIterator>
void hash_bucket<T>::sort_and_write(OutIterator out) {
  DASSERT_EQ(buffer.size(), 0);
  std::shared_ptr<sarray<std::string>::reader_type> reader = sink->get_reader();
  // prepare the begin row and end row for each chunk.
  size_t segment_start = 0;
  for (size_t i = 0; i < segmentid; ++i) {
    segment_start += reader->segment_length(i);
  }

  // each chunk stores a sequential read of the segments,
  // and elements in each chunk are already sorted.
  std::vector<sarray_reader_buffer<std::string>> chunk_readers;

  size_t prev_row_start = segment_start;
  for (size_t i = 0; i < chunk_size.size(); ++i) {
    size_t row_start = prev_row_start;
    size_t row_end = row_start + chunk_size[i];
    prev_row_start = row_end;
    chunk_readers.push_back(sarray_reader_buffer<std::string>(reader, row_start, row_end));
  }

  // id of the chunks that still have elements.
  std::unordered_set<size_t> remaining_chunks;

  // merge the chunks and write to the out iterator
  std::vector< std::pair<value_type, size_t> > pq;
  // comparator for the pair type
  auto pair_comparator = [=](const std::pair<value_type, size_t>& a,
      const std::pair<value_type, size_t>& b) {
    return !comparator(a.first, b.first);
  };

  // insert one element from each chunk into the priority queue.
  for (size_t i = 0; i < chunk_readers.size(); ++i) {
    if (chunk_readers[i].has_next()) {
      pq.push_back({deserialize(chunk_readers[i].next()), i});
      remaining_chunks.insert(i);
    }
  }
  std::make_heap(pq.begin(), pq.end(), pair_comparator);

  bool is_first_elem = true;
  flexible_type prev_value;
  while (!pq.empty()) {
    size_t id;
    flexible_type value;
    std::tie(value, id) = pq.front();
    std::pop_heap(pq.begin(), pq.end(), pair_comparator);
    pq.pop_back();
    if (deduplicate) {
      if ((value != prev_value) || is_first_elem) {
        prev_value = value;
        *out = std::move(value);
        ++out;
        is_first_elem = false;
      }
    } else {
      *out = std::move(value);
      ++out;
    }
    if (chunk_readers[id].has_next()) {
      pq.push_back({deserialize(chunk_readers[id].next()), id});
      std::push_heap(pq.begin(), pq.end(), pair_comparator);
    } else {
      remaining_chunks.erase(id);
    }
  }

  // At most one chunk will be left
  ASSERT_TRUE(remaining_chunks.size() <= 1);
  if (remaining_chunks.size()) {
    size_t id = *remaining_chunks.begin();
    while(chunk_readers[id].has_next()) {
      flexible_type value = deserialize(chunk_readers[id].next());
      if (deduplicate) {
        if ((value != prev_value) || is_first_elem) {
          prev_value = value;
          *out = std::move(value);
          ++out;
          is_first_elem = false;
        }
      } else {
        *out = std::move(value);
        ++out;
      }
    }
  }
}

/// Save the buffer into the sarray backend.
template<typename T>
void hash_bucket<T>::save_buffer(std::vector<value_type>& swap_buffer) {
  std::sort(swap_buffer.begin(), swap_buffer.end(), comparator);
  if (deduplicate) {
    auto iter = std::unique(swap_buffer.begin(), swap_buffer.end());
    swap_buffer.resize(std::distance(swap_buffer.begin(), iter));
  }
  sink_mutex.lock();
  auto iter = swap_buffer.begin();
  oarchive oarc;
  while(iter != swap_buffer.end()) {
    oarc << std::move(*iter);
    (*out_iter) = std::string(oarc.buf, oarc.off);
    ++iter;
    ++out_iter;
    oarc.off = 0;
  }
  free(oarc.buf);
  chunk_size.push_back(swap_buffer.size());
  sink_mutex.unlock();
}

// Explicit template class instantiation
typedef sarray<flexible_type>::iterator __sarray_iterator__;
typedef sframe::iterator __sframe_iterator__;
typedef std::insert_iterator<std::vector<flexible_type>> __vec_iterator__;

template class hash_bucket<flexible_type>;
template class hash_bucket<std::vector<flexible_type>>;

template void hash_bucket<flexible_type>::sort_and_write<__sarray_iterator__>(__sarray_iterator__ out_iter);
template void hash_bucket<flexible_type>::sort_and_write<__vec_iterator__>(__vec_iterator__ out_iter);
template void hash_bucket<std::vector<flexible_type>>::sort_and_write<__sframe_iterator__>(__sframe_iterator__ out_iter);
} // end of turicreate
