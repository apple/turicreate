/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UTIL_HOPSCOTCH_SET_HPP
#define TURI_UTIL_HOPSCOTCH_SET_HPP

#include <core/generics/hopscotch_table.hpp>

#include <core/storage/serialization/serialization_includes.hpp>


#define _HOPSCOTCH_SET_DEFAULT_HASH std::hash<Key>



namespace turi {



  /**
   * A hopscotch hash set. More or less similar
   * interface as boost::unordered_set, not necessarily
   * entirely STL compliant.
   * Really should only be used to store small keys and trivial values.
   *
   * \tparam Key The key of the set
   * \tparam Hash The hash functor type. Defaults to std::hash<Key> if C++11 is
   *              available. Otherwise defaults to boost::hash<Key>
   * \tparam KeyEqual The functor used to identify object equality. Defaults to
   *                  std::equal_to<Key>
   */
  template <typename Key,
            typename Hash = _HOPSCOTCH_SET_DEFAULT_HASH,
            typename KeyEqual = std::equal_to<Key> >
  class hopscotch_set {

  public:
    // public typedefs
    typedef Key                                      value_type;
    typedef size_t                                   size_type;
    typedef Hash                                     hasher;
    typedef KeyEqual equality_function;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef const value_type* const_pointer;
    typedef const value_type& const_reference;


    typedef Key                    storage_type;

    typedef hopscotch_table<storage_type,
                            Hash,
                            KeyEqual> container_type;

    typedef typename container_type::iterator iterator;
    typedef typename container_type::const_iterator const_iterator;

  private:


    // The primary storage. Used by all sequential accessors.
    container_type* container;

    // the hash function to use. hashes a pair<key, value> to hash(key)
    hasher hashfun;

    // the equality function to use. Tests equality on only the first
    // element of the pair
    equality_function equalfun;

    container_type* create_new_container(size_t size) {
      return new container_type(size, hashfun, equalfun);
    }

    void destroy_all() {
      delete container;
      container = NULL;
    }

    // rehashes the hash table to one which is double the size
    container_type* rehash_to_new_container(size_t newsize = (size_t)(-1)) {
      /*
         std::cerr << "Rehash at " << container->size() << "/"
         << container->capacity() << ": "
         << container->load_factor() << std::endl;
       */
      // rehash
      if (newsize == (size_t)(-1)) newsize = container->size() * 2;
      container_type* newcontainer = create_new_container(newsize);
      const_iterator citer = begin();
      while (citer != end()) {
        DASSERT_TRUE(newcontainer->insert(*citer) != newcontainer->end());
        ++citer;
      }
      return newcontainer;
    }

    // Inserts a value into the hash table. This does not check
    // if the key already exists, and may produce duplicate values.
    iterator do_insert(const value_type &v) {
      iterator iter = container->insert(v);

      if (iter != container->end()) {
          return iter;
      }
      else {
        container_type* newcontainer = rehash_to_new_container();
        iter = newcontainer->insert(v);
        DASSERT_TRUE(iter != newcontainer->end());
        std::swap(container, newcontainer);
        delete newcontainer;
        return iter;
      }
    }

  public:

    hopscotch_set(size_t initialsize = 32,
                  Hash hashfun = Hash(),
                  KeyEqual equalfun = KeyEqual()):
                            container(NULL),
                            hashfun(hashfun), equalfun(equalfun) {
      container = create_new_container(initialsize);
    }

    hopscotch_set(const hopscotch_set& h):
                            hashfun(h.hashfun), equalfun(h.equalfun) {
      container = create_new_container(h.capacity());
      (*container) = *(h.container);
    }


    // only increases
    void rehash(size_t s) {
      if (s > capacity()) {
        container_type* newcontainer = rehash_to_new_container(s);
        std::swap(container, newcontainer);
        delete newcontainer;
      }
    }

    ~hopscotch_set() {
      destroy_all();
    }

    hasher hash_function() const {
      return hashfun;
    }

    KeyEqual key_eq() const {
      return equalfun;
    }

    hopscotch_set& operator=(const hopscotch_set& other) {
      (*container) = *(other.container);
      hashfun = other.hashfun;
      equalfun = other.equalfun;
      return *this;
    }

    size_type size() const {
      return container->size();
    }

    iterator begin() {
      return container->begin();
    }

    iterator end() {
      return container->end();
    }


    const_iterator begin() const {
      return container->begin();
    }

    const_iterator end() const {
      return container->end();
    }


    std::pair<iterator, bool> insert(const value_type& v) {
      iterator i = find(v);
      if (i != end()) return std::make_pair(i, false);
      else return std::make_pair(do_insert(v), true);
    }


    iterator insert(const_iterator hint, const value_type& v) {
      return insert(v).first;
    }

    iterator find(value_type const& v) {
      return container->find(v);
    }

    const_iterator find(value_type const& v) const {
      return container->find(v);
    }

    size_t count(value_type const& v) const {
      return container->count(v);
    }


    bool erase(iterator iter) {
      return container->erase(iter);
    }

    bool erase(value_type const& v) {
      return container->erase(v);
    }

    void swap(hopscotch_set& other) {
      std::swap(container, other.container);
      std::swap(hashfun, other.hashfun);
      std::swap(equalfun, other.equalfun);
    }

    void clear() {
      destroy_all();
      container = create_new_container(128);
    }


    size_t capacity() const {
      return container->capacity();
    }


    float load_factor() const {
      return container->load_factor();
    }

    void save(oarchive &oarc) const {
      oarc << size() << capacity();
      const_iterator iter = begin();
      while (iter != end()) {
        oarc << (*iter);
        ++iter;
      }
    }


    void load(iarchive &iarc) {
      size_t s, c;
      iarc >> s >> c;
      if (capacity() != c) {
        destroy_all();
        container = create_new_container(c);
      }
      else {
        container->clear();
      }
      for (size_t i = 0;i < s; ++i) {
        value_type v;
        iarc >> v;
        insert(v);
      }
    }
  };

}; // end of turi namespace

#endif
