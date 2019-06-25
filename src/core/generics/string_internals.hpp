/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_STRING_INTERNALS_H_
#define TURI_STRING_INTERNALS_H_

namespace turi { namespace gl_string_internal {

static constexpr size_t npos = size_t(-1);

GL_HOT_INLINE_FLATTEN
static inline
int _compare(const char* __s1, const char* __s2, size_t __n) noexcept {
  for (; __n; --__n, ++__s1, ++__s2) {
    if (*__s1 < *__s2)
      return -1;
    if (*__s2 < *__s1)
      return 1;
  }
  return 0;
}

GL_HOT_INLINE_FLATTEN
static inline
const char* _find(const char* __s, size_t __n, const char& __a) noexcept {
  for (; __n; --__n) {
    if (*__s == __a)
      return __s;
    ++__s;
  }
  return nullptr;
}

GL_HOT_INLINE_FLATTEN
static inline
size_t str_find(const char *p, size_t sz, char c, size_t pos) noexcept {
  if (pos >= sz)
    return npos;
  const char* r = _find(p + pos, sz - pos, c);
  if (r == 0)
    return npos;
  return static_cast<size_t>(r - p);
}

GL_HOT_INLINE_FLATTEN
static inline
size_t str_find(const char *p, size_t sz, const char* s, size_t pos, size_t n) noexcept {
  if (pos > sz || sz - pos < n)
    return npos;
  if (n == 0)
    return pos;
  const char* r = std::search(p + pos, p + sz, s, s + n);
  if (r == p + sz)
    return npos;
  return static_cast<size_t>(r - p);
}

// str_rfind
GL_HOT_INLINE_FLATTEN
static inline size_t str_rfind(const char *p, size_t sz, char c, size_t pos) noexcept {
  if (sz < 1)
    return npos;
  if (pos < sz)
    ++pos;
  else
    pos = sz;

  for (const char* ps = p + pos; ps != p;) {
    if (*--ps == c)
      return static_cast<size_t>(ps - p);
  }

  return npos;
}

GL_HOT_INLINE_FLATTEN
static inline size_t str_rfind(
    const char *p, size_t sz, const char* s, size_t pos, size_t n) noexcept {

  pos = std::min(pos, sz);
  if (n < sz - pos)
    pos += n;
  else
    pos = sz;

  const char* r = std::find_end(p, p + pos, s, s + n);

  if (n > 0 && r == p + pos)
    return npos;

  return static_cast<size_t>(r - p);
}

// str_find_first_of
GL_HOT_INLINE_FLATTEN
static inline size_t str_find_first_of(
    const char *p, size_t sz, const char* s, size_t pos, size_t n) noexcept {

  if (pos >= sz || n == 0)
    return npos;
  const char* r = std::find_first_of(p + pos, p + sz, s, s + n);
  if (r == p + sz)
    return npos;
  return static_cast<size_t>(r - p);
}


// str_find_last_of
GL_HOT_INLINE_FLATTEN
static inline size_t str_find_last_of(
    const char *p, size_t sz, const char* s, size_t pos, size_t n) noexcept {

  if (n != 0) {
    if (pos < sz)
      ++pos;
    else
      pos = sz;
    for (const char* ps = p + pos; ps != p;) {
      const char* r = _find(s, n, *--ps);
      if (r)
        return static_cast<size_t>(ps - p);
    }
  }

  return npos;
}

// str_find_first_not_of
GL_HOT_INLINE_FLATTEN
static inline size_t str_find_first_not_of(
    const char *p, size_t sz, const char* s, size_t pos, size_t n) noexcept {
  if (pos < sz) {
    const char* pe = p + sz;
    for (const char* ps = p + pos; ps != pe; ++ps)
      if (_find(s, n, *ps) == 0)
        return static_cast<size_t>(ps - p);
  }
  return npos;
}


GL_HOT_INLINE_FLATTEN
static inline size_t str_find_first_not_of(
    const char *p, size_t sz, char c, size_t pos) noexcept {

  if (pos < sz) {
    const char* pe = p + sz;
    for (const char* ps = p + pos; ps != pe; ++ps)
      if(*ps != c)
        return static_cast<size_t>(ps - p);
  }
  return npos;
}

// str_find_last_not_of
GL_HOT_INLINE_FLATTEN
static inline size_t str_find_last_not_of(
    const char *p, size_t sz, const char* s, size_t pos, size_t n) noexcept {

  if (pos < sz)
    ++pos;
  else
    pos = sz;

  for (const char* ps = p + pos; ps != p;)
    if (_find(s, n, *--ps) == 0)
      return static_cast<size_t>(ps - p);
  return npos;
}

GL_HOT_INLINE_FLATTEN
static inline size_t str_find_last_not_of(
    const char *p, size_t sz, char c, size_t pos) noexcept {

  if (pos < sz)
    ++pos;
  else
    pos = sz;

  for (const char* ps = p + pos; ps != p;)
    if ((*--ps) != c)
      return static_cast<size_t>(ps - p);
  return npos;
}

GL_HOT_INLINE_FLATTEN
static inline int compare(const char* s1, const char* s2, size_t n) {
  for (; n; --n, ++s1, ++s2) {
    if (*s1 < *s2) return -1;
    if (*s2 < *s1) return 1;
  }
  return 0;
}

}}

#endif /* TURI_STRING_INTERNALS_H_ */
