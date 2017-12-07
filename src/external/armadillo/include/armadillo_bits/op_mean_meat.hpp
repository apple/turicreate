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


//! \addtogroup op_mean
//! @{



template<typename T1>
inline
void
op_mean::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_mean>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 1), "mean(): parameter 'dim' must be 0 or 1" );

  const Proxy<T1> P(in.m);

  if(P.is_alias(out) == false)
    {
    op_mean::apply_noalias(out, P, dim);
    }
  else
    {
    Mat<eT> tmp;

    op_mean::apply_noalias(tmp, P, dim);

    out.steal_mem(tmp);
    }
  }



template<typename T1>
inline
void
op_mean::apply_noalias(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  if(is_Mat<typename Proxy<T1>::stored_type>::value)
    {
    op_mean::apply_noalias_unwrap(out, P, dim);
    }
  else
    {
    op_mean::apply_noalias_proxy(out, P, dim);
    }
  }



template<typename T1>
inline
void
op_mean::apply_noalias_unwrap(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result  T;

  typedef typename Proxy<T1>::stored_type P_stored_type;

  const unwrap<P_stored_type> tmp(P.Q);

  const typename unwrap<P_stored_type>::stored_type& X = tmp.M;

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  if(dim == 0)
    {
    out.set_size((X_n_rows > 0) ? 1 : 0, X_n_cols);

    if(X_n_rows == 0)  { return; }

    eT* out_mem = out.memptr();

    for(uword col=0; col < X_n_cols; ++col)
      {
      out_mem[col] = op_mean::direct_mean( X.colptr(col), X_n_rows );
      }
    }
  else
  if(dim == 1)
    {
    out.zeros(X_n_rows, (X_n_cols > 0) ? 1 : 0);

    if(X_n_cols == 0)  { return; }

    eT* out_mem = out.memptr();

    for(uword col=0; col < X_n_cols; ++col)
      {
      const eT* col_mem = X.colptr(col);

      for(uword row=0; row < X_n_rows; ++row)
        {
        out_mem[row] += col_mem[row];
        }
      }

    out /= T(X_n_cols);

    for(uword row=0; row < X_n_rows; ++row)
      {
      if(arma_isfinite(out_mem[row]) == false)
        {
        out_mem[row] = op_mean::direct_mean_robust( X, row );
        }
      }
    }
  }



template<typename T1>
arma_hot
inline
void
op_mean::apply_noalias_proxy(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result  T;

  const uword P_n_rows = P.get_n_rows();
  const uword P_n_cols = P.get_n_cols();

  if(dim == 0)
    {
    out.set_size((P_n_rows > 0) ? 1 : 0, P_n_cols);

    if(P_n_rows == 0)  { return; }

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

      out_mem[col] = (val1 + val2) / T(P_n_rows);
      }
    }
  else
  if(dim == 1)
    {
    out.zeros(P_n_rows, (P_n_cols > 0) ? 1 : 0);

    if(P_n_cols == 0)  { return; }

    eT* out_mem = out.memptr();

    for(uword col=0; col < P_n_cols; ++col)
    for(uword row=0; row < P_n_rows; ++row)
      {
      out_mem[row] += P.at(row,col);
      }

    out /= T(P_n_cols);
    }

  if(out.is_finite() == false)
    {
    // TODO: replace with dedicated handling to avoid unwrapping
    op_mean::apply_noalias_unwrap(out, P, dim);
    }
  }



//
// cubes



template<typename T1>
inline
void
op_mean::apply(Cube<typename T1::elem_type>& out, const OpCube<T1,op_mean>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 2), "mean(): parameter 'dim' must be 0 or 1 or 2" );

  const ProxyCube<T1> P(in.m);

  if(P.is_alias(out) == false)
    {
    op_mean::apply_noalias(out, P, dim);
    }
  else
    {
    Cube<eT> tmp;

    op_mean::apply_noalias(tmp, P, dim);

    out.steal_mem(tmp);
    }
  }



template<typename T1>
inline
void
op_mean::apply_noalias(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  if(is_Cube<typename ProxyCube<T1>::stored_type>::value)
    {
    op_mean::apply_noalias_unwrap(out, P, dim);
    }
  else
    {
    op_mean::apply_noalias_proxy(out, P, dim);
    }
  }



template<typename T1>
inline
void
op_mean::apply_noalias_unwrap(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result  T;

  typedef typename ProxyCube<T1>::stored_type P_stored_type;

  const unwrap_cube<P_stored_type> U(P.Q);

  const Cube<eT>& X = U.M;

  const uword X_n_rows   = X.n_rows;
  const uword X_n_cols   = X.n_cols;
  const uword X_n_slices = X.n_slices;

  if(dim == 0)
    {
    out.set_size((X_n_rows > 0) ? 1 : 0, X_n_cols, X_n_slices);

    if(X_n_rows == 0)  { return; }

    for(uword slice=0; slice < X_n_slices; ++slice)
      {
      eT* out_mem = out.slice_memptr(slice);

      for(uword col=0; col < X_n_cols; ++col)
        {
        out_mem[col] = op_mean::direct_mean( X.slice_colptr(slice,col), X_n_rows );
        }
      }
    }
  else
  if(dim == 1)
    {
    out.zeros(X_n_rows, (X_n_cols > 0) ? 1 : 0, X_n_slices);

    if(X_n_cols == 0)  { return; }

    for(uword slice=0; slice < X_n_slices; ++slice)
      {
      eT* out_mem = out.slice_memptr(slice);

      for(uword col=0; col < X_n_cols; ++col)
        {
        const eT* col_mem = X.slice_colptr(slice,col);

        for(uword row=0; row < X_n_rows; ++row)
          {
          out_mem[row] += col_mem[row];
          }
        }

      const Mat<eT> tmp('j', X.slice_memptr(slice), X_n_rows, X_n_cols);

      for(uword row=0; row < X_n_rows; ++row)
        {
        out_mem[row] /= T(X_n_cols);

        if(arma_isfinite(out_mem[row]) == false)
          {
          out_mem[row] = op_mean::direct_mean_robust( tmp, row );
          }
        }
      }
    }
  else
  if(dim == 2)
    {
    out.zeros(X_n_rows, X_n_cols, (X_n_slices > 0) ? 1 : 0);

    if(X_n_slices == 0)  { return; }

    eT* out_mem = out.memptr();

    for(uword slice=0; slice < X_n_slices; ++slice)
      {
      arrayops::inplace_plus(out_mem, X.slice_memptr(slice), X.n_elem_slice );
      }

    out /= T(X_n_slices);

    podarray<eT> tmp(X_n_slices);

    for(uword col=0; col < X_n_cols; ++col)
    for(uword row=0; row < X_n_rows; ++row)
      {
      if(arma_isfinite(out.at(row,col,0)) == false)
        {
        for(uword slice=0; slice < X_n_slices; ++slice)
          {
          tmp[slice] = X.at(row,col,slice);
          }

        out.at(row,col,0) = op_mean::direct_mean_robust(tmp.memptr(), X_n_slices);
        }
      }
    }
  }



template<typename T1>
arma_hot
inline
void
op_mean::apply_noalias_proxy(Cube<typename T1::elem_type>& out, const ProxyCube<T1>& P, const uword dim)
  {
  arma_extra_debug_sigprint();

  op_mean::apply_noalias_unwrap(out, P, dim);

  // TODO: implement specialised handling
  }




//



template<typename eT>
inline
eT
op_mean::direct_mean(const eT* const X, const uword n_elem)
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  const eT result = arrayops::accumulate(X, n_elem) / T(n_elem);

  return arma_isfinite(result) ? result : op_mean::direct_mean_robust(X, n_elem);
  }



template<typename eT>
inline
eT
op_mean::direct_mean_robust(const eT* const X, const uword n_elem)
  {
  arma_extra_debug_sigprint();

  // use an adapted form of the mean finding algorithm from the running_stat class

  typedef typename get_pod_type<eT>::result T;

  uword i,j;

  eT r_mean = eT(0);

  for(i=0, j=1; j<n_elem; i+=2, j+=2)
    {
    const eT Xi = X[i];
    const eT Xj = X[j];

    r_mean = r_mean + (Xi - r_mean)/T(j);    // we need i+1, and j is equivalent to i+1 here
    r_mean = r_mean + (Xj - r_mean)/T(j+1);
    }


  if(i < n_elem)
    {
    const eT Xi = X[i];

    r_mean = r_mean + (Xi - r_mean)/T(i+1);
    }

  return r_mean;
  }



template<typename eT>
inline
eT
op_mean::direct_mean(const Mat<eT>& X, const uword row)
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  const uword X_n_cols = X.n_cols;

  eT val = eT(0);

  uword i,j;
  for(i=0, j=1; j < X_n_cols; i+=2, j+=2)
    {
    val += X.at(row,i);
    val += X.at(row,j);
    }

  if(i < X_n_cols)
    {
    val += X.at(row,i);
    }

  const eT result = val / T(X_n_cols);

  return arma_isfinite(result) ? result : op_mean::direct_mean_robust(X, row);
  }



template<typename eT>
inline
eT
op_mean::direct_mean_robust(const Mat<eT>& X, const uword row)
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  const uword X_n_cols = X.n_cols;

  eT r_mean = eT(0);

  for(uword col=0; col < X_n_cols; ++col)
    {
    r_mean = r_mean + (X.at(row,col) - r_mean)/T(col+1);
    }

  return r_mean;
  }



template<typename eT>
inline
eT
op_mean::mean_all(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;
  const uword X_n_elem = X.n_elem;

  if(X_n_elem == 0)
    {
    arma_debug_check(true, "mean(): object has no elements");

    return Datum<eT>::nan;
    }

  eT val = eT(0);

  if(X_n_rows == 1)
    {
    const Mat<eT>& A = X.m;

    const uword start_row = X.aux_row1;
    const uword start_col = X.aux_col1;

    const uword end_col_p1 = start_col + X_n_cols;

    uword i,j;
    for(i=start_col, j=start_col+1; j < end_col_p1; i+=2, j+=2)
      {
      val += A.at(start_row, i);
      val += A.at(start_row, j);
      }

    if(i < end_col_p1)
      {
      val += A.at(start_row, i);
      }
    }
  else
    {
    for(uword col=0; col < X_n_cols; ++col)
      {
      val += arrayops::accumulate(X.colptr(col), X_n_rows);
      }
    }

  const eT result = val / T(X_n_elem);

  return arma_isfinite(result) ? result : op_mean::mean_all_robust(X);
  }



template<typename eT>
inline
eT
op_mean::mean_all_robust(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  const uword start_row = X.aux_row1;
  const uword start_col = X.aux_col1;

  const uword end_row_p1 = start_row + X_n_rows;
  const uword end_col_p1 = start_col + X_n_cols;

  const Mat<eT>& A = X.m;


  eT r_mean = eT(0);

  if(X_n_rows == 1)
    {
    uword i=0;

    for(uword col = start_col; col < end_col_p1; ++col, ++i)
      {
      r_mean = r_mean + (A.at(start_row,col) - r_mean)/T(i+1);
      }
    }
  else
    {
    uword i=0;

    for(uword col = start_col; col < end_col_p1; ++col)
    for(uword row = start_row; row < end_row_p1; ++row, ++i)
      {
      r_mean = r_mean + (A.at(row,col) - r_mean)/T(i+1);
      }
    }

  return r_mean;
  }



template<typename eT>
inline
eT
op_mean::mean_all(const diagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  const uword X_n_elem = X.n_elem;

  if(X_n_elem == 0)
    {
    arma_debug_check(true, "mean(): object has no elements");

    return Datum<eT>::nan;
    }

  eT val = eT(0);

  for(uword i=0; i<X_n_elem; ++i)
    {
    val += X[i];
    }

  const eT result = val / T(X_n_elem);

  return arma_isfinite(result) ? result : op_mean::mean_all_robust(X);
  }



template<typename eT>
inline
eT
op_mean::mean_all_robust(const diagview<eT>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  const uword X_n_elem = X.n_elem;

  eT r_mean = eT(0);

  for(uword i=0; i<X_n_elem; ++i)
    {
    r_mean = r_mean + (X[i] - r_mean)/T(i+1);
    }

  return r_mean;
  }



template<typename T1>
inline
typename T1::elem_type
op_mean::mean_all(const Op<T1,op_vectorise_col>& X)
  {
  arma_extra_debug_sigprint();

  return op_mean::mean_all(X.m);
  }



template<typename T1>
inline
typename T1::elem_type
op_mean::mean_all(const Base<typename T1::elem_type, T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T1>   tmp(X.get_ref());
  const Mat<eT>& A = tmp.M;

  const uword A_n_elem = A.n_elem;

  if(A_n_elem == 0)
    {
    arma_debug_check(true, "mean(): object has no elements");

    return Datum<eT>::nan;
    }

  return op_mean::direct_mean(A.memptr(), A_n_elem);
  }



template<typename eT>
arma_inline
eT
op_mean::robust_mean(const eT A, const eT B)
  {
  return A + (B - A)/eT(2);
  }



template<typename T>
arma_inline
std::complex<T>
op_mean::robust_mean(const std::complex<T>& A, const std::complex<T>& B)
  {
  return A + (B - A)/T(2);
  }



//! @}
