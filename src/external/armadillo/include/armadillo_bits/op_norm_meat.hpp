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


//! \addtogroup op_norm
//! @{



template<typename T1>
arma_hot
inline
typename T1::pod_type
op_norm::vec_norm_1(const Proxy<T1>& P, const typename arma_not_cx<typename T1::elem_type>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const bool have_direct_mem = (is_Mat<typename Proxy<T1>::stored_type>::value) || (is_subview_col<typename Proxy<T1>::stored_type>::value);

  if(have_direct_mem)
    {
    const quasi_unwrap<typename Proxy<T1>::stored_type> tmp(P.Q);

    return op_norm::vec_norm_1_direct_std(tmp.M);
    }

  typedef typename T1::pod_type T;

  T acc = T(0);

  if(Proxy<T1>::use_at == false)
    {
    typename Proxy<T1>::ea_type A = P.get_ea();

    const uword N = P.get_n_elem();

    T acc1 = T(0);
    T acc2 = T(0);

    uword i,j;
    for(i=0, j=1; j<N; i+=2, j+=2)
      {
      acc1 += std::abs(A[i]);
      acc2 += std::abs(A[j]);
      }

    if(i < N)
      {
      acc1 += std::abs(A[i]);
      }

    acc = acc1 + acc2;
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_rows == 1)
      {
      for(uword col=0; col<n_cols; ++col)
        {
        acc += std::abs(P.at(0,col));
        }
      }
    else
      {
      T acc1 = T(0);
      T acc2 = T(0);

      for(uword col=0; col<n_cols; ++col)
        {
        uword i,j;

        for(i=0, j=1; j<n_rows; i+=2, j+=2)
          {
          acc1 += std::abs(P.at(i,col));
          acc2 += std::abs(P.at(j,col));
          }

        if(i < n_rows)
          {
          acc1 += std::abs(P.at(i,col));
          }
        }

      acc = acc1 + acc2;
      }
    }

  return acc;
  }



template<typename T1>
arma_hot
inline
typename T1::pod_type
op_norm::vec_norm_1(const Proxy<T1>& P, const typename arma_cx_only<typename T1::elem_type>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;
  typedef typename T1::pod_type   T;

  T acc = T(0);

  if(Proxy<T1>::use_at == false)
    {
    typename Proxy<T1>::ea_type A = P.get_ea();

    const uword N = P.get_n_elem();

    for(uword i=0; i<N; ++i)
      {
      const std::complex<T>& X = A[i];

      const T a = X.real();
      const T b = X.imag();

      acc += std::sqrt( (a*a) + (b*b) );
      }
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_rows == 1)
      {
      for(uword col=0; col<n_cols; ++col)
        {
        const std::complex<T>& X = P.at(0,col);

        const T a = X.real();
        const T b = X.imag();

        acc += std::sqrt( (a*a) + (b*b) );
        }
      }
    else
      {
      for(uword col=0; col<n_cols; ++col)
      for(uword row=0; row<n_rows; ++row)
        {
        const std::complex<T>& X = P.at(row,col);

        const T a = X.real();
        const T b = X.imag();

        acc += std::sqrt( (a*a) + (b*b) );
        }
      }
    }

  if( (acc != T(0)) && arma_isfinite(acc) )
    {
    return acc;
    }
  else
    {
    arma_extra_debug_print("op_norm::vec_norm_1(): detected possible underflow or overflow");

    const quasi_unwrap<typename Proxy<T1>::stored_type> R(P.Q);

    const uword N     = R.M.n_elem;
    const eT*   R_mem = R.M.memptr();

    T max_val = priv::most_neg<T>();

    for(uword i=0; i<N; ++i)
      {
      const std::complex<T>& X = R_mem[i];

      const T a = std::abs(X.real());
      const T b = std::abs(X.imag());

      if(a > max_val)  { max_val = a; }
      if(b > max_val)  { max_val = b; }
      }

    if(max_val == T(0))  { return T(0); }

    T alt_acc = T(0);

    for(uword i=0; i<N; ++i)
      {
      const std::complex<T>& X = R_mem[i];

      const T a = X.real() / max_val;
      const T b = X.imag() / max_val;

      alt_acc += std::sqrt( (a*a) + (b*b) );
      }

    return ( alt_acc * max_val );
    }
  }



template<typename eT>
arma_hot
inline
eT
op_norm::vec_norm_1_direct_std(const Mat<eT>& X)
  {
  arma_extra_debug_sigprint();

  const uword N = X.n_elem;
  const eT*   A = X.memptr();

  if(N < uword(32))
    {
    return op_norm::vec_norm_1_direct_mem(N,A);
    }
  else
    {
    #if defined(ARMA_USE_ATLAS)
      {
      return atlas::cblas_asum(N,A);
      }
    #elif defined(ARMA_USE_BLAS)
      {
      return blas::asum(N,A);
      }
    #else
      {
      return op_norm::vec_norm_1_direct_mem(N,A);
      }
    #endif
    }
  }



template<typename eT>
arma_hot
inline
eT
op_norm::vec_norm_1_direct_mem(const uword N, const eT* A)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_SIMPLE_LOOPS) || (defined(__FINITE_MATH_ONLY__) && (__FINITE_MATH_ONLY__ > 0))
    {
    eT acc1 = eT(0);

    if(memory::is_aligned(A))
      {
      memory::mark_as_aligned(A);

      for(uword i=0; i<N; ++i)  { acc1 += std::abs(A[i]); }
      }
    else
      {
      for(uword i=0; i<N; ++i)  { acc1 += std::abs(A[i]); }
      }

    return acc1;
    }
  #else
    {
    eT acc1 = eT(0);
    eT acc2 = eT(0);

    uword j;

    for(j=1; j<N; j+=2)
      {
      const eT tmp_i = (*A);  A++;
      const eT tmp_j = (*A);  A++;

      acc1 += std::abs(tmp_i);
      acc2 += std::abs(tmp_j);
      }

    if((j-1) < N)
      {
      acc1 += std::abs(*A);
      }

    return (acc1 + acc2);
    }
  #endif
  }



template<typename T1>
arma_hot
inline
typename T1::pod_type
op_norm::vec_norm_2(const Proxy<T1>& P, const typename arma_not_cx<typename T1::elem_type>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const bool have_direct_mem = (is_Mat<typename Proxy<T1>::stored_type>::value) || (is_subview_col<typename Proxy<T1>::stored_type>::value);

  if(have_direct_mem)
    {
    const quasi_unwrap<typename Proxy<T1>::stored_type> tmp(P.Q);

    return op_norm::vec_norm_2_direct_std(tmp.M);
    }

  typedef typename T1::pod_type T;

  T acc = T(0);

  if(Proxy<T1>::use_at == false)
    {
    typename Proxy<T1>::ea_type A = P.get_ea();

    const uword N = P.get_n_elem();

    T acc1 = T(0);
    T acc2 = T(0);

    uword i,j;

    for(i=0, j=1; j<N; i+=2, j+=2)
      {
      const T tmp_i = A[i];
      const T tmp_j = A[j];

      acc1 += tmp_i * tmp_i;
      acc2 += tmp_j * tmp_j;
      }

    if(i < N)
      {
      const T tmp_i = A[i];

      acc1 += tmp_i * tmp_i;
      }

    acc = acc1 + acc2;
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_rows == 1)
      {
      for(uword col=0; col<n_cols; ++col)
        {
        const T tmp = P.at(0,col);

        acc += tmp * tmp;
        }
      }
    else
      {
      for(uword col=0; col<n_cols; ++col)
        {
        uword i,j;
        for(i=0, j=1; j<n_rows; i+=2, j+=2)
          {
          const T tmp_i = P.at(i,col);
          const T tmp_j = P.at(j,col);

          acc += tmp_i * tmp_i;
          acc += tmp_j * tmp_j;
          }

        if(i < n_rows)
          {
          const T tmp_i = P.at(i,col);

          acc += tmp_i * tmp_i;
          }
        }
      }
    }


  const T sqrt_acc = std::sqrt(acc);

  if( (sqrt_acc != T(0)) && arma_isfinite(sqrt_acc) )
    {
    return sqrt_acc;
    }
  else
    {
    arma_extra_debug_print("op_norm::vec_norm_2(): detected possible underflow or overflow");

    const quasi_unwrap<typename Proxy<T1>::stored_type> tmp(P.Q);

    return op_norm::vec_norm_2_direct_robust(tmp.M);
    }
  }



template<typename T1>
arma_hot
inline
typename T1::pod_type
op_norm::vec_norm_2(const Proxy<T1>& P, const typename arma_cx_only<typename T1::elem_type>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;
  typedef typename T1::pod_type   T;

  T acc = T(0);

  if(Proxy<T1>::use_at == false)
    {
    typename Proxy<T1>::ea_type A = P.get_ea();

    const uword N = P.get_n_elem();

    for(uword i=0; i<N; ++i)
      {
      const std::complex<T>& X = A[i];

      const T a = X.real();
      const T b = X.imag();

      acc += (a*a) + (b*b);
      }
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_rows == 1)
      {
      for(uword col=0; col<n_cols; ++col)
        {
        const std::complex<T>& X = P.at(0,col);

        const T a = X.real();
        const T b = X.imag();

        acc += (a*a) + (b*b);
        }
      }
    else
      {
      for(uword col=0; col<n_cols; ++col)
      for(uword row=0; row<n_rows; ++row)
        {
        const std::complex<T>& X = P.at(row,col);

        const T a = X.real();
        const T b = X.imag();

        acc += (a*a) + (b*b);
        }
      }
    }

  const T sqrt_acc = std::sqrt(acc);

  if( (sqrt_acc != T(0)) && arma_isfinite(sqrt_acc) )
    {
    return sqrt_acc;
    }
  else
    {
    arma_extra_debug_print("op_norm::vec_norm_2(): detected possible underflow or overflow");

    const quasi_unwrap<typename Proxy<T1>::stored_type> R(P.Q);

    const uword N     = R.M.n_elem;
    const eT*   R_mem = R.M.memptr();

    T max_val = priv::most_neg<T>();

    for(uword i=0; i<N; ++i)
      {
      const T val_i = std::abs(R_mem[i]);

      if(val_i > max_val)  { max_val = val_i; }
      }

    if(max_val == T(0))  { return T(0); }

    T alt_acc = T(0);

    for(uword i=0; i<N; ++i)
      {
      const T val_i = std::abs(R_mem[i]) / max_val;

      alt_acc += val_i * val_i;
      }

    return ( std::sqrt(alt_acc) * max_val );
    }
  }



template<typename eT>
arma_hot
inline
eT
op_norm::vec_norm_2_direct_std(const Mat<eT>& X)
  {
  arma_extra_debug_sigprint();

  const uword N = X.n_elem;
  const eT*   A = X.memptr();

  eT result;

  if(N < uword(32))
    {
    result = op_norm::vec_norm_2_direct_mem(N,A);
    }
  else
    {
    #if defined(ARMA_USE_ATLAS)
      {
      result = atlas::cblas_nrm2(N,A);
      }
    #elif defined(ARMA_USE_BLAS)
      {
      result = blas::nrm2(N,A);
      }
    #else
      {
      result = op_norm::vec_norm_2_direct_mem(N,A);
      }
    #endif
    }

  if( (result != eT(0)) && arma_isfinite(result) )
    {
    return result;
    }
  else
    {
    arma_extra_debug_print("op_norm::vec_norm_2_direct_std(): detected possible underflow or overflow");

    return op_norm::vec_norm_2_direct_robust(X);
    }
  }



template<typename eT>
arma_hot
inline
eT
op_norm::vec_norm_2_direct_mem(const uword N, const eT* A)
  {
  arma_extra_debug_sigprint();

  eT acc;

  #if defined(ARMA_SIMPLE_LOOPS) || (defined(__FINITE_MATH_ONLY__) && (__FINITE_MATH_ONLY__ > 0))
    {
    eT acc1 = eT(0);

    if(memory::is_aligned(A))
      {
      memory::mark_as_aligned(A);

      for(uword i=0; i<N; ++i)  { const eT tmp_i = A[i];  acc1 += tmp_i * tmp_i; }
      }
    else
      {
      for(uword i=0; i<N; ++i)  { const eT tmp_i = A[i];  acc1 += tmp_i * tmp_i; }
      }

    acc = acc1;
    }
  #else
    {
    eT acc1 = eT(0);
    eT acc2 = eT(0);

    uword j;

    for(j=1; j<N; j+=2)
      {
      const eT tmp_i = (*A);  A++;
      const eT tmp_j = (*A);  A++;

      acc1 += tmp_i * tmp_i;
      acc2 += tmp_j * tmp_j;
      }

    if((j-1) < N)
      {
      const eT tmp_i = (*A);

      acc1 += tmp_i * tmp_i;
      }

    acc = acc1 + acc2;
    }
  #endif

  return std::sqrt(acc);
  }



template<typename eT>
arma_hot
inline
eT
op_norm::vec_norm_2_direct_robust(const Mat<eT>& X)
  {
  arma_extra_debug_sigprint();

  const uword N = X.n_elem;
  const eT*   A = X.memptr();

  eT max_val = priv::most_neg<eT>();

  uword j;

  for(j=1; j<N; j+=2)
    {
    eT val_i = (*A);  A++;
    eT val_j = (*A);  A++;

    val_i = std::abs(val_i);
    val_j = std::abs(val_j);

    if(val_i > max_val)  { max_val = val_i; }
    if(val_j > max_val)  { max_val = val_j; }
    }

  if((j-1) < N)
    {
    const eT val_i = std::abs(*A);

    if(val_i > max_val)  { max_val = val_i; }
    }

  if(max_val == eT(0))  { return eT(0); }

  const eT* B = X.memptr();

  eT acc1 = eT(0);
  eT acc2 = eT(0);

  for(j=1; j<N; j+=2)
    {
    eT val_i = (*B);  B++;
    eT val_j = (*B);  B++;

    val_i /= max_val;
    val_j /= max_val;

    acc1 += val_i * val_i;
    acc2 += val_j * val_j;
    }

  if((j-1) < N)
    {
    const eT val_i = (*B) / max_val;

    acc1 += val_i * val_i;
    }

  return ( std::sqrt(acc1 + acc2) * max_val );
  }



template<typename T1>
arma_hot
inline
typename T1::pod_type
op_norm::vec_norm_k(const Proxy<T1>& P, const int k)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::pod_type T;

  T acc = T(0);

  if(Proxy<T1>::use_at == false)
    {
    typename Proxy<T1>::ea_type A = P.get_ea();

    const uword N = P.get_n_elem();

    uword i,j;

    for(i=0, j=1; j<N; i+=2, j+=2)
      {
      acc += std::pow(std::abs(A[i]), k);
      acc += std::pow(std::abs(A[j]), k);
      }

    if(i < N)
      {
      acc += std::pow(std::abs(A[i]), k);
      }
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_rows != 1)
      {
      for(uword col=0; col < n_cols; ++col)
      for(uword row=0; row < n_rows; ++row)
        {
        acc += std::pow(std::abs(P.at(row,col)), k);
        }
      }
    else
      {
      for(uword col=0; col < n_cols; ++col)
        {
        acc += std::pow(std::abs(P.at(0,col)), k);
        }
      }
    }

  return std::pow(acc, T(1)/T(k));
  }



template<typename T1>
arma_hot
inline
typename T1::pod_type
op_norm::vec_norm_max(const Proxy<T1>& P)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::pod_type T;

  const uword N = P.get_n_elem();

  T max_val = (N != 1) ? priv::most_neg<T>() : std::abs(P[0]);

  if(Proxy<T1>::use_at == false)
    {
    typename Proxy<T1>::ea_type A = P.get_ea();

    uword i,j;
    for(i=0, j=1; j<N; i+=2, j+=2)
      {
      const T tmp_i = std::abs(A[i]);
      const T tmp_j = std::abs(A[j]);

      if(max_val < tmp_i) { max_val = tmp_i; }
      if(max_val < tmp_j) { max_val = tmp_j; }
      }

    if(i < N)
      {
      const T tmp_i = std::abs(A[i]);

      if(max_val < tmp_i) { max_val = tmp_i; }
      }
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_rows != 1)
      {
      for(uword col=0; col < n_cols; ++col)
      for(uword row=0; row < n_rows; ++row)
        {
        const T tmp = std::abs(P.at(row,col));

        if(max_val < tmp) { max_val = tmp; }
        }
      }
    else
      {
      for(uword col=0; col < n_cols; ++col)
        {
        const T tmp = std::abs(P.at(0,col));

        if(max_val < tmp) { max_val = tmp; }
        }
      }
    }

  return max_val;
  }



template<typename T1>
arma_hot
inline
typename T1::pod_type
op_norm::vec_norm_min(const Proxy<T1>& P)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::pod_type T;

  const uword N = P.get_n_elem();

  T min_val = (N != 1) ? priv::most_pos<T>() : std::abs(P[0]);

  if(Proxy<T1>::use_at == false)
    {
    typename Proxy<T1>::ea_type A = P.get_ea();

    uword i,j;
    for(i=0, j=1; j<N; i+=2, j+=2)
      {
      const T tmp_i = std::abs(A[i]);
      const T tmp_j = std::abs(A[j]);

      if(min_val > tmp_i) { min_val = tmp_i; }
      if(min_val > tmp_j) { min_val = tmp_j; }
      }

    if(i < N)
      {
      const T tmp_i = std::abs(A[i]);

      if(min_val > tmp_i) { min_val = tmp_i; }
      }
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_rows != 1)
      {
      for(uword col=0; col < n_cols; ++col)
      for(uword row=0; row < n_rows; ++row)
        {
        const T tmp = std::abs(P.at(row,col));

        if(min_val > tmp) { min_val = tmp; }
        }
      }
    else
      {
      for(uword col=0; col < n_cols; ++col)
        {
        const T tmp = std::abs(P.at(0,col));

        if(min_val > tmp) { min_val = tmp; }
        }
      }
    }

  return min_val;
  }



template<typename T1>
inline
typename T1::pod_type
op_norm::mat_norm_1(const Proxy<T1>& P)
  {
  arma_extra_debug_sigprint();

  // TODO: this can be sped up with a dedicated implementation
  return as_scalar( max( sum(abs(P.Q), 0), 1) );
  }



template<typename T1>
inline
typename T1::pod_type
op_norm::mat_norm_2(const Proxy<T1>& P)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::pod_type   T;

  Col<T> S;
  svd(S, P.Q);

  return (S.n_elem > 0) ? max(S) : T(0);
  }



template<typename T1>
inline
typename T1::pod_type
op_norm::mat_norm_inf(const Proxy<T1>& P)
  {
  arma_extra_debug_sigprint();

  // TODO: this can be sped up with a dedicated implementation
  return as_scalar( max( sum(abs(P.Q), 1), 0) );
  }



//
// norms for sparse matrices



template<typename T1>
inline
typename T1::pod_type
op_norm::mat_norm_1(const SpProxy<T1>& P)
  {
  arma_extra_debug_sigprint();

  // TODO: this can be sped up with a dedicated implementation
  return as_scalar( max( sum(abs(P.Q), 0), 1) );
  }



template<typename T1>
inline
typename T1::pod_type
op_norm::mat_norm_2(const SpProxy<T1>& P, const typename arma_real_only<typename T1::elem_type>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  // norm = sqrt( largest eigenvalue of (A^H)*A ), where ^H is the conjugate transpose
  // http://math.stackexchange.com/questions/4368/computing-the-largest-eigenvalue-of-a-very-large-sparse-matrix

  typedef typename T1::elem_type eT;
  typedef typename T1::pod_type   T;

  const unwrap_spmat<typename SpProxy<T1>::stored_type> tmp(P.Q);

  const SpMat<eT>& A = tmp.M;
  const SpMat<eT>  B = trans(A);

  const SpMat<eT>  C = (A.n_rows <= A.n_cols) ? (A*B) : (B*A);

  const Col<T> eigval = eigs_sym(C, 1);

  return (eigval.n_elem > 0) ? std::sqrt(eigval[0]) : T(0);
  }



template<typename T1>
inline
typename T1::pod_type
op_norm::mat_norm_2(const SpProxy<T1>& P, const typename arma_cx_only<typename T1::elem_type>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  //typedef typename T1::elem_type eT;
  typedef typename T1::pod_type   T;

  arma_ignore(P);
  arma_stop_logic_error("norm(): unimplemented norm type for complex sparse matrices");

  return T(0);

  // const unwrap_spmat<typename SpProxy<T1>::stored_type> tmp(P.Q);
  //
  // const SpMat<eT>& A = tmp.M;
  // const SpMat<eT>  B = trans(A);
  //
  // const SpMat<eT>  C = (A.n_rows <= A.n_cols) ? (A*B) : (B*A);
  //
  // const Col<eT> eigval = eigs_gen(C, 1);
  }



template<typename T1>
inline
typename T1::pod_type
op_norm::mat_norm_inf(const SpProxy<T1>& P)
  {
  arma_extra_debug_sigprint();

  // TODO: this can be sped up with a dedicated implementation
  return as_scalar( max( sum(abs(P.Q), 1), 0) );
  }



//! @}
