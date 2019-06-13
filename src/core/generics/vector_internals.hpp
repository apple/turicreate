/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_GL_VECTOR_INTERNALS_H_
#define TURI_GL_VECTOR_INTERNALS_H_

#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <iterator>
#include <memory>
#include <exception>
#include <type_traits>
#include <initializer_list>
#include <core/util/basic_types.hpp>
#include <core/logging/assertions.hpp>
#include <core/util/basic_types.hpp>
#include <utility>

#ifdef NDEBUG
#define _VEC_NDEBUG_NOEXCEPT noexcept
#else
#define _VEC_NDEBUG_NOEXCEPT
#endif

#define _VEC_ENABLE_IF_ITERATOR(InputIterator, T) \
typename std::enable_if<std::is_convertible<      \
   typename std::iterator_traits<                 \
      InputIterator>::value_type, T>::value>::type* = 0

#define _VEC_ENABLE_IF_CONVERTABLE(T1, T2)                              \
  typename std::enable_if<std::is_convertible<T1, T2>::value>::type* = 0

static_assert(std::is_convertible<
                typename std::iterator_traits<int*>::value_type, int>::value, "");

namespace turi { namespace gl_vector_internal {


// The main structure
template <typename T>
struct _vstruct {
  size_t size;
  size_t capacity;
  T data[0];
};

////////////////////////////////////////////////////////////////////////////////
// Some default sizes
template <typename T>
static constexpr size_t _empty_gl_vector_element_size() {

  // Allocate it so that the entire struct fits in 64 bytes, and 64
  // bytes is used.
  return std::max<size_t>(1, (64 - sizeof(_vstruct<T>)) / std::max<size_t>(1, sizeof(T)));
}

template <typename T>
GL_HOT_INLINE_FLATTEN
static inline size_t _round_up_to_stride(size_t n) {

  switch(sizeof(T)) {
    case 1: n = (n + 15) & ~size_t(15); break;
    case 2: n = (n + 7) & ~size_t(7); break;
    case 4: n = (n + 3) & ~size_t(3); break;
    case 8: n = (n + 1) & ~size_t(1); break;
    default: return n;
  }

  DASSERT_EQ( (n * sizeof(T)) % 16, 0);
  return n;
}

template <typename T>
static inline bool has_excess_storage(const _vstruct<T>* info) {
  return (capacity(info) > gl_vector_internal::_round_up_to_stride<T>(size(info)));
}


////////////////////////////////////////////////////////////////////////////////
// Allocation of the implementation

template <typename T>
GL_HOT_INLINE
static inline _vstruct<T>* _allocate_vstruct(size_t n) {

  const bool allocate_with_calloc = std::is_trivial<T>::value;

  n = _round_up_to_stride<T>(n);

  _vstruct<T>* info = (allocate_with_calloc
                       ? (_vstruct<T>*) calloc(sizeof(_vstruct<T>) + n*sizeof(T), 1)
                       : (_vstruct<T>*) malloc(sizeof(_vstruct<T>) + n*sizeof(T)));

  if(info == nullptr) throw std::bad_alloc();
  info->capacity = n;
  return info;
}


////////////////////////////////////////////////////////////////////////////////
// Destruction

template <typename T>
GL_HOT_INLINE_FLATTEN
static inline void _destroy_element(T* ptr) {
  if(!std::is_trivial<T>::value) {
    ptr->~T();
  }
}

template <typename T>
GL_HOT_INLINE_FLATTEN
static inline void _destroy_range(T* start, T* end) {
  if(!std::is_trivial<T>::value) {
    for(auto ptr = start; ptr != end; ++ptr)
      _destroy_element(ptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Utilities

template <typename T>
GL_HOT_INLINE
void uninitialized_move(T* first, T* last, T* dest) {
  memmove(dest, first, sizeof(T) * (last - first) );
}

template <typename T>
GL_HOT_INLINE
void uninitialized_move_backward(T* first, T* last, T* dest_last) {

  size_t n = last - first;
  memmove(dest_last - n, first, sizeof(T) * n);
}

////////////////////////////////////////////////////////////////////////////////

template <typename T>
GL_HOT_INLINE
void uninitialized_construct(T* first, const T* last) {

  if(std::is_trivial<T>::value) {
    memset(first, 0, (last - first) * sizeof(T));
  } else {
    for(;first != last; ++first) {
      new (first) T ();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Size

template <typename T>
GL_HOT_INLINE
static inline size_t size (const _vstruct<T>* info ) noexcept {
  return (info == nullptr) ? 0 : info->size;
}

template <typename T>
GL_HOT_INLINE
static inline size_t capacity (const _vstruct<T>* info ) noexcept {
  return (info == nullptr) ? 0 : info->capacity;
}

template <typename T>
GL_HOT_INLINE
static inline T* begin (_vstruct<T>* info ) noexcept {
  return (info == nullptr) ? nullptr : info->data;
}

template <typename T>
GL_HOT_INLINE
static inline const T* cbegin (const _vstruct<T>* info ) noexcept {
  return (info == nullptr) ? nullptr : info->data;
}

template <typename T>
GL_HOT_INLINE
static inline T* end (_vstruct<T>* info ) noexcept {
  return (info == nullptr) ? nullptr : info->data + info->size;
}

template <typename T>
GL_HOT_INLINE
static inline const T* cend (const _vstruct<T>* info ) noexcept {
  return (info == nullptr) ? nullptr : info->data + info->size;
}


template <typename T>
GL_HOT_INLINE
static inline void _check_pointer_valid(_vstruct<T>* info, const T* ptr, bool include_end_point) {

#ifndef NDEBUG
  if((info == nullptr && ptr != nullptr && !include_end_point)
     || (info != nullptr
         && (ptr < info->data
             || std::distance((const T*)(info->data), ptr) > truncate_check<int64_t>(size(info))
             || (!include_end_point && info->data + size(info) == ptr))))

    throw std::out_of_range("Position of iterator points outside valid range.");
#endif
}

////////////////////////////////////////////////////////////////////////////////
// Reallocation

template <typename T>
GL_HOT_INLINE
static inline void _extend_range(_vstruct<T>*& info, size_t n, bool extend_extra) {

  size_t new_capacity = std::max<size_t>(_empty_gl_vector_element_size<T>(), extend_extra ? (5 * n) / 4 : n);

  new_capacity = _round_up_to_stride<T>(new_capacity);

  if(info == nullptr) {
    info = (_vstruct<T>*) malloc(sizeof(_vstruct<T>) + new_capacity*sizeof(T));
    if(UNLIKELY(info == nullptr)) throw std::bad_alloc();
    info->size = 0;
  } else {
    info = (_vstruct<T>*) realloc(info, sizeof(_vstruct<T>) + new_capacity*sizeof(T));
    if(UNLIKELY(info == nullptr)) throw std::bad_alloc();
  }

  info->capacity = new_capacity;
}

////////////////////////////////////////////////////////////////////////////////
//
//  The implementations of the member functions
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Construction routines

template <typename T>
GL_HOT_INLINE
static inline _vstruct<T>* construct() {
  return nullptr;
}

////////////////////////////////////////////////////////////

template <typename T>
GL_HOT_INLINE
static inline _vstruct<T>* construct(size_t n, const T& val) {
  if(n == 0) return nullptr;
  _vstruct<T>* info = _allocate_vstruct<T>(n);
  info->size = n;
  std::uninitialized_fill(info->data, info->data + n, val);
  return info;
}


template <typename T>
GL_HOT_INLINE
static inline _vstruct<T>* construct_uninitialized(size_t capacity) {
  _vstruct<T>* info = _allocate_vstruct<T>(capacity);
  info->size = 0;
  return info;
}


////////////////////////////////////////////////////////////

template <typename T>
GL_HOT_INLINE
static inline _vstruct<T>* construct(size_t n) {
  size_t reserve_size = std::max(n, _empty_gl_vector_element_size<T>());
  _vstruct<T>* info = _allocate_vstruct<T>(reserve_size);
  info->size = n;
  new (&(info->data[0]))T[n];
  return info;
}

////////////////////////////////////////////////////////////
template <typename T, typename InputIterator>
GL_HOT_INLINE
static inline _vstruct<T>* construct(const InputIterator& first, const InputIterator& last,
                                     _VEC_ENABLE_IF_ITERATOR(InputIterator, T)) {
  long ln = std::distance(first, last);
  DASSERT_TRUE(ln >= 0);
  size_t n = ln;

  if(n == 0) return nullptr;

  _vstruct<T>* info = _allocate_vstruct<T>(n);

  info->size = n;
  std::uninitialized_copy(first, last, info->data);

  return info;
}

////////////////////////////////////////////////////////////////////////////////
// Destruction routines

template <typename T>
GL_HOT_INLINE
static inline void destroy(_vstruct<T>*& info) {
  if(info == nullptr)
    return;

  _destroy_range(info->data, info->data + info->size);
  free(info);
  info = nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// Resizing routines

template <typename T>
static inline void resize (_vstruct<T>*& info, size_t n) {
  if(info == nullptr) {
    info = construct<T>(n);
    return;
  }

  if(info->size == n)
    return;

  if(n < info->size) {
    _destroy_range(info->data + n, info->data + info->size);
  } else {
    if(n > info->capacity) {
      _extend_range(info, n, false);
    }
    uninitialized_construct(info->data + info->size, info->data + n);
  }

  info->size = n;
}

template <typename T>
GL_HOT_INLINE
static inline void resize (_vstruct<T>*& info, size_t n, const T& val) {
  if(info == nullptr) {
    info = construct<T>(n, val);
    return;
  }

  if(info->size == n)
    return;

  if(n < info->size) {
    _destroy_range(info->data + n, info->data + info->size);
  } else {
    if(n > info->capacity) {
      _extend_range(info, n, false);
    }
    std::uninitialized_fill(info->data + info->size, info->data + n, val);
  }

  info->size = n;
}

////////////////////////////////////////////////////////////////////////////////
// Element access routines

template <typename T>
GL_HOT_INLINE
static inline T& get_element(_vstruct<T>* info, size_t idx) _VEC_NDEBUG_NOEXCEPT {
#ifndef NDEBUG
  if(idx >= size(info))
    throw std::out_of_range(std::string("Index ") + std::to_string(idx)
                            + " outside of range (0, " + std::to_string(size(info)) + "].");
#endif

  return info->data[idx];
}

////////////////////////////////////////////////////////////////////////////////
// Reserve capacity

template <typename T>
GL_HOT_INLINE
static inline void reserve (_vstruct<T>*& info, size_t n) {
  if(n > capacity(info))
    _extend_range(info, n, false);
}

////////////////////////////////////////////////////////////////////////////////
// Assignment operators

template <typename T, class InputIterator>
static inline void assign (_vstruct<T>*& info, InputIterator first, InputIterator last,
                           _VEC_ENABLE_IF_ITERATOR(InputIterator, T)) {
  if(info == nullptr) {
    info = construct<T>(first, last);
    return;
  }

  _destroy_range(info->data, info->data + info->size);
  size_t n = std::distance(first, last);

  if(n > info->capacity)
    _extend_range(info, n, false);

  std::uninitialized_copy(first, last, info->data);
  info->size = n;
}

template <typename T>
GL_HOT_INLINE
static inline void assign(_vstruct<T>*& info, size_t n, const T& val) {
  if(info == nullptr) {
    info = construct(n, val);
    return;
  }

  _destroy_range(info->data, info->data + info->size);

  if(n > info->capacity)
    _extend_range(info, n, false);

  info->size = n;
  std::uninitialized_fill(info->data, info->data + info->size, val);
}

template <typename T>
GL_HOT_INLINE
static inline void assign(_vstruct<T>*& info, std::initializer_list<T> il) {
  assign(info, il.begin(), il.end());
}

template <typename T>
GL_HOT_INLINE
static inline void assign_move(_vstruct<T>*& info, _vstruct<T>*& other_info) {
  if(other_info != info) {
    gl_vector_internal::destroy(info);
    info = other_info;
    other_info = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Push back

template <typename T>
GL_HOT_INLINE
static inline void push_back(_vstruct<T>*& info, const T& val) {
  size_t s = size(info);
  if(s == capacity(info))
    _extend_range(info, s+1, true);

  new (&info->data[info->size]) T (val);
  ++info->size;
}

template <typename T>
GL_HOT_INLINE
static inline void push_back(_vstruct<T>*& info, T&& val) {
  if(info == nullptr) {
    info = construct_uninitialized<T>(_empty_gl_vector_element_size<T>());
  } else if(info->size == info->capacity) {
    _extend_range(info, info->size + 1, true);
  }

  new (&info->data[info->size]) T (std::forward<T>(val));
  ++info->size;
}

////////////////////////////////////////////////////////////////////////////////
// pop back

template <typename T>
GL_HOT_INLINE
static inline void pop_back(_vstruct<T>*& info) {

#ifndef NDEBUG
  if(size(info) == 0)
    throw std::out_of_range("pop_back called on empty gl_vector.");
#endif

  --info->size;
  _destroy_element(&info->data[info->size]);
}

////////////////////////////////////////////////////////////////////////////////
// emplace

template <typename T, class... Args>
GL_HOT_INLINE
T* emplace (_vstruct<T>*& info, const T* position, Args&&... args) {

  _check_pointer_valid(info, position, true);

  size_t idx;

  if(info == nullptr) {
    info = construct_uninitialized<T>(_empty_gl_vector_element_size<T>());
    idx = 0;
  } else {
    idx = std::distance((const T*)(info->data), position);

    if(info->capacity == info->size)
      _extend_range(info, info->size + 1, true);
  }

  T* ptr = info->data + idx;

  uninitialized_move_backward(ptr, info->data + info->size, info->data + info->size + 1);
  ++info->size;

  new (ptr) T (args...);

  return ptr;
}

////////////////////////////////////////////////////////////////////////////////
// emplace back

template <typename T, class... Args>
GL_HOT_INLINE
static inline void emplace_back (_vstruct<T>*& info, Args&&... args) {
  if(info == nullptr) {
    info = construct_uninitialized<T>(_empty_gl_vector_element_size<T>());
  } else if(info->size == info->capacity) {
    _extend_range(info, info->size + 1, true);
  }

  T* ptr = &(info->data[info->size]);
  ++info->size;
  new (ptr) T (args...);
}

////////////////////////////////////////////////////////////////////////////////
// Insertion

template <typename T, typename U>
GL_HOT_INLINE
T* insert (_vstruct<T>*& info,  const T* position, U&& val) {

  _check_pointer_valid(info, position, true);

  if(info == nullptr) {
    info = construct_uninitialized<T>(_empty_gl_vector_element_size<T>());
    info->size = 1;
    new (&info->data[0]) T (std::forward<U>(val));
    return info->data;
  }

  size_t idx = std::distance((const T*)(info->data), position);

  if(info->capacity == info->size) {
    _extend_range(info, info->size + 1, true);
  }

  ++(info->size);
  T* ptr = info->data + idx;

  if(position != info->data + info->size)
    uninitialized_move_backward(ptr, info->data + info->size - 1, info->data + info->size);

  new (ptr) T (std::forward<U>(val));

  return ptr;
}

////////////////////////////////////////////////////////////

template <typename T>
GL_HOT_INLINE
T* insert (_vstruct<T>*& info,  const T* position, size_t n, const T& val) {
  _check_pointer_valid(info, position, true);

  if(info == nullptr) {
    assign(info, n, val);
    return (UNLIKELY(info == nullptr)) ? nullptr : info->data;
  }

  size_t idx = std::distance((const T*)(info->data), position);

  if(n == 0)
    return info->data + idx;

  //  if(info->capacity < info->size + n)
  if(info->size + n > info->capacity)
    _extend_range(info, info->size + n, true);

  T* ptr = info->data + idx;

  if(position != info->data + info->size)
    uninitialized_move_backward(ptr, info->data + info->size, info->data + info->size + n);

  std::uninitialized_fill(ptr, ptr + n, val);

  info->size += n;

  return ptr;
}

////////////////////////////////////////////////////////////

// Do this to properly handle, e.g. x + x in the string case.
template <typename T, class InputIterator>
GL_HOT_INLINE
T* insert (_vstruct<T>*& info,  const T* position,
           const T* start, const T* end) {

  if(info != nullptr
     && UNLIKELY(info->data <= start && start <= info->data + info->size)) {

    std::vector<T> tmp(start, end);
    return _insert(info, position, tmp.begin(), tmp.end());
  }

  return _insert(info, position, start, end);
}

// The general case.
template <typename T, class InputIterator>
GL_HOT_INLINE
T* insert (_vstruct<T>*& info,  const T* position,
           InputIterator start, InputIterator end,
           _VEC_ENABLE_IF_ITERATOR(InputIterator, T)) {
  return _insert(info, position, start, end);
}

template <typename T, class InputIterator>
static inline
GL_HOT_INLINE_FLATTEN
bool _contains_iterator(_vstruct<T>*& info, const InputIterator& it) {
  return false;
}

template <typename T>
GL_HOT_INLINE_FLATTEN
static inline
bool _contains_iterator(
    _vstruct<T>*& info, const T* it) {
  ptrdiff_t delta = std::distance(cbegin(info), it);
  return (
    (delta >= 0) && (static_cast<int64_t>(delta) < static_cast<int64_t>(capacity(info))));
}

template <typename T, class InputIterator>
GL_HOT_NOINLINE
static T* _insert_with_overlapping_source_iterator
(_vstruct<T>*& info,  const T* position,
 InputIterator start, InputIterator end) {

  // The specialization below should be the only one that's actually called.
  DASSERT_TRUE(false);
  return nullptr;
}

template <typename T>
GL_HOT_NOINLINE
static T* _insert_with_overlapping_source_iterator
(_vstruct<T>*& info,  const T* position, const T* start, const T* end) {

  // Add in these checks to make sure that we're using
  // this function appropriately
#ifndef NDEBUG
  DASSERT_TRUE(info != nullptr);
  _check_pointer_valid(info, position, true);
  _check_pointer_valid(info, start, true);
  _check_pointer_valid(info, start, true);
#endif

  _vstruct<T>* temp = nullptr;
  assign(temp, start, end);
  return _insert(info, position, cbegin(temp), cend(temp));
}

// The base case.
template <typename T, class InputIterator>
GL_HOT_INLINE
T* _insert (_vstruct<T>*& info,  const T* position,
           InputIterator start, InputIterator end,
           _VEC_ENABLE_IF_ITERATOR(InputIterator, T)) {

  _check_pointer_valid(info, position, true);

  if(info == nullptr) {
    assign(info, start, end);
    return (UNLIKELY(info == nullptr)) ? nullptr : info->data;
  }

  size_t idx = std::distance((const T*)(info->data), position);

  if(start == end)
    return info->data + idx;

  size_t n = std::distance(start, end);

  if(info->size + n > capacity(info)) {
    // Need to check that extending the range
    // does not invalidate the iterators for start and end.
    if(UNLIKELY(_contains_iterator(info, start))) {
     return _insert_with_overlapping_source_iterator(
         info, position, start, end);
    }

    _extend_range(info, info->size + n, true);
  }


  if(position != info->data + info->size)
    uninitialized_move_backward(info->data + idx, info->data + info->size, info->data + info->size + n);

  std::uninitialized_copy(start, end, info->data + idx);

  info->size += n;

  return info->data + idx;
}

////////////////////////////////////////////////////////////

template <typename T>
GL_HOT_INLINE
T* erase (_vstruct<T>*& info,  const T* position) {

  _check_pointer_valid(info, position, false);

  size_t idx = std::distance((const T*)(info->data), position);

  _destroy_element(position);

  T* ptr = info->data + idx;
  uninitialized_move(ptr + 1, info->data + info->size, ptr);
  --info->size;

  return ptr;
}

////////////////////////////////////////////////////////////

template <typename T>
GL_HOT_INLINE
static inline T* erase(_vstruct<T>*& info,  const T* start, const T* end) {

  _check_pointer_valid(info, start, true);
  _check_pointer_valid(info, end, true);
  if(UNLIKELY(info == nullptr)) return nullptr;

  if(start > end)
    throw std::out_of_range("Start and ending iterators out of order.");

  size_t n = std::distance(start, end);

  size_t start_idx = std::distance((const T*)(info->data), start);
  size_t end_idx = std::distance((const T*)(info->data), end);

  if(start == end)
    return info->data + start_idx;

  _destroy_range(info->data + start_idx, info->data + end_idx);

  uninitialized_move(info->data + end_idx,
                     info->data + info->size,
                     info->data + start_idx);

  info->size -= n;

  return info->data + start_idx;
}

// Creates an empty space of size n, replacing everything from
// idx_start to idx_end.  idx_start to idx_start + n will then be free
// and uninitialized.
template <typename T>
GL_HOT_INLINE
static inline void _make_uninitialized_section (
    _vstruct<T>*& info, size_t idx_start, size_t idx_end, ptrdiff_t n) {

  DASSERT_LT(idx_start, idx_end);

  ptrdiff_t extend_amount = (ptrdiff_t(n) - (ptrdiff_t(idx_end) - ptrdiff_t(idx_start)));
  size_t new_size = size(info) + extend_amount;

  _destroy_range(info->data + idx_start, info->data + idx_end);

  if(extend_amount > 0) {
    if(new_size > capacity(info)) {
      _extend_range(info, new_size, true);
    }

    if(idx_end != info->size) {
      uninitialized_move_backward(info->data + idx_end, info->data + info->size, info->data + new_size);
    }

  } else if(extend_amount < 0) {

    DASSERT_TRUE(idx_end + extend_amount >= 0);

    uninitialized_move(info->data + idx_end,
                       info->data + info->size,
                       info->data + idx_end + extend_amount);
  }

  info->size = new_size;
}

template <typename T, class InputIterator>
GL_HOT_INLINE
static inline void replace (_vstruct<T>*& info,
                            const T* position_start,
                            const T* position_end,
                            InputIterator start,
                            InputIterator end,
                            _VEC_ENABLE_IF_ITERATOR(InputIterator, T)) {

  _check_pointer_valid(info, position_start, true);
  _check_pointer_valid(info, position_end, true);
  DASSERT_TRUE(position_start <= position_end);

  if(UNLIKELY(info == nullptr)) {
    DASSERT_TRUE(position_start == nullptr);
    DASSERT_TRUE(position_end == nullptr);
    assign(info, start, end);
    return;
  }

  if(UNLIKELY(position_start == position_end)) {
    insert(info, position_start, start, end);
    return;
  }

  size_t idx_start = std::distance((const T*)(info->data), position_start);
  size_t idx_end = std::distance((const T*)(info->data), position_end);
  ptrdiff_t n = std::distance(start, end);

  _make_uninitialized_section(info, idx_start, idx_end, n);

  std::uninitialized_copy(start, end, info->data + idx_start);
}

template <typename T>
GL_HOT_INLINE
static inline void replace (_vstruct<T>*& info,
                            const T* position_start,
                            const T* position_end,
                            size_t n, const T& val) {

  _check_pointer_valid(info, position_start, true);
  _check_pointer_valid(info, position_end, true);
  DASSERT_TRUE(position_start <= position_end);

  if(UNLIKELY(info == nullptr)) {
    DASSERT_TRUE(position_start == nullptr);
    DASSERT_TRUE(position_end == nullptr);
    assign(info, n, val);
    return;
  }

  if(UNLIKELY(position_start == position_end)) {
    insert(info, position_start, n, val);
    return;
  }

  size_t idx_start = std::distance((const T*)(info->data), position_start);
  size_t idx_end = std::distance((const T*)(info->data), position_end);

  _make_uninitialized_section(info, idx_start, idx_end, ptrdiff_t(n));

  std::uninitialized_fill(info->data + idx_start, info->data + idx_start + n, val);
}

////////////////////////////////////////////////////////////

template <typename T>
void clear(_vstruct<T>*& info) noexcept {
  if(size(info) == 0)
    return;

  _destroy_range(info->data, info->data + info->size);
  info->size = 0;
}



}}


#endif /* TURI_VECTOR_INTERNALS_H_ */
