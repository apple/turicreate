/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_VALUE_CONTAINER_MAPPER_H_
#define TURI_VALUE_CONTAINER_MAPPER_H_

#include <vector>
#include <map>
#include <cmath>
#include <set>

#include <core/util/code_optimization.hpp>
#include <sparsehash/dense_hash_set>
#include <core/generics/value_container_mapper_internal.hpp>

namespace turi {

/**
 * \ingroup util
 * A fast, specialized container to track lookups from a Value to a
 *  container holding that value (plus other things).  Essentially,
 *  this is very optimized version of std::map<Value,
 *  ValueContainer*>, which adds the following assumptions on the API
 *  and the ValueContainer type in order to be really fast.
 *
 *  1. The hash, implemented using a custom hashkey class (see below),
 *     is tracked explicitly with the value, and it is up to the user
 *     to track this.  This permits caching this value and more
 *     efficient storage.
 *
 *  2. The ValueContainer class must hold a hashkey_and_value
 *     container (see below); this is essentially a (hashkey, value)
 *     pair but with certain other optimizations.  This must be
 *     accessible via a get_hashkey_and_value() method in the value
 *     container, which is then used by
 *
 *  3. Pointers to the ValueContainer are what is stored, and it is
 *     assumed that a Value -> ValueContainer* mapping is valid if and
 *     only if the value container holds the same value.  Otherwise
 *     the find() method returns a nullptr.  (The invalidate function
 *     below sticks to this assumption; it just tracks things for lazy
 *     cleanup).
 *
 *  For a usage example, see ml/sketches/space_saving.hpp.
 *
 *  The value_container_mapper::hashkey class is initializable by
 *  value:
 *
 *    struct hashkey {
 *       hashkey();
 *       hashkey(const Value& v);
 *
 *       // A bunch of internal methods...
 *    };
 *
 *  The value_container_mapper::hashkey_and_value is initializable
 *  either by value (in which case the hashkey is created from a hash
 *  of the value), or by hashkey and value pair.  It also implements
 *  key() and value() methods to return the hashkey() and value()
 *  respectively:
 *
 *    struct hashkey_and_value {
 *       hashkey_and_value();
 *       hashkey_and_value(const Value& v);
 *       hashkey_and_value(const hashkey& hk, const Value& v);
 *
 *       hashkey key() const;
 *       const Value& value() const;
 *
 *       // A bunch of internal methods...
 *    };
 */
template <typename Value, typename ValueContainer>
class value_container_mapper {
 public:

  // Define the equality comparison and hash function comparison
  // stuff.  This allows us to use an intersecting hash still and have
  // it work.

  typedef vc_internal::vc_hashkey<Value> hashkey;
  typedef vc_internal::vc_hashkey_and_value<Value> hashkey_and_value;

  /**  Reserves internal storage for n elements.
   */
  void reserve(size_t n) { _reserve(n); }

  /**  Returns the current size of the hash table.
   */
  inline size_t size() const {
    return _size();
  }

  /**  Clears the hash table.
   */
  inline void clear() {
    _clear();
  }

  /**  Inserts a lookup index into the hash mapping.
   */
  void insert(const hashkey_and_value& hv, ValueContainer* v_ptr) GL_HOT_INLINE_FLATTEN {
    DASSERT_TRUE(hv.value() == v_ptr->get_hashkey_and_value().value());
    DASSERT_TRUE(hv.key() == v_ptr->get_hashkey_and_value().key());
    _insert(hv.key(), v_ptr);
  }

  /**  Inserts a lookup index into the hash mapping.
   *   Overload that pulls the value from v_ptr.
   *   \overload
   */
  void insert(const hashkey& hk, ValueContainer* v_ptr) GL_HOT_INLINE_FLATTEN {
    _insert(hk, v_ptr);
  }

  /** Returns the container associated with this key and value.  If
   *  it's not present or has been invalidated, returns nullptr.
   */
  ValueContainer* find(const hashkey_and_value& hv) GL_HOT_INLINE_FLATTEN {
    return _find_reference<ValueContainer*, value_container_mapper>(this, hv.key(), hv.value());
  }

  /** Same as above, but avoids a potential copy operation of the
   *  value if it is not stored in a hashkey_and_value instance
   *  already.
   *  \overload
   */
  ValueContainer* find(const hashkey& key, const Value& t) GL_HOT_INLINE_FLATTEN {
    return _find_reference<ValueContainer*, value_container_mapper>(this, key, t);
  }

  /** Returns the container associated with this (key, value).  If
   *  it's not present, returns nullptr.  Const overload.
   *  \overload
   */
  const ValueContainer* find(const hashkey_and_value& hv) const GL_HOT_INLINE_FLATTEN {
    return _find_reference<const ValueContainer*, const value_container_mapper>(this, hv.key(), hv.value());
  }

  /** Same as above, but avoids a potential copy operation of the
   *  value if it is not stored in a hashkey_and_value instance
   *  already.  Const overload.
   *  \overload
   */
  const ValueContainer* find(const hashkey& key, const Value& t) const GL_HOT_INLINE_FLATTEN {
    return _find_reference<const ValueContainer*, const value_container_mapper>(this, key, t);
  }

  /**  Marks a particular value_container as invalid. It is assumed,
   *   however, that as long as ValueContainer* holds the value, it is
   *   a valid key; otherwise it is not.  This function does some lazy
   *   cleanup, but may not erase the key.
   */
  void invalidate(const hashkey_and_value& hv, ValueContainer* v_ptr) GL_HOT_INLINE_FLATTEN {
    DASSERT_TRUE(hv.value() == v_ptr->get_hashkey_and_value().value());
    DASSERT_TRUE(hv.key() == v_ptr->get_hashkey_and_value().key());
    _invalidate(hv.key(), v_ptr);
  }

  /**  Marks a particular value_container as invalid. It is assumed,
   *   however, that as long as ValueContainer* holds the value, it is
   *   a valid key; otherwise it is not.  This function does some lazy
   *   cleanup, but may not erase the key.
   *
   *   Overload that pulls the value from v_ptr.
   *   \overload
   */
  void invalidate(const hashkey& hk, ValueContainer* v_ptr) GL_HOT_INLINE_FLATTEN {
    _invalidate(hk, v_ptr);
  }

 private:
  struct internal_value_type {
    hashkey first;
    mutable ValueContainer* second;
  };

  struct gdhs_hash {
    size_t operator()(const internal_value_type& kv_pair) const GL_HOT_INLINE_FLATTEN {
      return kv_pair.first.hash();
    }
  };

  struct gdhs_equality {
    bool operator()(const internal_value_type& kv_pair_1, const internal_value_type& kv_pair_2) const GL_HOT_INLINE_FLATTEN {

      if(hashkey::key_is_exact()) {
        return kv_pair_1.first == kv_pair_2.first;

      } else {

        if(kv_pair_1.first != kv_pair_2.first)
          return false;

        if(hashkey::use_explicit_delete()) {

          if(kv_pair_1.second == nullptr && kv_pair_2.second == nullptr)
            return true;

          return (kv_pair_1.second != nullptr
                  && kv_pair_2.second != nullptr
                  && (kv_pair_1.second->get_hashkey_and_value().value()
                      == kv_pair_2.second->get_hashkey_and_value().value()));

        } else {

          if(kv_pair_1.second == nullptr && kv_pair_2.second == nullptr)
            return true;

          return (kv_pair_1.second != nullptr
                  && kv_pair_2.second != nullptr
                  && kv_pair_1.second->get_hashkey_and_value().key() == kv_pair_1.first
                  && kv_pair_2.second->get_hashkey_and_value().key() == kv_pair_2.first
                  && (kv_pair_1.second->get_hashkey_and_value().value()
                      == kv_pair_2.second->get_hashkey_and_value().value()));
        }
      }
    }
  };

  typedef google::dense_hash_set<internal_value_type, gdhs_hash, gdhs_equality> hash_map_type;
  friend hash_map_type;

 public:

  value_container_mapper()
      : _kv_empty_value(hashkey_and_value(hashkey::as_empty(), Value()))
      , _kv_empty({hashkey::as_empty(), nullptr})
      , _kv_deleted_value(hashkey_and_value(hashkey::as_deleted(), Value()))
      , _kv_deleted({hashkey::as_deleted(), nullptr})
  {
    table.set_empty_key(_kv_empty);

    if(hashkey::use_explicit_delete())
      table.set_deleted_key(_kv_deleted);

    // A few internal consistency tests
    DASSERT_TRUE(gdhs_equality()(_kv_deleted, _kv_deleted));
    DASSERT_TRUE(gdhs_equality()(_kv_empty, _kv_empty));
    DASSERT_FALSE(gdhs_equality()(_kv_empty, _kv_deleted));
    DASSERT_FALSE(gdhs_equality()(_kv_deleted, _kv_empty));
  }


 private:

  /**  Reserve the proper amount for the table.
   */
  void _reserve(size_t n) GL_HOT_INLINE_FLATTEN {
    if(hashkey::use_explicit_delete()) {
      reserved_size = n;
      table.resize( n );
    } else {
      reserved_size = 3*n;
      table.resize( 3*n );
    }

    erase_counter = 0;
  }

  /**  Marks a key, value_container as invalid. It is assumed, however, that as long as
   *   ValueContainer* holds the value, it is a valid key; otherwise
   *   it is not.  This function does some lazy cleanup, but may not
   *   erase the key.
   */
  void _invalidate(const hashkey& key, ValueContainer* v_ptr) GL_HOT_INLINE_FLATTEN {

    internal_value_type kv_pair{key, v_ptr};

    if(UNLIKELY(hashkey::is_empty(kv_pair.first) )) {
      _erase_in_stack(kv_pair, _kv_empty, _kv_empty_overflow_stack);
      return;
    }

    if(UNLIKELY(hashkey::is_deleted(kv_pair.first) )) {
      _erase_in_stack(kv_pair, _kv_deleted, _kv_deleted_overflow_stack);
      return;
    }

    if(hashkey::use_explicit_delete()) {

      table.erase(kv_pair);

    } else {

      ++erase_counter;

      if(UNLIKELY(erase_counter >= reserved_size)) {
        refresh_hash_table();
        erase_counter = 0;
      }
    }
  }

  /**  Insert a key, valuecontainer pair into the hash table.
   */
  void _insert(const hashkey& key, ValueContainer* v_ptr) GL_HOT_INLINE_FLATTEN {
    internal_value_type kv_pair{key, v_ptr};

    if(UNLIKELY(hashkey::is_empty(kv_pair.first) )) {
      _insert_in_stack(kv_pair, _kv_empty, _kv_empty_overflow_stack);
      return;
    }

    if(UNLIKELY(hashkey::is_deleted(kv_pair.first) )) {
      _insert_in_stack(kv_pair, _kv_deleted, _kv_deleted_overflow_stack);
      return;
    }

    auto ret = table.insert(kv_pair);

    if(!hashkey::use_explicit_delete()) {
      ret.first->second = kv_pair.second;
    }
  }


  /**  Returns the current size of the hash table.
   */
  inline size_t _size() const {
    return ( (_kv_empty.second == nullptr ? 0 : 1)
             + _kv_empty_overflow_stack.size()
             + (_kv_deleted.second == nullptr ? 0 : 1)
             + _kv_deleted_overflow_stack.size()
             + table.size());
  }

  /**  Clears the hash table.
   */
  inline void _clear() {
    _kv_empty.second = nullptr;
    _kv_empty_overflow_stack.clear();
    _kv_deleted.second = nullptr;
    _kv_deleted_overflow_stack.clear();
    table.clear();
  }

 private:

  size_t reserved_size;
  size_t erase_counter = 0;

  ValueContainer _kv_empty_value;
  internal_value_type _kv_empty;
  std::vector<internal_value_type> _kv_empty_overflow_stack;

  ValueContainer _kv_deleted_value;
  internal_value_type _kv_deleted;
  std::vector<internal_value_type> _kv_deleted_overflow_stack;

  hash_map_type table;

  void _erase_in_stack(const internal_value_type& kv_pair,
                       internal_value_type& kv_base,
                       std::vector<internal_value_type>& stack) GL_HOT_NOINLINE_FLATTEN {

    if(hashkey::key_is_exact()) {
      DASSERT_TRUE(stack.empty());
      kv_base.second = nullptr;
    } else {

      // Test for value explicitly.
      DASSERT_TRUE(kv_base.second != nullptr);
      if(kv_base.second->get_hashkey_and_value().value()
         == kv_pair.second->get_hashkey_and_value().value()) {

        // This is the one; pull in anything from the stack
        kv_base.second = nullptr;

        if(!stack.empty()) {
          kv_base = stack.back();
          stack.pop_back();
        }

      } else {

        // Drop it from the stack.
        DASSERT_FALSE(stack.empty());

        size_t found_idx = size_t(-1);
        for(size_t idx = 0; idx < stack.size(); ++idx) {
          if(stack[idx].second->get_hashkey_and_value().value()
             == kv_pair.second->get_hashkey_and_value().value()) {
            found_idx = idx;
            break;
          }
        }
        DASSERT_FALSE(found_idx == size_t(-1));

        std::swap(stack[found_idx], stack.back());
        stack.pop_back();
      }
    }
  }

  mutable ValueContainer v_temp;

  /**  Return a reference to the key-value pair associated with a
   *   particular key.  Returns one with a nullptr for the
   *   value-pointer if it is not present.
   */
  template <typename _ReferenceType, typename _ThisType>
  GL_HOT_INLINE_FLATTEN
      static _ReferenceType _find_reference(_ThisType* c, const hashkey& key, const Value& t) {

    if(hashkey::key_is_exact()) {

      if(UNLIKELY(hashkey::is_empty(key) )) {
        return c->_kv_empty.second;
      }

      if(UNLIKELY(hashkey::is_deleted(key) )) {
        return c->_kv_deleted.second;
      }

      c->v_temp = ValueContainer(hashkey_and_value(key, t));

      auto it = c->table.find(internal_value_type{key, &c->v_temp});

      return ( (it == c->table.end()
                || (!hashkey::use_explicit_delete() && it->first != it->second->get_hashkey_and_value().key()))
               ? _ReferenceType(nullptr)
               : (it->second) );

    } else {

      typedef typename std::conditional<std::is_const<_ThisType>::value,
                                        const internal_value_type&, internal_value_type&>::type iv_ref;

      typedef typename std::conditional<std::is_const<_ThisType>::value,
                                        const std::vector<internal_value_type>&,
                                        std::vector<internal_value_type>& >::type iv_vect_ref;

      // Set in one of the local stacks.
      auto find_in_stack = [&](
                iv_ref kv_base,
                iv_vect_ref stack) GL_GCC_ONLY(GL_HOT_NOINLINE_FLATTEN) {

        if(LIKELY(kv_base.second == nullptr)) {
          return _ReferenceType(nullptr);
        }

        if(kv_base.second->get_hashkey_and_value().value() == t) {
          return _ReferenceType(kv_base.second);
        }

        for(size_t idx = 0; idx < stack.size(); ++idx) {
          if(stack[idx].second->get_hashkey_and_value().value() == t)
            return _ReferenceType(stack[idx].second);
        }

        return _ReferenceType(nullptr);
      };

      if(UNLIKELY(hashkey::is_empty(key) )) {
        return find_in_stack(c->_kv_empty, c->_kv_empty_overflow_stack);
      }

      if(UNLIKELY(hashkey::is_deleted(key))) {
        return find_in_stack(c->_kv_deleted, c->_kv_deleted_overflow_stack);
      }

      c->v_temp = ValueContainer(hashkey_and_value(key, t));

      auto it = c->table.find(internal_value_type{key, &c->v_temp});

      return ( (it == c->table.end()
                || (!hashkey::use_explicit_delete() && it->first != it->second->get_hashkey_and_value().key()))
               ? _ReferenceType(nullptr)
               : (it->second) );
    }
  }

  // Set in one of the local stacks.
  void _insert_in_stack(
      const internal_value_type& v,
      internal_value_type& kv_base,
      std::vector<internal_value_type>& stack) GL_HOT_NOINLINE_FLATTEN {

    if(hashkey::key_is_exact()) {
      DASSERT_TRUE(kv_base.second == nullptr);
      kv_base.second = v.second;
    } else {
      if(LIKELY(kv_base.second == nullptr)) {
        DASSERT_TRUE(stack.empty());
        kv_base.second = v.second;
      } else {
        stack.push_back(v);
      }
    }
  }

  /** Refreshes the hash table, clearing out the erased elements.
   */
  void refresh_hash_table() GL_HOT_NOINLINE_FLATTEN {

    hash_map_type alt_table;
    alt_table.set_empty_key(_kv_empty);
    alt_table.resize(reserved_size);

    for(auto it = table.begin(); it != table.end(); ++it) {
      if(it->second->get_hashkey_and_value().key() == it->first) {
        alt_table.insert(*it);
      }
    }

    table.swap(alt_table);
  }
};

}

#endif /* _VALUE_CONTAINER_MAPPER_H_ */
