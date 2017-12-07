/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ROW_MAJOR_MATRIX_HPP_
#define TURI_ROW_MAJOR_MATRIX_HPP_

#include <numerics/armadillo.hpp>
#include <util/code_optimization.hpp>

// Armadillo doesn't have the concept of a row major matrix, which
// is needed for efficiency in a number of the algorithms.  This class
// provides a simple wrapper around an armadillo class to simulate row-major
// access.
//

namespace turi {

template <class T>
class row_major_matrix {
 public: 
typedef T value_type;

  size_t n_rows = 0, n_cols = 0; 
 
 private:
  arma::Mat<T> _X; 

  inline void _internal_check() const {
    DASSERT_EQ(_X.n_rows, n_cols); 
    DASSERT_EQ(_X.n_cols, n_rows); 
  }

 public:
  row_major_matrix() 
    : n_rows(0)
    , n_cols(0)
    , _X(0,0)
  {}

  row_major_matrix(size_t _n_rows, size_t _n_cols) 
    : n_rows(_n_rows)
    , n_cols(_n_cols)
    , _X(n_cols, n_rows) 
  {}

 void zeros() { _X.zeros(); }

 void ones() { _X.ones(); } 

 void resize(size_t _n_rows, size_t _n_cols) {
   n_rows = _n_rows; 
   n_cols = _n_cols; 
   _X.resize(n_cols, n_rows); 
   _internal_check();
 }

 auto operator()(size_t i, size_t j) const -> decltype(_X(j,i)) {
   _internal_check();
  return _X(j, i);
 } 

 auto operator()(size_t i, size_t j) -> decltype(_X(j,i)) {
   _internal_check();
  return _X(j, i);
 } 

  GL_HOT_INLINE_FLATTEN 
  inline arma::Row<T> row(size_t i) const {
  _internal_check();
  
  // It doesn't work to return .col(i).t(); apparently there is a reference
  // to a temporary that goes away in that process.  Simulate this by 
  // returning a rowvec initialized from the memory so it doesn't copy anything.  
  //
  // The hurt is real.
  //
  return arma::Row<T>(const_cast<T*>(&(_X(0, i))), n_cols, false); 
 }
 
 template <typename V>
 void set_row(size_t i, V&& v) {
  _internal_check();
  _X.col(i) = v.t();
 } 
 
 template <typename V>
 void add_row(size_t i, V&& v) {
  _internal_check();
  _X.col(i) += v.t();
 } 
 
 auto tr_rows(size_t first_row, size_t last_row) const 
   -> decltype(_X.cols(first_row, last_row)) {
  _internal_check();
   return _X.cols(first_row, last_row); 
 }
 
 void fill(T v) { 
  _X.fill(v);
 }

 auto t() -> decltype(_X) {
   return _X; 
 }

 auto t() const -> decltype(_X) {
   return _X; 
 }

 auto X() const -> decltype(_X.t()) { 
   return _X.t(); 
 }


 template <typename A> 
 inline const row_major_matrix<value_type>& operator=(A&& X) { 
    _X = X.t(); 
    n_rows = _X.n_cols; 
    n_cols = _X.n_rows;
    return *this;
 }
 
inline const row_major_matrix<value_type>& operator=(const row_major_matrix<value_type>& X) { 
    _X = X._X; 
    n_rows = _X.n_cols; 
    n_cols = _X.n_rows;
    return *this;
 }
 
 template <typename A> 
 inline const row_major_matrix<value_type>& operator+=(A&& X) { 
    _X += X.t();  
    return *this;
 }
 

 inline const row_major_matrix<value_type>& operator+=(const row_major_matrix<value_type>& X) { 
    _X += X._X; 
    return *this; 
 }

 auto tr_tail_rows(size_t n) -> decltype(_X.tail_cols(n)) {
   return _X.tail_cols(n);
 }

 auto tr_tail_rows(size_t n) const -> decltype(_X.tail_cols(n)) {
   return _X.tail_cols(n);
 }

 auto tr_head_rows(size_t n) -> decltype(_X.head_cols(n)) {
   return _X.head_cols(n);
 }

 auto tr_head_rows(size_t n) const -> decltype(_X.head_cols(n)) {
   return _X.head_cols(n);
 }


 /** Serialization -- save.
   */
  void save(turi::oarchive& oarc) const {
    oarc << n_rows << n_cols;
    turi::serialize(oarc, _X.memptr(), n_rows*n_cols*sizeof(T));
  }

  /** Serialization -- load.
   */
  void load(turi::iarchive& iarc) {
    iarc >> n_rows >> n_cols;
    _X.resize(n_cols, n_rows);
    turi::deserialize(iarc, _X.memptr(), n_rows*n_cols*sizeof(T));
  }

};

template <typename T> 
static inline auto mean(const row_major_matrix<T>& X) 
  -> const decltype(arma::mean(X.X()))& { 
  return arma::mean(X.X()); 
}

template <typename T, typename C> 
static inline auto dot(const row_major_matrix<T>& X, const C& Y) 
-> decltype(arma::dot(X.X(), Y)) { 
  return arma::dot(X.X(), Y); 
}

template <typename T, typename C> 
static inline auto dot(const C& Y, const row_major_matrix<T>& X) 
 -> decltype(arma::dot(Y, X.X())) {
  return arma::dot(Y, X.X()); 
}


}

#endif
