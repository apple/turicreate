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



//! \addtogroup fn_eps
//! @{



template<typename T1>
arma_warn_unused
inline
const eOp<T1, eop_eps>
eps(const Base<typename T1::elem_type, T1>& X, const typename arma_not_cx<typename T1::elem_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  return eOp<T1, eop_eps>(X.get_ref());
  }



template<typename T1>
arma_warn_unused
inline
Mat< typename T1::pod_type >
eps(const Base< std::complex<typename T1::pod_type>, T1>& X, const typename arma_cx_only<typename T1::elem_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::pod_type   T;
  typedef typename T1::elem_type eT;

  const unwrap<T1>   tmp(X.get_ref());
  const Mat<eT>& A = tmp.M;

  Mat<T> out(A.n_rows, A.n_cols);

         T* out_mem = out.memptr();
  const eT*   A_mem =   A.memptr();

  const uword n_elem = A.n_elem;

  for(uword i=0; i<n_elem; ++i)
    {
    out_mem[i] = eop_aux::direct_eps( A_mem[i] );
    }

  return out;
  }



template<typename eT>
arma_warn_unused
arma_inline
typename arma_integral_only<eT>::result
eps(const eT& x)
  {
  arma_ignore(x);

  return eT(0);
  }



template<typename eT>
arma_warn_unused
arma_inline
typename arma_real_only<eT>::result
eps(const eT& x)
  {
  return eop_aux::direct_eps(x);
  }



template<typename T>
arma_warn_unused
arma_inline
typename arma_real_only<T>::result
eps(const std::complex<T>& x)
  {
  return eop_aux::direct_eps(x);
  }



//! @}
