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


//! \addtogroup fn_orth_null
//! @{



template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_real<typename T1::pod_type>::value, const Op<T1, op_orth> >::result
orth(const Base<typename T1::elem_type, T1>& X, const typename T1::pod_type tol = 0.0)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  return Op<T1, op_orth>(X.get_ref(), eT(tol));
  }



template<typename T1>
inline
typename enable_if2< is_real<typename T1::pod_type>::value, bool >::result
orth(Mat<typename T1::elem_type>& out, const Base<typename T1::elem_type, T1>& X, const typename T1::pod_type tol = 0.0)
  {
  arma_extra_debug_sigprint();

  const bool status = op_orth::apply_direct(out, X.get_ref(), tol);

  if(status == false)
    {
    arma_debug_warn("orth(): svd failed");
    }

  return status;
  }



//



template<typename T1>
arma_warn_unused
arma_inline
typename enable_if2< is_real<typename T1::pod_type>::value, const Op<T1, op_null> >::result
null(const Base<typename T1::elem_type, T1>& X, const typename T1::pod_type tol = 0.0)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  return Op<T1, op_null>(X.get_ref(), eT(tol));
  }



template<typename T1>
inline
typename enable_if2< is_real<typename T1::pod_type>::value, bool >::result
null(Mat<typename T1::elem_type>& out, const Base<typename T1::elem_type, T1>& X, const typename T1::pod_type tol = 0.0)
  {
  arma_extra_debug_sigprint();

  const bool status = op_null::apply_direct(out, X.get_ref(), tol);

  if(status == false)
    {
    arma_debug_warn("null(): svd failed");
    }

  return status;
  }



//! @}
