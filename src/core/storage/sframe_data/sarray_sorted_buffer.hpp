/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_SARRAY_SORTED_BUFFER_HPP
#define TURI_SFRAME_SARRAY_SORTED_BUFFER_HPP

#include<core/parallel/mutex.hpp>
#include<memory>
#include<vector>
#include<future>
#include<core/storage/sframe_data/sarray.hpp>
#include<core/storage/sframe_data/sframe.hpp>


namespace turi {


/**
 * \internal
 * \ingroup sframe_physical
 * \addtogroup sframe_internal SFrame Internal
 * \{
 */

/**
 * An SArray backed buffer that stores elements in sorted order.
 *
 * The container has an in memory buffer, and is backed by an sarray segment.
 * When the buffer is full, it is sorted and written into the sarray segment as a sorted chunk.
 *
 * - The add function is used to push element into the buffer.
 * - When finishing adding elements, close() can be called to close the buffer.
 * - The sort_and_write function then merges the sorted chunks and output to the destination array.
 * - When deduplicate is set in the constructor, the buffer will ignore duplicated items.
 */

template<typename T>
class sarray_sorted_buffer {
  typedef T value_type;
  typedef typename sarray<T>::iterator sink_iterator_type;
  typedef sarray<T> sink_type;
  typedef std::function<bool(const value_type&, const value_type&)> comparator_type;

 public:
   /// construct with given sarray and the segmentid as sink.
   sarray_sorted_buffer(size_t buffer_size,
                        comparator_type comparator,
                        bool deduplicate = false);

   sarray_sorted_buffer(const sarray_sorted_buffer& other) = delete;

   sarray_sorted_buffer& operator=(const sarray_sorted_buffer& other) = delete;

   /// Add a new element to the container.
   void add(value_type&& val, size_t thread_id = 0);
   void add(const value_type& val, size_t thread_id = 0);

   /// Sort all elements in the container and writes to the output.
   /// If deduplicate is true, only output unique elements.
   template<typename OutIterator>
   void sort_and_write(OutIterator out);

   size_t approx_size() const {
     if (sink->is_opened_for_write()){
       return 0;
     } else {
       size_t ret = 0;
       for (auto i : chunk_size) ret += i;
       return ret;
     }
   }

   /// Flush the last buffer, and close the sarray.
   void close();

  private:
   /// Writes the content into the sarray segment backend.
   void save_buffer(std::shared_ptr<std::vector<value_type>> swap_buffer);

   /// The sarray storing the elements.
   std::shared_ptr<sink_type> sink;

   /// Internal output iterator for the sarray_sink segment.
   sink_iterator_type out_iter;

   /// Storing the size of each sorted chunk.
   std::vector<size_t> chunk_size;

   /// Guarding the sarray sink from parallel access.
   turi::mutex sink_mutex;

   /// Buffer that stores the incoming elements.
   std::vector<std::vector<value_type>> buffer_array;

   /// The limit of the buffer size
   size_t buffer_size;

   /// Guarding the buffer from parallel access.
#ifdef  __APPLE__
   std::vector<simple_spinlock> buffer_mutex_array;
#else
   std::vector<turi::mutex> buffer_mutex_array;
#endif

   /// Comparator for sorting the values.
   comparator_type  comparator;

   /// If true only keep the unique items.
   bool deduplicate;
}; // end of sarray_sorted_buffer class

/// \}

/**************************************************************************/
/*                                                                        */
/*                             Implementation                             */
/*                                                                        */
/**************************************************************************/
template<typename T>
template<typename OutIterator>
void sarray_sorted_buffer<T>::sort_and_write(OutIterator out) {
  std::shared_ptr<typename sink_type::reader_type> reader = std::move(sink->get_reader());
  // prepare the begin row and end row for each chunk.
  size_t segment_start = 0;

  // each chunk stores a sequential read of the segments,
  // and elements in each chunk are already sorted.
  std::vector<sarray_reader_buffer<T>> chunk_readers;

  size_t prev_row_start = segment_start;
  for (size_t i = 0; i < chunk_size.size(); ++i) {
    size_t row_start = prev_row_start;
    size_t row_end = row_start + chunk_size[i];
    prev_row_start = row_end;
    chunk_readers.push_back(sarray_reader_buffer<T>(reader, row_start, row_end));
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
      pq.push_back({chunk_readers[i].next(), i});
      remaining_chunks.insert(i);
    }
  }
  std::make_heap(pq.begin(), pq.end(), pair_comparator);

  bool is_first_elem = true;
  value_type prev_value;
  while (!pq.empty()) {
    size_t id;
    value_type value;
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
      pq.push_back({chunk_readers[id].next(), id});
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
      value_type value = chunk_readers[id].next();
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

}
#endif
