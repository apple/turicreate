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


//! \addtogroup fn_prod
//! @{


//! \brief
//! Delayed product of elements of a matrix along a specified dimension (either rows or columns).
//! The result is stored in a dense matrix that has either one column or one row.
//! For dim = 0, find the sum of each column (i.e. traverse across rows)
//! For dim = 1, find the sum of each row (i.e. traverse across columns)
//! The default is dim = 0.
//! NOTE: this function works differently than in Matlab/Octave.

template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_prod>
prod
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

  return Op<T1, op_prod>(X, dim, 0);
  }



template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_prod>
prod
  (
  const T1& X,
  const uword dim,
  const typename enable_if<resolves_to_vector<T1>::value == true>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return Op<T1, op_prod>(X, dim, 0);
  }



template<typename T1>
arma_warn_unused
inline
typename T1::elem_type
prod
  (
  const T1& X,
  const arma_empty_class junk1 = arma_empty_class(),
  const typename enable_if<resolves_to_vector<T1>::value == true>::result* junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return op_prod::prod( X );
  }



template<typename T1>
arma_warn_unused
inline
typename T1::elem_type
prod(const Op<T1, op_prod>& in)
  {
  arma_extra_debug_sigprint();
  arma_extra_debug_print("prod(): two consecutive prod() calls detected");

  return op_prod::prod( in.m );
  }



template<typename T1>
arma_warn_unused
inline
const Op<Op<T1, op_prod>, op_prod>
prod(const Op<T1, op_prod>& in, const uword dim)
  {
  arma_extra_debug_sigprint();

  return Op<Op<T1, op_prod>, op_prod>(in, dim, 0);
  }



template<typename T>
arma_warn_unused
arma_inline
const typename arma_scalar_only<T>::result &
prod(const T& x)
  {
  return x;
  }



//! @}
