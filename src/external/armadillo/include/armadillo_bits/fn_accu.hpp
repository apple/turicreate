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


//! \addtogroup fn_accu
//! @{



template<typename T1>
arma_hot
inline
typename T1::elem_type
accu_proxy_linear(const Proxy<T1>& P)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  eT val = eT(0);

  typename Proxy<T1>::ea_type Pea = P.get_ea();

  const uword n_elem = P.get_n_elem();

  if( arma_config::openmp && Proxy<T1>::use_mp && mp_gate<eT>::eval(n_elem) )
    {
    #if defined(ARMA_USE_OPENMP)
      {
      // NOTE: using parallelisation with manual reduction workaround to take into account complex numbers;
      // NOTE: OpenMP versions lower than 4.0 do not support user-defined reduction

      const int   n_threads_max = mp_thread_limit::get();
      const uword n_threads_use = (std::min)(uword(podarray_prealloc_n_elem::val), uword(n_threads_max));
      const uword chunk_size    = n_elem / n_threads_use;

      podarray<eT> partial_accs(n_threads_use);

      #pragma omp parallel for schedule(static) num_threads(int(n_threads_use))
      for(uword thread_id=0; thread_id < n_threads_use; ++thread_id)
        {
        const uword start = (thread_id+0) * chunk_size;
        const uword endp1 = (thread_id+1) * chunk_size;

        eT acc = eT(0);
        for(uword i=start; i < endp1; ++i)  { acc += Pea[i]; }

        partial_accs[thread_id] = acc;
        }

      for(uword thread_id=0; thread_id < n_threads_use; ++thread_id)  { val += partial_accs[thread_id]; }

      for(uword i=(n_threads_use*chunk_size); i < n_elem; ++i)  { val += Pea[i]; }
      }
    #endif
    }
  else
    {
    #if defined(__FINITE_MATH_ONLY__) && (__FINITE_MATH_ONLY__ > 0)
      {
      if(P.is_aligned())
        {
        typename Proxy<T1>::aligned_ea_type Pea_aligned = P.get_aligned_ea();

        for(uword i=0; i<n_elem; ++i)  { val += Pea_aligned.at_alt(i); }
        }
      else
        {
        for(uword i=0; i<n_elem; ++i)  { val += Pea[i]; }
        }
      }
    #else
      {
      eT val1 = eT(0);
      eT val2 = eT(0);

      uword i,j;
      for(i=0, j=1; j < n_elem; i+=2, j+=2)  { val1 += Pea[i]; val2 += Pea[j]; }

      if(i < n_elem)  { val1 += Pea[i]; }

      val = val1 + val2;
      }
    #endif
    }

  return val;
  }



template<typename T1>
arma_hot
inline
typename T1::elem_type
accu_proxy_at(const Proxy<T1>& P)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  if(arma_config::openmp && Proxy<T1>::use_mp && mp_gate<eT>::eval(P.get_n_elem()))
    {
    return accu_proxy_at_mp(P);
    }

  const uword n_rows = P.get_n_rows();
  const uword n_cols = P.get_n_cols();

  eT val = eT(0);

  if(n_rows != 1)
    {
    eT val1 = eT(0);
    eT val2 = eT(0);

    for(uword col=0; col < n_cols; ++col)
      {
      uword i,j;
      for(i=0, j=1; j < n_rows; i+=2, j+=2)  { val1 += P.at(i,col); val2 += P.at(j,col); }

      if(i < n_rows)  { val1 += P.at(i,col); }
      }

    val = val1 + val2;
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)  { val += P.at(0,col); }
    }

  return val;
  }



template<typename T1>
arma_hot
inline
typename T1::elem_type
accu_proxy_at_mp(const Proxy<T1>& P)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  if(P.get_n_elem() == uword(0))
    {
    return eT(0);
    }

  eT val = eT(0);

  #if defined(ARMA_USE_OPENMP)
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_cols == 1)
      {
      const int   n_threads_max = mp_thread_limit::get();
      const uword n_threads_use = (std::min)(uword(podarray_prealloc_n_elem::val), uword(n_threads_max));
      const uword chunk_size    = n_rows / n_threads_use;

      podarray<eT> partial_accs(n_threads_use);

      #pragma omp parallel for schedule(static) num_threads(int(n_threads_use))
      for(uword thread_id=0; thread_id < n_threads_use; ++thread_id)
        {
        const uword start = (thread_id+0) * chunk_size;
        const uword endp1 = (thread_id+1) * chunk_size;

        eT acc = eT(0);
        for(uword i=start; i < endp1; ++i)  { acc += P.at(i,0); }

        partial_accs[thread_id] = acc;
        }

      for(uword thread_id=0; thread_id < n_threads_use; ++thread_id)  { val += partial_accs[thread_id]; }

      for(uword i=(n_threads_use*chunk_size); i < n_rows; ++i)  { val += P.at(i,0); }
      }
    else
    if(n_rows == 1)
      {
      const int   n_threads_max = mp_thread_limit::get();
      const uword n_threads_use = (std::min)(uword(podarray_prealloc_n_elem::val), uword(n_threads_max));
      const uword chunk_size    = n_cols / n_threads_use;

      podarray<eT> partial_accs(n_threads_use);

      #pragma omp parallel for schedule(static) num_threads(int(n_threads_use))
      for(uword thread_id=0; thread_id < n_threads_use; ++thread_id)
        {
        const uword start = (thread_id+0) * chunk_size;
        const uword endp1 = (thread_id+1) * chunk_size;

        eT acc = eT(0);
        for(uword i=start; i < endp1; ++i)  { acc += P.at(0,i); }

        partial_accs[thread_id] = acc;
        }

      for(uword thread_id=0; thread_id < n_threads_use; ++thread_id)  { val += partial_accs[thread_id]; }

      for(uword i=(n_threads_use*chunk_size); i < n_cols; ++i)  { val += P.at(0,i); }
      }
    else
      {
      podarray<eT> col_accs(n_cols);

      const int n_threads = mp_thread_limit::get();

      #pragma omp parallel for schedule(static) num_threads(n_threads)
      for(uword col=0; col < n_cols; ++col)
        {
        eT val1 = eT(0);
        eT val2 = eT(0);

        uword i,j;
        for(i=0, j=1; j < n_rows; i+=2, j+=2)  { val1 += P.at(i,col); val2 += P.at(j,col); }

        if(i < n_rows)  { val1 += P.at(i,col); }

        col_accs[col] = val1 + val2;
        }

      val = arrayops::accumulate(col_accs.memptr(), n_cols);
      }
    }
  #endif

  return val;
  }



//! accumulate the elements of a matrix
template<typename T1>
arma_warn_unused
arma_hot
inline
typename enable_if2< is_arma_type<T1>::value, typename T1::elem_type >::result
accu(const T1& X)
  {
  arma_extra_debug_sigprint();

  const Proxy<T1> P(X);

  if(is_Mat<typename Proxy<T1>::stored_type>::value || is_subview_col<typename Proxy<T1>::stored_type>::value)
    {
    const quasi_unwrap<typename Proxy<T1>::stored_type> tmp(P.Q);

    return arrayops::accumulate(tmp.M.memptr(), tmp.M.n_elem);
    }

  return (Proxy<T1>::use_at) ? accu_proxy_at(P) : accu_proxy_linear(P);
  }



//! explicit handling of multiply-and-accumulate
template<typename T1, typename T2>
arma_warn_unused
inline
typename T1::elem_type
accu(const eGlue<T1,T2,eglue_schur>& expr)
  {
  arma_extra_debug_sigprint();

  typedef eGlue<T1,T2,eglue_schur> expr_type;

  typedef typename expr_type::proxy1_type::stored_type P1_stored_type;
  typedef typename expr_type::proxy2_type::stored_type P2_stored_type;

  const bool have_direct_mem_1 = (is_Mat<P1_stored_type>::value) || (is_subview_col<P1_stored_type>::value);
  const bool have_direct_mem_2 = (is_Mat<P2_stored_type>::value) || (is_subview_col<P2_stored_type>::value);

  if(have_direct_mem_1 && have_direct_mem_2)
    {
    const quasi_unwrap<P1_stored_type> tmp1(expr.P1.Q);
    const quasi_unwrap<P2_stored_type> tmp2(expr.P2.Q);

    return op_dot::direct_dot(tmp1.M.n_elem, tmp1.M.memptr(), tmp2.M.memptr());
    }

  const Proxy<expr_type> P(expr);

  return (Proxy<expr_type>::use_at) ? accu_proxy_at(P) : accu_proxy_linear(P);
  }



//! explicit handling of Hamming norm (also known as zero norm)
template<typename T1>
arma_warn_unused
inline
uword
accu(const mtOp<uword,T1,op_rel_noteq>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const eT val = X.aux;

  const Proxy<T1> P(X.m);

  uword n_nonzero = 0;

  if(Proxy<T1>::use_at == false)
    {
    typedef typename Proxy<T1>::ea_type ea_type;

          ea_type A      = P.get_ea();
    const uword   n_elem = P.get_n_elem();

    for(uword i=0; i<n_elem; ++i)
      {
      n_nonzero += (A[i] != val) ? uword(1) : uword(0);
      }
    }
  else
    {
    const uword P_n_cols = P.get_n_cols();
    const uword P_n_rows = P.get_n_rows();

    if(P_n_rows == 1)
      {
      for(uword col=0; col < P_n_cols; ++col)
        {
        n_nonzero += (P.at(0,col) != val) ? uword(1) : uword(0);
        }
      }
    else
      {
      for(uword col=0; col < P_n_cols; ++col)
      for(uword row=0; row < P_n_rows; ++row)
        {
        n_nonzero += (P.at(row,col) != val) ? uword(1) : uword(0);
        }
      }
    }

  return n_nonzero;
  }



template<typename T1>
arma_warn_unused
inline
uword
accu(const mtOp<uword,T1,op_rel_eq>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const eT val = X.aux;

  const Proxy<T1> P(X.m);

  uword n_nonzero = 0;

  if(Proxy<T1>::use_at == false)
    {
    typedef typename Proxy<T1>::ea_type ea_type;

          ea_type A      = P.get_ea();
    const uword   n_elem = P.get_n_elem();

    for(uword i=0; i<n_elem; ++i)
      {
      n_nonzero += (A[i] == val) ? uword(1) : uword(0);
      }
    }
  else
    {
    const uword P_n_cols = P.get_n_cols();
    const uword P_n_rows = P.get_n_rows();

    if(P_n_rows == 1)
      {
      for(uword col=0; col < P_n_cols; ++col)
        {
        n_nonzero += (P.at(0,col) == val) ? uword(1) : uword(0);
        }
      }
    else
      {
      for(uword col=0; col < P_n_cols; ++col)
      for(uword row=0; row < P_n_rows; ++row)
        {
        n_nonzero += (P.at(row,col) == val) ? uword(1) : uword(0);
        }
      }
    }

  return n_nonzero;
  }



//! accumulate the elements of a subview (submatrix)
template<typename eT>
arma_warn_unused
arma_hot
inline
eT
accu(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  eT val = eT(0);

  if(X_n_rows == 1)
    {
    typedef subview_row<eT> sv_type;

    const sv_type& sv = reinterpret_cast<const sv_type&>(X);  // subview_row<eT> is a child class of subview<eT> and has no extra data

    const Proxy<sv_type> P(sv);

    val = accu_proxy_linear(P);
    }
  else
  if(X_n_cols == 1)
    {
    val = arrayops::accumulate( X.colptr(0), X_n_rows );
    }
  else
    {
    for(uword col=0; col < X_n_cols; ++col)
      {
      val += arrayops::accumulate( X.colptr(col), X_n_rows );
      }
    }

  return val;
  }



template<typename eT>
arma_warn_unused
arma_hot
inline
eT
accu(const subview_col<eT>& X)
  {
  arma_extra_debug_sigprint();

  return arrayops::accumulate( X.colptr(0), X.n_rows );
  }



//



template<typename T1>
arma_hot
inline
typename T1::elem_type
accu_cube_proxy_linear(const ProxyCube<T1>& P)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  eT val = eT(0);

  typename ProxyCube<T1>::ea_type Pea = P.get_ea();

  const uword n_elem = P.get_n_elem();

  if( arma_config::openmp && ProxyCube<T1>::use_mp && mp_gate<eT>::eval(n_elem) )
    {
    #if defined(ARMA_USE_OPENMP)
      {
      // NOTE: using parallelisation with manual reduction workaround to take into account complex numbers;
      // NOTE: OpenMP versions lower than 4.0 do not support user-defined reduction

      const int   n_threads_max = mp_thread_limit::get();
      const uword n_threads_use = (std::min)(uword(podarray_prealloc_n_elem::val), uword(n_threads_max));
      const uword chunk_size    = n_elem / n_threads_use;

      podarray<eT> partial_accs(n_threads_use);

      #pragma omp parallel for schedule(static) num_threads(int(n_threads_use))
      for(uword thread_id=0; thread_id < n_threads_use; ++thread_id)
        {
        const uword start = (thread_id+0) * chunk_size;
        const uword endp1 = (thread_id+1) * chunk_size;

        eT acc = eT(0);
        for(uword i=start; i < endp1; ++i)  { acc += Pea[i]; }

        partial_accs[thread_id] = acc;
        }

      for(uword thread_id=0; thread_id < n_threads_use; ++thread_id)  { val += partial_accs[thread_id]; }

      for(uword i=(n_threads_use*chunk_size); i < n_elem; ++i)  { val += Pea[i]; }
      }
    #endif
    }
  else
    {
    #if defined(__FINITE_MATH_ONLY__) && (__FINITE_MATH_ONLY__ > 0)
      {
      if(P.is_aligned())
        {
        typename ProxyCube<T1>::aligned_ea_type Pea_aligned = P.get_aligned_ea();

        for(uword i=0; i<n_elem; ++i)  { val += Pea_aligned.at_alt(i); }
        }
      else
        {
        for(uword i=0; i<n_elem; ++i)  { val += Pea[i]; }
        }
      }
    #else
      {
      eT val1 = eT(0);
      eT val2 = eT(0);

      uword i,j;
      for(i=0, j=1; j<n_elem; i+=2, j+=2)  { val1 += Pea[i]; val2 += Pea[j]; }

      if(i < n_elem) { val1 += Pea[i]; }

      val = val1 + val2;
      }
    #endif
    }

  return val;
  }



template<typename T1>
arma_hot
inline
typename T1::elem_type
accu_cube_proxy_at(const ProxyCube<T1>& P)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  if(arma_config::openmp && ProxyCube<T1>::use_mp && mp_gate<eT>::eval(P.get_n_elem()))
    {
    return accu_cube_proxy_at_mp(P);
    }

  const uword n_rows   = P.get_n_rows();
  const uword n_cols   = P.get_n_cols();
  const uword n_slices = P.get_n_slices();

  eT val1 = eT(0);
  eT val2 = eT(0);

  for(uword slice = 0; slice < n_slices; ++slice)
  for(uword col   = 0; col   < n_cols;   ++col  )
    {
    uword i,j;
    for(i=0, j=1; j<n_rows; i+=2, j+=2)  { val1 += P.at(i,col,slice); val2 += P.at(j,col,slice); }

    if(i < n_rows)  { val1 += P.at(i,col,slice); }
    }

  return (val1 + val2);
  }



template<typename T1>
arma_hot
inline
typename T1::elem_type
accu_cube_proxy_at_mp(const ProxyCube<T1>& P)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  eT val = eT(0);

  #if defined(ARMA_USE_OPENMP)
    {
    const uword n_rows   = P.get_n_rows();
    const uword n_cols   = P.get_n_cols();
    const uword n_slices = P.get_n_slices();

    podarray<eT> slice_accs(n_slices);

    const int n_threads = mp_thread_limit::get();

    #pragma omp parallel for schedule(static) num_threads(n_threads)
    for(uword slice = 0; slice < n_slices; ++slice)
      {
      eT val1 = eT(0);
      eT val2 = eT(0);

      for(uword col = 0; col < n_cols; ++col)
        {
        uword i,j;
        for(i=0, j=1; j<n_rows; i+=2, j+=2)  { val1 += P.at(i,col,slice);  val2 += P.at(j,col,slice); }

        if(i < n_rows)  { val1 += P.at(i,col,slice); }
        }

      slice_accs[slice] = val1 + val2;
      }

    val = arrayops::accumulate(slice_accs.memptr(), slice_accs.n_elem);
    }
  #endif

  return val;
  }



//! accumulate the elements of a cube
template<typename T1>
arma_warn_unused
arma_hot
inline
typename T1::elem_type
accu(const BaseCube<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  const ProxyCube<T1> P(X.get_ref());

  if(is_Cube<typename ProxyCube<T1>::stored_type>::value)
    {
    unwrap_cube<typename ProxyCube<T1>::stored_type> tmp(P.Q);

    return arrayops::accumulate(tmp.M.memptr(), tmp.M.n_elem);
    }

  return (ProxyCube<T1>::use_at) ? accu_cube_proxy_at(P) : accu_cube_proxy_linear(P);
  }



//! explicit handling of multiply-and-accumulate (cube version)
template<typename T1, typename T2>
arma_warn_unused
inline
typename T1::elem_type
accu(const eGlueCube<T1,T2,eglue_schur>& expr)
  {
  arma_extra_debug_sigprint();

  typedef eGlueCube<T1,T2,eglue_schur> expr_type;

  typedef typename ProxyCube<T1>::stored_type P1_stored_type;
  typedef typename ProxyCube<T2>::stored_type P2_stored_type;

  if(is_Cube<P1_stored_type>::value && is_Cube<P2_stored_type>::value)
    {
    const unwrap_cube<P1_stored_type> tmp1(expr.P1.Q);
    const unwrap_cube<P2_stored_type> tmp2(expr.P2.Q);

    return op_dot::direct_dot(tmp1.M.n_elem, tmp1.M.memptr(), tmp2.M.memptr());
    }

  const ProxyCube<expr_type> P(expr);

  return (ProxyCube<expr_type>::use_at) ? accu_cube_proxy_at(P) : accu_cube_proxy_linear(P);
  }



//



template<typename T>
arma_warn_unused
inline
const typename arma_scalar_only<T>::result &
accu(const T& x)
  {
  return x;
  }



//! accumulate values in a sparse object
template<typename T1>
arma_warn_unused
arma_hot
inline
typename enable_if2<is_arma_sparse_type<T1>::value, typename T1::elem_type>::result
accu(const T1& x)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const SpProxy<T1> p(x);

  if(SpProxy<T1>::use_iterator == false)
    {
    // direct counting
    return arrayops::accumulate(p.get_values(), p.get_n_nonzero());
    }
  else
    {
    typename SpProxy<T1>::const_iterator_type it     = p.begin();
    typename SpProxy<T1>::const_iterator_type it_end = p.end();

    eT result = eT(0);

    while(it != it_end)
      {
      result += (*it);
      ++it;
      }

    return result;
    }
  }



//! @}
