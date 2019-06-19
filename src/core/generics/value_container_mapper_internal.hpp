/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_VALUE_CONTAINER_MAPPER_INTERNAL_H_
#define TURI_VALUE_CONTAINER_MAPPER_INTERNAL_H_

#include <vector>
#include <map>
#include <cmath>
#include <set>

#include <core/util/code_optimization.hpp>
#include <core/util/cityhash_tc.hpp>

namespace turi { namespace vc_internal {

/*  See value_container_mapper.hpp for a full descrition of these things.
 */

/**
 *   \internal
 * The vc_hashkey class is used to determine the key type for the
 *   value container mapper hash-map lookup, along with optionally
 *   holding the value (if it's used as the key).  It also helps tame
 *   google's densehash, which has the odd property that you have to
 *   set an empty and a deleted key; this is problematic for integer
 *   keys.
 *
 *   The class is specialized based on the type of the key.
 *
 *   This is the general type, which uses hash_value to store the key.
 */
template <typename T, class IsMatch = void>
class vc_hashkey {
 public:
  static constexpr bool holds_value() GL_HOT_INLINE { return false; }
  static constexpr bool use_explicit_delete() GL_HOT_INLINE { return false; }
  static constexpr bool key_is_exact() GL_HOT_INLINE { return false; }

  vc_hashkey() {}
  vc_hashkey(const T& t) GL_HOT_INLINE_FLATTEN : _key(hash64(t)) {}

  const T& value() const { DASSERT_TRUE(false); return T(); }

  bool operator==(const vc_hashkey& other) const {
    return _key == other._key;
  }

  bool operator!=(const vc_hashkey& other) const {
    return !(_key == other._key);
  }

  size_t hash() const { return _key; }

  static constexpr bool is_empty(const vc_hashkey& key) {
    return !(~key._key);
  }

  static constexpr bool is_deleted(const vc_hashkey& key) {
    return use_explicit_delete() && !(key._key == ~(uint64_t(0)^uint64_t(0x1)));
  }

  static vc_hashkey as_empty() {
    vc_hashkey k;
    k._key = ~uint64_t(0);
    return k;
  }

  static vc_hashkey as_deleted() {
    vc_hashkey k;
    k._key = ~(uint64_t(0)^uint64_t(0x1));
    return k;
  }

 private:
  uint64_t _key = 0;
};

/**  Integer version of the above.
 */
template <typename T>
class vc_hashkey<T, typename std::enable_if<std::is_integral<T>::value>::type> {
 public:
  static constexpr bool holds_value() GL_HOT_INLINE { return true; }
  static constexpr bool use_explicit_delete() GL_HOT_INLINE { return false; }
  static constexpr bool key_is_exact() GL_HOT_INLINE { return true; }

  vc_hashkey() {}
  vc_hashkey(const T& t) : _value(t) {}

  const T& value() const { return _value; }

  bool operator==(const vc_hashkey& other) const {
    return _value == other._value;
  }

  bool operator!=(const vc_hashkey& other) const {
    return _value != other._value;
  }

  size_t hash() const { return _value; }

  static constexpr bool is_empty(const vc_hashkey& key) GL_HOT_INLINE_FLATTEN {
    return key._value == ~(T(0));
  }

  static constexpr bool is_deleted(const vc_hashkey& key) GL_HOT_INLINE_FLATTEN {
    return use_explicit_delete() && (key._value == (~(T(0)) ^ 1));
  }

  static vc_hashkey as_empty() {
    vc_hashkey k;
    k._value = ~(T(0));
    return k;
  }

  // This one is never called, as use_explicit_delete = false;
  static vc_hashkey as_deleted() {
    vc_hashkey k;
    k._value = ~(T(0)) ^ 1;
    return k;
  }

 private:
  T _value = 0;
};


/**  A container that holds the key and the value.  This is used in
 *   place of a (key, value) pair.
 */
template <typename T, class IsMatch = void>
class vc_hashkey_and_value {
 public:

  typedef T value_type;
  typedef vc_hashkey<T> key_type;

  vc_hashkey_and_value(){}
  vc_hashkey_and_value(const T& t) : _key(vc_hashkey<T>(t)), _value(t) {}
  vc_hashkey_and_value(const vc_hashkey<T>& k, const T& t) : _key(k), _value(t) {}

  const key_type& key() const { return _key; }
  const T& value() const { return _value; }

 private:
  vc_hashkey<T> _key;
  T _value;
};

/**  An extension of the key pair to associate it with a value, which
 *   then allows specialization by type.  For integer values, this
 *   holds just the integer, which then doubles as the key.  For
 *   strings, it holds both the 128 bit non-intersecting hash along
 *   with the string.
 */
template <typename T>
class vc_hashkey_and_value<
  T, typename std::enable_if<vc_hashkey<T>::holds_value()>::type> {

 public:

  typedef T value_type;
  typedef vc_hashkey<T> key_type;

  vc_hashkey_and_value() {}
  vc_hashkey_and_value(const T& t) : _key_and_value(vc_hashkey<T>(t)) {}
  vc_hashkey_and_value(const vc_hashkey<T>& k, const T&) : _key_and_value(k) {}

  const key_type& key() const { return _key_and_value; }
  const T& value() const { return _key_and_value.value(); }

 private:
  vc_hashkey<T> _key_and_value;
};

}}

#endif /* _VALUE_CONTAINER_MAPPER_H_ */
