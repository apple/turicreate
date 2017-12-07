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


//! \addtogroup fn_all
//! @{



template<typename T1>
arma_warn_unused
arma_inline
const mtOp<uword, T1, op_all>
all
  (
  const T1&   X,
  const uword dim = 0,
  const typename enable_if< is_arma_type<T1>::value       == true  >::result* junk1 = 0,
  const typename enable_if< resolves_to_vector<T1>::value == false >::result* junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return mtOp<uword, T1, op_all>(X, dim, 0);
  }



template<typename T1>
arma_warn_unused
arma_inline
const mtOp<uword, T1, op_all>
all
  (
  const T1&   X,
  const uword dim,
  const typename enable_if<resolves_to_vector<T1>::value == true>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return mtOp<uword, T1, op_all>(X, dim, 0);
  }



template<typename T1>
arma_warn_unused
inline
bool
all
  (
  const T1& X,
  const arma_empty_class junk1 = arma_empty_class(),
  const typename enable_if<resolves_to_vector<T1>::value == true>::result* junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return op_all::all_vec(X);
  }



template<typename T1>
arma_warn_unused
inline
bool
all(const mtOp<uword, T1, op_all>& in)
  {
  arma_extra_debug_sigprint();
  arma_extra_debug_print("all(): two consecutive calls to all() detected");

  return op_all::all_vec(in.m);
  }



template<typename T1>
arma_warn_unused
arma_inline
const Op< mtOp<uword, T1, op_all>, op_all>
all(const mtOp<uword, T1, op_all>& in, const uword dim)
  {
  arma_extra_debug_sigprint();

  return mtOp<uword, mtOp<uword, T1, op_all>, op_all>(in, dim, 0);
  }



//! @}
