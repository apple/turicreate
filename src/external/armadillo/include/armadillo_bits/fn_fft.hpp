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


//! \addtogroup fn_fft
//! @{



// 1D FFT & 1D IFFT



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_real<typename T1::elem_type>::value),
  const mtOp<std::complex<typename T1::pod_type>, T1, op_fft_real>
  >::result
fft(const T1& A)
  {
  arma_extra_debug_sigprint();

  return mtOp<std::complex<typename T1::pod_type>, T1, op_fft_real>(A, uword(0), uword(1));
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_real<typename T1::elem_type>::value),
  const mtOp<std::complex<typename T1::pod_type>, T1, op_fft_real>
  >::result
fft(const T1& A, const uword N)
  {
  arma_extra_debug_sigprint();

  return mtOp<std::complex<typename T1::pod_type>, T1, op_fft_real>(A, N, uword(0));
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_complex_strict<typename T1::elem_type>::value),
  const Op<T1, op_fft_cx>
  >::result
fft(const T1& A)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_fft_cx>(A, uword(0), uword(1));
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_complex_strict<typename T1::elem_type>::value),
  const Op<T1, op_fft_cx>
  >::result
fft(const T1& A, const uword N)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_fft_cx>(A, N, uword(0));
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_complex_strict<typename T1::elem_type>::value),
  const Op<T1, op_ifft_cx>
  >::result
ifft(const T1& A)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_ifft_cx>(A, uword(0), uword(1));
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_complex_strict<typename T1::elem_type>::value),
  const Op<T1, op_ifft_cx>
  >::result
ifft(const T1& A, const uword N)
  {
  arma_extra_debug_sigprint();

  return Op<T1, op_ifft_cx>(A, N, uword(0));
  }



//! @}
