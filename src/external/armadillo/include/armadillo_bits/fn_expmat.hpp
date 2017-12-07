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


//! \addtogroup fn_expmat
//! @{


template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  is_real<typename T1::pod_type>::value,
  const Op<T1,op_expmat>
  >::result
expmat(const Base<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  return Op<T1,op_expmat>(A.get_ref());
  }



template<typename T1>
inline
typename
enable_if2
  <
  is_real<typename T1::pod_type>::value,
  bool
  >::result
expmat(Mat<typename T1::elem_type>& B, const Base<typename T1::elem_type,T1>& A)
  {
  arma_extra_debug_sigprint();

  const bool status = op_expmat::apply_direct(B, A);

  if(status == false)
    {
    arma_debug_warn("expmat(): given matrix appears ill-conditioned");
    B.soft_reset();
    return false;
    }

  return true;
  }



//



template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_supported_blas_type<typename T1::elem_type>::value, const Op<T1, op_expmat_sym> >::result
expmat_sym(const Base<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_expmat_sym>(X.get_ref());
  }



template<typename T1>
inline
typename enable_if2< is_supported_blas_type<typename T1::elem_type>::value, bool >::result
expmat_sym(Mat<typename T1::elem_type>& Y, const Base<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  const bool status = op_expmat_sym::apply_direct(Y, X.get_ref());

  if(status == false)
    {
    Y.soft_reset();
    arma_debug_warn("expmat_sym(): transformation failed");
    }

  return status;
  }



//! @}
