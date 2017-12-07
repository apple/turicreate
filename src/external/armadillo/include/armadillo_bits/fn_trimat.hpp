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


//! \addtogroup fn_trimat
//! @{


template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_trimat>
trimatu(const Base<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_trimat>(X.get_ref(), 0, 0);
  }



template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_trimat>
trimatl(const Base<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_trimat>(X.get_ref(), 1, 0);
  }



template<typename T1>
arma_warn_unused
arma_inline
const SpOp<T1, spop_trimat>
trimatu(const SpBase<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  return SpOp<T1, spop_trimat>(X.get_ref(), 0, 0);
  }



template<typename T1>
arma_warn_unused
arma_inline
const SpOp<T1, spop_trimat>
trimatl(const SpBase<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  return SpOp<T1, spop_trimat>(X.get_ref(), 1, 0);
  }



//



template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_trimatl_ext>
trimatl(const Base<typename T1::elem_type,T1>& X, const sword k)
  {
  arma_extra_debug_sigprint();

  const uword row_offset = (k < 0) ? uword(-k) : uword(0);
  const uword col_offset = (k > 0) ? uword( k) : uword(0);

  return Op<T1, op_trimatl_ext>(X.get_ref(), row_offset, col_offset);
  }



template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_trimatu_ext>
trimatu(const Base<typename T1::elem_type,T1>& X, const sword k)
  {
  arma_extra_debug_sigprint();

  const uword row_offset = (k < 0) ? uword(-k) : uword(0);
  const uword col_offset = (k > 0) ? uword( k) : uword(0);

  return Op<T1, op_trimatu_ext>(X.get_ref(), row_offset, col_offset);
  }



// // TODO: implement for sparse matrices
// template<typename T1>
// arma_warn_unused
// arma_inline
// const SpOp<T1, spop_trimatu_ext>
// trimatu(const SpBase<typename T1::elem_type,T1>& X, const sword k)
//   {
//   arma_extra_debug_sigprint();
//
//   const uword row_offset = (k < 0) ? uword(-k) : uword(0);
//   const uword col_offset = (k > 0) ? uword( k) : uword(0);
//
//   return SpOp<T1, spop_trimatu_ext>(X.get_ref(), row_offset, col_offset);
//   }
//
//
//
// // TODO: implement for sparse matrices
// template<typename T1>
// arma_warn_unused
// arma_inline
// const SpOp<T1, spop_trimatl_ext>
// trimatl(const SpBase<typename T1::elem_type,T1>& X, const sword k)
//   {
//   arma_extra_debug_sigprint();
//
//   const uword row_offset = (k < 0) ? uword(-k) : uword(0);
//   const uword col_offset = (k > 0) ? uword( k) : uword(0);
//
//   return SpOp<T1, spop_trimatl_ext>(X.get_ref(), row_offset, col_offset);
//   }



//! @}
