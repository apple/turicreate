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


//! \addtogroup fn_min
//! @{


template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_min>
min
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

  return Op<T1, op_min>(X, dim, 0);
  }


template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_min>
min
  (
  const T1& X,
  const uword dim,
  const typename enable_if<resolves_to_vector<T1>::value == true>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return Op<T1, op_min>(X, dim, 0);
  }



template<typename T1>
arma_warn_unused
inline
typename T1::elem_type
min
  (
  const T1& X,
  const arma_empty_class junk1 = arma_empty_class(),
  const typename enable_if<resolves_to_vector<T1>::value == true>::result* junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return op_min::min(X);
  }



template<typename T1>
arma_warn_unused
inline
typename T1::elem_type
min(const Op<T1, op_min>& in)
  {
  arma_extra_debug_sigprint();
  arma_extra_debug_print("min(): two consecutive min() calls detected");

  return op_min::min(in.m);
  }



template<typename T1>
arma_warn_unused
arma_inline
const Op< Op<T1, op_min>, op_min>
min(const Op<T1, op_min>& in, const uword dim)
  {
  arma_extra_debug_sigprint();

  return Op< Op<T1, op_min>, op_min>(in, dim, 0);
  }



template<typename T>
arma_warn_unused
arma_inline
const typename arma_scalar_only<T>::result &
min(const T& x)
  {
  return x;
  }



//! element-wise minimum
template<typename T1, typename T2>
arma_warn_unused
arma_inline
typename
enable_if2
  <
  ( is_arma_type<T1>::value && is_arma_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value ),
  const Glue<T1, T2, glue_min>
  >::result
min
  (
  const T1& X,
  const T2& Y
  )
  {
  arma_extra_debug_sigprint();

  return Glue<T1, T2, glue_min>(X, Y);
  }



template<typename T1>
arma_warn_unused
arma_inline
const OpCube<T1, op_min>
min
  (
  const BaseCube<typename T1::elem_type, T1>& X,
  const uword dim = 0
  )
  {
  arma_extra_debug_sigprint();

  return OpCube<T1, op_min>(X.get_ref(), dim, 0);
  }



template<typename T1, typename T2>
arma_warn_unused
arma_inline
const GlueCube<T1, T2, glue_min>
min
  (
  const BaseCube<typename T1::elem_type, T1>& X,
  const BaseCube<typename T1::elem_type, T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  return GlueCube<T1, T2, glue_min>(X.get_ref(), Y.get_ref());
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_sparse_type<T1>::value == true) && (resolves_to_sparse_vector<T1>::value == true),
  typename T1::elem_type
  >::result
min(const T1& x)
  {
  arma_extra_debug_sigprint();

  return spop_min::vector_min(x);
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_sparse_type<T1>::value == true) && (resolves_to_sparse_vector<T1>::value == false),
  const SpOp<T1, spop_min>
  >::result
min(const T1& X, const uword dim = 0)
  {
  arma_extra_debug_sigprint();

  return SpOp<T1, spop_min>(X, dim, 0);
  }



template<typename T1>
arma_warn_unused
inline
typename T1::elem_type
min(const SpOp<T1, spop_min>& X)
  {
  arma_extra_debug_sigprint();
  arma_extra_debug_print("min(): two consecutive min() calls detected");

  return spop_min::vector_min(X.m);
  }



template<typename T1>
arma_warn_unused
inline
const SpOp< SpOp<T1, spop_min>, spop_min>
min(const SpOp<T1, spop_min>& in, const uword dim)
  {
  arma_extra_debug_sigprint();

  return SpOp< SpOp<T1, spop_min>, spop_min>(in, dim, 0);
  }



arma_warn_unused
inline
uword
min(const SizeMat& s)
  {
  return (std::min)(s.n_rows, s.n_cols);
  }



arma_warn_unused
inline
uword
min(const SizeCube& s)
  {
  return (std::min)( (std::min)(s.n_rows, s.n_cols), s.n_slices );
  }



//! @}
