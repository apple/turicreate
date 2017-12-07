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


//! \addtogroup op_dot
//! @{



//! for two arrays, generic version for non-complex values
template<typename eT>
arma_hot
arma_inline
typename arma_not_cx<eT>::result
op_dot::direct_dot_arma(const uword n_elem, const eT* const A, const eT* const B)
  {
  arma_extra_debug_sigprint();

  #if defined(__FINITE_MATH_ONLY__) && (__FINITE_MATH_ONLY__ > 0)
    {
    eT val = eT(0);

    for(uword i=0; i<n_elem; ++i)
      {
      val += A[i] * B[i];
      }

    return val;
    }
  #else
    {
    eT val1 = eT(0);
    eT val2 = eT(0);

    uword i, j;

    for(i=0, j=1; j<n_elem; i+=2, j+=2)
      {
      val1 += A[i] * B[i];
      val2 += A[j] * B[j];
      }

    if(i < n_elem)
      {
      val1 += A[i] * B[i];
      }

    return val1 + val2;
    }
  #endif
  }



//! for two arrays, generic version for complex values
template<typename eT>
arma_hot
inline
typename arma_cx_only<eT>::result
op_dot::direct_dot_arma(const uword n_elem, const eT* const A, const eT* const B)
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  T val_real = T(0);
  T val_imag = T(0);

  for(uword i=0; i<n_elem; ++i)
    {
    const std::complex<T>& X = A[i];
    const std::complex<T>& Y = B[i];

    const T a = X.real();
    const T b = X.imag();

    const T c = Y.real();
    const T d = Y.imag();

    val_real += (a*c) - (b*d);
    val_imag += (a*d) + (b*c);
    }

  return std::complex<T>(val_real, val_imag);
  }



//! for two arrays, float and double version
template<typename eT>
arma_hot
inline
typename arma_real_only<eT>::result
op_dot::direct_dot(const uword n_elem, const eT* const A, const eT* const B)
  {
  arma_extra_debug_sigprint();

  if( n_elem <= 32u )
    {
    return op_dot::direct_dot_arma(n_elem, A, B);
    }
  else
    {
    #if defined(ARMA_USE_ATLAS)
      {
      arma_extra_debug_print("atlas::cblas_dot()");

      return atlas::cblas_dot(n_elem, A, B);
      }
    #elif defined(ARMA_USE_BLAS)
      {
      arma_extra_debug_print("blas::dot()");

      return blas::dot(n_elem, A, B);
      }
    #else
      {
      return op_dot::direct_dot_arma(n_elem, A, B);
      }
    #endif
    }
  }



//! for two arrays, complex version
template<typename eT>
inline
arma_hot
typename arma_cx_only<eT>::result
op_dot::direct_dot(const uword n_elem, const eT* const A, const eT* const B)
  {
  if( n_elem <= 16u )
    {
    return op_dot::direct_dot_arma(n_elem, A, B);
    }
  else
    {
    #if defined(ARMA_USE_ATLAS)
      {
      arma_extra_debug_print("atlas::cblas_cx_dot()");

      return atlas::cblas_cx_dot(n_elem, A, B);
      }
    #elif defined(ARMA_USE_BLAS)
      {
      arma_extra_debug_print("blas::dot()");

      return blas::dot(n_elem, A, B);
      }
    #else
      {
      return op_dot::direct_dot_arma(n_elem, A, B);
      }
    #endif
    }
  }



//! for two arrays, integral version
template<typename eT>
arma_hot
inline
typename arma_integral_only<eT>::result
op_dot::direct_dot(const uword n_elem, const eT* const A, const eT* const B)
  {
  return op_dot::direct_dot_arma(n_elem, A, B);
  }




//! for three arrays
template<typename eT>
arma_hot
inline
eT
op_dot::direct_dot(const uword n_elem, const eT* const A, const eT* const B, const eT* C)
  {
  arma_extra_debug_sigprint();

  eT val = eT(0);

  for(uword i=0; i<n_elem; ++i)
    {
    val += A[i] * B[i] * C[i];
    }

  return val;
  }



template<typename T1, typename T2>
arma_hot
inline
typename T1::elem_type
op_dot::apply(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  const bool use_at = (Proxy<T1>::use_at) || (Proxy<T2>::use_at);

  const bool have_direct_mem = ((is_Mat<T1>::value || is_subview_col<T1>::value) && (is_Mat<T2>::value || is_subview_col<T2>::value));

  if(use_at || have_direct_mem)
    {
    const quasi_unwrap<T1> A(X);
    const quasi_unwrap<T2> B(Y);

    arma_debug_check( (A.M.n_elem != B.M.n_elem), "dot(): objects must have the same number of elements" );

    return op_dot::direct_dot(A.M.n_elem, A.M.memptr(), B.M.memptr());
    }
  else
    {
    if(is_subview_row<T1>::value && is_subview_row<T2>::value)
      {
      typedef typename T1::elem_type eT;

      const subview_row<eT>& A = reinterpret_cast< const subview_row<eT>& >(X);
      const subview_row<eT>& B = reinterpret_cast< const subview_row<eT>& >(Y);

      if( (A.m.n_rows == 1) && (B.m.n_rows == 1) )
        {
        arma_debug_check( (A.n_elem != B.n_elem), "dot(): objects must have the same number of elements" );

        const eT* A_mem = A.m.memptr();
        const eT* B_mem = B.m.memptr();

        return op_dot::direct_dot(A.n_elem, &A_mem[A.aux_col1], &B_mem[B.aux_col1]);
        }
      }

    const Proxy<T1> PA(X);
    const Proxy<T2> PB(Y);

    arma_debug_check( (PA.get_n_elem() != PB.get_n_elem()), "dot(): objects must have the same number of elements" );

    if(is_Mat<typename Proxy<T1>::stored_type>::value && is_Mat<typename Proxy<T2>::stored_type>::value)
      {
      const quasi_unwrap<typename Proxy<T1>::stored_type> A(PA.Q);
      const quasi_unwrap<typename Proxy<T2>::stored_type> B(PB.Q);

      return op_dot::direct_dot(A.M.n_elem, A.M.memptr(), B.M.memptr());
      }

    return op_dot::apply_proxy(PA,PB);
    }
  }



template<typename T1, typename T2>
arma_hot
inline
typename arma_not_cx<typename T1::elem_type>::result
op_dot::apply_proxy(const Proxy<T1>& PA, const Proxy<T2>& PB)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type      eT;
  typedef typename Proxy<T1>::ea_type ea_type1;
  typedef typename Proxy<T2>::ea_type ea_type2;

  const uword N = PA.get_n_elem();

  ea_type1 A = PA.get_ea();
  ea_type2 B = PB.get_ea();

  eT val1 = eT(0);
  eT val2 = eT(0);

  uword i,j;

  for(i=0, j=1; j<N; i+=2, j+=2)
    {
    val1 += A[i] * B[i];
    val2 += A[j] * B[j];
    }

  if(i < N)
    {
    val1 += A[i] * B[i];
    }

  return val1 + val2;
  }



template<typename T1, typename T2>
arma_hot
inline
typename arma_cx_only<typename T1::elem_type>::result
op_dot::apply_proxy(const Proxy<T1>& PA, const Proxy<T2>& PB)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result  T;

  typedef typename Proxy<T1>::ea_type ea_type1;
  typedef typename Proxy<T2>::ea_type ea_type2;

  const uword N = PA.get_n_elem();

  ea_type1 A = PA.get_ea();
  ea_type2 B = PB.get_ea();

  T val_real = T(0);
  T val_imag = T(0);

  for(uword i=0; i<N; ++i)
    {
    const std::complex<T> xx = A[i];
    const std::complex<T> yy = B[i];

    const T a = xx.real();
    const T b = xx.imag();

    const T c = yy.real();
    const T d = yy.imag();

    val_real += (a*c) - (b*d);
    val_imag += (a*d) + (b*c);
    }

  return std::complex<T>(val_real, val_imag);
  }



//
// op_norm_dot



template<typename T1, typename T2>
arma_hot
inline
typename T1::elem_type
op_norm_dot::apply(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;
  typedef typename T1::pod_type   T;

  const quasi_unwrap<T1> tmp1(X);
  const quasi_unwrap<T2> tmp2(Y);

  const Col<eT> A( const_cast<eT*>(tmp1.M.memptr()), tmp1.M.n_elem, false );
  const Col<eT> B( const_cast<eT*>(tmp2.M.memptr()), tmp2.M.n_elem, false );

  arma_debug_check( (A.n_elem != B.n_elem), "norm_dot(): objects must have the same number of elements" );

  const T denom = norm(A,2) * norm(B,2);

  return (denom != T(0)) ? ( op_dot::apply(A,B) / denom ) : eT(0);
  }



//
// op_cdot



template<typename eT>
arma_hot
inline
eT
op_cdot::direct_cdot_arma(const uword n_elem, const eT* const A, const eT* const B)
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  T val_real = T(0);
  T val_imag = T(0);

  for(uword i=0; i<n_elem; ++i)
    {
    const std::complex<T>& X = A[i];
    const std::complex<T>& Y = B[i];

    const T a = X.real();
    const T b = X.imag();

    const T c = Y.real();
    const T d = Y.imag();

    val_real += (a*c) + (b*d);
    val_imag += (a*d) - (b*c);
    }

  return std::complex<T>(val_real, val_imag);
  }



template<typename eT>
arma_hot
inline
eT
op_cdot::direct_cdot(const uword n_elem, const eT* const A, const eT* const B)
  {
  arma_extra_debug_sigprint();

  if( n_elem <= 32u )
    {
    return op_cdot::direct_cdot_arma(n_elem, A, B);
    }
  else
    {
    #if defined(ARMA_USE_BLAS)
      {
      arma_extra_debug_print("blas::gemv()");

      // using gemv() workaround due to compatibility issues with cdotc() and zdotc()

      const char trans   = 'C';

      const blas_int m   = blas_int(n_elem);
      const blas_int n   = 1;
      //const blas_int lda = (n_elem > 0) ? blas_int(n_elem) : blas_int(1);
      const blas_int inc = 1;

      const eT alpha     = eT(1);
      const eT beta      = eT(0);

      eT result[2];  // paranoia: using two elements instead of one

      //blas::gemv(&trans, &m, &n, &alpha, A, &lda, B, &inc, &beta, &result[0], &inc);
      blas::gemv(&trans, &m, &n, &alpha, A, &m, B, &inc, &beta, &result[0], &inc);

      return result[0];
      }
    #elif defined(ARMA_USE_ATLAS)
      {
      // TODO: use dedicated atlas functions cblas_cdotc_sub() and cblas_zdotc_sub() and retune threshold

      return op_cdot::direct_cdot_arma(n_elem, A, B);
      }
    #else
      {
      return op_cdot::direct_cdot_arma(n_elem, A, B);
      }
    #endif
    }
  }



template<typename T1, typename T2>
arma_hot
inline
typename T1::elem_type
op_cdot::apply(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  if( (is_Mat<T1>::value == true) && (is_Mat<T2>::value == true) )
    {
    return op_cdot::apply_unwrap(X,Y);
    }
  else
    {
    return op_cdot::apply_proxy(X,Y);
    }
  }



template<typename T1, typename T2>
arma_hot
inline
typename T1::elem_type
op_cdot::apply_unwrap(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T1> tmp1(X);
  const unwrap<T2> tmp2(Y);

  const Mat<eT>& A = tmp1.M;
  const Mat<eT>& B = tmp2.M;

  arma_debug_check( (A.n_elem != B.n_elem), "cdot(): objects must have the same number of elements" );

  return op_cdot::direct_cdot( A.n_elem, A.mem, B.mem );
  }



template<typename T1, typename T2>
arma_hot
inline
typename T1::elem_type
op_cdot::apply_proxy(const T1& X, const T2& Y)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result  T;

  typedef typename Proxy<T1>::ea_type ea_type1;
  typedef typename Proxy<T2>::ea_type ea_type2;

  const bool use_at = (Proxy<T1>::use_at) || (Proxy<T2>::use_at);

  if(use_at == false)
    {
    const Proxy<T1> PA(X);
    const Proxy<T2> PB(Y);

    const uword N = PA.get_n_elem();

    arma_debug_check( (N != PB.get_n_elem()), "cdot(): objects must have the same number of elements" );

    ea_type1 A = PA.get_ea();
    ea_type2 B = PB.get_ea();

    T val_real = T(0);
    T val_imag = T(0);

    for(uword i=0; i<N; ++i)
      {
      const std::complex<T> AA = A[i];
      const std::complex<T> BB = B[i];

      const T a = AA.real();
      const T b = AA.imag();

      const T c = BB.real();
      const T d = BB.imag();

      val_real += (a*c) + (b*d);
      val_imag += (a*d) - (b*c);
      }

    return std::complex<T>(val_real, val_imag);
    }
  else
    {
    return op_cdot::apply_unwrap( X, Y );
    }
  }



template<typename T1, typename T2>
arma_hot
inline
typename promote_type<typename T1::elem_type, typename T2::elem_type>::result
op_dot_mixed::apply(const T1& A, const T2& B)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type in_eT1;
  typedef typename T2::elem_type in_eT2;

  typedef typename promote_type<in_eT1, in_eT2>::result out_eT;

  const Proxy<T1> PA(A);
  const Proxy<T2> PB(B);

  const uword N = PA.get_n_elem();

  arma_debug_check( (N != PB.get_n_elem()), "dot(): objects must have the same number of elements" );

  out_eT acc = out_eT(0);

  for(uword i=0; i < N; ++i)
    {
    acc += upgrade_val<in_eT1,in_eT2>::apply(PA[i]) * upgrade_val<in_eT1,in_eT2>::apply(PB[i]);
    }

  return acc;
  }



//! @}
