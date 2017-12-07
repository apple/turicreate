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


//! \addtogroup fn_lu
//! @{



//! immediate lower upper decomposition, permutation info is embedded into L (similar to Matlab/Octave)
template<typename T1>
inline
bool
lu
  (
         Mat<typename T1::elem_type>&    L,
         Mat<typename T1::elem_type>&    U,
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  arma_debug_check( (&L == &U), "lu(): L and U are the same object");

  const bool status = auxlib::lu(L, U, X);

  if(status == false)
    {
    L.soft_reset();
    U.soft_reset();
    arma_debug_warn("lu(): decomposition failed");
    }

  return status;
  }



//! immediate lower upper decomposition, also providing the permutation matrix
template<typename T1>
inline
bool
lu
  (
         Mat<typename T1::elem_type>&    L,
         Mat<typename T1::elem_type>&    U,
         Mat<typename T1::elem_type>&    P,
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  arma_debug_check( ( (&L == &U) || (&L == &P) || (&U == &P) ), "lu(): two or more output objects are the same object");

  const bool status = auxlib::lu(L, U, P, X);

  if(status == false)
    {
    L.soft_reset();
    U.soft_reset();
    P.soft_reset();
    arma_debug_warn("lu(): decomposition failed");
    }

  return status;
  }



//! @}
