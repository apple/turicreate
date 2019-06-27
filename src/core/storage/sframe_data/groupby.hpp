/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SFRAME_GROUPBY_HPP
#define TURI_SFRAME_GROUPBY_HPP

#include <core/parallel/mutex.hpp>
#include<memory>
#include<vector>
#include<core/storage/sframe_data/sarray.hpp>
#include<core/storage/sframe_data/sframe.hpp>

namespace turi {


/**
 * \ingroup sframe_physical
 * \addtogroup groupby_aggregate Groupby Aggregation
 * \{
 */

/**
 * Group the sframe rows by the key_column.
 *
 * Like a sort, but not.
 */
sframe group(sframe sframe_in, std::string key_column);

// Forward declaration
template<typename T>
class hash_bucket;

/**
 * A container of a collection of "hash_bucket"s. Each hash_bucket
 * store the value in sorted order. If the element is added to bucket
 * by its hash_value, then all elements in the container are partially sorted,
 * or grouped.
 *
 * Below is an example of using the it to group an sframe by its first column.
 *
 * \code
 * typedef std::vector<flexible_type> valuetype;
 * sframe sf = ...;
 *
 * hash_bucket_container<std::vector<flexible_type>> hash_container(
 *    sf.num_segments(),
 *    [](const value_type& a, const value_type& b) { return a[0] < b[0]; }
 * );
 *
 * parallel_for(0, sf.num_segments(); [&](size_t i) {
 *   auto iter = sf.get_reader().begin(i);
 *   auto end = sf.get_reader().end(i);
 *   while (iter != end) {
 *     size_t hash = *iter[0].hash();
 *     hash_container.add(*iter, hash % hash_container.num_buckets());
 *     ++iter;
 *   }
 * });
 *
 * sframe outsf;
 * hash_container.sort_and_write(outsf);
 *
 * \endcode
 *
 * Each hash_bucket has an in memory buffer, and is backed by an sarray segment.
 * When the buffer is full, it is sorted and written into the sarray segment as
 * a sorted chunk.
 *
 * The sort_and_write function then merges the sorted chunks and write out to
 * a new sarray or sframe.
 */
template<typename T>
class hash_bucket_container {

 public:
  // type of the stored value
  typedef T value_type;

 private:
  // type of the sarray disk backend.
  typedef sarray<std::string> sink_type;

  //  type of the comparator used for comparing the values within each bucket.
  typedef std::function<bool(const value_type&, const value_type&)> comparator_type;

 public:
  /// Constructs a container with n buckets, and a comparator for sorting the values.
  hash_bucket_container(
    size_t num_buckets,
    comparator_type comparator = std::less<value_type>()
    ) {
    sarray_sink.reset(new sink_type());
    sarray_sink->open_for_write(num_buckets);
    for (size_t i = 0; i < num_buckets; ++i) {
      buckets.push_back(new hash_bucket<value_type>(buffer_size, sarray_sink, i, comparator));
    }
  };

  // Delete copy and copy assignment
  hash_bucket_container(const hash_bucket_container& other) = delete;
  hash_bucket_container& operator=(const hash_bucket_container& other) = delete;

  // Destructor
  ~hash_bucket_container() { for(auto bucket_ptr : buckets) { delete bucket_ptr; } }

  // Add a new element to the specified bucket.
  void add(const value_type& val, size_t bucketid) {
    DASSERT_LT(bucketid, buckets.size());
    buckets[bucketid]->add(val);
  };

  // Sort each bucket and write out the result to an sarray or sframe.
  template<typename SIterableType>
  void sort_and_write(SIterableType& out) {
    parallel_for (0, num_buckets(), [&](size_t i) { buckets[i]->flush();} );
    sarray_sink->close();
    typedef typename SIterableType::iterator OutIterator;
    DASSERT_EQ(out.num_segments(), buckets.size());
    parallel_for(0, buckets.size(),
                 [&](size_t i) {
                   buckets[i]->template sort_and_write<OutIterator>(out.get_output_iterator(i));
                 });
    out.close();
  };

  // returns the number of buckets in the container.
  size_t num_buckets() const { return buckets.size(); }

 private:

  // buffer size for each hash bucket.
  // optimal size is about sqrt(N)
  const size_t buffer_size = 1024 * 1024;

  // vector of hash bucket which stores elements in sorted order.
  std::vector<hash_bucket<value_type>*> buckets;

  // the disk backend shared by all the buckets for dumping the buffer.
  std::shared_ptr<sarray<std::string>> sarray_sink;
};

/**
 * Storing elements that gets hashed to the bucket in sorted order.
 *
 * The container has an in memory buffer, and is backed by an sarray segment. When the buffer is full, it is sorted and written into the sarray segment as a sorted chunk.
 *
 * The sort_and_write function then merges the sorted chunks and output to the destination array.
 */
template<typename T>
class hash_bucket {
  typedef T value_type;
  typedef sarray<std::string>::iterator sink_iterator_type;
  typedef sarray<std::string> sink_type;
  typedef std::function<bool(const value_type&, const value_type&)> comparator_type;

 public:
   /// construct with given sarray and the segmentid as sink.
   hash_bucket(size_t buffer_size,
               std::shared_ptr<sink_type> sink,
               size_t segmentid,
               comparator_type comparator,
               bool deduplicate = false);

   hash_bucket(const hash_bucket& other) = delete;

   hash_bucket& operator=(const hash_bucket& other) = delete;

   /// Add a new element to the container.
   void add(const value_type& val);
   void add(value_type&& val);

   /// Flush the last buffer
   void flush();

   /// Sort all elements in the container and writes to the output.
   /// If deduplicate is true, only output unique elements.
   template<typename OutIterator>
   void sort_and_write(OutIterator out);


  private:
   /// Writes the content into the sarray segment backend.
   void save_buffer(std::vector<value_type>& swap_buffer);

   /// The segment id to dump the buffer.
   size_t segmentid;

   /// The sarray storing the elements.
   std::shared_ptr<sarray<std::string>> sink;

   /// Internal output iterator for the sarray_sink segment.
   sink_iterator_type out_iter;

   /// Storing the size of each sorted chunk.
   std::vector<size_t> chunk_size;

   /// Guarding the sarray sink from parallel access.
   turi::mutex sink_mutex;

   /// Buffer that stores the incoming elements.
   std::vector<value_type> buffer;

   /// The limit of the buffer size.
   size_t buffer_size;

   /// Guarding the buffer from parallel access.
#ifdef  __APPLE__
   simple_spinlock buffer_mutex;
#else
   turi::mutex buffer_mutex;
#endif

   /// Comparator for sorting the values.
   comparator_type  comparator;

   /// If true only keep the unique items.
   bool deduplicate;

  private:
   inline value_type deserialize(const std::string& buf) {
     value_type ret;
     iarchive iarc(buf.c_str(), buf.length());
     iarc >> ret;
     return ret;
   };
};

/// \}
} // end of turi
#endif
