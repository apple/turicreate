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


//! \addtogroup fn_cond
//! @{


template<typename T1>
arma_warn_unused
inline
typename enable_if2<is_supported_blas_type<typename T1::elem_type>::value, typename T1::pod_type>::result
cond(const Base<typename T1::elem_type, T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::pod_type T;

  Col<T> S;

  const bool status = auxlib::svd_dc(S, X);

  if(status == false)
    {
    arma_debug_warn("cond(): svd failed");

    return T(0);
    }

  if(S.n_elem > 0)
    {
    return T( max(S) / min(S) );
    }
  else
    {
    return T(0);
    }
  }



template<typename T1>
arma_warn_unused
inline
typename enable_if2<is_supported_blas_type<typename T1::elem_type>::value, typename T1::pod_type>::result
rcond(const Base<typename T1::elem_type, T1>& X)
  {
  arma_extra_debug_sigprint();

  return auxlib::rcond(X.get_ref());
  }



//! @}
