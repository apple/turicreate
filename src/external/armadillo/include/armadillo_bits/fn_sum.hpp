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


//! \addtogroup fn_sum
//! @{


template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_sum>
sum
  (
  const T1& X,
  const uword dim = 0,
  const typename enable_if< is_arma_type<T1>::value       == true  >::result* junk1 = 0,
  const typename enable_if< resolves_to_vector<T1>::value == false >::result* junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return Op<T1, op_sum>(X, dim, 0);
  }



template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_sum>
sum
  (
  const T1& X,
  const uword dim,
  const typename enable_if< resolves_to_vector<T1>::value == true >::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return Op<T1, op_sum>(X, dim, 0);
  }



template<typename T1>
arma_warn_unused
inline
typename T1::elem_type
sum
  (
  const T1& X,
  const arma_empty_class junk1 = arma_empty_class(),
  const typename enable_if< resolves_to_vector<T1>::value == true >::result* junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return accu(X);
  }



template<typename T1>
arma_warn_unused
inline
typename T1::elem_type
sum(const Op<T1, op_sum>& in)
  {
  arma_extra_debug_sigprint();
  arma_extra_debug_print("sum(): two consecutive sum() calls detected");

  return accu(in.m);
  }



template<typename T1>
arma_warn_unused
arma_inline
const Op<Op<T1, op_sum>, op_sum>
sum(const Op<T1, op_sum>& in, const uword dim)
  {
  arma_extra_debug_sigprint();

  return Op<Op<T1, op_sum>, op_sum>(in, dim, 0);
  }



template<typename T>
arma_warn_unused
arma_inline
const typename arma_scalar_only<T>::result &
sum(const T& x)
  {
  return x;
  }



//! sum of cube
template<typename T1>
arma_warn_unused
arma_inline
const OpCube<T1, op_sum>
sum
  (
  const BaseCube<typename T1::elem_type,T1>& X,
  const uword dim = 0
  )
  {
  arma_extra_debug_sigprint();

  return OpCube<T1, op_sum>(X.get_ref(), dim, 0);
  }



//! sum of sparse object
template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_sparse_type<T1>::value == true) && (resolves_to_sparse_vector<T1>::value == true),
  typename T1::elem_type
  >::result
sum(const T1& x)
  {
  arma_extra_debug_sigprint();

  // sum elements
  return accu(x);
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_sparse_type<T1>::value == true) && (resolves_to_sparse_vector<T1>::value == false),
  const SpOp<T1,spop_sum>
  >::result
sum(const T1& x, const uword dim = 0)
  {
  arma_extra_debug_sigprint();

  return SpOp<T1,spop_sum>(x, dim, 0);
  }



template<typename T1>
arma_warn_unused
inline
typename T1::elem_type
sum(const SpOp<T1, spop_sum>& in)
  {
  arma_extra_debug_sigprint();
  arma_extra_debug_print("sum(): two consecutive sum() calls detected");

  return accu(in.m);
  }



template<typename T1>
arma_warn_unused
arma_inline
const SpOp<SpOp<T1, spop_sum>, spop_sum>
sum(const SpOp<T1, spop_sum>& in, const uword dim)
  {
  arma_extra_debug_sigprint();

  return SpOp<SpOp<T1, spop_sum>, spop_sum>(in, dim, 0);
  }



//! @}
