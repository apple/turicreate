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


//! \addtogroup fn_sort
//! @{



template<typename T1>
arma_warn_unused
arma_inline
typename
enable_if2
  <
  (is_arma_type<T1>::value),
  const Op<T1, op_sort_default>
  >::result
sort
  (
  const T1& X
  )
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_sort_default>(X, 0, 0);
  }



//! NOTE: don't use this form: it will be removed
template<typename T1>
arma_deprecated
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value),
  const Op<T1, op_sort_default>
  >::result
sort
  (
  const T1&   X,
  const uword sort_type
  )
  {
  arma_extra_debug_sigprint();

  // arma_debug_warn("sort(X,uword) is deprecated and will be removed; change to sort(X,sort_direction)");

  return Op<T1, op_sort_default>(X, sort_type, 0);
  }



//! NOTE: don't use this form: it will be removed
template<typename T1>
arma_deprecated
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value),
  const Op<T1, op_sort>
  >::result
sort
  (
  const T1&   X,
  const uword sort_type,
  const uword dim
  )
  {
  arma_extra_debug_sigprint();

  // arma_debug_warn("sort(X,uword,uword) is deprecated and will be removed; change to sort(X,sort_direction,dim)");

  return Op<T1, op_sort>(X, sort_type, dim);
  }



template<typename T1, typename T2>
arma_warn_unused
inline
typename
enable_if2
  <
  ( (is_arma_type<T1>::value) && (is_same_type<T2, char>::value) ),
  const Op<T1, op_sort_default>
  >::result
sort
  (
  const T1&   X,
  const T2*   sort_direction
  )
  {
  arma_extra_debug_sigprint();

  const char sig = (sort_direction != NULL) ? sort_direction[0] : char(0);

  arma_debug_check( (sig != 'a') && (sig != 'd'), "sort(): unknown sort direction");

  const uword sort_type = (sig == 'a') ? 0 : 1;

  return Op<T1, op_sort_default>(X, sort_type, 0);
  }



template<typename T1, typename T2>
arma_warn_unused
inline
typename
enable_if2
  <
  ( (is_arma_type<T1>::value) && (is_same_type<T2, char>::value) ),
  const Op<T1, op_sort>
  >::result
sort
  (
  const T1&   X,
  const T2*   sort_direction,
  const uword dim
  )
  {
  arma_extra_debug_sigprint();

  const char sig = (sort_direction != NULL) ? sort_direction[0] : char(0);

  arma_debug_check( (sig != 'a') && (sig != 'd'), "sort(): unknown sort direction");

  const uword sort_type = (sig == 'a') ? 0 : 1;

  return Op<T1, op_sort>(X, sort_type, dim);
  }



//! @}
