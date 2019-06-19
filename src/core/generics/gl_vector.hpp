/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_GL_VECTOR_H_
#define TURI_GL_VECTOR_H_

#include <core/generics/vector_internals.hpp>
#include <core/generics/vector_serialization.hpp>
#include <core/generics/is_memmovable.hpp>
#include <algorithm>

namespace turi {

template <typename _T>
class gl_vector {
  static_assert(is_memmovable<_T>::value, "gl_vector requires T to be memmovable");
 public:
  typedef _T value_type;

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

  // static constexpr _inplace_values_allowed() { return sizeof(value_type) <= (sizeof(void*) - 1); }
  // static constexpr _max_num_inplace_values() { return (sizeof(void*) - 1) / sizeof(value_type); }

 private:

  /// The structur holding all the information
  gl_vector_internal::_vstruct<value_type> *info;

 public:
  template <class InputIterator>
  gl_vector (InputIterator first, InputIterator last, _VEC_ENABLE_IF_ITERATOR(InputIterator, value_type))
      : info(gl_vector_internal::construct<value_type, InputIterator>(first, last) )
  {}

  gl_vector () noexcept
      : info(nullptr)
  {}

  explicit gl_vector (size_type n)
  : info(gl_vector_internal::construct<value_type>(n))
  {}

  gl_vector (size_type n, const value_type& val)
      : info(gl_vector_internal::construct<value_type>(n, val))
  {}

  gl_vector (gl_vector&& x) {
    if(&x != this) {
      info = x.info;
      x.info = nullptr;
    }
  }

  gl_vector (const gl_vector& v)
      : info(gl_vector_internal::construct<value_type>(v.begin(), v.end()))
  {}


  explicit gl_vector (const std::vector<value_type>& v)
      : info(gl_vector_internal::construct<value_type>(v.begin(), v.end()))
  {}

  gl_vector (std::initializer_list<value_type> il)
      : info(gl_vector_internal::construct<value_type>(il.begin(), il.end()) )
  {}

  template <typename T1>
  gl_vector (std::initializer_list<T1> il, _VEC_ENABLE_IF_CONVERTABLE(T1, value_type))
      : info(gl_vector_internal::construct<value_type>(il.begin(), il.end()) )
  {}

  ~gl_vector() {
    if(info != nullptr) {
      gl_vector_internal::destroy(info);
      info = nullptr;
    }
  }

  const gl_vector& operator=(const gl_vector& v) {
    assign(v);
    return *this;
  }

  const gl_vector& operator=(gl_vector&& v) {
    assign(std::move(v));
    return *this;
  }

  const gl_vector& operator=(std::initializer_list<value_type> il) {
    assign(il);
    return *this;
  }

  // Do this to resolve annoying conflict resolution issues
  template <typename T, typename = typename std::enable_if<std::is_same<T, value_type>::value>::type>
  const gl_vector& operator=(const std::vector<T>& v) {
    gl_vector_internal::assign(info, v.begin(), v.end());
    return *this;
  }

  operator std::vector<value_type>() const {
    return std::vector<value_type>(cbegin(), cend());
  }

  const_iterator begin() const noexcept { return gl_vector_internal::cbegin(info); }
  iterator begin() noexcept { return gl_vector_internal::begin(info); }

  const_reverse_iterator rbegin() const noexcept {
    return std::reverse_iterator<const_iterator>(end());
  }

  reverse_iterator rbegin() noexcept {
    return std::reverse_iterator<iterator>(end());
  }

  const_iterator end() const noexcept { return gl_vector_internal::cend(info); }
  iterator end() noexcept { return gl_vector_internal::end(info); }

  reverse_iterator rend() noexcept { return std::reverse_iterator<iterator>(begin()); }
  const_reverse_iterator rend() const noexcept { return std::reverse_iterator<const_iterator>(begin()); }

  const_iterator cbegin() const noexcept { return begin(); }

  const_reverse_iterator crbegin() const noexcept { return rbegin(); }

  const_iterator cend() const noexcept { return end(); }

  const_reverse_iterator crend() const noexcept { return rend(); }

  size_type size() const noexcept { return gl_vector_internal::size(info); }

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
      this->swap(gl_vector(this->begin(), this->end()));
    }
  }

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

  value_type* data() noexcept { return info == nullptr ? nullptr : info->data; }

  const value_type* data() const noexcept { return info == nullptr ? nullptr : info->data; }

  template <class InputIterator>
  void assign (const InputIterator& first, const InputIterator& last,
               _VEC_ENABLE_IF_ITERATOR(InputIterator, value_type)) {
    gl_vector_internal::assign(info, first, last);
  }

  void assign(size_type n, const value_type& val) {
    gl_vector_internal::assign(info, n, val);
  }

  void assign(std::initializer_list<value_type> il) {
    gl_vector_internal::assign(info, il);
  }

  void assign(const gl_vector& v) {
    if(v.info != info)
      gl_vector_internal::assign(info, v.begin(), v.end());
  }

  void assign(gl_vector&& v) {
    gl_vector_internal::assign_move(info, v.info);
  }

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

  void pop_back() {
    gl_vector_internal::pop_back(info);
  }

  iterator insert (const_iterator position, size_type n, const value_type& val) {
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

  iterator erase (const_iterator position) {
    return gl_vector_internal::erase(info, position);
  }

  iterator erase (const_iterator first, const_iterator last) {
    return gl_vector_internal::erase(info, first, last);
  }

  void swap (gl_vector& x) noexcept {
    std::swap(info, x.info);
  }

  void swap (gl_vector&& x) noexcept {
    std::swap(info, x.info);
  }

  void clear() noexcept {
    gl_vector_internal::clear(info);
  }

};

////////////////////////////////////////////////////////////////////////////////
// operator overloads

template <class T>
bool operator== (const gl_vector<T>& lhs, const gl_vector<T>& rhs) {
  return (lhs.size() == rhs.size()
          && std::equal(lhs.begin(), lhs.end(), rhs.begin()));
}

template <class T>
bool operator!= (const gl_vector<T>& lhs, const gl_vector<T>& rhs) {
  return !(lhs == rhs);
}


template <typename T>
struct is_memmovable<gl_vector<T>> {
  static constexpr bool value = is_memmovable<T>::value;
};

} // turi

namespace std {

template <class T>
inline void swap (turi::gl_vector<T>& a, turi::gl_vector<T>& b) noexcept {
  a.swap(b);
}

}


#endif /* TURI_GL_VECTOR_H_ */
