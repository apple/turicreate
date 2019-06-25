/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SKETCHES_SPACE_SAVING_SKETCH_HPP
#define TURI_SKETCHES_SPACE_SAVING_SKETCH_HPP

#include <vector>
#include <map>
#include <cmath>
#include <set>
#include <core/generics/value_container_mapper.hpp>


namespace turi {
namespace sketches {

/**
 * \ingroup sketching
 * This class implements the Space Saving Sketch as described in
 * Ahmed Metwally † Divyakant Agrawal Amr El Abbadi. Efficient Computation of
 * Frequent and Top-k Elements in Data Streams.
 *
 * It provides an efficient one pass scan of all the data and provides an
 * estimate all the frequently occuring elements, with guarantees that all
 * elements with occurances >= \epsilon N will be reported.
 *
 * \code
 *   space_saving ss;
 *   // repeatedly call
 *   ss.add(stuff);
 *   // will return an array containing all the elements tracked
 *   // not all elements may be truly frequent items
 *   ss.frequent_items()
 *   // will return an array containing all the elements tracked which are
 *   // guaranteed to have occurances >= \epsilon N
 * \endcode
 *
 */
template <typename T>
class space_saving {
 public:
  /**
   * Constructs a save saving sketch using 1 / epsilon buckets.
   * The resultant hyperloglog datastructure will 1 / epsilon memory, and
   * guarantees that all elements with occurances >= \epsilon N will be reported.
   */
  space_saving(double epsilon = 0.0001)
  {
    initialize(epsilon);
    init_data_structures();
  }

  /**
   * Initalizes a save saving sketch using 1 / epsilon buckets.  The
   * resultant hyperloglog datastructure will use O(1 / epsilon)
   * memory, and guarantees that all elements with occurances >=
   * \epsilon N will be reported.
   */
  void initialize(double epsilon = 0.0001) {
    clear();

    // capacity = 1.0 / epsilon. add one to avoid rounding problems around the
    // value of \epsilon N
    m_max_capacity = size_t(std::ceil(1.0 / epsilon) + 1);
    m_epsilon = epsilon;
  }

  /**  Clears everything out.
   */
  void clear() {
    m_size = 0;
    n_entries = 0;
    base_level = 1;
    base_level_candidate_count = 0;
    cached_insertion_key = hashkey();
    cached_insertion_value = nullptr;
    global_map.clear();
  }

  /**
   * Adds an item with a specified count to the sketch.
   */
  void add(const T& t, size_t count = 1) {
    add_impl(t, count, 0);
  }

  /**
   * Returns the number of elements inserted into the sketch.
   */
  size_t size() const {
    return m_size;
  }

  /**
   * Returns all the elements tracked by the sketch as well as an
   * estimated count. The estimated can be a large overestimate.
   */
  std::vector<std::pair<T, size_t> > frequent_items() const {
    std::vector<std::pair<T, size_t> > ret;

    if(n_entries < entries.size()) {
      // We have all the items exactly; just return them.
      ret.resize(n_entries);
      for(size_t i = 0; i < n_entries; ++i) {
        const entry& e = entries[i];
        ret[i] = {e.value(), e.count};
      }
    } else {

      size_t threshhold = std::max(size_t(1), size_t(m_epsilon * size()));

      for(size_t i = 0; i < n_entries; ++i) {
        const entry& e = entries[i];

        if (e.count >= threshhold)
          ret.push_back(std::make_pair(e.value(), e.count));
      }
    }

    return ret;
  }


  /**
   * Returns all the elements tracked by the sketch as well as an
   * estimated count. All elements returned are guaranteed to have
   * occurance >= epsilon * m_size
   */
  std::vector<std::pair<T, size_t> > guaranteed_frequent_items() const {
    std::vector<std::pair<T, size_t> > ret;

    if(n_entries < entries.size()) {
      // We have all the items exactly; just return them.
      ret.resize(n_entries);
      for(size_t i = 0; i < n_entries; ++i) {
        const entry& e = entries[i];
        ret[i] = {e.value(), e.count};
      }
    } else {
      size_t threshhold = std::max(size_t(1), size_t(m_epsilon * size()));

      for(size_t i = 0; i < n_entries; ++i) {
        const entry& e = entries[i];

        if (e.count - e.error >= threshhold)
          ret.push_back(std::make_pair(e.value(), e.count));
      }
    }

    return ret;
  }

  /**
   * Merges a second space saving sketch into the current sketch
   */
  template <typename U>
  typename std::enable_if<std::is_convertible<U, T>::value, void>::type
  /* void */ combine(const space_saving<U>& other) {
    _combine(other);
  }

  ~space_saving() { }

 private:

  template <class U> friend class space_saving;

  // Total count added thus far
  size_t m_size = 0;

  // --- Other Tracking Internal Variables you don't have to care about ---
  // number of unique values to track
  size_t m_max_capacity = 0;
  double m_epsilon = 1;

  struct entry;

  typedef value_container_mapper<T, entry> global_map_type;
  typedef typename global_map_type::hashkey hashkey;
  typedef typename global_map_type::hashkey_and_value hashkey_and_value;

  // The container structure for the entry
  struct entry {
    typedef typename global_map_type::hashkey_and_value hashkey_and_value;

    size_t count = 0;
    size_t error = 0;

    entry(){}

    entry(hashkey_and_value&& _kv)
    : kv(_kv)
    {}

    hashkey_and_value kv;

    inline const hashkey_and_value& get_hashkey_and_value() const { return kv; }

    inline const T& value() const { return kv.value(); }
  };

  /** The entries are arranged so that the base level is built and
   *  maintained as a sequential partition starting at 0, and the rest
   *  are maintained as a sequential partition starting at n-1 and
   *  going down.  If the base_level is empty when it is needed, the
   *  other partition is scanned and things are rearranged so that the
   *  base level is now the lowest level present and the base level
   *  partition is correct.
   *
   *  The number of unique elements is thus (base_level_end +
   *  (m_max_capacity - overflow_level_start) ).
   */
  size_t n_entries = 0;
  std::vector<entry> entries;

  // These control the base levels
  size_t base_level = 1;

  std::vector<entry*> base_level_candidates;
  size_t base_level_candidate_count = 0;

  global_map_type global_map;

  // Need to eliminate the reference in some way.
  hashkey cached_insertion_key;
  entry* cached_insertion_value;

  /** A generic index buffer.  Used in a couple of places.
   */
  std::vector<size_t> index_buffer;


  /** Initialize the data structures.
   */
  void init_data_structures() {
    entries.resize(m_max_capacity);
    n_entries = 0;

    index_buffer.resize(m_max_capacity);

    base_level = 1;
    base_level_candidates.resize(m_max_capacity);
    base_level_candidate_count = 0;

    /**  Turns out it handles the deletion and insertion dynamics
     *   better with just a little extra space.
     */
    global_map.reserve(5 * m_max_capacity / 4);

    cached_insertion_value = nullptr;
  }

  /**  Called when the base level is needed but it's empty.
   */
  void regenerate_base_level() GL_HOT_NOINLINE_FLATTEN {

    _debug_check_level_integrity();

    DASSERT_EQ(base_level_candidate_count, 0);

    ////////////////////////////////////////////////////////////
    // State explicitly some of the assumptions.

    size_t min_value = std::numeric_limits<size_t>::max();

    for(size_t i = 0; i < m_max_capacity; ++i) {

      size_t count = entries[i].count;

      if(count < min_value) {
        base_level_candidate_count = 0;
        min_value = count;
        base_level_candidates[base_level_candidate_count++] = &entries[i];
      } else if(count == min_value) {
        base_level_candidates[base_level_candidate_count++] = &entries[i];
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Now need to pull all of them together

    base_level = min_value;

    _debug_check_level_integrity();
  }

  /** Increment the value of one of the elements.
   */
  entry* increment_element(
      entry* element, size_t count_incr, size_t error_incr) GL_HOT_INLINE_FLATTEN {

    _debug_check_level_integrity();

    element->count += count_incr;
    element->error += error_incr;
    return element;
  }

  /** Insert a new element.
   */
  entry* insert_element(const hashkey& key, const T& t,
                        size_t count, size_t error) GL_HOT_NOINLINE_FLATTEN {

    DASSERT_LT(n_entries, m_max_capacity);

    size_t dest_index = n_entries;
    ++n_entries;

    entries[dest_index] = entry(hashkey_and_value(key, t));

    entries[dest_index].count = count;
    entries[dest_index].error = error;

    if(count == base_level)
      base_level_candidates[base_level_candidate_count++] = &(entries[dest_index]);

    global_map.insert(entries[dest_index].get_hashkey_and_value(), &(entries[dest_index]));

    // Make sure we're okay here
    _debug_check_level_integrity();

    return &(entries[dest_index]);
  }

  /** Change and increment the count of a value.
   */
  entry* change_and_increment_value(
      const hashkey& key, const T& t, size_t count, size_t error) GL_HOT_NOINLINE_FLATTEN {

    if(base_level_candidate_count > 0
       && base_level_candidates.back()->count != base_level) {

      while(base_level_candidate_count > 0
            && base_level_candidates[base_level_candidate_count - 1]->count != base_level) {
        --base_level_candidate_count;
      }
    }

    if(UNLIKELY(base_level_candidate_count == 0)) {
      regenerate_base_level();
    }

    // We're started here.
    DASSERT_GT(base_level_candidate_count, 0);

    size_t idx = base_level_candidate_count - 1;

    entry* e = base_level_candidates[idx];

    DASSERT_EQ(e->count, base_level);

    global_map.invalidate(e->get_hashkey_and_value(), e);

    e->error = e->count + error;
    e->count += count;
    e->kv = hashkey_and_value(key, t);

    global_map.insert(key, e);

    --base_level_candidate_count;

    _debug_check_level_integrity();

    return e;
  }

  /**  Add in an element.
   */
  void add_impl(const T& t, size_t count = 1, size_t error = 0) GL_HOT {
    _debug_check_level_integrity();

    m_size += count;

    hashkey key(t);

    // One optimization -- if the key is the same as the previous one,
    // avoid a hash table lookup.  This will be seen a lot in sorted arrays, etc.
    if(cached_insertion_value != nullptr
       && cached_insertion_key == key
       && (hashkey::key_is_exact()
           || (cached_insertion_value->value() == t) ) ) {

      increment_element(cached_insertion_value, count, error);

    } else {

      cached_insertion_key = key;
      entry *ee_ptr = global_map.find(key, t);

      if(ee_ptr != nullptr) {
        increment_element(ee_ptr, count, error);
        cached_insertion_value = ee_ptr;
      } else {

        /* If we're full. */
        if(n_entries == m_max_capacity) {
          cached_insertion_value = change_and_increment_value(key, t, count, error);
        } else {
          cached_insertion_value = insert_element(key, t, count, error);
        }
      }
    }

#ifndef NDEBUG
    if(cached_insertion_value != nullptr) {
      const entry* e = global_map.find(cached_insertion_key,
                                       cached_insertion_value->value());
      DASSERT_TRUE(e == cached_insertion_value);
    }
#endif
  }

  /* For combining entries and casting between types.  we need to cast
   * and recreate the key if they are different.
   */
  template <typename OtherEntry>
  inline entry* _find_eptr(const OtherEntry& e,
                           typename std::enable_if<std::is_same<OtherEntry, entry>::value >::type* = 0) {
    return global_map.find(e.get_hashkey_and_value());
  }

  template <typename OtherEntry>
  inline entry* _find_eptr(const OtherEntry& e,
                           typename std::enable_if<!std::is_same<OtherEntry, entry>::value >::type* = 0) {
    return global_map.find(hashkey(e.value()), T(e.value()));
  }

  /* For combining entries and casting between types.  we need to cast
   * and recreate the key if they are different.
   */
  template <typename OtherEntry>
  inline const entry& _cast_entry(const OtherEntry& e,
                                  typename std::enable_if<std::is_same<OtherEntry, entry>::value >::type* = 0) {
    return e;
  }

  template <typename OtherEntry>
  inline entry _cast_entry(const OtherEntry& e,
                           typename std::enable_if<!std::is_same<OtherEntry, entry>::value >::type* = 0) {
    entry ret;
    ret.count = e.count;
    ret.error = e.error;
    ret.kv = hashkey_and_value(T(e.value()));
    return ret;
  }

  /** Combine this heap with another one.
   */
  template <typename U>
  GL_HOT_NOINLINE_FLATTEN
  void _combine(const space_saving<U>& other) {
    /**
     * Pankaj K. Agarwal, Graham Cormode, Zengfeng Huang,
     * Jeff M. Phillips, Zhewei Wei, and Ke Yi.  Mergeable Summaries.
     * 31st ACM Symposium on Principals of Database Systems (PODS). May 2012.
     */

    _debug_check_level_integrity(true);
    other._debug_check_level_integrity(true);

    if(other.m_size == 0)
      return;

    // Start by getting the sizes for the partitions based on the
    // current entries.
    std::map<size_t, size_t> part_sizes;

    // First, go through and update the partition sizes from the other
    // sketch, copying over the counts and errors where they deal with
    // these.
    for(size_t i = 0; i < other.n_entries; ++i) {

      entry* e_ptr = _find_eptr(other.entries[i]);

      if(e_ptr != nullptr) {
        e_ptr->count += other.entries[i].count;
        e_ptr->error += other.entries[i].error;
      } else {
        ++part_sizes[other.entries[i].count];
      }
    }

    for(size_t i = 0; i < n_entries; ++i) {
      ++part_sizes[entries[i].count];
    }

    // Okay, now we have all the partition sizes.  Now just need to go
    // backwards and count out the partitions.

    size_t current_position = m_max_capacity;

    for(auto it = part_sizes.rbegin(); it != part_sizes.rend(); ++it) {

      size_t level = it->first;
      size_t part_size = it->second;

      if(part_size >= current_position) {
        base_level = level;
        break;
      } else {
        current_position -= part_size;
      }
    }

    if(base_level == 0) {
      // None here.
      base_level = part_sizes.begin()->first;
      current_position += part_sizes.begin()->second;
    }

    size_t num_base_entries_left = current_position;
    size_t write_position = 0;

    // Now, go through and move everything over
    std::vector<entry> alt_entries(m_max_capacity);

    auto write_entry = [&](const entry& e) {

      size_t lvl = e.count;

      if(lvl < base_level)
        return;

      if(lvl == base_level) {
        if(num_base_entries_left == 0)
          return;
        --num_base_entries_left;
      }

      DASSERT_LT(write_position, m_max_capacity);
      alt_entries[write_position] = e;
      ++write_position;
    };

    for(size_t i = 0; i < n_entries; ++i)
      write_entry(_cast_entry(entries[i]));

    for(size_t i = 0; i < other.n_entries; ++i) {
      if(_find_eptr(other.entries[i]) == nullptr)
        write_entry(_cast_entry(other.entries[i]));
    }

    // Go through and update the base level stuff

    n_entries = write_position;
    m_size += other.m_size;

    entries = std::move(alt_entries);

    // Now, go through and fill up the hash map correctly
    global_map.clear();

    for(size_t i = 0; i < n_entries; ++i) {
      global_map.insert(entries[i].get_hashkey_and_value(), &(entries[i]));
    }

    cached_insertion_value = nullptr;
    base_level_candidate_count = 0;

    _debug_check_level_integrity(true);
    other._debug_check_level_integrity(true);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Debug stuff

  void _debug_check_level_integrity(bool force_check = false) const GL_HOT_INLINE_FLATTEN {
#ifndef NDEBUG
#ifdef ENABLE_SKETCH_CONSISTENCY_CHECKS
    force_check = true;
#endif

    DASSERT_LE(n_entries, m_size);

    if(!force_check)
      return;

    std::set<const entry*> bl_set(base_level_candidates.begin(),
                                  base_level_candidates.begin() + base_level_candidate_count);

    // Tests the invariant conditions on the entries vector and the level partitions

    for(size_t i = 0; i < n_entries; ++i) {
      ASSERT_GE(entries[i].count, base_level);

      // Make sure all of them are tracked (even if they are not all
      // valid).
      if(entries[i].count == base_level && base_level_candidate_count != 0)
        ASSERT_TRUE(bl_set.find(&entries[i]) != bl_set.end());

      const entry* e = global_map.find(entries[i].get_hashkey_and_value());

      ASSERT_TRUE(e != nullptr);
      ASSERT_EQ(e, &(entries[i]));
    }

    // okay, we're all good!
#endif
  }

};

} // sketch
} // namespace turi
#endif
