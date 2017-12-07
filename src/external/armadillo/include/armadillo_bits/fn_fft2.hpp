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


//! \addtogroup fn_fft2
//! @{



// 2D FFT & 2D IFFT



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  Mat< std::complex<typename T1::pod_type> >
  >::result
fft2(const T1& A)
  {
  arma_extra_debug_sigprint();

  // not exactly efficient, but "better-than-nothing" implementation

  typedef typename T1::pod_type T;

  Mat< std::complex<T> > B = fft(A);

  // for square matrices, strans() will work out that an inplace transpose can be done,
  // hence we can potentially avoid creating a temporary matrix

  B = strans(B);

  return strans( fft(B) );
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  is_arma_type<T1>::value,
  Mat< std::complex<typename T1::pod_type> >
  >::result
fft2(const T1& A, const uword n_rows, const uword n_cols)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T1>   tmp(A);
  const Mat<eT>& B = tmp.M;

  const bool do_resize = (B.n_rows != n_rows) || (B.n_cols != n_cols);

  return fft2( do_resize ? resize(B,n_rows,n_cols) : B );
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_complex_strict<typename T1::elem_type>::value),
  Mat< std::complex<typename T1::pod_type> >
  >::result
ifft2(const T1& A)
  {
  arma_extra_debug_sigprint();

  // not exactly efficient, but "better-than-nothing" implementation

  typedef typename T1::pod_type T;

  Mat< std::complex<T> > B = ifft(A);

  // for square matrices, strans() will work out that an inplace transpose can be done,
  // hence we can potentially avoid creating a temporary matrix

  B = strans(B);

  return strans( ifft(B) );
  }



template<typename T1>
arma_warn_unused
inline
typename
enable_if2
  <
  (is_arma_type<T1>::value && is_complex_strict<typename T1::elem_type>::value),
  Mat< std::complex<typename T1::pod_type> >
  >::result
ifft2(const T1& A, const uword n_rows, const uword n_cols)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T1>   tmp(A);
  const Mat<eT>& B = tmp.M;

  const bool do_resize = (B.n_rows != n_rows) || (B.n_cols != n_cols);

  return ifft2( do_resize ? resize(B,n_rows,n_cols) : B );
  }



//! @}
