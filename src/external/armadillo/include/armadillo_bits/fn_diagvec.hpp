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


//! \addtogroup fn_diagvec
//! @{


//! extract a diagonal from a matrix
template<typename T1>
arma_warn_unused
arma_inline
const Op<T1, op_diagvec>
diagvec(const Base<typename T1::elem_type,T1>& X, const sword diag_id = 0)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_diagvec>(X.get_ref(), ((diag_id < 0) ? -diag_id : diag_id), ((diag_id < 0) ? 1 : 0) );
  }



template<typename T1>
arma_warn_unused
arma_inline
const SpOp<T1, spop_diagvec>
diagvec(const SpBase<typename T1::elem_type,T1>& X, const sword diag_id = 0)
  {
  arma_extra_debug_sigprint();

  return SpOp<T1, spop_diagvec>(X.get_ref(), ((diag_id < 0) ? -diag_id : diag_id), ((diag_id < 0) ? 1 : 0) );
  }



//! @}
