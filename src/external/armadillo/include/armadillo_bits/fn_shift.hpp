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



//! \addtogroup fn_shift
//! @{


template<typename T1>
arma_warn_unused
arma_inline
typename
enable_if2
  <
  (is_arma_type<T1>::value),
  const Op<T1, op_shift_default>
  >::result
shift
  (
  const T1&   X,
  const sword N
  )
  {
  arma_extra_debug_sigprint();

  const uword len = (N < 0) ? uword(-N) : uword(N);
  const uword neg = (N < 0) ? uword( 1) : uword(0);

  return Op<T1, op_shift_default>(X, len, neg, uword(0), 'j');
  }



template<typename T1>
arma_warn_unused
arma_inline
typename
enable_if2
  <
  (is_arma_type<T1>::value),
  const Op<T1, op_shift>
  >::result
shift
  (
  const T1&   X,
  const sword N,
  const uword dim
  )
  {
  arma_extra_debug_sigprint();

  const uword len = (N < 0) ? uword(-N) : uword(N);
  const uword neg = (N < 0) ? uword( 1) : uword(0);

  return Op<T1, op_shift>(X, len, neg, dim, 'j');
  }



//! @}
