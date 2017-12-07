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


//! \addtogroup fn_clamp
//! @{



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  is_arma_type<T1>::value && is_cx<typename T1::elem_type>::no,
  const mtOp<typename T1::elem_type, T1, op_clamp>
  >::result
clamp(const T1& X, const typename T1::elem_type min_val, const typename T1::elem_type max_val)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (min_val > max_val), "clamp(): min_val has to be smaller than max_val" );

  return mtOp<typename T1::elem_type, T1, op_clamp>(mtOp_dual_aux_indicator(), X, min_val, max_val);
  }



template<typename T1>
arma_warn_unused
inline
const mtOpCube<typename T1::elem_type, T1, op_clamp>
clamp(const BaseCube<typename T1::elem_type,T1>& X, const typename T1::elem_type min_val, const typename T1::elem_type max_val, typename arma_not_cx<typename T1::elem_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  arma_debug_check( (min_val > max_val), "clamp(): min_val has to be smaller than max_val" );

  return mtOpCube<typename T1::elem_type, T1, op_clamp>(mtOpCube_dual_aux_indicator(), X.get_ref(), min_val, max_val);
  }



//! @}
