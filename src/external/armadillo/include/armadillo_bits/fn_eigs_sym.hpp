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


//! \addtogroup fn_eigs_sym
//! @{


//! eigenvalues of symmetric real sparse matrix X
template<typename T1>
arma_warn_unused
inline
Col<typename T1::pod_type>
eigs_sym
  (
  const SpBase<typename T1::elem_type,T1>& X,
  const uword                              n_eigvals,
  const char*                              form = "lm",
  const typename T1::elem_type             tol  = 0.0,
  const typename arma_real_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  Mat<typename T1::elem_type> eigvec;
  Col<typename T1::pod_type > eigval;

  const bool status = sp_auxlib::eigs_sym(eigval, eigvec, X, n_eigvals, form, tol);

  if(status == false)
    {
    eigval.soft_reset();
    arma_stop_runtime_error("eigs_sym(): decomposition failed");
    }

  return eigval;
  }



//! eigenvalues of symmetric real sparse matrix X
template<typename T1>
inline
bool
eigs_sym
  (
           Col<typename T1::pod_type >&    eigval,
  const SpBase<typename T1::elem_type,T1>& X,
  const uword                              n_eigvals,
  const char*                              form = "lm",
  const typename T1::elem_type             tol  = 0.0,
  const typename arma_real_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  Mat<typename T1::elem_type> eigvec;

  const bool status = sp_auxlib::eigs_sym(eigval, eigvec, X, n_eigvals, form, tol);

  if(status == false)
    {
    eigval.soft_reset();
    arma_debug_warn("eigs_sym(): decomposition failed");
    }

  return status;
  }



//! eigenvalues and eigenvectors of symmetric real sparse matrix X
template<typename T1>
inline
bool
eigs_sym
  (
           Col<typename T1::pod_type >&    eigval,
           Mat<typename T1::elem_type>&    eigvec,
  const SpBase<typename T1::elem_type,T1>& X,
  const uword                              n_eigvals,
  const char*                              form = "lm",
  const typename T1::elem_type             tol  = 0.0,
  const typename arma_real_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  arma_debug_check( void_ptr(&eigval) == void_ptr(&eigvec), "eigs_sym(): parameter 'eigval' is an alias of parameter 'eigvec'" );

  const bool status = sp_auxlib::eigs_sym(eigval, eigvec, X, n_eigvals, form, tol);

  if(status == false)
    {
    eigval.soft_reset();
    eigvec.soft_reset();
    arma_debug_warn("eigs_sym(): decomposition failed");
    }

  return status;
  }



//! @}
