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


//! \addtogroup fn_diagmat
//! @{


//! interpret a matrix or a vector as a diagonal matrix (i.e. off-diagonal entries are zero)
template<typename T1>
arma_warn_unused
arma_inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const Op<T1, op_diagmat>
  >::result
diagmat(const T1& X)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_diagmat>(X);
  }



//! create a matrix with the k-th diagonal set to the given vector
template<typename T1>
arma_warn_unused
arma_inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  const Op<T1, op_diagmat2>
  >::result
diagmat(const T1& X, const sword k)
  {
  arma_extra_debug_sigprint();

  const uword row_offset = (k < 0) ? uword(-k) : uword(0);
  const uword col_offset = (k > 0) ? uword( k) : uword(0);

  return Op<T1, op_diagmat2>(X, row_offset, col_offset);
  }



template<typename T1>
arma_warn_unused
inline
const SpOp<T1, spop_diagmat>
diagmat(const SpBase<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  return SpOp<T1, spop_diagmat>(X.get_ref());
  }



template<typename T1>
arma_warn_unused
inline
const SpOp<T1, spop_diagmat2>
diagmat(const SpBase<typename T1::elem_type,T1>& X, const sword k)
  {
  arma_extra_debug_sigprint();

  const uword row_offset = (k < 0) ? uword(-k) : uword(0);
  const uword col_offset = (k > 0) ? uword( k) : uword(0);

  return SpOp<T1, spop_diagmat2>(X.get_ref(), row_offset, col_offset);
  }



//! @}
