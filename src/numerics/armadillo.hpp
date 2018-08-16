/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ARMADILLO_BRIDGE_HPP_
#define TURI_ARMADILLO_BRIDGE_HPP_

// Builds a thin wrapper on top of the armadillo library, providing support for


#include <armadillo>
#include <numerics/sparse_vector.hpp>
#include <numerics/row_major_matrix.hpp>
#include <logger/assertions.hpp>
#include <typeinfo>
#include <limits>
#include <sstream>

namespace turi {

using arma::exp;
using arma::approx_equal;

template <typename C, typename T>
static inline void inplace_elementwise_max(C& container, const T& v) {
  for(auto& e : container) {
    e = std::max<typename std::remove_reference<decltype(e)>::type>(e, v);
  }
}

template <typename C, typename T>
static inline void inplace_elementwise_min(C& container, const T& v) {
  for(auto& e : container) {
    e = std::min<typename std::remove_reference<decltype(e)>::type>(e, v);
  }
}

template <typename C, typename T>
static inline void inplace_elementwise_clip(C& container, const T& min, const T& max) {
  for(auto& e : container) {
    typedef typename std::remove_reference<decltype(e)>::type elem_t; 
    e = std::min<elem_t>(std::max<elem_t>(e, max), min);
  }
}

template <typename Src, typename Dest, typename T>
static inline void elementwise_max_copy(Src& src, const Dest& dest, const T& v) {
  typedef typename std::remove_reference<decltype(dest(0))>::type elem_t; 
  for(size_t i = 0; i < dest.size(); ++i) { 
    dest(i) = std::max<elem_t>(src(i), v); 
  }
}

template <typename Src, typename Dest, typename T>
static inline void elementwise_min_copy(Src& src, const Dest& dest, const T& v) {
  typedef typename std::remove_reference<decltype(dest(0))>::type elem_t; 
  for(size_t i = 0; i < dest.size(); ++i) { 
    dest(i) = std::min<elem_t>(src(i), v); 
  }
}

////////////////////////////////////////////////////////////////////////////////
//
//  Tests.
#define ENABLE_IF_ARMA_VEC(v) \
  typename std::enable_if<std::is_convertible<decltype(arma::sum(v)), double>::value>::type* = 0

#define ENABLE_IF_ARMA_MAT(v) \
  typename std::enable_if<std::is_convertible<decltype(arma::sum(arma::sum(v))), double>::value \
      && !std::is_convertible<decltype(arma::sum(v)), double>::value>::type* = 0

///////////////////////////////////
// 
template <typename C> 
static inline auto _arma_tsum(C&& v, ENABLE_IF_ARMA_VEC(v))
 -> decltype(arma::sum(v)) { 
  return arma::sum(v);
}

template <typename C> 
static inline auto _arma_tsum(C&& v, ENABLE_IF_ARMA_MAT(v)) 
 -> decltype(arma::sum(arma::sum(v))) { 
  return arma::sum(arma::sum(v));
}

///////////////////////////////////
// Total sum 

template <typename C>
static inline auto total_sum(C&& v) -> decltype(_arma_tsum(v)) {
  return _arma_tsum(v);
}

template <typename T, typename I>
static inline typename sparse_vector<T, I>::value_type
total_sum(const sparse_vector<T, I>& container) {
  typename sparse_vector<T, I>::value_type v = 0;
  for(const auto& p : container) {
    v += p.second;
  }
  return v;
}

///////////////////////////////////
// Squared Norm 

template <typename C>
static inline auto squared_norm(C&& v) -> decltype(_arma_tsum(arma::square(v))) {
  return _arma_tsum(arma::square(v));
}

template <typename T, typename I>
static inline typename sparse_vector<T, I>::value_type
squared_norm(const sparse_vector<T, I>& container) {
  typename sparse_vector<T, I>::value_type v = 0;
  for(const auto& p : container) {
    v += p.second * p.second;
  }
  return v;
}


////////////////////////////////////////////////////////////////////////////////
//
//  Additional dot implementations.

using arma::dot;

// Dot
template <typename ArmaVec, typename U, typename Index>
static inline U dot(ArmaVec&& x, const sparse_vector<U, Index>& y, ENABLE_IF_ARMA_VEC(x)) {

 U r = 0;

 for(auto p : y) {
  r += x[p.first] * p.second;
 }

 return r;
}

template <typename ArmaVec, typename U, typename Index>
static inline U dot(const sparse_vector<U, Index>& x, ArmaVec&& y, ENABLE_IF_ARMA_VEC(y)) {
  return dot(y, x);
}

////////////////////////////
// operator *
template <typename U, typename Index>
arma::vec operator*(const arma::mat& X, const sparse_vector<U, Index>& v) {

  if(v.size() != X.n_cols) {
    throw std::logic_error(
        "Number of columns does not match vector size in matrix multiply.");
  }

  arma::vec ret(X.n_rows);

  if(v.num_nonzeros() == 0) {
    ret.zeros();
    return ret;
  }

  auto it = v.begin();

  ret = X.col(it->first) * it->second;
  ++it;

  for(;it != v.end(); ++it) {
    ret += X.col(it->first) * it->second;
  }

  return ret;
}

///////////////////////////
// Interface operators between arma and our sparse vector.

template <typename T, typename U, typename Index>
arma::Col<T>& operator+=(arma::Col<T>& x, const sparse_vector<U, Index>& y) {

  for(auto p : y) {
    x(p.first) += p.second;
  }

  return x;
}

/**
 * Solve for A*x = b using ldlt solving.
 *  A must be symmetric positive definite.
 */
template <typename T>
arma::Col<T>  solve_ldlt(const arma::Mat<T>& _A, const arma::Col<T>& b) {

#ifdef ARMA_USE_LAPACK 
  arma::Mat<T> A = _A;

	if(!A.is_square()) {
		throw std::invalid_argument("LDLT decomposition requires a square matrix.");
	}

  int N = A.n_rows;
  char lower = 'L';
  DASSERT_TRUE(b.n_rows == b.n_rows);

  T lquery;
  int lwork = -1, info, one = 1;
	arma::Col<int> fact;
	fact.set_size(N);
  arma::lapack::sytrf(&lower, &N, const_cast<T*>(A.memptr()), &N,
            fact.memptr(), &lquery, &lwork, &info);
  lwork = int(lquery);

  T *work = new T[lwork];
  arma::lapack::sytrf(&lower, &N, const_cast<T*>(A.memptr()), &N,
            fact.memptr(), work, &lwork, &info);
  delete [] work;

  DASSERT_FALSE(info < 0); // Lapack error
  if (info == 0) {
    arma::Col<T> x = b;
    arma::lapack::sytrs(&lower, &N, &one, const_cast<T*>(A.memptr()), &N,
                        fact.memptr(), x.memptr(), &N, &info);
    return x;
  } else {
    return arma::solve(_A, b);
  }
#else
  return arma::solve(_A, b);
#endif
}

/**
 *  Solve for A*x = b using ldlt ving
 *  A must be symmetric positive semi-definite
 */
template <typename... T, typename b_t>
auto solve_ldlt(const row_major_matrix<T...>& A, b_t&& b)
  -> decltype(solve_ldlt(A.X_raw(),b.eval())) {
  return solve_ldlt(A.X_raw(), b.eval());
}

template <typename AT, typename BT>
auto solve_ldlt(AT&& A, BT&& b) -> decltype(solve_ldlt(A.eval(), b.eval())) {
   return solve_ldlt(A.eval(), b.eval());
}





// Serialization stuff
//
namespace archive_detail {

////////////////////////////////////////////////////////////////////////////////
// Serialization routines for armadillo containers.


template <typename OutArcType, typename T>
struct serialize_impl<OutArcType, arma::Mat<T>, false> {

  static void exec(OutArcType& arc, const arma::Mat<T>& X) {
    arc << (size_t)X.n_rows << (size_t)X.n_cols;
    turi::serialize(arc, X.memptr(), X.n_rows*X.n_cols*sizeof(T));
  }
};

template <typename InArcType, typename T>
struct deserialize_impl<InArcType, arma::Mat<T>, false> {

  static void exec(InArcType& arc, arma::Mat<T>& X) {
    size_t nrows, ncols;
    arc >> nrows >> ncols;
    X.resize(nrows, ncols);
    turi::deserialize(arc, X.memptr(), X.n_rows*X.n_cols*sizeof(T));
  }
};

template <typename OutArcType, typename T>
struct serialize_impl<OutArcType, arma::Col<T>, false> {

  static void exec(OutArcType& arc, const arma::Col<T>& X) {
    arc << size_t(X.n_elem);
    arc << size_t(1);
    turi::serialize(arc, X.memptr(), X.n_elem*sizeof(T));
  }
};

template <typename InArcType, typename T>
struct deserialize_impl<InArcType, arma::Col<T>, false> {

  static void exec(InArcType& arc, arma::Col<T>& X) {
    size_t nelem1, nelem2;
    arc >> nelem1 >> nelem2;
    ASSERT_TRUE(nelem1 == 1 || nelem2 == 1);
    size_t nelem = nelem1 * nelem2;
    X.resize(nelem);
    turi::deserialize(arc, X.memptr(), X.n_elem*sizeof(T));
  }
};


template <typename OutArcType, typename T>
struct serialize_impl<OutArcType, arma::Row<T>, false> {

  static void exec(OutArcType& arc, const arma::Row<T>& X) {
    arc << size_t(1);
    arc << size_t(X.n_elem);
    turi::serialize(arc, X.memptr(), X.n_elem*sizeof(T));
  }
};

template <typename InArcType, typename T>
struct deserialize_impl<InArcType, arma::Row<T>, false> {

  static void exec(InArcType& arc, arma::Row<T>& X) {
    size_t nelem1, nelem2;
    arc >> nelem1 >> nelem2;
    ASSERT_TRUE(nelem1 == 1 || nelem2 == 1);
    size_t nelem = nelem1 * nelem2;
    X.resize(nelem);
    turi::deserialize(arc, X.memptr(), X.n_elem*sizeof(T));
  }
};


}}

#endif
