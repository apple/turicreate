/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SPARSE_VECTOR_HPP_
#define TURI_SPARSE_VECTOR_HPP_

#include <vector>
#include <cmath>
#include <numerics/armadillo.hpp>
#include <type_traits>
#include <serialization/serialization_includes.hpp>

namespace turi {


template <typename Value, typename Index = size_t>
class sparse_vector {
  public:
    typedef Value value_type;
    typedef Index index_type;
    typedef std::pair<index_type, value_type> _element_type;
    typedef std::vector<_element_type> _container_type;

    typedef typename _container_type::iterator iterator;
    typedef typename _container_type::const_iterator const_iterator;

    inline sparse_vector(size_t size = 0)
    : _size(size)
    {
    }

    void resize(index_type new_size){
      _size = new_size;
      if(!_data.empty() && _data.back().first >= new_size) {

        auto it = std::lower_bound(_data.begin(), _data.end(),
            _element_type{_size, 0},
            [](const _element_type& p1, const _element_type& p2) {
              return p1.first < p2.first;
            });

        if(it != _data.end()) {
          DASSERT_GE(it->first, _size);
        }
        _data.resize(it - _data.begin());

#ifndef NDEBUG
        if(!_data.empty()) {
          DASSERT_LT(_data.back().first, _size);
        }
#endif
      }
      _internal_check();
    }

    void reserve(index_type n) { _data.reserve(n); }

    inline void clear(){ _data.clear(); }
    inline void zeros(){ clear(); }

    void insert(index_type idx, value_type val){
      auto it = _find_element(idx);
      if(it != _data.end() && it->first == idx) { 
        it->second = val;
      } else {
        _data.insert(it, {idx, val}); 
      }
    }

    inline operator arma::Col<value_type>() const {
      arma::Col<value_type> ret(size());
      ret.zeros();
      for(auto p : *this) { 
        ret(p.first) = p.second;
      }
      return ret; 
    }
    
    inline operator arma::Row<value_type>() const {
      arma::Row<value_type> ret(size());
      ret.zeros();
      for(auto p : *this) { 
        ret(p.first) = p.second;
      }
      return ret; 
    }

    inline arma::Col<value_type> to_dense() const {
      return (arma::Col<value_type>)(*this); 
    }

    inline size_t num_nonzeros() const { return _data.size(); }
    inline size_t size() const { return _size; }

    inline iterator begin() { return _data.begin(); }
    inline const_iterator cbegin() const { return _data.cbegin(); }
    inline const_iterator begin() const { return cbegin(); }

    inline iterator end() { return _data.end(); }
    inline const_iterator cend() const { return _data.cend(); }
    inline const_iterator end() const { return cend(); }

    inline value_type operator()(index_type idx) const {
      DASSERT_LT(idx, _size);
      auto it = _find_element(idx);
      if(it != _data.end() && it->first == idx) {
        return it->second;
      } else {
        return 0;
      }
    }

    inline value_type& operator()(index_type idx) {
      DASSERT_LT(idx, _size);
      auto it = _find_element(idx);
      if(it != _data.end() && it->first == idx) {
        return it->second;
      } else {
        value_type& ret = _data.insert(it, {idx, 0})->second;
        return ret; 
      }
    }


    inline bool is_finite() const {
      for(auto p : _data) {
        if(!std::isfinite(p.second)) return false;
      }
      return true;
    }

    template <typename T>
    const sparse_vector& operator/=(const T& t) {
      for(auto& p : _data) {
        p.second /= t;
      }

      return *this;
    }

    template <typename T>
    const sparse_vector& operator*=(const T& t) {
      for(auto& p : _data) {
        p.second *= t;
      }

      return *this;
    }

  /** Serialization -- save.
   */
  void save(turi::oarchive& oarc) const {
    size_t version = 1;

    oarc << version; 
    
    oarc << (size_t)_size << (size_t)_data.size();
    for (const auto& p : _data) {
      oarc << (size_t)p.first << (double)p.second;
    }
  }

  /** Serialization -- load.
   */
  void load(turi::iarchive& iarc) {
    size_t version;
    iarc >> version;
    ASSERT_EQ(version, 1); 
    

    size_t nnz;
    iarc >> _size;
    iarc >> nnz;
    _data.resize(nnz);

    size_t index;
    double value;
    for(size_t i = 0; i < nnz; i++) {
      iarc >> index >> value;
      _data[i] = {index, value};
    }

    _internal_check();
  }

  private:
    index_type _size;
    _container_type _data;


    void _add_element_to_end(index_type idx, value_type val) {
      DASSERT_LT(idx, _size);

      if(!_data.empty()) {
        DASSERT_GT(idx, _data.back().first);
      }

      _data.push_back({idx, val});
    }

    const_iterator _find_element_c(index_type idx) const GL_HOT_INLINE_FLATTEN {
      if(_data.empty()) {
        return _data.begin();
      } else if(idx > _data.back().first) {
        return _data.end();
      } else {

        auto it = std::lower_bound(_data.begin(), _data.end(),
            _element_type{idx, 0},
            [](const _element_type& p1, const _element_type& p2) {
              return p1.first < p2.first;
            });

        return it;
      }
    }

    const_iterator _find_element(index_type idx) const GL_HOT_INLINE_FLATTEN {
      return _find_element_c(idx);
    }

    iterator _find_element(index_type idx) GL_HOT_INLINE_FLATTEN {
      return _data.begin() + (_find_element_c(idx) - _data.begin());
    }


    void _internal_check() const { 
#ifndef NDEBUG
      for(size_t i = 0; i < _data.size(); ++i) { 
        DASSERT_LT(_data[i].first, _size); 
        if(i != 0) {
          DASSERT_LT(_data[i-1].first, _data[i].first); 
        }
      }
#endif
    }
};

template <typename T, typename I, typename F>
GL_HOT_INLINE_FLATTEN
static inline T bi_aggregate(const sparse_vector<T, I>& a, const sparse_vector<T, I>& b, F&& bi_func) { 
  
  T d = 0; 
    
  auto it_a = a.begin();
  auto it_b = b.begin();

  while (true) {
    if (it_a == a.end()) {
      while (it_b != b.end()) {
        d += bi_func(0, it_b->second);
        ++it_b;
      }
      break;
    }

    if (it_b == b.end()) {
      while (it_a != a.end()) {
        d += bi_func(it_a->second, 0);
        ++it_a;
      }
      break;
    }

    if (it_a->first < it_b->first) {
      d += bi_func(it_a->second, 0);
      ++it_a;
    } else if (it_a->first > it_b->first) {
      d += bi_func(0, it_b->second);
      ++it_b;
    } else {
      d += bi_func(it_a->second, it_b->second);
      ++it_a, ++it_b;
    }
  }

  return d;  
}


template <typename T, typename Index>
T dot(const sparse_vector<T, Index>& a, const sparse_vector<T, Index>& b) {
  return bi_aggregate(a, b, [](T x, T y) { return x*y; }); 
}

template <typename T, typename Index, typename... Args>
bool approx_equal(const sparse_vector<T, Index>& a, const sparse_vector<T, Index>& b, Args... args) {
  return arma::approx_equal((arma::Col<T>)(a), (arma::Col<T>)(b), args...);
}




}
#endif
