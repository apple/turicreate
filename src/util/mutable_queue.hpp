/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// Probabilistic Reasoning Library (PRL)
// Copyright 2005, 2008 (see AUTHORS.txt for a list of contributors)
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef TURI_MUTABLE_PRIORITY_QUEUE_HPP
#define TURI_MUTABLE_PRIORITY_QUEUE_HPP

#include <vector>
#include <map>
#include <algorithm>
#include <boost/unordered_map.hpp>

namespace turi {

  // Deprecated judy has trick
  // template <typename T, typename Compare, typename Hash>
  // class index_type_selector;
  // template <typename T, typename Compare>
  // class index_type_selector<T, Compare, void> {
  //   public:
  //    typedef std::map<T, size_t, Compare> index_map_type;
  // };
  // template <typename T, typename Compare, typename Hash>
  // class index_type_selector {
  //   public:
  //        typedef judy_map_m<T, size_t, Hash, Compare> index_map_type;
  // };



  /**
   * A heap implementation of a priority queue that supports external
   * priorities and priority updates. Both template arguments must be
   * Assignable, EqualityComparable, and LessThanComparable.
   *
   * @param T
   *        the type of items stored in the priority queue.
   * @param Priority
   *        the type used to prioritize items.
   *
   * @see Boost's mutable_queue in boost/pending/mutable_queue.hpp
   * @todo Add a comparator
   *
   * \ingroup util
   */
  template <typename T, typename Priority>
  class mutable_queue {
  public:

    //! An element of the heap.
    typedef typename std::pair<T, Priority> heap_element;

  protected:

    //! The storage type of the index map
    typedef boost::unordered_map<T, size_t> index_map_type;

    //typedef judy_map_m<T, size_t, Compare> index_map_type;
    // Deprecated judy hash trick
    // typedef typename index_type_selector<T, Compare, Hash>::
    // index_map_type index_map_type;

    //! The heap used to store the elements. The first element is unused.
    std::vector<heap_element> heap;

    //! The map used to map from items to indexes in the heap.
    index_map_type index_map;

    //! Returns the index of the left child of the supplied index.
    size_t left(size_t i) const {
      return 2 * i;
    }

    //! Returns the index of the right child of the supplied index.
    size_t right(size_t i) const {
      return 2 * i + 1;
    }

    //! Returns the index of the parent of the supplied index.
    size_t parent(size_t i) const {
      return i / 2;
    }

    //! Extracts the priority at a heap location.
    Priority priority_at(size_t i) {
      return heap[i].second;
    }

    //! Compares the priorities at two heap locations.
    bool less(size_t i, size_t j) {
      return heap[i].second < heap[j].second;
    }

    //! Swaps the heap locations of two elements.
    void swap(size_t i, size_t j) {
      std::swap(heap[i], heap[j]);
      index_map[heap[i].first] = i;
      index_map[heap[j].first] = j;
    }

    //! The traditional heapify function.
    void heapify(size_t i) {
      size_t l = left(i);
      size_t r = right(i);
      size_t s = size();
      size_t largest = i;
      if ((l <= s) && less(i, l))
        largest = l;
      if ((r <= s) && less(largest, r))
        largest = r;
      if (largest != i) {
        swap(i, largest);
        heapify(largest);
      }
    }

  public:
    //! Default constructor.
    mutable_queue()
      : heap(1, std::make_pair(T(), Priority())) { }

    mutable_queue(const mutable_queue& other) :
    heap(other.heap), index_map(other.index_map) { }

    mutable_queue& operator=(const mutable_queue& other) {
      index_map = other.index_map;
      heap = other.heap;
      return *this;
    }

    //! Returns the number of elements in the heap.
    size_t size() const {
      return heap.size() - 1;
    }

    //! Returns true iff the queue is empty.
    bool empty() const {
      return size() == 0;
    }

    //! Returns true if the queue contains the given value
    bool contains(const T& item) const {
      return index_map.count(item) > 0;
    }

    //! Enqueues a new item in the queue.
    void push(T item, Priority priority) {
      heap.push_back(std::make_pair(item, priority));
      size_t i = size();
      index_map[item] = i;
      while ((i > 1) && (priority_at(parent(i)) <= priority)) {
        swap(i, parent(i));
        i = parent(i);
      }
    }

    //! Accesses the item with maximum priority in the queue.
    const std::pair<T, Priority>& top() const {
      assert(!empty());
      return heap[1];
    }

    /**
     * Removes the item with maximum priority from the queue, and
     * returns it with its priority.
     */
    std::pair<T, Priority> pop() {
      assert(!empty());
      heap_element top = heap[1];
      swap(1, size());
      heap.pop_back();
      heapify(1);
      index_map.erase(top.first);
      return top;
    }

    //! Returns the weight associated with a key
    Priority get(T item) const {
      typename index_map_type::const_iterator iter = index_map.find(item);
      assert(iter != index_map.end());
      size_t i = iter->second;
      return heap[i].second;
    }

    //! Returns the priority associated with a key
    Priority operator[](T item) const {
      return get(item);
    }

    /**
     * Updates the priority associated with a item in the queue. This
     * function fails if the item is not already present.
    */
    void update(T item, Priority priority) {
      // Verify that the item is currently in the queue
      typename index_map_type::const_iterator iter = index_map.find(item);
      assert(iter != index_map.end());
      // If it is already present update the priority
      size_t i = iter->second;
      heap[i].second = priority;
      while ((i > 1) && (priority_at(parent(i)) < priority)) {
        swap(i, parent(i));
        i = parent(i);
      }
      heapify(i);
    }

    /**
     * Updates the priority associated with a item in the queue.
     * If the item is not already present, insert it.
    */
    void push_or_update(T item, Priority priority) {
      // Verify that the item is currently in the queue
      typename index_map_type::const_iterator iter = index_map.find(item);
      if(iter != index_map.end()) {
        // If it is already present update the priority
        size_t i = iter->second;
        heap[i].second = priority;
        while ((i > 1) && (priority_at(parent(i)) < priority)) {
          swap(i, parent(i));
          i = parent(i);
        }
        heapify(i);
      }
      else {
        push(item, priority);
      }
    }

    /**
     * If item is already in the queue, sets its priority to the maximum
     * of the old priority and the new one. If the item is not in the queue,
     * adds it to the queue.
     *
     * returns true if the items was not already
     */
    bool insert_max(T item, Priority priority) {
      // determine if the item is already in the queue
      typename index_map_type::const_iterator iter = index_map.find(item);
      if(iter != index_map.end()) { // already present
        // If it is already present update the priority
        size_t i = iter->second;
        heap[i].second = std::max(priority, heap[i].second);
        // If the priority went up move the priority until its greater
        // than its parent
        while ((i > 1) && (priority_at(parent(i)) <= priority)) {
          swap(i, parent(i));
          i = parent(i);
        }
        // Trickle down if necessary
        heapify(i);  // This should not be necessary
        return false;
      } else { // not already present so simply add
        push(item, priority);
        return true;
      }
    }

    /**
     * If item is already in the queue, sets its priority to the sum
     * of the old priority and the new one. If the item is not in the queue,
     * adds it to the queue.
     *
     * returns true if the item was already present
     */
    bool insert_cumulative(T item, Priority priority) {
      // determine if the item is already in the queue
      typename index_map_type::const_iterator iter = index_map.find(item);
      if(iter != index_map.end()) { // already present
        // If it is already present update the priority
        size_t i = iter->second;
        heap[i].second = priority + heap[i].second;
        // If the priority went up move the priority until its greater
        // than its parent
        while ((i > 1) && (priority_at(parent(i)) <= priority)) {
          swap(i, parent(i));
          i = parent(i);
        }
        // Trickle down if necessary
        heapify(i);  // This should not be necessary
        return false;
      } else { // not already present so simply add
        push(item, priority);
        return true;
      }
    } // end of insert cumulative


    //! Returns the values (key-priority pairs) in the priority queue
    const std::vector<heap_element>& values() const {
      return heap;
    }

    //! Clears all the values (equivalent to stl clear)
    void clear() {
      heap.clear();
      heap.push_back(std::make_pair(T(), Priority()));
      index_map.clear();
    }

    //! Remove an item from the queue
    bool remove(T item) {
      // Ensure that the element is in the queue
      typename index_map_type::iterator iter = index_map.find(item);
      // only if the element is present in the first place do we need
      // remove it
      if(iter != index_map.end()) {
        size_t i = iter->second;
        swap(i, size());
        heap.pop_back();
        heapify(i);
        // erase the element from the index map
        index_map.erase(iter);
        return true;
      }
      return false;
    }

  }; // class mutable_queue





//   // define a blank cosntant for the mutable queue
// #define BLANK (size_t(-1))

//   template <typename Priority>
//   class mutable_queue<size_t, Priority> {
//   public:


//     //! An element of the heap.
//     typedef typename std::pair<size_t, Priority> heap_element;

//     typedef size_t index_type;

//   protected:

//     //! The storage type of the index map
//     typedef std::vector<index_type> index_map_type;

//     //! The heap used to store the elements. The first element is unused.
//     std::vector<heap_element> heap;

//     //! The map used to map from items to indexes in the heap.
//     index_map_type index_map;

//     //! Returns the index of the left child of the supplied index.
//     size_t left(size_t i) const {
//       return 2 * i;
//     }

//     //! Returns the index of the right child of the supplied index.
//     size_t right(size_t i) const {
//       return 2 * i + 1;
//     }

//     //! Returns the index of the parent of the supplied index.
//     size_t parent(size_t i) const {
//       return i / 2;
//     }

//     //! Extracts the priority at a heap location.
//     Priority priority_at(size_t i) {
//       return heap[i].second;
//     }

//     //! Compares the priorities at two heap locations.
//     bool less(size_t i, size_t j) {
//       assert( i < heap.size() );
//       assert( j < heap.size() );
//       return heap[i].second < heap[j].second;
//     }

//     //! Swaps the heap locations of two elements.
//     void swap(size_t i, size_t j) {
//       if(i == j) return;
//       std::swap(heap[i], heap[j]);
//       assert(heap[i].first < index_map.size());
//       assert(heap[j].first < index_map.size());
//       index_map[heap[i].first] = i;
//       index_map[heap[j].first] = j;
//     }

//     //! The traditional heapify function.
//     void heapify(size_t i) {
//       size_t l = left(i);
//       size_t r = right(i);
//       size_t s = size();
//       size_t largest = i;
//       if ((l <= s) && less(i, l))
//         largest = l;
//       if ((r <= s) && less(largest, r))
//         largest = r;
//       if (largest != i) {
//         swap(i, largest);
//         heapify(largest);
//       }
//     }

//   public:
//     //! Default constructor.
//     mutable_queue()
//       :	heap(1, std::make_pair(-1, Priority())) { }

//     //! Returns the number of elements in the heap.
//     size_t size() const {
//       assert(heap.size() > 0);
//       return heap.size() - 1;
//     }

//     //! Returns true iff the queue is empty.
//     bool empty() const {
//       return size() == 0;
//     }

//     //! Returns true if the queue contains the given value
//     bool contains(const size_t& item) const {
//       return item < index_map.size() &&
//         index_map[item] != BLANK;
//     }

//     //! Enqueues a new item in the queue.
//     void push(size_t item, Priority priority) {
//       assert(item != BLANK);
//       heap.push_back(std::make_pair(item, priority));
//       size_t i = size();
//       if ( !(item < index_map.size()) ) {
//         index_map.resize(item + 1, BLANK);
//       }
//       // Bubble up
//       index_map[item] = i;
//       while ((i > 1) && (priority_at(parent(i)) < priority)) {
//         swap(i, parent(i));
//         i = parent(i);
//       }
//     }

//     //! Accesses the item with maximum priority in the queue.
//     const std::pair<size_t, Priority>& top() const {
//       assert(heap.size() > 1);
//       return heap[1];
//     }

//     /**
//      * Removes the item with maximum priority from the queue, and
//      * returns it with its priority.
//      */
//     std::pair<size_t, Priority> pop() {
//       assert(heap.size() > 1);
//       heap_element top = heap[1];
//       assert(top.first < index_map.size());
//       swap(1, size());
//       heap.pop_back();
//       heapify(1);
//       index_map[top.first] = BLANK;
//       return top;
//     }

//     //! Returns the weight associated with a key
//     Priority get(size_t item) const {
//       assert(item < index_map.size());
//       assert(index_map[item] != BLANK);
//       return heap[index_map[item]].second;
//     }

//     //! Returns the priority associated with a key
//     Priority operator[](size_t item) const {
//       return get(item);
//     }

//     /**
//      * Updates the priority associated with a item in the queue. This
//      * function fails if the item is not already present.
//     */
//     void update(size_t item, Priority priority) {
//       assert(item < index_map.size());
//       size_t i = index_map[item];
//       heap[i].second = priority;
//       while ((i > 1) && (priority_at(parent(i)) < priority)) {
//         swap(i, parent(i));
//         i = parent(i);
//       }
//       heapify(i);
//     }

//     /**
//      * If item is already in the queue, sets its priority to the maximum
//      * of the old priority and the new one. If the item is not in the queue,
//      * adds it to the queue.
//      */
//     void insert_max(size_t item, Priority priority) {
//       assert(item != BLANK);
//       if(!contains(item))
//         push(item, priority);
//       else {
//         Priority effective_priority = std::max(get(item), priority);
//         update(item, effective_priority);
//       }
//     }

//     //! Returns the values (key-priority pairs) in the priority queue
//     const std::vector<heap_element>& values() const {
//       return heap;
//     }

//     //! Clears all the values (equivalent to stl clear)
//     void clear() {
//       heap.clear();
//       heap.push_back(std::make_pair(-1, Priority()));
//       index_map.clear();
//     }

//     /**
//      * Remove an item from the queue returning true if the item was
//      * originally present
//      */
//     bool remove(size_t item) {
//       if(contains(item)) {
//         assert(size() > 0);
//         assert(item < index_map.size());
//         size_t i = index_map[item];
//         assert(i != BLANK);
//         swap(i, size());
//         heap.pop_back();
//         heapify(i);
//         // erase the element from the index map
//         index_map[item] = BLANK;
//         return true;
//       } else {
//         // Item was not present
//         return false;
//       }
//     }
//   }; // class mutable_queue

// #undef BLANK





} // namespace turi

#endif // #ifndef TURI_MUTABLE_PRIORITY_QUEUE_HPP
