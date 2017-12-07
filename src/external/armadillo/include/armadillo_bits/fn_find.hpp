// Copyright 2008-2016 Conrad Sanderson (http://conradsanderson.id.au)
// Copyright 2008-2016 National ICT Australia (NICTA)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------------


//! \addtogroup fn_find
//! @{



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const mtOp<uword, T1, op_find_simple>
  >::result
find(const T1& X)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_find_simple>(X);
  }



template<typename T1>
arma_warn_unused
inline
const mtOp<uword, T1, op_find>
find(const Base<typename T1::elem_type,T1>& X, const uword k, const char* direction = "first")
  {
  arma_extra_debug_sigprint();

  const char sig = (direction != NULL) ? direction[0] : char(0);

  arma_debug_check
    (
    ( (sig != 'f') && (sig != 'F') && (sig != 'l') && (sig != 'L') ),
    "find(): direction must be \"first\" or \"last\""
    );

  const uword type = ( (sig == 'f') || (sig == 'F') ) ? 0 : 1;

  return mtOp<uword, T1, op_find>(X.get_ref(), k, type);
  }



//



template<typename T1>
arma_warn_unused
inline
uvec
find(const BaseCube<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_cube<T1> tmp(X.get_ref());

  const Mat<eT> R( const_cast< eT* >(tmp.M.memptr()), tmp.M.n_elem, 1, false );

  return find(R);
  }



template<typename T1>
arma_warn_unused
inline
uvec
find(const BaseCube<typename T1::elem_type,T1>& X, const uword k, const char* direction = "first")
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_cube<T1> tmp(X.get_ref());

  const Mat<eT> R( const_cast< eT* >(tmp.M.memptr()), tmp.M.n_elem, 1, false );

  return find(R, k, direction);
  }



template<typename T1, typename op_rel_type>
arma_warn_unused
inline
uvec
find(const mtOpCube<uword, T1, op_rel_type>& X, const uword k = 0, const char* direction = "first")
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_cube<T1> tmp(X.m);

  const Mat<eT> R( const_cast< eT* >(tmp.M.memptr()), tmp.M.n_elem, 1, false );

  return find( mtOp<uword, Mat<eT>, op_rel_type>(R, X.aux), k, direction );
  }



template<typename T1, typename T2, typename glue_rel_type>
arma_warn_unused
inline
uvec
find(const mtGlueCube<uword, T1, T2, glue_rel_type>& X, const uword k = 0, const char* direction = "first")
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT1;
  typedef typename T2::elem_type eT2;

  const unwrap_cube<T1> tmp1(X.A);
  const unwrap_cube<T2> tmp2(X.B);

  arma_debug_assert_same_size( tmp1.M, tmp2.M, "relational operator" );

  const Mat<eT1> R1( const_cast< eT1* >(tmp1.M.memptr()), tmp1.M.n_elem, 1, false );
  const Mat<eT2> R2( const_cast< eT2* >(tmp2.M.memptr()), tmp2.M.n_elem, 1, false );

  return find( mtGlue<uword, Mat<eT1>, Mat<eT2>, glue_rel_type>(R1, R2), k, direction );
  }



//



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const mtOp<uword, T1, op_find_finite>
  >::result
find_finite(const T1& X)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_find_finite>(X);
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const mtOp<uword, T1, op_find_nonfinite>
  >::result
find_nonfinite(const T1& X)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, T1, op_find_nonfinite>(X);
  }



//



template<typename T1>
arma_warn_unused
inline
uvec
find_finite(const BaseCube<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_cube<T1> tmp(X.get_ref());

  const Mat<eT> R( const_cast< eT* >(tmp.M.memptr()), tmp.M.n_elem, 1, false );

  return find_finite(R);
  }



template<typename T1>
arma_warn_unused
inline
uvec
find_nonfinite(const BaseCube<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_cube<T1> tmp(X.get_ref());

  const Mat<eT> R( const_cast< eT* >(tmp.M.memptr()), tmp.M.n_elem, 1, false );

  return find_nonfinite(R);
  }



//! @}
