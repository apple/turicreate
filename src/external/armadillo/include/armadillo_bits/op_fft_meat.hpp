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



//! \addtogroup op_fft
//! @{



//
// op_fft_real



template<typename T1>
inline
void
op_fft_real::apply( Mat< std::complex<typename T1::pod_type> >& out, const mtOp<std::complex<typename T1::pod_type>,T1,op_fft_real>& in )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::pod_type         in_eT;
  typedef typename std::complex<in_eT> out_eT;

  const Proxy<T1> P(in.m);

  const uword n_rows = P.get_n_rows();
  const uword n_cols = P.get_n_cols();
  const uword n_elem = P.get_n_elem();

  const bool is_vec = ( (n_rows == 1) || (n_cols == 1) );

  const uword N_orig = (is_vec)              ? n_elem         : n_rows;
  const uword N_user = (in.aux_uword_b == 0) ? in.aux_uword_a : N_orig;

  fft_engine<out_eT,false> worker(N_user);

  // no need to worry about aliasing, as we're going from a real object to complex complex, which by definition cannot alias

  if(is_vec)
    {
    (n_cols == 1) ? out.set_size(N_user, 1) : out.set_size(1, N_user);

    if( (out.n_elem == 0) || (N_orig == 0) )
      {
      out.zeros();
      return;
      }

    if( (N_user == 1) && (N_orig >= 1) )
      {
      out[0] = out_eT( P[0] );
      return;
      }

    podarray<out_eT> data(N_user);

    out_eT* data_mem = data.memptr();

    if(N_user > N_orig)  { arrayops::fill_zeros( &data_mem[N_orig], (N_user - N_orig) ); }

    const uword N = (std::min)(N_user, N_orig);

    if(Proxy<T1>::use_at == false)
      {
      typename Proxy<T1>::ea_type X = P.get_ea();

      for(uword i=0; i < N; ++i)  { data_mem[i] = out_eT( X[i], in_eT(0) ); }
      }
    else
      {
      if(n_cols == 1)
        {
        for(uword i=0; i < N; ++i)  { data_mem[i] = out_eT( P.at(i,0), in_eT(0) ); }
        }
      else
        {
        for(uword i=0; i < N; ++i)  { data_mem[i] = out_eT( P.at(0,i), in_eT(0) ); }
        }
      }

    worker.run( out.memptr(), data_mem );
    }
  else
    {
    // process each column seperately

    out.set_size(N_user, n_cols);

    if( (out.n_elem == 0) || (N_orig == 0) )
      {
      out.zeros();
      return;
      }

    if( (N_user == 1) && (N_orig >= 1) )
      {
      for(uword col=0; col < n_cols; ++col)  { out.at(0,col) = out_eT( P.at(0,col) ); }

      return;
      }

    podarray<out_eT> data(N_user);

    out_eT* data_mem = data.memptr();

    if(N_user > N_orig)  { arrayops::fill_zeros( &data_mem[N_orig], (N_user - N_orig) ); }

    const uword N = (std::min)(N_user, N_orig);

    for(uword col=0; col < n_cols; ++col)
      {
      for(uword i=0; i < N; ++i)  { data_mem[i] = P.at(i, col); }

      worker.run( out.colptr(col), data_mem );
      }
    }
  }



//
// op_fft_cx


template<typename T1>
inline
void
op_fft_cx::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_fft_cx>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> P(in.m);

  if(P.is_alias(out) == false)
    {
    op_fft_cx::apply_noalias<T1,false>(out, P, in.aux_uword_a, in.aux_uword_b);
    }
  else
    {
    Mat<eT> tmp;

    op_fft_cx::apply_noalias<T1,false>(tmp, P, in.aux_uword_a, in.aux_uword_b);

    out.steal_mem(tmp);
    }
  }



template<typename T1, bool inverse>
inline
void
op_fft_cx::apply_noalias(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword a, const uword b)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_rows = P.get_n_rows();
  const uword n_cols = P.get_n_cols();
  const uword n_elem = P.get_n_elem();

  const bool is_vec = ( (n_rows == 1) || (n_cols == 1) );

  const uword N_orig = (is_vec) ? n_elem : n_rows;
  const uword N_user = (b == 0) ? a      : N_orig;

  fft_engine<eT,inverse> worker(N_user);

  if(is_vec)
    {
    (n_cols == 1) ? out.set_size(N_user, 1) : out.set_size(1, N_user);

    if( (out.n_elem == 0) || (N_orig == 0) )
      {
      out.zeros();
      return;
      }

    if( (N_user == 1) && (N_orig >= 1) )
      {
      out[0] = P[0];
      return;
      }

    if( (N_user > N_orig) || (is_Mat<typename Proxy<T1>::stored_type>::value == false) )
      {
      podarray<eT> data(N_user);

      eT* data_mem = data.memptr();

      if(N_user > N_orig)  { arrayops::fill_zeros( &data_mem[N_orig], (N_user - N_orig) ); }

      op_fft_cx::copy_vec( data_mem, P, (std::min)(N_user, N_orig) );

      worker.run( out.memptr(), data_mem );
      }
    else
      {
      const unwrap< typename Proxy<T1>::stored_type > tmp(P.Q);

      worker.run( out.memptr(), tmp.M.memptr() );
      }
    }
  else
    {
    // process each column seperately

    out.set_size(N_user, n_cols);

    if( (out.n_elem == 0) || (N_orig == 0) )
      {
      out.zeros();
      return;
      }

    if( (N_user == 1) && (N_orig >= 1) )
      {
      for(uword col=0; col < n_cols; ++col)  { out.at(0,col) = P.at(0,col); }

      return;
      }

    if( (N_user > N_orig) || (is_Mat<typename Proxy<T1>::stored_type>::value == false) )
      {
      podarray<eT> data(N_user);

      eT* data_mem = data.memptr();

      if(N_user > N_orig)  { arrayops::fill_zeros( &data_mem[N_orig], (N_user - N_orig) ); }

      const uword N = (std::min)(N_user, N_orig);

      for(uword col=0; col < n_cols; ++col)
        {
        for(uword i=0; i < N; ++i)  { data_mem[i] = P.at(i, col); }

        worker.run( out.colptr(col), data_mem );
        }
      }
    else
      {
      const unwrap< typename Proxy<T1>::stored_type > tmp(P.Q);

      for(uword col=0; col < n_cols; ++col)
        {
        worker.run( out.colptr(col), tmp.M.colptr(col) );
        }
      }
    }


  // correct the scaling for the inverse transform
  if(inverse == true)
    {
    typedef typename get_pod_type<eT>::result T;

    const T k = T(1) / T(N_user);

    eT* out_mem = out.memptr();

    const uword out_n_elem = out.n_elem;

    for(uword i=0; i < out_n_elem; ++i)  { out_mem[i] *= k; }
    }
  }



template<typename T1>
arma_hot
inline
void
op_fft_cx::copy_vec(typename Proxy<T1>::elem_type* dest, const Proxy<T1>& P, const uword N)
  {
  arma_extra_debug_sigprint();

  if(is_Mat< typename Proxy<T1>::stored_type >::value == true)
    {
    op_fft_cx::copy_vec_unwrap(dest, P, N);
    }
  else
    {
    op_fft_cx::copy_vec_proxy(dest, P, N);
    }
  }



template<typename T1>
arma_hot
inline
void
op_fft_cx::copy_vec_unwrap(typename Proxy<T1>::elem_type* dest, const Proxy<T1>& P, const uword N)
  {
  arma_extra_debug_sigprint();

  const unwrap< typename Proxy<T1>::stored_type > tmp(P.Q);

  arrayops::copy(dest, tmp.M.memptr(), N);
  }



template<typename T1>
arma_hot
inline
void
op_fft_cx::copy_vec_proxy(typename Proxy<T1>::elem_type* dest, const Proxy<T1>& P, const uword N)
  {
  arma_extra_debug_sigprint();

  if(Proxy<T1>::use_at == false)
    {
    typename Proxy<T1>::ea_type X = P.get_ea();

    for(uword i=0; i < N; ++i)  { dest[i] = X[i]; }
    }
  else
    {
    if(P.get_n_cols() == 1)
      {
      for(uword i=0; i < N; ++i)  { dest[i] = P.at(i,0); }
      }
    else
      {
      for(uword i=0; i < N; ++i)  { dest[i] = P.at(0,i); }
      }
    }
  }



//
// op_ifft_cx


template<typename T1>
inline
void
op_ifft_cx::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_ifft_cx>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> P(in.m);

  if(P.is_alias(out) == false)
    {
    op_fft_cx::apply_noalias<T1,true>(out, P, in.aux_uword_a, in.aux_uword_b);
    }
  else
    {
    Mat<eT> tmp;

    op_fft_cx::apply_noalias<T1,true>(tmp, P, in.aux_uword_a, in.aux_uword_b);

    out.steal_mem(tmp);
    }
  }



//! @}
