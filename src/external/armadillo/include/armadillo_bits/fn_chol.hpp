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


//! \addtogroup fn_chol
//! @{



template<typename T1>
arma_warn_unused
inline
typename enable_if2< is_supported_blas_type<typename T1::elem_type>::value, const Op<T1, op_chol> >::result
chol
  (
  const Base<typename T1::elem_type,T1>& X,
  const char* layout = "upper"
  )
  {
  arma_extra_debug_sigprint();

  const char sig = (layout != NULL) ? layout[0] : char(0);

  arma_debug_check( ((sig != 'u') && (sig != 'l')), "chol(): layout must be \"upper\" or \"lower\"" );

  return Op<T1, op_chol>(X.get_ref(), ((sig == 'u') ? 0 : 1), 0 );
  }



template<typename T1>
inline
typename enable_if2< is_supported_blas_type<typename T1::elem_type>::value, bool >::result
chol
  (
         Mat<typename T1::elem_type>&    out,
  const Base<typename T1::elem_type,T1>& X,
  const char* layout = "upper"
  )
  {
  arma_extra_debug_sigprint();

  const char sig = (layout != NULL) ? layout[0] : char(0);

  arma_debug_check( ((sig != 'u') && (sig != 'l')), "chol(): layout must be \"upper\" or \"lower\"" );

  const bool status = auxlib::chol(out, X.get_ref(), ((sig == 'u') ? 0 : 1));

  if(status == false)
    {
    out.soft_reset();
    arma_debug_warn("chol(): decomposition failed");
    }

  return status;
  }



//! @}
