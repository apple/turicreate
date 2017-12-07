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


//! \addtogroup fn_pinv
//! @{



template<typename T1>
arma_warn_unused
inline
typename enable_if2< is_real<typename T1::pod_type>::value, const Op<T1, op_pinv> >::result
pinv
  (
  const Base<typename T1::elem_type,T1>& X,
  const typename T1::pod_type            tol    = 0.0,
  const char*                            method = "dc"
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const char sig = (method != NULL) ? method[0] : char(0);

  arma_debug_check( ((sig != 's') && (sig != 'd')), "pinv(): unknown method specified" );

  return (sig == 'd') ? Op<T1, op_pinv>(X.get_ref(), eT(tol), 1, 0) : Op<T1, op_pinv>(X.get_ref(), eT(tol), 0, 0);
  }



template<typename T1>
inline
typename enable_if2< is_real<typename T1::pod_type>::value, bool >::result
pinv
  (
         Mat<typename T1::elem_type>&    out,
  const Base<typename T1::elem_type,T1>& X,
  const typename T1::pod_type            tol    = 0.0,
  const char*                            method = "dc"
  )
  {
  arma_extra_debug_sigprint();

  const char sig = (method != NULL) ? method[0] : char(0);

  arma_debug_check( ((sig != 's') && (sig != 'd')), "pinv(): unknown method specified" );

  const bool use_divide_and_conquer = (sig == 'd');

  const bool status = op_pinv::apply_direct(out, X.get_ref(), tol, use_divide_and_conquer);

  if(status == false)
    {
    arma_debug_warn("pinv(): svd failed");
    }

  return status;
  }



//! @}
