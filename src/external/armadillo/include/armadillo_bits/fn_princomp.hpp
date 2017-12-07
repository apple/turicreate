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


//! \addtogroup fn_princomp
//! @{



//! \brief
//! principal component analysis -- 4 arguments version
//! coeff_out    -> principal component coefficients
//! score_out    -> projected samples
//! latent_out   -> eigenvalues of principal vectors
//! tsquared_out -> Hotelling's T^2 statistic
template<typename T1>
inline
bool
princomp
  (
         Mat<typename T1::elem_type>&    coeff_out,
         Mat<typename T1::elem_type>&    score_out,
         Col<typename T1::pod_type>&     latent_out,
         Col<typename T1::elem_type>&    tsquared_out,
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const bool status = op_princomp::direct_princomp(coeff_out, score_out, latent_out, tsquared_out, X);

  if(status == false)
    {
    coeff_out.soft_reset();
    score_out.soft_reset();
    latent_out.soft_reset();
    tsquared_out.soft_reset();

    arma_debug_warn("princomp(): decomposition failed");
    }

  return status;
  }



//! \brief
//! principal component analysis -- 3 arguments version
//! coeff_out    -> principal component coefficients
//! score_out    -> projected samples
//! latent_out   -> eigenvalues of principal vectors
template<typename T1>
inline
bool
princomp
  (
         Mat<typename T1::elem_type>&    coeff_out,
         Mat<typename T1::elem_type>&    score_out,
         Col<typename T1::pod_type>&     latent_out,
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const bool status = op_princomp::direct_princomp(coeff_out, score_out, latent_out, X);

  if(status == false)
    {
    coeff_out.soft_reset();
    score_out.soft_reset();
    latent_out.soft_reset();

    arma_debug_warn("princomp(): decomposition failed");
    }

  return status;
  }



//! \brief
//! principal component analysis -- 2 arguments version
//! coeff_out    -> principal component coefficients
//! score_out    -> projected samples
template<typename T1>
inline
bool
princomp
  (
         Mat<typename T1::elem_type>&    coeff_out,
         Mat<typename T1::elem_type>&    score_out,
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const bool status = op_princomp::direct_princomp(coeff_out, score_out, X);

  if(status == false)
    {
    coeff_out.soft_reset();
    score_out.soft_reset();

    arma_debug_warn("princomp(): decomposition failed");
    }

  return status;
  }



//! \brief
//! principal component analysis -- 1 argument version
//! coeff_out    -> principal component coefficients
template<typename T1>
inline
bool
princomp
  (
         Mat<typename T1::elem_type>&    coeff_out,
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const bool status = op_princomp::direct_princomp(coeff_out, X);

  if(status == false)
    {
    coeff_out.soft_reset();

    arma_debug_warn("princomp(): decomposition failed");
    }

  return status;
  }



template<typename T1>
arma_warn_unused
inline
const Op<T1, op_princomp>
princomp
  (
  const Base<typename T1::elem_type,T1>& X,
  const typename arma_blas_type_only<typename T1::elem_type>::result* junk = 0
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return Op<T1, op_princomp>(X.get_ref());
  }



//! @}
