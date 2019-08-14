/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UTIL_HOPSCOTCH_HASH_HPP
#define TURI_UTIL_HOPSCOTCH_HASH_HPP

#include <core/generics/hopscotch_table.hpp>

#include <core/storage/serialization/serialization_includes.hpp>


#define _HOPSCOTCH_MAP_DEFAULT_HASH std::hash<Key>



namespace turi {



  /**
   * A hopscotch hash map. More or less similar
   * interface as boost::unordered_map, not necessarily
   * entirely STL compliant.
   * Really should only be used to store small keys and trivial values.
   *
   * \tparam Key The key of the map
   * \tparam Value The value to store for each key
   * \tparam Hash The hash functor type. Defaults to std::hash<Key> if C++11 is
   *              available. Otherwise defaults to boost::hash<Key>
   * \tparam KeyEqual The functor used to identify object equality. Defaults to
   *                  std::equal_to<Key>
   */
  template <typename Key,
            typename Value,
            typename Hash = _HOPSCOTCH_MAP_DEFAULT_HASH,
            typename KeyEqual = std::equal_to<Key> >
  class hopscotch_map {

  public:
    // public typedefs
    typedef Key                                      key_type;
    typedef std::pair<Key, Value>                    value_type;
    typedef Value                                    mapped_type;
    typedef size_t                                   size_type;
    typedef Hash                                     hasher;
    typedef KeyEqual equality_function;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef const value_type* const_pointer;
    typedef const value_type& const_reference;


    typedef std::pair<Key, Value>                    storage_type;

    struct hash_redirect{
      Hash hashfun;
      hash_redirect(Hash h): hashfun(h) { }
      size_t operator()(const storage_type& v) const {
        return hashfun(v.first);
      }
    };
    struct key_equal_redirect{
      KeyEqual keyeq;
      key_equal_redirect(KeyEqual k): keyeq(k) { }
      bool operator()(const storage_type& v, const storage_type& v2) const {
        return keyeq(v.first, v2.first);
      }
    };

    typedef hopscotch_table<storage_type,
                            hash_redirect,
                            key_equal_redirect> container_type;

    typedef boost::unordered_map<key_type, mapped_type, Hash> spill_type;

    struct const_iterator{
      typedef std::forward_iterator_tag iterator_category;
      typedef const typename hopscotch_map::value_type value_type;
      typedef size_t difference_type;
      typedef value_type* pointer;
      typedef value_type& reference;

      friend class hopscotch_map;

      const hopscotch_map* ptr;
      typename hopscotch_map::container_type::const_iterator iter;
      typename hopscotch_map::spill_type::const_iterator iter2;
      bool in_spill;

      const_iterator():ptr(NULL) {}


      const_iterator operator++() {
        if (!in_spill) {
          ++iter;
          if (iter == ptr->container->end()) {
            in_spill = true;
          }
        } else {
          ++iter2;
        }
        return *this;
      }

      const_iterator operator++(int) {
        iterator cur = *this;
        ++(*this);
        return cur;
      }


      reference operator*() {
        if (!in_spill) return (*iter);
        else return *reinterpret_cast<pointer>(&(*iter2));
      }

      pointer operator->() {
        if (!in_spill) return &(*iter);
        else return reinterpret_cast<pointer>(&(*iter2));
        // this is annoying. but it unfortunately has to be this way
      }

      bool operator==(const const_iterator it) const {
        return ptr == it.ptr &&
            ((!in_spill && iter == it.iter) ||
            (in_spill && iter2 == it.iter2));
      }

      bool operator!=(const const_iterator iter) const {
        return !((*this) == iter);
      }
    private:
      const_iterator(const hopscotch_map* map,
          typename container_type::const_iterator iter,
          typename spill_type::const_iterator iter2):
        ptr(map), iter(iter), iter2(iter2), in_spill(iter == ptr->container->end()) { }
    };

    struct iterator {
      typedef std::forward_iterator_tag iterator_category;
      typedef typename hopscotch_map::value_type value_type;
      typedef size_t difference_type;
      typedef value_type* pointer;
      typedef value_type& reference;

      friend class hopscotch_map;

      hopscotch_map* ptr;
      typename hopscotch_map::container_type::iterator iter;
      typename hopscotch_map::spill_type::iterator iter2;
      bool in_spill;

      iterator():ptr(NULL) {}


      operator const_iterator() const {
        const_iterator it(ptr, iter, iter2);
        return it;
      }

      iterator operator++() {
        if (!in_spill) {
          ++iter;
          // if I went past the end of the main array,
          // go to the spill array.
          if (iter == ptr->container->end()) {
            in_spill = true;
          }
        } else {
          ++iter2;
        }
        return *this;
      }

      iterator operator++(int) {
        iterator cur = *this;
        ++(*this);
        return cur;
      }


      reference operator*() {
        if (!in_spill) return (*iter);
        else return *reinterpret_cast<pointer>(&(*iter2));
      }

      pointer operator->() {
        if (!in_spill) return &(*iter);
        else return reinterpret_cast<pointer>(&(*iter2));
      }

      bool operator==(const iterator it) const {
        return ptr == it.ptr &&
            ((!in_spill && iter == it.iter) ||
            (in_spill && iter2 == it.iter2));
      }

      bool operator!=(const iterator iter) const {
        return !((*this) == iter);
      }
    private:
      iterator(hopscotch_map* map,
          typename container_type::iterator iter,
          typename spill_type::iterator iter2):
        ptr(map), iter(iter), iter2(iter2), in_spill(iter == ptr->container->end()) { }
    };



  private:


    // The primary storage. Used by all sequential accessors.
    container_type* container;

    // excess elements which refuse to be inserted go here.
    spill_type  spill;

    // the hash function to use. hashes a pair<key, value> to hash(key)
    hash_redirect hashfun;

    // the equality function to use. Tests equality on only the first
    // element of the pair
    key_equal_redirect equalfun;

    container_type* create_new_container(size_t size) {
      return new container_type(size, hashfun, equalfun);
    }

    void destroy_all() {
      delete container;
      spill.clear();
      container = NULL;
    }

    // rehashes the hash table to one which is double the size
    void rehash_to_new_container(size_t newsize = (size_t)(-1)) {
      /*
         std::cerr << "Rehash at " << container->size() << "/"
         << container->capacity() << ": "
         << container->load_factor() << std::endl;
       */
      // rehash
      if (newsize == (size_t)(-1)) newsize = size() * 2;
      container_type* newcontainer = create_new_container(newsize);
      const_iterator citer = begin();
      spill_type newspill;
      while (citer != end()) {
        if(newcontainer->insert(*citer) == newcontainer->end()) {
          newspill.insert(*citer);
        }
        ++citer;
      }
      std::swap(container, newcontainer);
      std::swap(spill, newspill);
      delete newcontainer;
    }

    // Inserts a value into the hash table. This does not check
    // if the key already exists, and may produce duplicate values.
    iterator do_insert(const value_type &v) {
      typename container_type::iterator iter = container->insert(v);

      if (iter != container->end()) {
          return iterator(this, iter, spill.begin());
      }
      else {
        if (load_factor() > 0.8) {
          rehash_to_new_container();
          iter = container->insert(v);
          if(iter != container->end()) {
            return iterator(this, iter, spill.begin());
          }
          else {
            return iterator(this, container->end(), spill.insert(v).first);
          }
        } else {
          // we have a *really* terrible hash function.
          // use the spill
          return iterator(this, container->end(), spill.insert(v).first);
        }
      }
    }

  public:

    hopscotch_map(Hash hashfun = Hash(),
                  KeyEqual equalfun = KeyEqual()):
                            container(NULL),
                            hashfun(hashfun), equalfun(equalfun) {
      container = create_new_container(32);
    }

    hopscotch_map(const hopscotch_map& h):
                            hashfun(h.hashfun), equalfun(h.equalfun) {
      container = create_new_container(h.capacity());
      (*container) = *(h.container);
      spill = h.spill;
    }

    // only increases
    void rehash(size_t s) {
      if (s > capacity()) {
        rehash_to_new_container(s);
      }
    }

    ~hopscotch_map() {
      destroy_all();
    }

    hasher hash_function() const {
      return hashfun.hashfun;
    }

    KeyEqual key_eq() const {
      return equalfun.equalfun;
    }

    hopscotch_map& operator=(const hopscotch_map& other) {
      (*container) = *(other.container);
      hashfun = other.hashfun;
      equalfun = other.equalfun;
      return *this;
    }

    size_type size() const {
      return container->size() + spill.size();
    }

    iterator begin() {
      return iterator(this, container->begin(), spill.begin());
    }

    iterator end() {
      return iterator(this, container->end(), spill.end());
    }


    const_iterator begin() const {
      return const_iterator(this, container->begin(), spill.begin());
    }

    const_iterator end() const {
      return const_iterator(this, container->end(), spill.end());
    }


    std::pair<iterator, bool> insert(const value_type& v) {
      iterator i = find(v.first);
      if (i != end()) return std::make_pair(i, false);
      else return std::make_pair(do_insert(v), true);
    }


    iterator insert(const_iterator hint, const value_type& v) {
      return insert(v).first;
    }

    iterator find(key_type const& k) {
      value_type v(k, mapped_type());
      typename container_type::iterator iter = container->find(v);
      if (iter != container->end()) {
        return iterator(this, iter, spill.begin());
      } else {
        return iterator(this, iter, spill.find(k));
      }
    }

    const_iterator find(key_type const& k) const {
      value_type v(k, mapped_type());
      typename container_type::iterator iter = container->find(v);
      if (iter != container->end()) {
        return const_iterator(this, iter, spill.begin());
      } else {
        return const_iterator(this, iter, spill.find(k));
      }
    }

    size_t count(key_type const& k) const {
      value_type v(k, mapped_type());
      return container->count(v) || spill.count(k);
    }


    bool erase(iterator iter) {
      return container->erase(iter.iter) || spill.erase(iter.iter2);
    }

    bool erase(key_type const& k) {
      value_type v(k, mapped_type());
      return container->erase(v) || spill.erase(k);
    }

    void swap(hopscotch_map& other) {
      std::swap(container, other.container);
      std::swap(spill, other.spill);
      std::swap(hashfun, other.hashfun);
      std::swap(equalfun, other.equalfun);
    }

    mapped_type& operator[](const key_type& i) {
      iterator iter = find(i);
      value_type tmp(i, mapped_type());
      if (iter == end()) iter = do_insert(tmp);
      return iter->second;
    }

    void clear() {
      destroy_all();
      container = create_new_container(128);
    }


    size_t capacity() const {
      return container->capacity() + spill.size();
    }


    float load_factor() const {
      return float(size()) / capacity();
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

    void put(const value_type &v) {
      // try to insert into the container
      (*this)[v.first] = v.second;
    }

    void put(const Key& k, const Value& v) {
      (*this)[k] = v;
    }

    std::pair<bool, Value> get(const Key& k) const {
      const_iterator iter = find(k);
      return std::make_pair(iter == end(), iter->second);
    }
  };

}; // end of turi namespace

#endif
