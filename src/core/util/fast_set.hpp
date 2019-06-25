/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UTIL_FAST_SET
#define TURI_UTIL_FAST_SET

#include <vector>
#include <algorithm>
#include <core/util/cuckoo_set_pow2.hpp>

/**
 * Contains a fast set implementation and utilities.
 */

namespace turi {

/**
 * \ingroup util
 * Radix sort implementation from https://github.com/gorset/radix
 * \verbatim
 *  Copyright 2011 Erik Gorset. All rights reserved.
 *  Redistribution and use in source and binary forms, with or without modification, are
 *  permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of
 *  conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list
 *  of conditions and the following disclaimer in the documentation and/or other materials
 *  provided with the distribution.
 *  THIS SOFTWARE IS PROVIDED BY Erik Gorset ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 *  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Erik Gorset OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *  The views and conclusions contained in the software and documentation are those of the
 *  authors and should not be interpreted as representing official policies, either expressed
 *  or implied, of Erik Gorset.
 *\endverbatim
 */
template<typename T>
void radix_sort(T* array, int offset, int end, int shift) {
  int x, y;
  T value, temp;
  int last[256] = { 0 }, pointer[256];

  for (x=offset; x<end; ++x) {
    ++last[(array[x] >> shift) & 0xFF];
  }

  last[0] += offset;
  pointer[0] = offset;
  for (x=1; x<256; ++x) {
    pointer[x] = last[x-1];
    last[x] += last[x-1];
  }

  for (x=0; x<256; ++x) {
    while (pointer[x] != last[x]) {
      value = array[pointer[x]];
      y = (value >> shift) & 0xFF;
      while (x != y) {
        temp = array[pointer[y]];
        array[pointer[y]++] = value;
        value = temp;
        y = (value >> shift) & 0xFF;
      }
      array[pointer[x]++] = value;
    }
  }

  if (shift > 0) {
    shift -= 8;
    for (x=0; x<256; ++x) {
      temp = x > 0 ? pointer[x] - pointer[x-1] : pointer[0] - offset;
      if (temp > 64) {
        radix_sort(array, pointer[x] - temp, pointer[x], shift);
      } else if (temp > 1) {
        std::sort(array + (pointer[x] - temp), array + pointer[x]);
      }
    }
  }
}


// A fast set storing values in either a vector of sorted VIDs
// or a hash set (cuckoo hash).
// If the number of elements is greater than HASH_THRESHOLD,
// the hash set is used. Otherwise the vector is used.
template<typename T>
struct fast_set {

  const static size_t HASH_THRESHOLD = 64;

  std::vector<T> vec;
  turi::cuckoo_set_pow2<T, 3> *cset;
  fast_set(): cset(NULL) { }
  fast_set(const fast_set& v):cset(NULL) {
    (*this) = v;
  }

  fast_set& operator=(const fast_set& v) {
    if (this == &v) return *this;
    vec = v.vec;
    if (v.cset != NULL) {
      // allocate the cuckoo set if the other side is using a cuckoo set
      // or clear if I alrady have one
      if (cset == NULL) {
        cset = new turi::cuckoo_set_pow2<T, 3>(-1, 0, 2 * v.cset->size());
      }
      else {
        cset->clear();
      }
      (*cset) = *(v.cset);
    }
    else {
      // if the other side is not using a cuckoo set, lets not use a cuckoo set
      // either
      if (cset != NULL) {
        delete cset;
        cset = NULL;
      }
    }
    return *this;
  }

  ~fast_set() {
    if (cset != NULL) delete cset;
  }

  // assigns a vector of vertex IDs to this storage.
  // this function will clear the contents of the fast_set
  // and reconstruct it.
  // If the assigned values has length >= HASH_THRESHOLD,
  // we will allocate a cuckoo set to store it. Otherwise,
  // we just store a sorted vector
  void assign(const std::vector<T>& _vec) {
    clear();
    if (_vec.size() >= HASH_THRESHOLD) {
        // move to cset
        cset = new turi::cuckoo_set_pow2<T, 3>(-1, 0, 2 * _vec.size());
        for(T& v: _vec) {
          cset->insert(v);
        }
    }
    else {
      vec = _vec;
      if (vec.size() > 64) {
        radix_sort(&(vec[0]), 0, vec.size(), 24);
      }
      else {
        std::sort(vec.begin(), vec.end());
      }
      typename std::vector<T>::iterator new_end = std::unique(vec.begin(), vec.end());
      vec.erase(new_end, vec.end());
    }
  }

  void clear() {
    vec.clear();
    if (cset != NULL) {
      delete cset;
      cset = NULL;
    }
  }

  size_t size() const {
    return cset == NULL ? vec.size() : cset->size();
  }
};

/*
  A simple counting iterator which can be used as an insert iterator.
  but only counts the number of elements inserted. Useful for
  use with counting the size of an intersection using std::set_intersection
*/
template <typename T>
struct counting_inserter {
  size_t* i;
  counting_inserter(size_t* i):i(i) { }
  counting_inserter& operator++() {
    ++(*i);
    return *this;
  }
  void operator++(int) {
    ++(*i);
  }

  struct empty_val {
    empty_val operator=(const T&) { return empty_val(); }
  };

  empty_val operator*() {
    return empty_val();
  }

  typedef empty_val reference;
};


/*
 * Computes the size of the intersection of two fast_set's
 */
template<typename T>
static uint32_t count_set_intersect(
             const fast_set<T>& smaller_set,
             const fast_set<T>& larger_set) {
  if (smaller_set.size() > larger_set.size()) {
    return count_set_intersect(larger_set, smaller_set);
  }
  if (smaller_set.cset == NULL && larger_set.cset == NULL) {
    size_t i = 0;
    counting_inserter<T> iter(&i);
    std::set_intersection(smaller_set.vid_vec.begin(), smaller_set.vid_vec.end(),
                          larger_set.vid_vec.begin(), larger_set.vid_vec.end(),
                          iter);
    return i;
  }
  else if (smaller_set.cset == NULL && larger_set.cset != NULL) {
    size_t i = 0;
    for (T& value : smaller_set.vec) {
      i += larger_set.cset->count(value);
    }
    return i;
  }
  else if (smaller_set.cset != NULL && larger_set.cset == NULL) {
    size_t i = 0;
    for (T& value : larger_set.vec) {
      i += smaller_set.cset->count(value);
    }
    return i;
  }
  else {
    size_t i = 0;
    for (T& value: *(smaller_set.cset)) {
      i += larger_set.cset->count(value);
    }
    return i;
  }
}

}
#endif
