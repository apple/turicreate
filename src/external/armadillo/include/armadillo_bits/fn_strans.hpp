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


//! \addtogroup fn_strans
//! @{



template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_strans>
strans
  (
  const T1& X,
  const typename enable_if< is_arma_type<T1>::value == true >::result* junk1 = 0,
  const typename arma_cx_only<typename T1::elem_type>::result*         junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return Op<T1, op_strans>(X);
  }



// NOTE: for non-complex objects, deliberately returning op_htrans instead of op_strans,
// NOTE: due to currently more optimisations available when using op_htrans, especially by glue_times
template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_htrans>
strans
  (
  const T1& X,
  const typename enable_if< is_arma_type<T1>::value == true >::result* junk1 = 0,
  const typename arma_not_cx<typename T1::elem_type>::result*          junk2 = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  return Op<T1, op_htrans>(X);
  }



//
// handling of sparse matrices


template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  is_arma_sparse_type<T1>::value,
  const SpOp<T1,spop_strans>
  >::result
strans(const T1& x)
  {
  arma_extra_debug_sigprint();

  return SpOp<T1,spop_strans>(x);
  }



//! @}
