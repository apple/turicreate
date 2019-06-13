/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_GL_STRING_H_
#define TURI_GL_STRING_H_

#include <core/generics/vector_internals.hpp>
#include <core/generics/string_internals.hpp>
#include <core/util/cityhash_tc.hpp>
#include <core/generics/is_memmovable.hpp>
#include <algorithm>

namespace turi {

class gl_string {
 public:
  typedef char value_type;

  typedef value_type& reference;
  typedef const value_type& const_reference;

  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  typedef value_type* iterator;
  typedef const value_type* const_iterator;

  typedef std::reverse_iterator<value_type*> reverse_iterator;
  typedef std::reverse_iterator<const value_type*> const_reverse_iterator;

  typedef ptrdiff_t difference_type;
  typedef size_t size_type;

  static constexpr size_t npos = gl_string_internal::npos;

 private:

  /// The structur holding all the information
  gl_vector_internal::_vstruct<value_type> *info;

 public:

  template <class InputIterator>
  gl_string (InputIterator first, InputIterator last, _VEC_ENABLE_IF_ITERATOR(InputIterator, value_type))
      : info(gl_vector_internal::construct<value_type, InputIterator>(first, last) )
  {}

  gl_string ()
      : info(nullptr)
  {}

  explicit gl_string (size_t n)
  : info(gl_vector_internal::construct<value_type>(n))
  {}

  gl_string (size_t n, const value_type& val)
      : info(gl_vector_internal::construct<value_type>(n, val))
  {}

  gl_string(const gl_string& v)
  : info(gl_vector_internal::construct<value_type>(v.begin(), v.end()))
  {}

  gl_string (gl_string&& x) noexcept {
    if(&x != this) {
      info = x.info;
      x.info = nullptr;
    }
  }

  explicit gl_string (const std::string& v)
      : info(gl_vector_internal::construct<value_type>(v.begin(), v.end()))
  {}

  gl_string (const char* d)
  : info(gl_vector_internal::construct<value_type>(d, d + strlen(d)))
  {}

  gl_string (const char* d, size_t n)
      : info(gl_vector_internal::construct<value_type>(d, d + n))
  {}

  gl_string (std::initializer_list<value_type> il)
      : info(gl_vector_internal::construct<value_type>(il.begin(), il.end()) )
  {}

  template <typename T1>
  gl_string (std::initializer_list<T1> il, _VEC_ENABLE_IF_CONVERTABLE(T1, value_type))
      : info(gl_vector_internal::construct<value_type>(il.begin(), il.end()) )
  {}

  gl_string(const gl_string& str, size_t pos, size_t n = npos) {
    info = gl_vector_internal::construct<value_type>(str._iter_at(pos), str._iter_at(pos, n));
  }

  ~gl_string() {
    if(info != nullptr) {
      gl_vector_internal::destroy(info);
      info = nullptr;
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Properties and Sizing

  size_type size() const noexcept { return gl_vector_internal::size(info); }
  size_type length() const noexcept { return size(); }

  size_type max_size() const noexcept { return std::numeric_limits<size_t>::max() / sizeof(value_type); }

  inline void resize (size_type n) {
    if(gl_vector_internal::size(info) != n)
      gl_vector_internal::resize(info, n);
    DASSERT_EQ(gl_vector_internal::size(info), n);
  }

  void resize (size_type n, const value_type& val) {
    gl_vector_internal::resize(info, n, val);
  }

  size_type capacity() const noexcept { return gl_vector_internal::capacity(info); }

  bool empty() const noexcept {
    return size() == 0;
  }

  void reserve (size_type n) {
    gl_vector_internal::reserve(info, n);
  }

  void shrink_to_fit() {
    if(gl_vector_internal::has_excess_storage(info)) {
      this->swap(gl_string(this->begin(), this->end()));
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Element access

  reference operator[] (size_type idx) _VEC_NDEBUG_NOEXCEPT {
    return gl_vector_internal::get_element(info, idx);
  }

  const_reference operator[] (size_type idx) const _VEC_NDEBUG_NOEXCEPT {
    return gl_vector_internal::get_element(info, idx);
  }

  reference at (size_type idx) _VEC_NDEBUG_NOEXCEPT {
    return gl_vector_internal::get_element(info, idx);
  }

  const_reference at (size_type idx) const _VEC_NDEBUG_NOEXCEPT {
    return gl_vector_internal::get_element(info, idx);
  }

  reference front() _VEC_NDEBUG_NOEXCEPT {
    DASSERT_NE(gl_vector_internal::size(info), 0);
    return gl_vector_internal::get_element(info, 0);
  }

  const_reference front() const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_NE(gl_vector_internal::size(info), 0);
    return gl_vector_internal::get_element(info, 0);
  }

  reference back() _VEC_NDEBUG_NOEXCEPT {
    DASSERT_NE(gl_vector_internal::size(info), 0);
    return gl_vector_internal::get_element(info, info->size - 1);
  }

  const_reference back() const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_NE(gl_vector_internal::size(info), 0);
    return gl_vector_internal::get_element(info, info->size - 1);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Assignment

  const gl_string& operator=(const gl_string& v) {
    if(&v != this)
      gl_vector_internal::assign(info, v.begin(), v.end());
    return *this;
  }

  const gl_string& operator=(gl_string&& v) {
    assign(std::move(v));
    return *this;
  }

  const gl_string& operator=(std::initializer_list<value_type> il) {
    gl_vector_internal::assign(info, il.begin(), il.end());
    return *this;
  }

  const gl_string& operator=(const std::string& v) {
    gl_vector_internal::assign(info, v.begin(), v.end());
    return *this;
  }

  const gl_string& operator=(const char* v) {
    gl_vector_internal::assign(info, v, v + std::strlen(v));
    return *this;
  }

  const gl_string& operator=(value_type c) {
    gl_vector_internal::assign(info, 1, c);
    return *this;
  }

  template <class InputIterator>
  void assign (const InputIterator& first, const InputIterator& last,
               _VEC_ENABLE_IF_ITERATOR(InputIterator, value_type)) {
    gl_vector_internal::assign(info, first, last);
  }

  void assign(size_t n, const value_type& val) {
    gl_vector_internal::assign(info, n, val);
  }

  void assign(std::initializer_list<value_type> il) {
    gl_vector_internal::assign(info, il);
  }

  gl_string& assign(const gl_string& str) {
    gl_vector_internal::assign(info, str.begin(), str.end());
    return *this;
  }

  gl_string& assign(gl_string&& str) {
    gl_vector_internal::assign_move(info, str.info);
    return *this;
  }

  gl_string& assign(const gl_string& str, size_t pos, size_t n = npos) {
    gl_vector_internal::assign<value_type>(info, str._iter_at(pos), str._iter_at(pos, n));
    return *this;
  }

  gl_string& assign(const value_type* s) {
    gl_vector_internal::assign<value_type>(info, s, s + std::strlen(s));
    return *this;
  }

  gl_string& assign(const value_type* s, size_t n) {
    gl_vector_internal::assign<value_type>(info, s, s + n);
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Insertion

  void push_back (const value_type& val) {
    gl_vector_internal::push_back(info, val);
  }

  void push_back (value_type&& val) {
    gl_vector_internal::push_back(info, std::forward<value_type>(val));
  }


  template <class... Args>
  iterator emplace (const_iterator position, Args&&... args) {
    return gl_vector_internal::emplace(info, position, args...);
  }

  template <class... Args>
  void emplace_back (Args&&... args) {
    gl_vector_internal::emplace_back(info, args...);
  }

  iterator insert (const_iterator position, size_t n, const value_type& val) {
    return gl_vector_internal::insert(info, position, n, val);
  }

  template <typename U>
  iterator insert (const_iterator position, U&& val) {
    return gl_vector_internal::insert(info, position, std::forward<U>(val));
  }

  template <class InputIterator>
  iterator insert (const_iterator position, InputIterator first, InputIterator last,
                   _VEC_ENABLE_IF_ITERATOR(InputIterator, value_type) ) {
    DASSERT_GE(std::distance(first, last), 0);
    return gl_vector_internal::insert(info, position, first, last);
  }

  iterator insert (const_iterator position, std::initializer_list<value_type> il) {
    return gl_vector_internal::insert(info, position, il.begin(), il.end());
  }

  gl_string& insert(size_t pos1, const gl_string& str) {
    gl_vector_internal::insert(info, _iter_at(pos1), str.begin(), str.end());
    return *this;
  }

  gl_string& insert(size_t pos1, const gl_string& str,
                    size_t pos2, size_t n=npos) {
    gl_vector_internal::insert(info, _iter_at(pos1), str._iter_at(pos2), str._iter_at(pos2, n));
    return *this;
  }

  gl_string& insert(size_t pos, const value_type* s, size_t n=npos) {
    gl_vector_internal::insert(info, _iter_at(pos), s, s + ((n == npos) ? strlen(s) : n));
    return *this;
  }

  gl_string& insert(size_t pos, size_t n, value_type c) {
    gl_vector_internal::insert(info, _iter_at(pos), n, c);
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Erasing

  void clear() _VEC_NDEBUG_NOEXCEPT {
    gl_vector_internal::clear(info);
  }

  void pop_back() {
    gl_vector_internal::pop_back(info);
  }

  iterator erase (const_iterator position) {
    return gl_vector_internal::erase(info, position);
  }

  iterator erase (const_iterator first, const_iterator last) {
    return gl_vector_internal::erase(info, first, last);
  }

  gl_string& erase(size_t pos = 0, size_t n = npos) {
    gl_vector_internal::erase(info, _iter_at(pos), _iter_at(pos, n));
    return *this;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Casting

  operator std::string() const {
    return std::string(cbegin(), cend());
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Data access / iterators

  const_iterator begin() const noexcept { return info == nullptr ? nullptr : info->data; }
  iterator begin() noexcept { return info == nullptr ? nullptr : info->data; }

  const_reverse_iterator rbegin() const noexcept {
    return std::reverse_iterator<const_iterator>(end());
  }

  reverse_iterator rbegin() noexcept {
    return std::reverse_iterator<iterator>(end());
  }

  const_iterator end() const noexcept { return (info == nullptr) ? nullptr : (info->data + info->size); }
  iterator end() noexcept { return (info == nullptr) ? nullptr : info->data + info->size; }

  reverse_iterator rend() noexcept { return std::reverse_iterator<iterator>(begin()); }
  const_reverse_iterator rend() const noexcept { return std::reverse_iterator<const_iterator>(begin()); }

  const_iterator cbegin() const noexcept { return begin(); }

  const_reverse_iterator crbegin() const noexcept { return rbegin(); }

  const_iterator cend() const noexcept { return end(); }

  const_reverse_iterator crend() const noexcept { return rend(); }

  value_type* data() noexcept { return info == nullptr ? nullptr : info->data; }

  const value_type* data() const noexcept { return info == nullptr ? nullptr : info->data; }


  ////////////////////////////////////////////////////////////////////////////////
  // Swaps

  void swap (gl_string& x) _VEC_NDEBUG_NOEXCEPT {
    std::swap(info, x.info);
  }

  void swap (gl_string&& x) _VEC_NDEBUG_NOEXCEPT {
    std::swap(info, x.info);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // String specific methods.
  ////////////////////////////////////////////////////////////////////////////////

  gl_string& append(const gl_string& str) {
    gl_vector_internal::insert(info, end(), str.begin(), str.end());
    return *this;
  }

  gl_string& append(const gl_string& str, size_t pos, size_t n = npos) {
    DASSERT_LE(pos, str.size());
    gl_vector_internal::insert(info, end(), str._iter_at(pos), str._iter_at(pos, n));
    return *this;
  }

  gl_string& append(const char* s, size_t n) {
    gl_vector_internal::insert(info, end(), s, s + n);
    return *this;
  }

  gl_string& append(const char* s) {
    gl_vector_internal::insert(info, end(), s, s + std::strlen(s));
    return *this;
  }

  gl_string& append(size_t n, value_type c) {
    gl_vector_internal::insert(info, end(), n, c);
    return *this;
  }

  template<class InputIterator>
  gl_string& append(InputIterator first, InputIterator last,
                    _VEC_ENABLE_IF_ITERATOR(InputIterator, value_type)) {
    gl_vector_internal::insert(info, end(), first, last);
    return *this;
  }

  gl_string& append(std::initializer_list<value_type> il) {
    gl_vector_internal::insert(info, end(), il.begin(), il.end());
    return *this;
  }

  gl_string& operator+=(const gl_string& str) {
    append(str);
    return *this;
  }

  gl_string& operator+=(const value_type* s) {
    append(s);
    return *this;
  }

  gl_string& operator+=(value_type c) {
    append(1, c);
    return *this;
  }

  gl_string& operator+=(std::initializer_list<value_type> il) {
    append(il);
    return *this;
  }

  gl_string& replace(size_t pos1, size_t n1, const gl_string& str) {
    gl_vector_internal::replace(info, _iter_at(pos1), _iter_at(pos1, n1), str.begin(), str.end());
    return *this;
  }

  gl_string& replace(size_t pos1, size_t n1, const gl_string& str,
                     size_t pos2, size_t n2=npos) {
    gl_vector_internal::replace(info,
                                _iter_at(pos1), _iter_at(pos1, n1),
                                str._iter_at(pos2), str._iter_at(pos2, n2));
    return *this;
  }

  gl_string& replace(size_t pos, size_t n1, const value_type* s, size_t n2) {
    gl_vector_internal::replace(info, _iter_at(pos), _iter_at(pos, n1), s, s + n2);
    return *this;
  }

  gl_string& replace(size_t pos, size_t n1, const value_type* s) {
    gl_vector_internal::replace(info, _iter_at(pos), _iter_at(pos, n1), s, s + std::strlen(s));
    return *this;
  }

  gl_string& replace(size_t pos, size_t n1, size_t n2, value_type c) {
    gl_vector_internal::replace(info, _iter_at(pos), _iter_at(pos, n1), n2, c);
    return *this;
  }

  gl_string& replace(const_iterator i1, const_iterator i2, const gl_string& str) {
    gl_vector_internal::replace(info, i1, i2, str.begin(), str.end());
    return *this;
  }

  gl_string& replace(const_iterator i1, const_iterator i2, const value_type* s, size_t n) {
    gl_vector_internal::replace(info, i1, i2, s, s + n);
    return *this;
  }

  gl_string& replace(const_iterator i1, const_iterator i2, const value_type* s) {
    gl_vector_internal::replace(info, i1, i2, s, s + strlen(s));
    return *this;
  }

  gl_string& replace(const_iterator i1, const_iterator i2, size_t n, value_type c) {
    gl_vector_internal::replace(info, i1, i2, n, c);
    return *this;
  }

  template<class InputIterator>
  gl_string& replace(const_iterator i1, const_iterator i2, InputIterator j1, InputIterator j2) {
    gl_vector_internal::replace(info, i1, i2, j1, j2);
    return *this;
  }

  gl_string& replace(const_iterator i1, const_iterator i2, std::initializer_list<value_type> il) {
    gl_vector_internal::replace(info, i1, i2, il.begin(), il.end());
    return *this;
  }

  size_t copy(value_type* s, size_t n, size_t pos = 0) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_LE(pos, size());
    size_t sz = size();
    size_t _end = std::min(sz, n + pos);
    std::copy(cbegin() + pos, cbegin() + _end, s);
    return _end - pos;
  }

  gl_string substr(size_t pos = 0, size_t n = npos) const {
    return gl_string(_iter_at(pos), _iter_at(pos, n));
  }

  size_t find(const gl_string& str, size_t pos = 0) const _VEC_NDEBUG_NOEXCEPT {
    return gl_string_internal::str_find(cbegin(), size(), str.cbegin(), pos, str.size());
  }

  size_t find(const value_type* s, size_t pos, size_t n) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(n == 0 || s != nullptr);
    return gl_string_internal::str_find(cbegin(), size(), s, pos, n);
  }

  size_t find(const value_type* s, size_t pos = 0) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(s != nullptr);
    return gl_string_internal::str_find(cbegin(), size(), s, pos, std::strlen(s));
  }

  size_t find(value_type c, size_t pos = 0) const _VEC_NDEBUG_NOEXCEPT {
    return gl_string_internal::str_find(cbegin(), size(), c, pos);
  }

  size_t rfind(const gl_string& str, size_t pos = npos) const _VEC_NDEBUG_NOEXCEPT {
    return gl_string_internal::str_rfind(cbegin(), size(), str.cbegin(), pos, str.size());
  }

  size_t rfind(const value_type* s, size_t pos, size_t n) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(n == 0 || s != nullptr);
    return gl_string_internal::str_rfind(cbegin(), size(), s, pos, n);
  }

  size_t rfind(const value_type* s, size_t pos = npos) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(s != nullptr);
    return gl_string_internal::str_rfind(cbegin(), size(), s, pos, std::strlen(s));
  }

  size_t rfind(value_type c, size_t pos = npos) const _VEC_NDEBUG_NOEXCEPT {
    return gl_string_internal::str_rfind(cbegin(), size(), c, pos);
  }

  size_t find_first_of(const gl_string& str, size_t pos = 0) const _VEC_NDEBUG_NOEXCEPT {
    return gl_string_internal::str_find_first_of(cbegin(), size(), str.cbegin(), pos, str.size());
  }

  size_t find_first_of(const value_type* s, size_t pos, size_t n) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(n == 0 || s != nullptr);
    return gl_string_internal::str_find_first_of(cbegin(), size(), s, pos, n);
  }

  size_t find_first_of(const value_type* s, size_t pos = 0) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(s != nullptr);
    return gl_string_internal::str_find_first_of(cbegin(), size(), s, pos, std::strlen(s));
  }

  size_t find_first_of(value_type c, size_t pos = 0) const _VEC_NDEBUG_NOEXCEPT {
    return find(c, pos);
  }

  size_t find_last_of(const gl_string& str, size_t pos = npos) const _VEC_NDEBUG_NOEXCEPT {
    return gl_string_internal::str_find_last_of(cbegin(), size(), str.cbegin(), pos, str.size());
  }

  size_t find_last_of(const value_type* s, size_t pos, size_t n) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(n == 0 || s != nullptr);
    return gl_string_internal::str_find_last_of(cbegin(), size(), s, pos, n);
  }

  size_t find_last_of(const value_type* s, size_t pos = npos) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(s != nullptr);
    return gl_string_internal::str_find_last_of(cbegin(), size(), s, pos, std::strlen(s));
  }

  size_t find_last_of(value_type c, size_t pos = npos) const _VEC_NDEBUG_NOEXCEPT {
    return rfind(c, pos);
  }

  size_t find_first_not_of(const gl_string& str, size_t pos = 0) const _VEC_NDEBUG_NOEXCEPT {
    return gl_string_internal::str_find_first_not_of(cbegin(), size(), str.cbegin(), pos, str.size());
  }

  size_t find_first_not_of(const value_type* s, size_t pos, size_t n) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(n == 0 || s != nullptr);
    return gl_string_internal::str_find_first_not_of(cbegin(), size(), s, pos, n);
  }

  size_t find_first_not_of(const value_type* s, size_t pos = 0) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(s != nullptr);
    return gl_string_internal::str_find_first_not_of(cbegin(), size(), s, pos, std::strlen(s));
  }

  size_t find_first_not_of(value_type c, size_t pos = 0) const _VEC_NDEBUG_NOEXCEPT {
    return gl_string_internal::str_find_first_not_of(cbegin(), size(), c, pos);
  }

  size_t find_last_not_of(const gl_string& str, size_t pos = npos) const _VEC_NDEBUG_NOEXCEPT {
    return gl_string_internal::str_find_last_not_of(cbegin(), size(), str.cbegin(), pos, str.size());
  }

  size_t find_last_not_of(const value_type* s, size_t pos, size_t n) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(n == 0 || s != nullptr);
    return gl_string_internal::str_find_last_not_of(cbegin(), size(), s, pos, n);
  }

  size_t find_last_not_of(const value_type* s, size_t pos = npos) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(s != nullptr);
    return gl_string_internal::str_find_last_not_of(cbegin(), size(), s, pos, std::strlen(s));
  }

  size_t find_last_not_of(value_type c, size_t pos = npos) const _VEC_NDEBUG_NOEXCEPT {
    return gl_string_internal::str_find_last_not_of(cbegin(), size(), c, pos);
  }

  int compare(const gl_string& str) const _VEC_NDEBUG_NOEXCEPT {
    size_t lhs_sz = size();
    size_t rhs_sz = str.size();
    int result = gl_string_internal::compare(data(), str.data(), std::min(lhs_sz, rhs_sz));
    if (result != 0)     return result;
    if (lhs_sz < rhs_sz) return -1;
    if (lhs_sz > rhs_sz) return 1;
    return 0;
  }

  int compare(const std::string& str) const _VEC_NDEBUG_NOEXCEPT {
    size_t lhs_sz = size();
    size_t rhs_sz = str.size();
    int result = gl_string_internal::compare(data(), str.data(), std::min(lhs_sz, rhs_sz));
    if (result != 0)     return result;
    if (lhs_sz < rhs_sz) return -1;
    if (lhs_sz > rhs_sz) return 1;
    return 0;
  }

  int compare(size_t pos1, size_t n1, const gl_string& str) const _VEC_NDEBUG_NOEXCEPT {
    return compare(pos1, n1, str.cbegin(), str.size());
  }

  int compare(size_t pos1, size_t n1, const gl_string& str,
              size_t pos2, size_t n2=npos) const _VEC_NDEBUG_NOEXCEPT {

    size_type sz = str.size();
    DASSERT_LE(pos2, sz);
    return compare(pos1, n1, str.data() + pos2, std::min(n2, sz - pos2));
  }

  int compare(const value_type* s) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(s != nullptr);
    return compare(0, npos, s, std::strlen(s));
  }

  int compare(size_t pos1, size_t n1, const value_type* s) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(s != nullptr);
    return compare(pos1, n1, s, std::strlen(s));
  }

  int compare(size_t pos1, size_t n1, const value_type* s, size_t n2) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_TRUE(n2 == 0 || s != nullptr);
    size_type sz = size();
    DASSERT_LE(pos1, sz);
    DASSERT_FALSE(n2 == npos);
    size_type rlen = std::min(n1, sz - pos1);
    int r = gl_string_internal::compare(data() + pos1, s, std::min(rlen, n2));
    if (r == 0) {
      if (rlen < n2)  r = -1;
      else if (rlen > n2) r = 1;
    }
    return r;
  }

  ////////////////////////////////////////////////////////////////////////////////

 private:

  const_iterator _iter_at(size_t pos) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_LE(pos, size());
    return cbegin() + pos;
  }

  const_iterator _iter_at(size_t pos, size_t n) const _VEC_NDEBUG_NOEXCEPT {
    DASSERT_LE(pos, size());
    return n == npos ? cend() : cbegin() + std::min(size(), pos + n);
  }

  iterator _iter_at(size_t pos) _VEC_NDEBUG_NOEXCEPT {
    DASSERT_LE(pos, size());
    return begin() + pos;
  }

  iterator _iter_at(size_t pos, size_t n) _VEC_NDEBUG_NOEXCEPT {
    DASSERT_LE(pos, size());
    return n == npos ? end() : begin() + std::min(size(), pos + n);
  }
};

////////////////////////////////////////////////////////////////////////////////
// operator overloads

// operator==
GL_HOT_INLINE_FLATTEN static inline
bool operator==(const gl_string& lhs, const gl_string& rhs) noexcept {
  return (lhs.size() == rhs.size()
          && std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin()));
}


GL_HOT_INLINE_FLATTEN static inline
bool operator==(const char* lhs, const gl_string& rhs) noexcept {
  return (std::strlen(lhs) == rhs.size()
          && std::equal(rhs.cbegin(), rhs.cend(), lhs));
}

GL_HOT_INLINE_FLATTEN static inline
bool operator==(const gl_string& lhs, const char* rhs) noexcept {
  return (std::strlen(rhs) == lhs.size()
          && std::equal(lhs.cbegin(), lhs.cend(), rhs));
}

GL_HOT_INLINE_FLATTEN static inline
bool operator==(const gl_string& lhs, const std::string& rhs) noexcept {
  return (lhs.size() == rhs.size()
          && std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin()));
}

GL_HOT_INLINE_FLATTEN static inline
bool operator==(const std::string& lhs, const gl_string& rhs) noexcept {
  return (lhs.size() == rhs.size()
          && std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin()));
}


// operator!=
GL_HOT_INLINE_FLATTEN static inline
bool operator!=(const gl_string& lhs, const gl_string& rhs) noexcept {
  return !(lhs == rhs);
}


GL_HOT_INLINE_FLATTEN static inline
bool operator!=(const char* lhs, const gl_string& rhs) noexcept {
  return !(lhs == rhs);
}


GL_HOT_INLINE_FLATTEN static inline bool
operator!=(const gl_string& lhs, const char* rhs) noexcept {
  return !(lhs == rhs);
}

// operator!=
GL_HOT_INLINE_FLATTEN static inline
bool operator!=(const std::string& lhs, const gl_string& rhs) noexcept {
  return !(lhs == rhs);
}

// operator!=
GL_HOT_INLINE_FLATTEN static inline
bool operator!=(const gl_string& lhs, const std::string& rhs) noexcept {
  return !(lhs == rhs);
}


// operator<
GL_HOT_INLINE_FLATTEN static inline
bool operator< (const gl_string& lhs, const gl_string& rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

// operator<
GL_HOT_INLINE_FLATTEN static inline
bool operator< (const gl_string& lhs, const std::string& rhs) noexcept {
  return lhs.compare(rhs) < 0;
}

// operator<
GL_HOT_INLINE_FLATTEN static inline
bool operator< (const std::string& lhs, const gl_string& rhs) noexcept {
  return rhs.compare(lhs) > 0;
}

GL_HOT_INLINE_FLATTEN static inline
bool operator< (const gl_string& lhs, const char* rhs) noexcept {
  return lhs.compare(rhs) < 0;
}


GL_HOT_INLINE_FLATTEN static inline
bool operator< (const char* lhs, const gl_string& rhs) noexcept {
  return rhs.compare(lhs) > 0;
}

// operator>


GL_HOT_INLINE_FLATTEN static inline
bool operator> (const gl_string& lhs, const gl_string& rhs) noexcept {
  return rhs < lhs;
}


GL_HOT_INLINE_FLATTEN static inline
bool operator> (const gl_string& lhs, const std::string& rhs) noexcept {
  return rhs < lhs;
}


GL_HOT_INLINE_FLATTEN static inline
bool operator> (const std::string& lhs, const gl_string& rhs) noexcept {
  return rhs < lhs;
}

GL_HOT_INLINE_FLATTEN static inline
bool operator> (const gl_string& lhs, const char* rhs) noexcept {
  return rhs < lhs;
}


GL_HOT_INLINE_FLATTEN static inline
bool operator> (const char* lhs, const gl_string& rhs) noexcept {
  return rhs < lhs;
}

// operator<=


GL_HOT_INLINE_FLATTEN static inline
bool operator<=(const gl_string& lhs, const gl_string& rhs) noexcept {
  return !(rhs < lhs);
}

GL_HOT_INLINE_FLATTEN static inline
bool operator<=(const gl_string& lhs, const std::string& rhs) noexcept {
  return !(rhs < lhs);
}

GL_HOT_INLINE_FLATTEN static inline
bool operator<=(const std::string& lhs, const gl_string& rhs) noexcept {
  return !(rhs < lhs);
}


GL_HOT_INLINE_FLATTEN static inline
bool operator<=(const gl_string& lhs, const char* rhs) noexcept {
  return !(rhs < lhs);
}


GL_HOT_INLINE_FLATTEN static inline
bool operator<=(const char* lhs, const gl_string& rhs) noexcept {
  return !(rhs < lhs);
}

// operator>=
GL_HOT_INLINE_FLATTEN static inline
bool operator>=(const gl_string& lhs, const gl_string& rhs) noexcept {
  return !(lhs < rhs);
}


GL_HOT_INLINE_FLATTEN static inline
bool operator>=(const gl_string& lhs, const std::string& rhs) noexcept {
  return !(lhs < rhs);
}


GL_HOT_INLINE_FLATTEN static inline
bool operator>=(const std::string& lhs, const gl_string& rhs) noexcept {
  return !(lhs < rhs);
}

GL_HOT_INLINE_FLATTEN static inline
bool operator>=(const gl_string& lhs, const char* rhs) noexcept {
  return !(lhs < rhs);
}


GL_HOT_INLINE_FLATTEN static inline
bool operator>=(const char* lhs, const gl_string& rhs) noexcept {
  return !(lhs < rhs);
}

// operator +
GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(const gl_string& lhs, const gl_string& rhs) {
  size_t lhs_sz = lhs.size();
  size_t rhs_sz = rhs.size();
  gl_string r;
  r.reserve(lhs_sz + rhs_sz);
  r.assign(lhs.data(), lhs_sz);
  r.append(rhs.data(), rhs_sz);
  return r;
}

// operator +
GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(const gl_string& lhs, const std::string& rhs) {
  size_t lhs_sz = lhs.size();
  size_t rhs_sz = rhs.size();
  gl_string r;
  r.reserve(lhs_sz + rhs_sz);
  r.assign(lhs.data(), lhs_sz);
  r.append(rhs.data(), rhs_sz);
  return r;
}

// operator +
GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(const std::string& lhs, const gl_string& rhs) {
  size_t lhs_sz = lhs.size();
  size_t rhs_sz = rhs.size();
  gl_string r;
  r.reserve(lhs_sz + rhs_sz);
  r.assign(lhs.data(), lhs_sz);
  r.append(rhs.data(), rhs_sz);
  return r;
}


GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(const char* lhs , const gl_string& rhs) {
  size_t lhs_sz = std::strlen(lhs);
  size_t rhs_sz = rhs.size();
  gl_string r;
  r.reserve(lhs_sz + rhs_sz);
  r.assign(lhs, lhs + lhs_sz);
  r.append(rhs.data(), rhs_sz);
  return r;
}


GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(char lhs, const gl_string& rhs) {
  size_t rhs_sz = rhs.size();
  gl_string r;
  r.reserve(1 + rhs_sz);
  r.assign(1, lhs);
  r.append(rhs.data(), rhs_sz);
  return r;
}


GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(const gl_string& lhs, const char* rhs) {
  size_t lhs_sz = lhs.size();
  size_t rhs_sz = std::strlen(rhs);
  gl_string r;
  r.reserve(lhs_sz + rhs_sz);
  r.assign(lhs.data(), lhs_sz);
  r.append(rhs, rhs_sz);
  return r;
}

GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(const gl_string& lhs, char rhs) {
  size_t lhs_sz = lhs.size();
  gl_string r;
  r.reserve(lhs_sz + 1);
  r.assign(lhs.data(), lhs_sz);
  r.push_back(rhs);
  return r;
}

GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(gl_string&& lhs, const gl_string& rhs) {
  return std::move(lhs.append(rhs));
}


GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(const gl_string& lhs, gl_string&& rhs) {
  return std::move(rhs.insert(0, lhs));
}


GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(gl_string&& lhs, gl_string&& rhs) {
  return std::move(lhs.append(rhs));
}


GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(const char* lhs , gl_string&& rhs) {
  return std::move(rhs.insert(0, lhs));
}


GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(char lhs, gl_string&& rhs) {
  rhs.insert(rhs.begin(), lhs);
  return std::move(rhs);
}

GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(gl_string&& lhs, const char* rhs) {
  return std::move(lhs.append(rhs));
}

GL_HOT_INLINE_FLATTEN static inline
gl_string operator+(gl_string&& lhs, char rhs) {
  lhs.push_back(rhs);
  return std::move(lhs);
}
} // End namespace turi

////////////////////////////////////////////////////////////////////////////////
// Conversion routines to gl_strings

#include <core/generics/string_conversion_internals.hpp>

namespace turi {

static inline gl_string to_gl_string(int val) {
  return gl_string_internal::as_string("%d", val);
}

static inline gl_string to_gl_string(unsigned val) {
  return gl_string_internal::as_string("%u", val);
}

static inline gl_string to_gl_string(long val)  {
  return gl_string_internal::as_string("%ld", val);
}

static inline gl_string to_gl_string(unsigned long val)  {
  return gl_string_internal::as_string("%lu", val);
}

static inline gl_string to_gl_string(long long val)  {
  return gl_string_internal::as_string("%lld", val);
}

static inline gl_string to_gl_string(unsigned long long val)  {
  return gl_string_internal::as_string("%llu", val);
}

static inline gl_string to_gl_string(float val)  {
  return gl_string_internal::as_string("%f", val);
}

static inline gl_string to_gl_string(double val)  {
  return gl_string_internal::as_string("%f", val);
}

static inline gl_string to_gl_string(long double val)  {
  return gl_string_internal::as_string("%Lf", val);
}

////////////////////////////////////////////////////////////////////////////////
}

#include <core/generics/string_stream_internals.hpp>

namespace turi {

template<class _Traits>
static inline
std::basic_ostream<char, _Traits>& operator<<(std::basic_ostream<char, _Traits>& os, const gl_string& s) {
  for(auto it = s.begin(); it != s.end(); ++it) {
    os << *it;
  }
  return os;
}

template<class _Traits>
std::basic_istream<char, _Traits>&
operator>>(std::basic_istream<char, _Traits>& is, gl_string& str) {
  return gl_string_internals::stream_in(is, str);
}

}

////////////////////////////////////////////////////////////////////////////////
// Overloads of operators for standard std string operations

namespace std {

static inline
std::istream& getline(std::istream& is, turi::gl_string& str, char dlm='\n') {
  return turi::gl_string_internals::getline(is, str, dlm);
}

static inline
std::istream& getline(std::istream&& is, turi::gl_string& str, char dlm='\n') {
  return turi::gl_string_internals::getline(is, str, dlm);
}

static inline int stoi(const turi::gl_string& str, size_t* idx = 0, int base = 10) {
  return std::stoi(std::string(str.begin(), str.end()), idx, base);
}

static inline long stol(const turi::gl_string& str, size_t* idx = 0, int base = 10) {
  return std::stol(std::string(str.begin(), str.end()), idx, base);
}

static inline unsigned long stoul (const turi::gl_string& str, size_t* idx = 0, int base = 10) {
  return std::stoul(std::string(str.begin(), str.end()), idx, base);
}

static inline unsigned long long stoull(const turi::gl_string& str, size_t* idx = 0, int base = 10) {
  return std::stoull(std::string(str.begin(), str.end()), idx, base);
}

static inline float stof (const turi::gl_string& str, size_t* idx = 0) {
  return std::stoll(std::string(str.begin(), str.end()), idx);
}

static inline double stod (const turi::gl_string& str, size_t* idx = 0) {
  return std::stod(std::string(str.begin(), str.end()), idx);
}

static inline long double stold(const turi::gl_string& str, size_t* idx = 0) {
  return std::stold(std::string(str.begin(), str.end()), idx);
}

static inline void swap (turi::gl_string& a, turi::gl_string& b) noexcept {
  a.swap(b);
}

template <> struct hash<turi::gl_string>
    : public unary_function<turi::gl_string, size_t> {

  size_t operator()(const turi::gl_string& s) const {
    return turi::hash64(s.data(), s.size());
  }
};

}

// is memmovable
namespace turi {

template <>
struct is_memmovable<gl_string> {
  static constexpr bool value = true;
};

}

#include <core/generics/string_serialization.hpp>

#endif
