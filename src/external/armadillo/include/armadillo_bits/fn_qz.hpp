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


//! \addtogroup fn_qz
//! @{



//! QZ decomposition for pair of N-by-N general matrices A and B
template<typename T1, typename T2>
inline
typename
enable_if2
  <
  is_supported_blas_type<typename T1::elem_type>::value,
  bool
  >::result
qz
  (
         Mat<typename T1::elem_type>&    AA,
         Mat<typename T1::elem_type>&    BB,
         Mat<typename T1::elem_type>&    Q,
         Mat<typename T1::elem_type>&    Z,
  const Base<typename T1::elem_type,T1>& A_expr,
  const Base<typename T1::elem_type,T2>& B_expr,
  const char*                            select = "none"
  )
  {
  arma_extra_debug_sigprint();

  const char sig = (select != NULL) ? select[0] : char(0);

  arma_debug_check( ( (sig != 'n') && (sig != 'l') && (sig != 'r') && (sig != 'i') && (sig != 'o') ), "qz(): unknown select form" );

  const bool status = auxlib::qz(AA, BB, Q, Z, A_expr.get_ref(), B_expr.get_ref(), sig);

  if(status == false)
    {
    AA.reset();
    BB.reset();
    Q.reset();
    Z.reset();
    arma_debug_warn("qz(): decomposition failed");
    }

  return status;
  }



//! @}
