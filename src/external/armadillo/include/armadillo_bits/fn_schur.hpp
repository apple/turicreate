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


//! \addtogroup fn_schur
//! @{


template<typename T1>
inline
bool
schur
  (
         Mat<typename T1::elem_type>&    S,
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  Mat<eT> U;

  const bool status = auxlib::schur(U, S, X.get_ref(), false);

  if(status == false)
    {
    S.soft_reset();
    arma_debug_warn("schur(): decomposition failed");
    }

  return status;
  }



template<typename T1>
arma_warn_unused
inline
Mat<typename T1::elem_type>
schur
  (
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  Mat<eT> S;
  Mat<eT> U;

  const bool status = auxlib::schur(U, S, X.get_ref(), false);

  if(status == false)
    {
    S.soft_reset();
    arma_stop_runtime_error("schur(): decomposition failed");
    }

  return S;
  }



template<typename T1>
inline
bool
schur
  (
         Mat<typename T1::elem_type>&    U,
         Mat<typename T1::elem_type>&    S,
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  arma_debug_check( void_ptr(&U) == void_ptr(&S), "schur(): 'U' is an alias of 'S'" );

  const bool status = auxlib::schur(U, S, X.get_ref(), true);

  if(status == false)
    {
    U.soft_reset();
    S.soft_reset();
    arma_debug_warn("schur(): decomposition failed");
    }

  return status;
  }



//! @}
