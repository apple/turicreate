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


//! \addtogroup fn_find_unique
//! @{



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const mtOp<uword,T1,op_find_unique>
  >::result
find_unique
  (
  const T1&  X,
  const bool ascending_indices = true
  )
  {
  arma_extra_debug_sigprint();

  return mtOp<uword,T1,op_find_unique>(X, ((ascending_indices) ? uword(1) : uword(0)), uword(0));
  }



template<typename T1>
arma_warn_unused
inline
uvec
find_unique
  (
  const BaseCube<typename T1::elem_type,T1>& X,
  const bool ascending_indices = true
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_cube<T1> tmp(X.get_ref());

  const Mat<eT> R( const_cast< eT* >(tmp.M.memptr()), tmp.M.n_elem, 1, false );

  return find_unique(R,ascending_indices);
  }



//! @}
