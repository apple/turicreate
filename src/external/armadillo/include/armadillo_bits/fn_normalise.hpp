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


//! \addtogroup fn_normalise
//! @{



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && (resolves_to_vector<T1>::value == true)),
  const Op<T1, op_normalise_vec>
  >::result
normalise
  (
  const T1&   X,
  const uword p = uword(2),
  const arma_empty_class junk1 = arma_empty_class(),
  const typename arma_real_or_cx_only<typename T1::elem_type>::result* junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return Op<T1, op_normalise_vec>(X, p, 0);
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && (resolves_to_vector<T1>::value == false)),
  const Op<T1, op_normalise_mat>
  >::result
normalise
  (
  const T1&   X,
  const uword p = uword(2),
  const uword dim = 0,
  const typename arma_real_or_cx_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return Op<T1, op_normalise_mat>(X, p, dim);
  }



//! @}
