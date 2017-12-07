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


//! \addtogroup op_sum
//! @{



template<typename T1>
arma_hot
inline
void
op_sum::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_sum>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 1), "sum(): parameter 'dim' must be 0 or 1" );

  const Proxy<T1> P(in.m);

  if(P.is_alias(out) == false)
    {
    op_sum::apply_noalias(out, P, dim);
    }
  else
    {
    Mat<eT> tmp;

    op_sum::apply_noalias(tmp, P, dim);

    out.steal_mem(tmp);
    }
  }



template<typename T1>
arma_hot
inline
void
op_sum::apply_noalias(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  if(is_Mat<typename Proxy<T1>::stored_type>::value)
    {
    op_sum::apply_noalias_unwrap(out, P, dim);
    }
  else
    {
    op_sum::apply_noalias_proxy(out, P, dim);
    }
  }



template<typename T1>
arma_hot
inline
void
op_sum::apply_noalias_unwrap(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  typedef typename Proxy<T1>::stored_type P_stored_type;

  const unwrap<P_stored_type> tmp(P.Q);

  const typename unwrap<P_stored_type>::stored_type& X = tmp.M;

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  if(dim == 0)
    {
    out.set_size(1, X_n_cols);

    eT* out_mem = out.memptr();

    for(uword col=0; col < X_n_cols; ++col)
      {
      out_mem[col] = arrayops::accumulate( X.colptr(col), X_n_rows );
      }
    }
  else
    {
    out.zeros(X_n_rows, 1);

    eT* out_mem = out.memptr();

    for(uword col=0; col < X_n_cols; ++col)
      {
      arrayops::inplace_plus( out_mem, X.colptr(col), X_n_rows );
      }
    }
  }



template<typename T1>
arma_hot
inline
void
op_sum::apply_noalias_proxy(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  if( arma_config::openmp && Proxy<T1>::use_mp && mp_gate<eT>::eval(P.get_n_elem()) )
    {
    op_sum::apply_noalias_proxy_mp(out, P, dim);

    return;
    }

  const uword P_n_rows = P.get_n_rows();
  const uword P_n_cols = P.get_n_cols();

  if(dim == 0)
    {
    out.set_size(1, P_n_cols);

    eT* out_mem = out.memptr();

    for(uword col=0; col < P_n_cols; ++col)
      {
      eT val1 = eT(0);
      eT val2 = eT(0);

      uword i,j;
      for(i=0, j=1; j < P_n_rows; i+=2, j+=2)
        {
        val1 += P.at(i,col);
        val2 += P.at(j,col);
        }

      if(i < P_n_rows)
        {
        val1 += P.at(i,col);
        }

      out_mem[col] = (val1 + val2);
      }
    }
  else
    {
    out.zeros(P_n_rows, 1);

    eT* out_mem = out.memptr();

    for(uword col=0; col < P_n_cols; ++col)
    for(uword row=0; row < P_n_rows; ++row)
      {
      out_mem[row] += P.at(row,col);
      }
    }
  }



template<typename T1>
arma_hot
inline
void
op_sum::apply_noalias_proxy_mp(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_OPENMP)
    {
    typedef typename T1::elem_type eT;

    const uword P_n_rows = P.get_n_rows();
    const uword P_n_cols = P.get_n_cols();

    const int n_threads = mp_thread_limit::get();

    if(dim == 0)
      {
      out.set_size(1, P_n_cols);

      eT* out_mem = out.memptr();

      #pragma omp parallel for schedule(static) num_threads(n_threads)
      for(uword col=0; col < P_n_cols; ++col)
        {
        eT val1 = eT(0);
        eT val2 = eT(0);

        uword i,j;
        for(i=0, j=1; j < P_n_rows; i+=2, j+=2)
          {
          val1 += P.at(i,col);
          val2 += P.at(j,col);
          }

        if(i < P_n_rows)
          {
          val1 += P.at(i,col);
          }

        out_mem[col] = (val1 + val2);
        }
      }
    else
      {
      out.set_size(P_n_rows, 1);

      eT* out_mem = out.memptr();

      #pragma omp parallel for schedule(static) num_threads(n_threads)
      for(uword row=0; row < P_n_rows; ++row)
        {
        eT acc = eT(0);
        for(uword col=0; col < P_n_cols; ++col)
          {
          acc += P.at(row,col);
          }

        out_mem[row] = acc;
        }
      }
    }
  #else
    {
    arma_ignore(out);
    arma_ignore(P);
    arma_ignore(dim);
    }
  #endif
  }



//
// cubes



template<typename T1>
arma_hot
inline
void
op_sum::apply(Cube<typename T1::elem_type>& out, const OpCube<T1,op_sum>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 2), "sum(): parameter 'dim' must be 0 or 1 or 2" );

  const ProxyCube<T1> P(in.m);

  if(P.is_alias(out) == false)
    {
    op_sum::apply_noalias(out, P, dim);
    }
  else
    {
    Cube<eT> tmp;

    op_sum::apply_noalias(tmp, P, dim);

    out.steal_mem(tmp);
    }
  }



template<typename T1>
arma_hot
inline
void
op_sum::apply_noalias(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  if(is_Cube<typename ProxyCube<T1>::stored_type>::value)
    {
    op_sum::apply_noalias_unwrap(out, P, dim);
    }
  else
    {
    op_sum::apply_noalias_proxy(out, P, dim);
    }
  }



template<typename T1>
arma_hot
inline
void
op_sum::apply_noalias_unwrap(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  typedef typename ProxyCube<T1>::stored_type P_stored_type;

  const unwrap_cube<P_stored_type> tmp(P.Q);

  const Cube<eT>& X = tmp.M;

  const uword X_n_rows   = X.n_rows;
  const uword X_n_cols   = X.n_cols;
  const uword X_n_slices = X.n_slices;

  if(dim == 0)
    {
    out.set_size(1, X_n_cols, X_n_slices);

    for(uword slice=0; slice < X_n_slices; ++slice)
      {
      eT* out_mem = out.slice_memptr(slice);

      for(uword col=0; col < X_n_cols; ++col)
        {
        out_mem[col] = arrayops::accumulate( X.slice_colptr(slice,col), X_n_rows );
        }
      }
    }
  else
  if(dim == 1)
    {
    out.zeros(X_n_rows, 1, X_n_slices);

    for(uword slice=0; slice < X_n_slices; ++slice)
      {
      eT* out_mem = out.slice_memptr(slice);

      for(uword col=0; col < X_n_cols; ++col)
        {
        arrayops::inplace_plus( out_mem, X.slice_colptr(slice,col), X_n_rows );
        }
      }
    }
  else
  if(dim == 2)
    {
    out.zeros(X_n_rows, X_n_cols, 1);

    eT* out_mem = out.memptr();

    for(uword slice=0; slice < X_n_slices; ++slice)
      {
      arrayops::inplace_plus(out_mem, X.slice_memptr(slice), X.n_elem_slice );
      }
    }
  }



template<typename T1>
arma_hot
inline
void
op_sum::apply_noalias_proxy(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  if( arma_config::openmp && ProxyCube<T1>::use_mp && mp_gate<eT>::eval(P.get_n_elem()) )
    {
    op_sum::apply_noalias_proxy_mp(out, P, dim);

    return;
    }

  const uword P_n_rows   = P.get_n_rows();
  const uword P_n_cols   = P.get_n_cols();
  const uword P_n_slices = P.get_n_slices();

  if(dim == 0)
    {
    out.set_size(1, P_n_cols, P_n_slices);

    for(uword slice=0; slice < P_n_slices; ++slice)
      {
      eT* out_mem = out.slice_memptr(slice);

      for(uword col=0; col < P_n_cols; ++col)
        {
        eT val1 = eT(0);
        eT val2 = eT(0);

        uword i,j;
        for(i=0, j=1; j < P_n_rows; i+=2, j+=2)
          {
          val1 += P.at(i,col,slice);
          val2 += P.at(j,col,slice);
          }

        if(i < P_n_rows)
          {
          val1 += P.at(i,col,slice);
          }

        out_mem[col] = (val1 + val2);
        }
      }
    }
  else
  if(dim == 1)
    {
    out.zeros(P_n_rows, 1, P_n_slices);

    for(uword slice=0; slice < P_n_slices; ++slice)
      {
      eT* out_mem = out.slice_memptr(slice);

      for(uword col=0; col < P_n_cols; ++col)
      for(uword row=0; row < P_n_rows; ++row)
        {
        out_mem[row] += P.at(row,col,slice);
        }
      }
    }
  else
  if(dim == 2)
    {
    out.zeros(P_n_rows, P_n_cols, 1);

    for(uword slice=0; slice < P_n_slices; ++slice)
      {
      for(uword col=0; col < P_n_cols; ++col)
        {
        eT* out_mem = out.slice_colptr(0,col);

        for(uword row=0; row < P_n_rows; ++row)
          {
          out_mem[row] += P.at(row,col,slice);
          }
        }
      }
    }
  }



template<typename T1>
arma_hot
inline
void
op_sum::apply_noalias_proxy_mp(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_OPENMP)
    {
    typedef typename T1::elem_type eT;

    const uword P_n_rows   = P.get_n_rows();
    const uword P_n_cols   = P.get_n_cols();
    const uword P_n_slices = P.get_n_slices();

    const int n_threads = mp_thread_limit::get();

    if(dim == 0)
      {
      out.set_size(1, P_n_cols, P_n_slices);

      #pragma omp parallel for schedule(static) num_threads(n_threads)
      for(uword slice=0; slice < P_n_slices; ++slice)
        {
        eT* out_mem = out.slice_memptr(slice);

        for(uword col=0; col < P_n_cols; ++col)
          {
          eT val1 = eT(0);
          eT val2 = eT(0);

          uword i,j;
          for(i=0, j=1; j < P_n_rows; i+=2, j+=2)
            {
            val1 += P.at(i,col,slice);
            val2 += P.at(j,col,slice);
            }

          if(i < P_n_rows)
            {
            val1 += P.at(i,col,slice);
            }

          out_mem[col] = (val1 + val2);
          }
        }
      }
    else
    if(dim == 1)
      {
      out.zeros(P_n_rows, 1, P_n_slices);

      #pragma omp parallel for schedule(static) num_threads(n_threads)
      for(uword slice=0; slice < P_n_slices; ++slice)
        {
        eT* out_mem = out.slice_memptr(slice);

        for(uword col=0; col < P_n_cols; ++col)
        for(uword row=0; row < P_n_rows; ++row)
          {
          out_mem[row] += P.at(row,col,slice);
          }
        }
      }
    else
    if(dim == 2)
      {
      out.zeros(P_n_rows, P_n_cols, 1);

      if(P_n_cols >= P_n_rows)
        {
        #pragma omp parallel for schedule(static) num_threads(n_threads)
        for(uword col=0; col < P_n_cols; ++col)
          {
          for(uword row=0; row < P_n_rows; ++row)
            {
            eT acc = eT(0);
            for(uword slice=0; slice < P_n_slices; ++slice)
              {
              acc += P.at(row,col,slice);
              }

            out.at(row,col,0) = acc;
            }
          }
        }
      else
        {
        #pragma omp parallel for schedule(static) num_threads(n_threads)
        for(uword row=0; row < P_n_rows; ++row)
          {
          for(uword col=0; col < P_n_cols; ++col)
            {
            eT acc = eT(0);
            for(uword slice=0; slice < P_n_slices; ++slice)
              {
              acc += P.at(row,col,slice);
              }

            out.at(row,col,0) = acc;
            }
          }
        }
      }
    }
  #else
    {
    arma_ignore(out);
    arma_ignore(P);
    arma_ignore(dim);
    }
  #endif
  }



//! @}
