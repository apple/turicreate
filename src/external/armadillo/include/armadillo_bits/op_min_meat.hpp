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


//! \addtogroup op_min
//! @{



template<typename T1>
inline
void
op_min::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_min>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 1), "min(): parameter 'dim' must be 0 or 1");

  const quasi_unwrap<T1> U(in.m);
  const Mat<eT>& X = U.M;

  if(U.is_alias(out) == false)
    {
    op_min::apply_noalias(out, X, dim);
    }
  else
    {
    Mat<eT> tmp;

    op_min::apply_noalias(tmp, X, dim);

    out.steal_mem(tmp);
    }
  }



template<typename eT>
inline
void
op_min::apply_noalias(Mat<eT>& out, const Mat<eT>& X, const uword dim, const typename arma_not_cx<eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  if(dim == 0)
    {
    arma_extra_debug_print("op_min::apply(): dim = 0");

    out.set_size((X_n_rows > 0) ? 1 : 0, X_n_cols);

    if(X_n_rows == 0)  { return; }

    eT* out_mem = out.memptr();

    for(uword col=0; col<X_n_cols; ++col)
      {
      out_mem[col] = op_min::direct_min( X.colptr(col), X_n_rows );
      }
    }
  else
  if(dim == 1)
    {
    arma_extra_debug_print("op_min::apply(): dim = 1");

    out.set_size(X_n_rows, (X_n_cols > 0) ? 1 : 0);

    if(X_n_cols == 0)  { return; }

    eT* out_mem = out.memptr();

    arrayops::copy(out_mem, X.colptr(0), X_n_rows);

    for(uword col=1; col<X_n_cols; ++col)
      {
      const eT* col_mem = X.colptr(col);

      for(uword row=0; row<X_n_rows; ++row)
        {
        const eT col_val = col_mem[row];

        if(col_val < out_mem[row])  { out_mem[row] = col_val; }
        }
      }
    }
  }



template<typename eT>
inline
void
op_min::apply_noalias(Mat<eT>& out, const Mat<eT>& X, const uword dim, const typename arma_cx_only<eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  if(dim == 0)
    {
    arma_extra_debug_print("op_min::apply(): dim = 0");

    out.set_size((X_n_rows > 0) ? 1 : 0, X_n_cols);

    if(X_n_rows == 0)  { return; }

    eT* out_mem = out.memptr();

    for(uword col=0; col<X_n_cols; ++col)
      {
      out_mem[col] = op_min::direct_min( X.colptr(col), X_n_rows );
      }
    }
  else
  if(dim == 1)
    {
    arma_extra_debug_print("op_min::apply(): dim = 1");

    out.set_size(X_n_rows, (X_n_cols > 0) ? 1 : 0);

    if(X_n_cols == 0)  { return; }

    eT* out_mem = out.memptr();

    for(uword row=0; row<X_n_rows; ++row)
      {
      out_mem[row] = op_min::direct_min( X, row );
      }
    }
  }



template<typename T1>
inline
void
op_min::apply(Cube<typename T1::elem_type>& out, const OpCube<T1,op_min>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 2), "min(): parameter 'dim' must be 0 or 1 or 2" );

  const unwrap_cube<T1> U(in.m);
  const Cube<eT>& X =   U.M;

  if(&out != &X)
    {
    op_min::apply_noalias(out, X, dim);
    }
  else
    {
    Cube<eT> tmp;

    op_min::apply_noalias(tmp, X, dim);

    out.steal_mem(tmp);
    }
  }



template<typename eT>
inline
void
op_min::apply_noalias(Cube<eT>& out, const Cube<eT>& X, const uword dim, const typename arma_not_cx<eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const uword X_n_rows   = X.n_rows;
  const uword X_n_cols   = X.n_cols;
  const uword X_n_slices = X.n_slices;

  if(dim == 0)
    {
    arma_extra_debug_print("op_min::apply(): dim = 0");

    out.set_size((X_n_rows > 0) ? 1 : 0, X_n_cols, X_n_slices);

    if(X_n_rows == 0)  { return; }

    for(uword slice=0; slice < X_n_slices; ++slice)
      {
      eT* out_mem = out.slice_memptr(slice);

      for(uword col=0; col < X_n_cols; ++col)
        {
        out_mem[col] = op_min::direct_min( X.slice_colptr(slice,col), X_n_rows );
        }
      }
    }
  else
  if(dim == 1)
    {
    arma_extra_debug_print("op_min::apply(): dim = 1");

    out.set_size(X_n_rows, (X_n_cols > 0) ? 1 : 0, X_n_slices);

    if(X_n_cols == 0)  { return; }

    for(uword slice=0; slice < X_n_slices; ++slice)
      {
      eT* out_mem = out.slice_memptr(slice);

      arrayops::copy(out_mem, X.slice_colptr(slice,0), X_n_rows);

      for(uword col=1; col < X_n_cols; ++col)
        {
        const eT* col_mem = X.slice_colptr(slice,col);

        for(uword row=0; row < X_n_rows; ++row)
          {
          const eT col_val = col_mem[row];

          if(col_val < out_mem[row])  { out_mem[row] = col_val; }
          }
        }
      }
    }
  else
  if(dim == 2)
    {
    arma_extra_debug_print("op_min::apply(): dim = 2");

    out.set_size(X_n_rows, X_n_cols, (X_n_slices > 0) ? 1 : 0);

    if(X_n_slices == 0)  { return; }

    const uword N = X.n_elem_slice;

    eT* out_mem = out.slice_memptr(0);

    arrayops::copy(out_mem, X.slice_memptr(0), N);

    for(uword slice=1; slice < X_n_slices; ++slice)
      {
      const eT* X_mem = X.slice_memptr(slice);

      for(uword i=0; i < N; ++i)
        {
        const eT val = X_mem[i];

        if(val < out_mem[i])  { out_mem[i] = val; }
        }
      }
    }
  }



template<typename eT>
inline
void
op_min::apply_noalias(Cube<eT>& out, const Cube<eT>& X, const uword dim, const typename arma_cx_only<eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  const uword X_n_rows   = X.n_rows;
  const uword X_n_cols   = X.n_cols;
  const uword X_n_slices = X.n_slices;

  if(dim == 0)
    {
    arma_extra_debug_print("op_min::apply(): dim = 0");

    out.set_size((X_n_rows > 0) ? 1 : 0, X_n_cols, X_n_slices);

    if(X_n_rows == 0)  { return; }

    for(uword slice=0; slice < X_n_slices; ++slice)
      {
      eT* out_mem = out.slice_memptr(slice);

      for(uword col=0; col < X_n_cols; ++col)
        {
        out_mem[col] = op_min::direct_min( X.slice_colptr(slice,col), X_n_rows );
        }
      }
    }
  else
  if(dim == 1)
    {
    arma_extra_debug_print("op_min::apply(): dim = 1");

    out.set_size(X_n_rows, (X_n_cols > 0) ? 1 : 0, X_n_slices);

    if(X_n_cols == 0)  { return; }

    for(uword slice=0; slice < X_n_slices; ++slice)
      {
      eT* out_mem = out.slice_memptr(slice);

      const Mat<eT> tmp('j', X.slice_memptr(slice), X_n_rows, X_n_cols);

      for(uword row=0; row < X_n_rows; ++row)
        {
        out_mem[row] = op_min::direct_min(tmp, row);
        }
      }
    }
  else
  if(dim == 2)
    {
    arma_extra_debug_print("op_min::apply(): dim = 2");

    out.set_size(X_n_rows, X_n_cols, (X_n_slices > 0) ? 1 : 0);

    if(X_n_slices == 0)  { return; }

    const uword N = X.n_elem_slice;

    eT* out_mem = out.slice_memptr(0);

    arrayops::copy(out_mem, X.slice_memptr(0), N);

    for(uword slice=1; slice < X_n_slices; ++slice)
      {
      const eT* X_mem = X.slice_memptr(slice);

      for(uword i=0; i < N; ++i)
        {
        const eT& val = X_mem[i];

        if(std::abs(val) < std::abs(out_mem[i]))  { out_mem[i] = val; }
        }
      }
    }
  }



template<typename eT>
inline
eT
op_min::direct_min(const eT* const X, const uword n_elem)
  {
  arma_extra_debug_sigprint();

  eT min_val = priv::most_pos<eT>();

  uword i,j;
  for(i=0, j=1; j<n_elem; i+=2, j+=2)
    {
    const eT X_i = X[i];
    const eT X_j = X[j];

    if(X_i < min_val) { min_val = X_i; }
    if(X_j < min_val) { min_val = X_j; }
    }

  if(i < n_elem)
    {
    const eT X_i = X[i];

    if(X_i < min_val) { min_val = X_i; }
    }

  return min_val;
  }



template<typename eT>
inline
eT
op_min::direct_min(const eT* const X, const uword n_elem, uword& index_of_min_val)
  {
  arma_extra_debug_sigprint();

  eT min_val = priv::most_pos<eT>();

  uword best_index = 0;

  uword i,j;
  for(i=0, j=1; j<n_elem; i+=2, j+=2)
    {
    const eT X_i = X[i];
    const eT X_j = X[j];

    if(X_i < min_val)
      {
      min_val    = X_i;
      best_index = i;
      }

    if(X_j < min_val)
      {
      min_val    = X_j;
      best_index = j;
      }
    }

  if(i < n_elem)
    {
    const eT X_i = X[i];

    if(X_i < min_val)
      {
      min_val    = X_i;
      best_index = i;
      }
    }

  index_of_min_val = best_index;

  return min_val;
  }



template<typename eT>
inline
eT
op_min::direct_min(const Mat<eT>& X, const uword row)
  {
  arma_extra_debug_sigprint();

  const uword X_n_cols = X.n_cols;

  eT min_val = priv::most_pos<eT>();

  uword i,j;
  for(i=0, j=1; j < X_n_cols; i+=2, j+=2)
    {
    const eT tmp_i = X.at(row,i);
    const eT tmp_j = X.at(row,j);

    if(tmp_i < min_val) { min_val = tmp_i; }
    if(tmp_j < min_val) { min_val = tmp_j; }
    }

  if(i < X_n_cols)
    {
    const eT tmp_i = X.at(row,i);

    if(tmp_i < min_val) { min_val = tmp_i; }
    }

  return min_val;
  }



template<typename eT>
inline
eT
op_min::min(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  if(X.n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    return Datum<eT>::nan;
    }

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  eT min_val = priv::most_pos<eT>();

  if(X_n_rows == 1)
    {
    const Mat<eT>& A = X.m;

    const uword start_row = X.aux_row1;
    const uword start_col = X.aux_col1;

    const uword end_col_p1 = start_col + X_n_cols;

    uword i,j;
    for(i=start_col, j=start_col+1; j < end_col_p1; i+=2, j+=2)
      {
      const eT tmp_i = A.at(start_row, i);
      const eT tmp_j = A.at(start_row, j);

      if(tmp_i < min_val) { min_val = tmp_i; }
      if(tmp_j < min_val) { min_val = tmp_j; }
      }

    if(i < end_col_p1)
      {
      const eT tmp_i = A.at(start_row, i);

      if(tmp_i < min_val) { min_val = tmp_i; }
      }
    }
  else
    {
    for(uword col=0; col < X_n_cols; ++col)
      {
      min_val = (std::min)(min_val, op_min::direct_min(X.colptr(col), X_n_rows));
      }
    }

  return min_val;
  }



template<typename T1>
inline
typename arma_not_cx<typename T1::elem_type>::result
op_min::min(const Base<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> P(X.get_ref());

  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    return Datum<eT>::nan;
    }

  eT min_val = priv::most_pos<eT>();

  if(Proxy<T1>::use_at == false)
    {
    typedef typename Proxy<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    uword i,j;

    for(i=0, j=1; j<n_elem; i+=2, j+=2)
      {
      const eT tmp_i = A[i];
      const eT tmp_j = A[j];

      if(tmp_i < min_val) { min_val = tmp_i; }
      if(tmp_j < min_val) { min_val = tmp_j; }
      }

    if(i < n_elem)
      {
      const eT tmp_i = A[i];

      if(tmp_i < min_val) { min_val = tmp_i; }
      }
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_rows == 1)
      {
      uword i,j;
      for(i=0, j=1; j < n_cols; i+=2, j+=2)
        {
        const eT tmp_i = P.at(0,i);
        const eT tmp_j = P.at(0,j);

        if(tmp_i < min_val) { min_val = tmp_i; }
        if(tmp_j < min_val) { min_val = tmp_j; }
        }

      if(i < n_cols)
        {
        const eT tmp_i = P.at(0,i);

        if(tmp_i < min_val) { min_val = tmp_i; }
        }
      }
    else
      {
      for(uword col=0; col < n_cols; ++col)
        {
        uword i,j;
        for(i=0, j=1; j < n_rows; i+=2, j+=2)
          {
          const eT tmp_i = P.at(i,col);
          const eT tmp_j = P.at(j,col);

          if(tmp_i < min_val) { min_val = tmp_i; }
          if(tmp_j < min_val) { min_val = tmp_j; }
          }

        if(i < n_rows)
          {
          const eT tmp_i = P.at(i,col);

          if(tmp_i < min_val) { min_val = tmp_i; }
          }
        }
      }
    }

  return min_val;
  }



template<typename T1>
inline
typename arma_not_cx<typename T1::elem_type>::result
op_min::min(const BaseCube<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const ProxyCube<T1> P(X.get_ref());

  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    return Datum<eT>::nan;
    }

  eT min_val = priv::most_pos<eT>();

  if(ProxyCube<T1>::use_at == false)
    {
    typedef typename ProxyCube<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    uword i,j;

    for(i=0, j=1; j<n_elem; i+=2, j+=2)
      {
      const eT tmp_i = A[i];
      const eT tmp_j = A[j];

      if(tmp_i < min_val) { min_val = tmp_i; }
      if(tmp_j < min_val) { min_val = tmp_j; }
      }

    if(i < n_elem)
      {
      const eT tmp_i = A[i];

      if(tmp_i < min_val) { min_val = tmp_i; }
      }
    }
  else
    {
    const uword n_rows   = P.get_n_rows();
    const uword n_cols   = P.get_n_cols();
    const uword n_slices = P.get_n_slices();

    for(uword slice=0; slice < n_slices; ++slice)
    for(uword   col=0;   col < n_cols;   ++col  )
    for(uword   row=0;   row < n_rows;   ++row  )
      {
      const eT tmp = P.at(row,col,slice);

      if(tmp < min_val) { min_val = tmp; }
      }
    }

  return min_val;
  }



template<typename T1>
inline
typename arma_not_cx<typename T1::elem_type>::result
op_min::min_with_index(const Proxy<T1>& P, uword& index_of_min_val)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    return Datum<eT>::nan;
    }

  eT    best_val   = priv::most_pos<eT>();
  uword best_index = 0;

  if(Proxy<T1>::use_at == false)
    {
    typedef typename Proxy<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    for(uword i=0; i<n_elem; ++i)
      {
      const eT tmp = A[i];

      if(tmp < best_val)  { best_val = tmp;  best_index = i; }
      }
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_rows == 1)
      {
      for(uword i=0; i < n_cols; ++i)
        {
        const eT tmp = P.at(0,i);

        if(tmp < best_val)  { best_val = tmp;  best_index = i; }
        }
      }
    else
    if(n_cols == 1)
      {
      for(uword i=0; i < n_rows; ++i)
        {
        const eT tmp = P.at(i,0);

        if(tmp < best_val)  { best_val = tmp;  best_index = i; }
        }
      }
    else
      {
      uword count = 0;

      for(uword col=0; col < n_cols; ++col)
      for(uword row=0; row < n_rows; ++row)
        {
        const eT tmp = P.at(row,col);

        if(tmp < best_val)  { best_val = tmp;  best_index = count; }

        ++count;
        }
      }
    }

  index_of_min_val = best_index;

  return best_val;
  }



template<typename T1>
inline
typename arma_not_cx<typename T1::elem_type>::result
op_min::min_with_index(const ProxyCube<T1>& P, uword& index_of_min_val)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    return Datum<eT>::nan;
    }

  eT    best_val   = priv::most_pos<eT>();
  uword best_index = 0;

  if(ProxyCube<T1>::use_at == false)
    {
    typedef typename ProxyCube<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    for(uword i=0; i < n_elem; ++i)
      {
      const eT tmp = A[i];

      if(tmp < best_val)  { best_val = tmp;  best_index = i; }
      }
    }
  else
    {
    const uword n_rows   = P.get_n_rows();
    const uword n_cols   = P.get_n_cols();
    const uword n_slices = P.get_n_slices();

    uword count = 0;

    for(uword slice=0; slice < n_slices; ++slice)
    for(uword   col=0;   col < n_cols;   ++col  )
    for(uword   row=0;   row < n_rows;   ++row  )
      {
      const eT tmp = P.at(row,col,slice);

      if(tmp < best_val)  { best_val = tmp;  best_index = count; }

      ++count;
      }
    }

  index_of_min_val = best_index;

  return best_val;
  }



template<typename T>
inline
std::complex<T>
op_min::direct_min(const std::complex<T>* const X, const uword n_elem)
  {
  arma_extra_debug_sigprint();

  uword index   = 0;
  T     min_val = priv::most_pos<T>();

  for(uword i=0; i<n_elem; ++i)
    {
    const T tmp_val = std::abs(X[i]);

    if(tmp_val < min_val)
      {
      min_val = tmp_val;
      index   = i;
      }
    }

  return X[index];
  }



template<typename T>
inline
std::complex<T>
op_min::direct_min(const std::complex<T>* const X, const uword n_elem, uword& index_of_min_val)
  {
  arma_extra_debug_sigprint();

  uword index   = 0;
  T     min_val = priv::most_pos<T>();

  for(uword i=0; i<n_elem; ++i)
    {
    const T tmp_val = std::abs(X[i]);

    if(tmp_val < min_val)
      {
      min_val = tmp_val;
      index   = i;
      }
    }

  index_of_min_val = index;

  return X[index];
  }



template<typename T>
inline
std::complex<T>
op_min::direct_min(const Mat< std::complex<T> >& X, const uword row)
  {
  arma_extra_debug_sigprint();

  const uword X_n_cols = X.n_cols;

  uword index   = 0;
  T     min_val = priv::most_pos<T>();

  for(uword col=0; col<X_n_cols; ++col)
    {
    const T tmp_val = std::abs(X.at(row,col));

    if(tmp_val < min_val)
      {
      min_val = tmp_val;
      index   = col;
      }
    }

  return X.at(row,index);
  }



template<typename T>
inline
std::complex<T>
op_min::min(const subview< std::complex<T> >& X)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT;

  if(X.n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    return Datum<eT>::nan;
    }

  const Mat<eT>& A = X.m;

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  const uword start_row = X.aux_row1;
  const uword start_col = X.aux_col1;

  const uword end_row_p1 = start_row + X_n_rows;
  const uword end_col_p1 = start_col + X_n_cols;

  T min_val = priv::most_pos<T>();

  uword best_row = 0;
  uword best_col = 0;

  if(X_n_rows == 1)
    {
    best_col = 0;

    for(uword col=start_col; col < end_col_p1; ++col)
      {
      const T tmp_val = std::abs( A.at(start_row, col) );

      if(tmp_val < min_val)
        {
        min_val  = tmp_val;
        best_col = col;
        }
      }

    best_row = start_row;
    }
  else
    {
    for(uword col=start_col; col < end_col_p1; ++col)
    for(uword row=start_row; row < end_row_p1; ++row)
      {
      const T tmp_val = std::abs( A.at(row, col) );

      if(tmp_val < min_val)
        {
        min_val  = tmp_val;
        best_row = row;
        best_col = col;
        }
      }
    }

  return A.at(best_row, best_col);
  }



template<typename T1>
inline
typename arma_cx_only<typename T1::elem_type>::result
op_min::min(const Base<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result T;

  const Proxy<T1> P(X.get_ref());

  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    return Datum<eT>::nan;
    }

  T min_val = priv::most_pos<T>();

  if(Proxy<T1>::use_at == false)
    {
    typedef typename Proxy<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    uword index = 0;

    for(uword i=0; i<n_elem; ++i)
      {
      const T tmp = std::abs(A[i]);

      if(tmp < min_val)
        {
        min_val = tmp;
        index   = i;
        }
      }

    return( A[index] );
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    uword best_row = 0;
    uword best_col = 0;

    if(n_rows == 1)
      {
      for(uword col=0; col < n_cols; ++col)
        {
        const T tmp = std::abs(P.at(0,col));

        if(tmp < min_val)
          {
          min_val  = tmp;
          best_col = col;
          }
        }
      }
    else
      {
      for(uword col=0; col < n_cols; ++col)
      for(uword row=0; row < n_rows; ++row)
        {
        const T tmp = std::abs(P.at(row,col));

        if(tmp < min_val)
          {
          min_val = tmp;

          best_row = row;
          best_col = col;
          }
        }
      }

    return P.at(best_row, best_col);
    }
  }



template<typename T1>
inline
typename arma_cx_only<typename T1::elem_type>::result
op_min::min(const BaseCube<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result T;

  const ProxyCube<T1> P(X.get_ref());

  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    return Datum<eT>::nan;
    }

  T min_val = priv::most_pos<T>();

  if(ProxyCube<T1>::use_at == false)
    {
    typedef typename ProxyCube<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    uword index = 0;

    for(uword i=0; i<n_elem; ++i)
      {
      const T tmp = std::abs(A[i]);

      if(tmp < min_val)
        {
        min_val = tmp;
        index   = i;
        }
      }

    return( A[index] );
    }
  else
    {
    const uword n_rows   = P.get_n_rows();
    const uword n_cols   = P.get_n_cols();
    const uword n_slices = P.get_n_slices();

    eT min_val_orig = eT(0);

    for(uword slice=0; slice < n_slices; ++slice)
    for(uword   col=0;   col < n_cols;   ++col  )
    for(uword   row=0;   row < n_rows;   ++row  )
      {
      const eT tmp_orig = P.at(row,col,slice);
      const  T tmp      = std::abs(tmp_orig);

      if(tmp < min_val)
        {
        min_val      = tmp;
        min_val_orig = tmp_orig;
        }
      }

    return min_val_orig;
    }
  }



template<typename T1>
inline
typename arma_cx_only<typename T1::elem_type>::result
op_min::min_with_index(const Proxy<T1>& P, uword& index_of_min_val)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result T;

  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    return Datum<eT>::nan;
    }

  T best_val = priv::most_pos<T>();

  if(Proxy<T1>::use_at == false)
    {
    typedef typename Proxy<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    uword best_index = 0;

    for(uword i=0; i<n_elem; ++i)
      {
      const T tmp = std::abs(A[i]);

      if(tmp < best_val)  { best_val = tmp;  best_index = i; }
      }

    index_of_min_val = best_index;

    return( A[best_index] );
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    uword best_row   = 0;
    uword best_col   = 0;
    uword best_index = 0;

    if(n_rows == 1)
      {
      for(uword col=0; col < n_cols; ++col)
        {
        const T tmp = std::abs(P.at(0,col));

        if(tmp < best_val)  { best_val = tmp;  best_col = col; }
        }

      best_index = best_col;
      }
    else
    if(n_cols == 1)
      {
      for(uword row=0; row < n_rows; ++row)
        {
        const T tmp = std::abs(P.at(row,0));

        if(tmp < best_val)  { best_val = tmp;  best_row = row; }
        }

      best_index = best_row;
      }
    else
      {
      uword count = 0;

      for(uword col=0; col < n_cols; ++col)
      for(uword row=0; row < n_rows; ++row)
        {
        const T tmp = std::abs(P.at(row,col));

        if(tmp < best_val)
          {
          best_val = tmp;

          best_row = row;
          best_col = col;

          best_index = count;
          }

        ++count;
        }
      }

    index_of_min_val = best_index;

    return P.at(best_row, best_col);
    }
  }



template<typename T1>
inline
typename arma_cx_only<typename T1::elem_type>::result
op_min::min_with_index(const ProxyCube<T1>& P, uword& index_of_min_val)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result T;

  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    return Datum<eT>::nan;
    }

  T best_val = priv::most_pos<T>();

  if(ProxyCube<T1>::use_at == false)
    {
    typedef typename ProxyCube<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    uword best_index = 0;

    for(uword i=0; i < n_elem; ++i)
      {
      const T tmp = std::abs(A[i]);

      if(tmp < best_val)  { best_val = tmp;  best_index = i; }
      }

    index_of_min_val = best_index;

    return( A[best_index] );
    }
  else
    {
    const uword n_rows   = P.get_n_rows();
    const uword n_cols   = P.get_n_cols();
    const uword n_slices = P.get_n_slices();

    eT    best_val_orig = eT(0);
    uword best_index    = 0;
    uword count         = 0;

    for(uword slice=0; slice < n_slices; ++slice)
    for(uword   col=0;   col < n_cols;   ++col  )
    for(uword   row=0;   row < n_rows;   ++row  )
      {
      const eT tmp_orig = P.at(row,col,slice);
      const  T tmp      = std::abs(tmp_orig);

      if(tmp < best_val)
        {
        best_val      = tmp;
        best_val_orig = tmp_orig;
        best_index    = count;
        }

      ++count;
      }

    index_of_min_val = best_index;

    return best_val_orig;
    }
  }



//! @}
