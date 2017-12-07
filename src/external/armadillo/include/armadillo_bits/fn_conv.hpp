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


//! \addtogroup fn_conv
//! @{



//! Convolution, which is also equivalent to polynomial multiplication and FIR digital filtering.

template<typename T1, typename T2>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  const Glue<T1, T2, glue_conv>
  >::result
conv(const T1& A, const T2& B, const char* shape = "full")
  {
  arma_extra_debug_sigprint();

  const char sig = (shape != NULL) ? shape[0] : char(0);

  arma_debug_check( ((sig != 'f') && (sig != 's')), "conv(): unsupported value of 'shape' parameter" );

  const uword mode = (sig == 's') ? uword(1) : uword(0);

  return Glue<T1, T2, glue_conv>(A, B, mode);
  }



template<typename T1, typename T2>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_arma_type<T2>::value && is_same_type<typename T1::elem_type, typename T2::elem_type>::value),
  const Glue<T1, T2, glue_conv2>
  >::result
conv2(const T1& A, const T2& B, const char* shape = "full")
  {
  arma_extra_debug_sigprint();

  const char sig = (shape != NULL) ? shape[0] : char(0);

  arma_debug_check( ((sig != 'f') && (sig != 's')), "conv2(): unsupported value of 'shape' parameter" );

  const uword mode = (sig == 's') ? uword(1) : uword(0);

  return Glue<T1, T2, glue_conv2>(A, B, mode);
  }



//! @}
