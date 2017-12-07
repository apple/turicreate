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


//! \addtogroup subview_each
//! @{


//
//
// subview_each_common

template<typename parent, unsigned int mode>
inline
subview_each_common<parent,mode>::subview_each_common(const parent& in_P)
  : P(in_P)
  {
  arma_extra_debug_sigprint();
  }



template<typename parent, unsigned int mode>
arma_inline
const Mat<typename parent::elem_type>&
subview_each_common<parent,mode>::get_mat_ref_helper(const Mat<typename parent::elem_type>& X) const
  {
  return X;
  }



template<typename parent, unsigned int mode>
arma_inline
const Mat<typename parent::elem_type>&
subview_each_common<parent,mode>::get_mat_ref_helper(const subview<typename parent::elem_type>& X) const
  {
  return X.m;
  }



template<typename parent, unsigned int mode>
arma_inline
const Mat<typename parent::elem_type>&
subview_each_common<parent,mode>::get_mat_ref() const
  {
  return get_mat_ref_helper(P);
  }



template<typename parent, unsigned int mode>
inline
void
subview_each_common<parent,mode>::check_size(const Mat<typename parent::elem_type>& A) const
  {
  if(arma_config::debug == true)
    {
    if(mode == 0)
      {
      if( (A.n_rows != P.n_rows) || (A.n_cols != 1) )
        {
        arma_stop_logic_error( incompat_size_string(A) );
        }
      }
    else
      {
      if( (A.n_rows != 1) || (A.n_cols != P.n_cols) )
        {
        arma_stop_logic_error( incompat_size_string(A) );
        }
      }
    }
  }



template<typename parent, unsigned int mode>
arma_cold
inline
const std::string
subview_each_common<parent,mode>::incompat_size_string(const Mat<typename parent::elem_type>& A) const
  {
  std::stringstream tmp;

  if(mode == 0)
    {
    tmp << "each_col(): incompatible size; expected " << P.n_rows << "x1" << ", got " << A.n_rows << 'x' << A.n_cols;
    }
  else
    {
    tmp << "each_row(): incompatible size; expected 1x" << P.n_cols << ", got " << A.n_rows << 'x' << A.n_cols;
    }

  return tmp.str();
  }



//
//
// subview_each1



template<typename parent, unsigned int mode>
inline
subview_each1<parent,mode>::~subview_each1()
  {
  arma_extra_debug_sigprint();
  }



template<typename parent, unsigned int mode>
inline
subview_each1<parent,mode>::subview_each1(const parent& in_P)
  : subview_each_common<parent,mode>::subview_each_common(in_P)
  {
  arma_extra_debug_sigprint();
  }



template<typename parent, unsigned int mode>
template<typename T1>
inline
void
subview_each1<parent,mode>::operator= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  parent& p = access::rw(subview_each_common<parent,mode>::P);

  const unwrap_check<T1> tmp( in.get_ref(), (*this).get_mat_ref() );
  const Mat<eT>& A     = tmp.M;

  subview_each_common<parent,mode>::check_size(A);

  const eT*   A_mem    = A.memptr();
  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  if(mode == 0) // each column
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      arrayops::copy( p.colptr(i), A_mem, p_n_rows );
      }
    }
  else // each row
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      arrayops::inplace_set( p.colptr(i), A_mem[i], p_n_rows);
      }
    }
  }



template<typename parent, unsigned int mode>
template<typename T1>
inline
void
subview_each1<parent,mode>::operator+= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  parent& p = access::rw(subview_each_common<parent,mode>::P);

  const unwrap_check<T1> tmp( in.get_ref(), (*this).get_mat_ref() );
  const Mat<eT>& A     = tmp.M;

  subview_each_common<parent,mode>::check_size(A);

  const eT*   A_mem    = A.memptr();
  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  if(mode == 0) // each column
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      arrayops::inplace_plus( p.colptr(i), A_mem, p_n_rows );
      }
    }
  else // each row
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      arrayops::inplace_plus( p.colptr(i), A_mem[i], p_n_rows);
      }
    }
  }



template<typename parent, unsigned int mode>
template<typename T1>
inline
void
subview_each1<parent,mode>::operator-= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  parent& p = access::rw(subview_each_common<parent,mode>::P);

  const unwrap_check<T1> tmp( in.get_ref(), (*this).get_mat_ref() );
  const Mat<eT>& A     = tmp.M;

  subview_each_common<parent,mode>::check_size(A);

  const eT*   A_mem    = A.memptr();
  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  if(mode == 0) // each column
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      arrayops::inplace_minus( p.colptr(i), A_mem, p_n_rows );
      }
    }
  else // each row
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      arrayops::inplace_minus( p.colptr(i), A_mem[i], p_n_rows);
      }
    }
  }



template<typename parent, unsigned int mode>
template<typename T1>
inline
void
subview_each1<parent,mode>::operator%= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  parent& p = access::rw(subview_each_common<parent,mode>::P);

  const unwrap_check<T1> tmp( in.get_ref(), (*this).get_mat_ref() );
  const Mat<eT>& A     = tmp.M;

  subview_each_common<parent,mode>::check_size(A);

  const eT*   A_mem    = A.memptr();
  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  if(mode == 0) // each column
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      arrayops::inplace_mul( p.colptr(i), A_mem, p_n_rows );
      }
    }
  else // each row
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      arrayops::inplace_mul( p.colptr(i), A_mem[i], p_n_rows);
      }
    }
  }



template<typename parent, unsigned int mode>
template<typename T1>
inline
void
subview_each1<parent,mode>::operator/= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  parent& p = access::rw(subview_each_common<parent,mode>::P);

  const unwrap_check<T1> tmp( in.get_ref(), (*this).get_mat_ref() );
  const Mat<eT>& A     = tmp.M;

  subview_each_common<parent,mode>::check_size(A);

  const eT*   A_mem    = A.memptr();
  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  if(mode == 0) // each column
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      arrayops::inplace_div( p.colptr(i), A_mem, p_n_rows );
      }
    }
  else // each row
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      arrayops::inplace_div( p.colptr(i), A_mem[i], p_n_rows);
      }
    }
  }



//
//
// subview_each2



template<typename parent, unsigned int mode, typename TB>
inline
subview_each2<parent,mode,TB>::~subview_each2()
  {
  arma_extra_debug_sigprint();
  }



template<typename parent, unsigned int mode, typename TB>
inline
subview_each2<parent,mode,TB>::subview_each2(const parent& in_P, const Base<uword, TB>& in_indices)
  : subview_each_common<parent,mode>::subview_each_common(in_P)
  , base_indices(in_indices)
  {
  arma_extra_debug_sigprint();
  }



template<typename parent, unsigned int mode, typename TB>
inline
void
subview_each2<parent,mode,TB>::check_indices(const Mat<uword>& indices) const
  {
  if(mode == 0)
    {
    arma_debug_check( ((indices.is_vec() == false) && (indices.is_empty() == false)), "each_col(): list of indices must be a vector" );
    }
  else
    {
    arma_debug_check( ((indices.is_vec() == false) && (indices.is_empty() == false)), "each_row(): list of indices must be a vector" );
    }
  }



template<typename parent, unsigned int mode, typename TB>
template<typename T1>
inline
void
subview_each2<parent,mode,TB>::operator= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  parent& p = access::rw(subview_each_common<parent,mode>::P);

  const unwrap_check<T1> tmp( in.get_ref(), (*this).get_mat_ref() );
  const Mat<eT>& A     = tmp.M;

  subview_each_common<parent,mode>::check_size(A);

  const unwrap_check_mixed<TB> U( base_indices.get_ref(), (*this).get_mat_ref() );

  check_indices(U.M);

  const eT*   A_mem    = A.memptr();
  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  const uword* indices_mem = U.M.memptr();
  const uword  N           = U.M.n_elem;

  if(mode == 0) // each column
    {
    for(uword i=0; i < N; ++i)
      {
      const uword col = indices_mem[i];

      arma_debug_check( (col >= p_n_cols), "each_col(): index out of bounds" );

      arrayops::copy( p.colptr(col), A_mem, p_n_rows );
      }
    }
  else // each row
    {
    for(uword i=0; i < N; ++i)
      {
      const uword row = indices_mem[i];

      arma_debug_check( (row >= p_n_rows), "each_row(): index out of bounds" );

      for(uword col=0; col < p_n_cols; ++col)
        {
        p.at(row,col) = A_mem[col];
        }
      }
    }
  }



template<typename parent, unsigned int mode, typename TB>
template<typename T1>
inline
void
subview_each2<parent,mode,TB>::operator+= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  parent& p = access::rw(subview_each_common<parent,mode>::P);

  const unwrap_check<T1> tmp( in.get_ref(), (*this).get_mat_ref() );
  const Mat<eT>& A     = tmp.M;

  subview_each_common<parent,mode>::check_size(A);

  const unwrap_check_mixed<TB> U( base_indices.get_ref(), (*this).get_mat_ref() );

  check_indices(U.M);

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  const uword* indices_mem = U.M.memptr();
  const uword  N           = U.M.n_elem;

  if(mode == 0) // each column
    {
    const eT* A_mem = A.memptr();

    for(uword i=0; i < N; ++i)
      {
      const uword col = indices_mem[i];

      arma_debug_check( (col >= p_n_cols), "each_col(): index out of bounds" );

      arrayops::inplace_plus( p.colptr(col), A_mem, p_n_rows );
      }
    }
  else // each row
    {
    for(uword i=0; i < N; ++i)
      {
      const uword row = indices_mem[i];

      arma_debug_check( (row >= p_n_rows), "each_row(): index out of bounds" );

      p.row(row) += A;
      }
    }
  }



template<typename parent, unsigned int mode, typename TB>
template<typename T1>
inline
void
subview_each2<parent,mode,TB>::operator-= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  parent& p = access::rw(subview_each_common<parent,mode>::P);

  const unwrap_check<T1> tmp( in.get_ref(), (*this).get_mat_ref() );
  const Mat<eT>& A     = tmp.M;

  subview_each_common<parent,mode>::check_size(A);

  const unwrap_check_mixed<TB> U( base_indices.get_ref(), (*this).get_mat_ref() );

  check_indices(U.M);

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  const uword* indices_mem = U.M.memptr();
  const uword  N           = U.M.n_elem;

  if(mode == 0) // each column
    {
    const eT* A_mem = A.memptr();

    for(uword i=0; i < N; ++i)
      {
      const uword col = indices_mem[i];

      arma_debug_check( (col >= p_n_cols), "each_col(): index out of bounds" );

      arrayops::inplace_minus( p.colptr(col), A_mem, p_n_rows );
      }
    }
  else // each row
    {
    for(uword i=0; i < N; ++i)
      {
      const uword row = indices_mem[i];

      arma_debug_check( (row >= p_n_rows), "each_row(): index out of bounds" );

      p.row(row) -= A;
      }
    }
  }



template<typename parent, unsigned int mode, typename TB>
template<typename T1>
inline
void
subview_each2<parent,mode,TB>::operator%= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  parent& p = access::rw(subview_each_common<parent,mode>::P);

  const unwrap_check<T1> tmp( in.get_ref(), (*this).get_mat_ref() );
  const Mat<eT>& A     = tmp.M;

  subview_each_common<parent,mode>::check_size(A);

  const unwrap_check_mixed<TB> U( base_indices.get_ref(), (*this).get_mat_ref() );

  check_indices(U.M);

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  const uword* indices_mem = U.M.memptr();
  const uword  N           = U.M.n_elem;

  if(mode == 0) // each column
    {
    const eT* A_mem = A.memptr();

    for(uword i=0; i < N; ++i)
      {
      const uword col = indices_mem[i];

      arma_debug_check( (col >= p_n_cols), "each_col(): index out of bounds" );

      arrayops::inplace_mul( p.colptr(col), A_mem, p_n_rows );
      }
    }
  else // each row
    {
    for(uword i=0; i < N; ++i)
      {
      const uword row = indices_mem[i];

      arma_debug_check( (row >= p_n_rows), "each_row(): index out of bounds" );

      p.row(row) %= A;
      }
    }
  }



template<typename parent, unsigned int mode, typename TB>
template<typename T1>
inline
void
subview_each2<parent,mode,TB>::operator/= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  parent& p = access::rw(subview_each_common<parent,mode>::P);

  const unwrap_check<T1> tmp( in.get_ref(), (*this).get_mat_ref() );
  const Mat<eT>& A     = tmp.M;

  subview_each_common<parent,mode>::check_size(A);

  const unwrap_check_mixed<TB> U( base_indices.get_ref(), (*this).get_mat_ref() );

  check_indices(U.M);

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  const uword* indices_mem = U.M.memptr();
  const uword  N           = U.M.n_elem;

  if(mode == 0) // each column
    {
    const eT* A_mem = A.memptr();

    for(uword i=0; i < N; ++i)
      {
      const uword col = indices_mem[i];

      arma_debug_check( (col >= p_n_cols), "each_col(): index out of bounds" );

      arrayops::inplace_div( p.colptr(col), A_mem, p_n_rows );
      }
    }
  else // each row
    {
    for(uword i=0; i < N; ++i)
      {
      const uword row = indices_mem[i];

      arma_debug_check( (row >= p_n_rows), "each_row(): index out of bounds" );

      p.row(row) /= A;
      }
    }
  }



//
//
// subview_each1_aux



template<typename parent, unsigned int mode, typename T2>
inline
Mat<typename parent::elem_type>
subview_each1_aux::operator_plus
  (
  const subview_each1<parent,mode>&          X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = X.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out(p_n_rows, p_n_cols);

  const quasi_unwrap<T2> tmp(Y.get_ref());
  const Mat<eT>& A     = tmp.M;

  X.check_size(A);

  const eT* A_mem = A.memptr();

  if(mode == 0) // each column
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = p_mem[row] + A_mem[row];
        }
      }
    }

  if(mode == 1) // each row
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      const eT A_val = A_mem[i];

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = p_mem[row] + A_val;
        }
      }
    }

  return out;
  }



template<typename parent, unsigned int mode, typename T2>
inline
Mat<typename parent::elem_type>
subview_each1_aux::operator_minus
  (
  const subview_each1<parent,mode>&          X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = X.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out(p_n_rows, p_n_cols);

  const quasi_unwrap<T2> tmp(Y.get_ref());
  const Mat<eT>& A     = tmp.M;

  X.check_size(A);

  const eT* A_mem = A.memptr();

  if(mode == 0) // each column
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = p_mem[row] - A_mem[row];
        }
      }
    }

  if(mode == 1) // each row
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      const eT A_val = A_mem[i];

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = p_mem[row] - A_val;
        }
      }
    }

  return out;
  }



template<typename T1, typename parent, unsigned int mode>
inline
Mat<typename parent::elem_type>
subview_each1_aux::operator_minus
  (
  const Base<typename parent::elem_type,T1>& X,
  const subview_each1<parent,mode>&          Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = Y.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out(p_n_rows, p_n_cols);

  const quasi_unwrap<T1> tmp(X.get_ref());
  const Mat<eT>& A     = tmp.M;

  Y.check_size(A);

  const eT* A_mem = A.memptr();

  if(mode == 0) // each column
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = A_mem[row] - p_mem[row];
        }
      }
    }

  if(mode == 1) // each row
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      const eT A_val = A_mem[i];

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = A_val - p_mem[row];
        }
      }
    }

  return out;
  }



template<typename parent, unsigned int mode, typename T2>
inline
Mat<typename parent::elem_type>
subview_each1_aux::operator_schur
  (
  const subview_each1<parent,mode>&          X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = X.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out(p_n_rows, p_n_cols);

  const quasi_unwrap<T2> tmp(Y.get_ref());
  const Mat<eT>& A     = tmp.M;

  X.check_size(A);

  const eT* A_mem = A.memptr();

  if(mode == 0) // each column
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = p_mem[row] * A_mem[row];
        }
      }
    }

  if(mode == 1) // each row
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      const eT A_val = A_mem[i];

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = p_mem[row] * A_val;
        }
      }
    }

  return out;
  }



template<typename parent, unsigned int mode, typename T2>
inline
Mat<typename parent::elem_type>
subview_each1_aux::operator_div
  (
  const subview_each1<parent,mode>&          X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = X.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out(p_n_rows, p_n_cols);

  const quasi_unwrap<T2> tmp(Y.get_ref());
  const Mat<eT>& A     = tmp.M;

  X.check_size(A);

  const eT* A_mem = A.memptr();

  if(mode == 0) // each column
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = p_mem[row] / A_mem[row];
        }
      }
    }

  if(mode == 1) // each row
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      const eT A_val = A_mem[i];

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = p_mem[row] / A_val;
        }
      }
    }

  return out;
  }



template<typename T1, typename parent, unsigned int mode>
inline
Mat<typename parent::elem_type>
subview_each1_aux::operator_div
  (
  const Base<typename parent::elem_type,T1>& X,
  const subview_each1<parent,mode>&          Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = Y.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out(p_n_rows, p_n_cols);

  const quasi_unwrap<T1> tmp(X.get_ref());
  const Mat<eT>& A     = tmp.M;

  Y.check_size(A);

  const eT* A_mem = A.memptr();

  if(mode == 0) // each column
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = A_mem[row] / p_mem[row];
        }
      }
    }

  if(mode == 1) // each row
    {
    for(uword i=0; i < p_n_cols; ++i)
      {
      const eT*   p_mem =   p.colptr(i);
            eT* out_mem = out.colptr(i);

      const eT A_val = A_mem[i];

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = A_val / p_mem[row];
        }
      }
    }

  return out;
  }



//
//
// subview_each2_aux



template<typename parent, unsigned int mode, typename TB, typename T2>
inline
Mat<typename parent::elem_type>
subview_each2_aux::operator_plus
  (
  const subview_each2<parent,mode,TB>&       X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = X.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out = p;

  const quasi_unwrap<T2> tmp(Y.get_ref());
  const Mat<eT>& A     = tmp.M;

  const unwrap<TB> U(X.base_indices.get_ref());

  X.check_size(A);
  X.check_indices(U.M);

  const uword* indices_mem = U.M.memptr();
  const uword  N           = U.M.n_elem;

  if(mode == 0)  // process columns
    {
    const eT* A_mem = A.memptr();

    for(uword i=0; i < N; ++i)
      {
      const uword col = indices_mem[i];

      arma_debug_check( (col >= p_n_cols), "each_col(): index out of bounds" );

      arrayops::inplace_plus( out.colptr(col), A_mem, p_n_rows );
      }
    }

  if(mode == 1)  // process rows
    {
    for(uword i=0; i < N; ++i)
      {
      const uword row = indices_mem[i];

      arma_debug_check( (row >= p_n_rows), "each_row(): index out of bounds" );

      out.row(row) += A;
      }
    }

  return out;
  }



template<typename parent, unsigned int mode, typename TB, typename T2>
inline
Mat<typename parent::elem_type>
subview_each2_aux::operator_minus
  (
  const subview_each2<parent,mode,TB>&       X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = X.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out = p;

  const quasi_unwrap<T2> tmp(Y.get_ref());
  const Mat<eT>& A     = tmp.M;

  const unwrap<TB> U(X.base_indices.get_ref());

  X.check_size(A);
  X.check_indices(U.M);

  const uword* indices_mem = U.M.memptr();
  const uword  N           = U.M.n_elem;

  if(mode == 0)  // process columns
    {
    const eT* A_mem = A.memptr();

    for(uword i=0; i < N; ++i)
      {
      const uword col = indices_mem[i];

      arma_debug_check( (col >= p_n_cols), "each_col(): index out of bounds" );

      arrayops::inplace_minus( out.colptr(col), A_mem, p_n_rows );
      }
    }

  if(mode == 1)  // process rows
    {
    for(uword i=0; i < N; ++i)
      {
      const uword row = indices_mem[i];

      arma_debug_check( (row >= p_n_rows), "each_row(): index out of bounds" );

      out.row(row) -= A;
      }
    }

  return out;
  }



template<typename T1, typename parent, unsigned int mode, typename TB>
inline
Mat<typename parent::elem_type>
subview_each2_aux::operator_minus
  (
  const Base<typename parent::elem_type,T1>& X,
  const subview_each2<parent,mode,TB>&       Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = Y.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out = p;

  const quasi_unwrap<T1> tmp(X.get_ref());
  const Mat<eT>& A     = tmp.M;

  const unwrap<TB> U(Y.base_indices.get_ref());

  Y.check_size(A);
  Y.check_indices(U.M);

  const uword* indices_mem = U.M.memptr();
  const uword  N           = U.M.n_elem;

  if(mode == 0)  // process columns
    {
    const eT* A_mem = A.memptr();

    for(uword i=0; i < N; ++i)
      {
      const uword col = indices_mem[i];

      arma_debug_check( (col >= p_n_cols), "each_col(): index out of bounds" );

      const eT*   p_mem =   p.colptr(col);
            eT* out_mem = out.colptr(col);

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = A_mem[row] - p_mem[row];
        }
      }
    }

  if(mode == 1)  // process rows
    {
    for(uword i=0; i < N; ++i)
      {
      const uword row = indices_mem[i];

      arma_debug_check( (row >= p_n_rows), "each_row(): index out of bounds" );

      out.row(row) = A - p.row(row);
      }
    }

  return out;
  }



template<typename parent, unsigned int mode, typename TB, typename T2>
inline
Mat<typename parent::elem_type>
subview_each2_aux::operator_schur
  (
  const subview_each2<parent,mode,TB>&       X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = X.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out = p;

  const quasi_unwrap<T2> tmp(Y.get_ref());
  const Mat<eT>& A     = tmp.M;

  const unwrap<TB> U(X.base_indices.get_ref());

  X.check_size(A);
  X.check_indices(U.M);

  const uword* indices_mem = U.M.memptr();
  const uword  N           = U.M.n_elem;

  if(mode == 0)  // process columns
    {
    const eT* A_mem = A.memptr();

    for(uword i=0; i < N; ++i)
      {
      const uword col = indices_mem[i];

      arma_debug_check( (col >= p_n_cols), "each_col(): index out of bounds" );

      arrayops::inplace_mul( out.colptr(col), A_mem, p_n_rows );
      }
    }

  if(mode == 1)  // process rows
    {
    for(uword i=0; i < N; ++i)
      {
      const uword row = indices_mem[i];

      arma_debug_check( (row >= p_n_rows), "each_row(): index out of bounds" );

      out.row(row) %= A;
      }
    }

  return out;
  }



template<typename parent, unsigned int mode, typename TB, typename T2>
inline
Mat<typename parent::elem_type>
subview_each2_aux::operator_div
  (
  const subview_each2<parent,mode,TB>&       X,
  const Base<typename parent::elem_type,T2>& Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = X.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out = p;

  const quasi_unwrap<T2> tmp(Y.get_ref());
  const Mat<eT>& A     = tmp.M;

  const unwrap<TB> U(X.base_indices.get_ref());

  X.check_size(A);
  X.check_indices(U.M);

  const uword* indices_mem = U.M.memptr();
  const uword  N           = U.M.n_elem;

  if(mode == 0)  // process columns
    {
    const eT* A_mem = A.memptr();

    for(uword i=0; i < N; ++i)
      {
      const uword col = indices_mem[i];

      arma_debug_check( (col >= p_n_cols), "each_col(): index out of bounds" );

      arrayops::inplace_div( out.colptr(col), A_mem, p_n_rows );
      }
    }

  if(mode == 1)  // process rows
    {
    for(uword i=0; i < N; ++i)
      {
      const uword row = indices_mem[i];

      arma_debug_check( (row >= p_n_rows), "each_row(): index out of bounds" );

      out.row(row) /= A;
      }
    }

  return out;
  }



template<typename T1, typename parent, unsigned int mode, typename TB>
inline
Mat<typename parent::elem_type>
subview_each2_aux::operator_div
  (
  const Base<typename parent::elem_type,T1>& X,
  const subview_each2<parent,mode,TB>&       Y
  )
  {
  arma_extra_debug_sigprint();

  typedef typename parent::elem_type eT;

  const parent& p = Y.P;

  const uword p_n_rows = p.n_rows;
  const uword p_n_cols = p.n_cols;

  Mat<eT> out = p;

  const quasi_unwrap<T1> tmp(X.get_ref());
  const Mat<eT>& A     = tmp.M;

  const unwrap<TB> U(Y.base_indices.get_ref());

  Y.check_size(A);
  Y.check_indices(U.M);

  const uword* indices_mem = U.M.memptr();
  const uword  N           = U.M.n_elem;

  if(mode == 0)  // process columns
    {
    const eT* A_mem = A.memptr();

    for(uword i=0; i < N; ++i)
      {
      const uword col = indices_mem[i];

      arma_debug_check( (col >= p_n_cols), "each_col(): index out of bounds" );

      const eT*   p_mem =   p.colptr(col);
            eT* out_mem = out.colptr(col);

      for(uword row=0; row < p_n_rows; ++row)
        {
        out_mem[row] = A_mem[row] / p_mem[row];
        }
      }
    }

  if(mode == 1)  // process rows
    {
    for(uword i=0; i < N; ++i)
      {
      const uword row = indices_mem[i];

      arma_debug_check( (row >= p_n_rows), "each_row(): index out of bounds" );

      out.row(row) = A / p.row(row);
      }
    }

  return out;
  }



//! @}
