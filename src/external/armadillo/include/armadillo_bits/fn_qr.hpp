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


//! \addtogroup fn_qr
//! @{



//! QR decomposition
template<typename T1>
inline
bool
qr
  (
         Mat<typename T1::elem_type>&    Q,
         Mat<typename T1::elem_type>&    R,
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  arma_debug_check( (&Q == &R), "qr(): Q and R are the same object");

  const bool status = auxlib::qr(Q, R, X);

  if(status == false)
    {
    Q.soft_reset();
    R.soft_reset();
    arma_debug_warn("qr(): decomposition failed");
    }

  return status;
  }



//! economical QR decomposition
template<typename T1>
inline
bool
qr_econ
  (
         Mat<typename T1::elem_type>&    Q,
         Mat<typename T1::elem_type>&    R,
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  arma_debug_check( (&Q == &R), "qr_econ(): Q and R are the same object");

  const bool status = auxlib::qr_econ(Q, R, X);

  if(status == false)
    {
    Q.soft_reset();
    R.soft_reset();
    arma_debug_warn("qr_econ(): decomposition failed");
    }

  return status;
  }



//! @}
