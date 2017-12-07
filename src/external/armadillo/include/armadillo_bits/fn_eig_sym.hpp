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


//! \addtogroup fn_eig_sym
//! @{


//! Eigenvalues of real/complex symmetric/hermitian matrix X
template<typename T1>
inline
bool
eig_sym
  (
         Col<typename T1::pod_type>&     eigval,
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  // unwrap_check not used as T1::elem_type and T1::pod_type may not be the same.
  // furthermore, it doesn't matter if X is an alias of eigval, as auxlib::eig_sym() makes a copy of X

  const bool status = auxlib::eig_sym(eigval, X);

  if(status == false)
    {
    eigval.soft_reset();
    arma_debug_warn("eig_sym(): decomposition failed");
    }

  return status;
  }



//! Eigenvalues of real/complex symmetric/hermitian matrix X
template<typename T1>
arma_warn_unused
inline
Col<typename T1::pod_type>
eig_sym
  (
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  Col<typename T1::pod_type> out;
  const bool status = auxlib::eig_sym(out, X);

  if(status == false)
    {
    out.soft_reset();
    arma_stop_runtime_error("eig_sym(): decomposition failed");
    }

  return out;
  }



//! Eigenvalues and eigenvectors of real/complex symmetric/hermitian matrix X
template<typename T1>
inline
bool
eig_sym
  (
         Col<typename T1::pod_type>&     eigval,
         Mat<typename T1::elem_type>&    eigvec,
  const Base<typename T1::elem_type,T1>& X,
  const char* method =                   "dc",
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  const char sig = (method != NULL) ? method[0] : char(0);

  arma_debug_check( ((sig != 's') && (sig != 'd')),         "eig_sym(): unknown method specified"                             );
  arma_debug_check( void_ptr(&eigval) == void_ptr(&eigvec), "eig_sym(): parameter 'eigval' is an alias of parameter 'eigvec'" );

  const Proxy<T1> P(X.get_ref());

  const bool is_alias = P.is_alias(eigvec);

  Mat<eT>  eigvec_tmp;
  Mat<eT>& eigvec_out = (is_alias == false) ? eigvec : eigvec_tmp;

  bool status = false;

  if(sig == 'd')       { status = auxlib::eig_sym_dc(eigval, eigvec_out, P.Q); }

  if(status == false)  { status = auxlib::eig_sym(eigval, eigvec_out, P.Q);    }

  if(status == false)
    {
    eigval.soft_reset();
    eigvec.soft_reset();
    arma_debug_warn("eig_sym(): decomposition failed");
    }
  else
    {
    if(is_alias)  { eigvec.steal_mem(eigvec_tmp); }
    }

  return status;
  }



//! @}
