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


//! \addtogroup fn_var
//! @{



template<typename T1>
arma_warn_unused
inline
const mtOp<typename T1::pod_type, T1, op_var>
var
  (
  const T1& X,
  const uword norm_type = 0,
  const uword dim = 0,
  const typename enable_if< is_arma_type<T1>::value       == true  >::result* junk1 = 0,
  const typename enable_if< resolves_to_vector<T1>::value == false >::result* junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return mtOp<typename T1::pod_type, T1, op_var>(X, norm_type, dim);
  }



template<typename T1>
arma_warn_unused
inline
const mtOp<typename T1::pod_type, T1, op_var>
var
  (
  const T1& X,
  const uword norm_type,
  const uword dim,
  const typename enable_if<resolves_to_vector<T1>::value == true>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return mtOp<typename T1::pod_type, T1, op_var>(X, norm_type, dim);
  }



template<typename T1>
arma_warn_unused
inline
typename T1::pod_type
var
  (
  const T1& X,
  const uword norm_type = 0,
  const arma_empty_class junk1 = arma_empty_class(),
  const typename enable_if<resolves_to_vector<T1>::value == true>::result* junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return op_var::var_vec( X, norm_type );
  }



template<typename T>
arma_warn_unused
arma_inline
const typename arma_scalar_only<T>::result
var(const T&)
  {
  return T(0);
  }



template<typename T1>
arma_warn_unused
inline
const mtSpOp<typename T1::pod_type, T1, spop_var>
var
  (
  const T1& X,
  const uword norm_type = 0,
  const uword dim = 0,
  const typename enable_if< is_arma_sparse_type<T1>::value       == true  >::result* junk1 = 0,
  const typename enable_if< resolves_to_sparse_vector<T1>::value == false >::result* junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return mtSpOp<typename T1::pod_type, T1, spop_var>(X, norm_type, dim);
  }



template<typename T1>
arma_warn_unused
inline
const mtSpOp<typename T1::pod_type, T1, spop_var>
var
  (
  const T1& X,
  const uword norm_type,
  const uword dim = 0,
  const typename enable_if<resolves_to_sparse_vector<T1>::value == true>::result* junk1 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);

  return mtSpOp<typename T1::pod_type, T1, spop_var>(X, norm_type, dim);
  }



template<typename T1>
arma_warn_unused
inline
typename T1::pod_type
var
  (
  const T1& X,
  const uword norm_type = 0,
  const arma_empty_class junk1 = arma_empty_class(),
  const typename enable_if<resolves_to_sparse_vector<T1>::value == true>::result* junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return spop_var::var_vec(X, norm_type);
  }



//! @}
