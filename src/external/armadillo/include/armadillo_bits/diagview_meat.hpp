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


//! \addtogroup diagview
//! @{


template<typename eT>
inline
diagview<eT>::~diagview()
  {
  arma_extra_debug_sigprint();
  }


template<typename eT>
arma_inline
diagview<eT>::diagview(const Mat<eT>& in_m, const uword in_row_offset, const uword in_col_offset, const uword in_len)
  : m(in_m)
  , row_offset(in_row_offset)
  , col_offset(in_col_offset)
  , n_rows(in_len)
  , n_elem(in_len)
  {
  arma_extra_debug_sigprint();
  }



//! set a diagonal of our matrix using a diagonal from a foreign matrix
template<typename eT>
inline
void
diagview<eT>::operator= (const diagview<eT>& x)
  {
  arma_extra_debug_sigprint();

  diagview<eT>& d = *this;

  arma_debug_check( (d.n_elem != x.n_elem), "diagview: diagonals have incompatible lengths");

        Mat<eT>& d_m = const_cast< Mat<eT>& >(d.m);
  const Mat<eT>& x_m = x.m;

  if(&d_m != &x_m)
    {
    const uword d_n_elem     = d.n_elem;
    const uword d_row_offset = d.row_offset;
    const uword d_col_offset = d.col_offset;

    const uword x_row_offset = x.row_offset;
    const uword x_col_offset = x.col_offset;

    uword ii,jj;
    for(ii=0, jj=1; jj < d_n_elem; ii+=2, jj+=2)
      {
      const eT tmp_i = x_m.at(ii + x_row_offset, ii + x_col_offset);
      const eT tmp_j = x_m.at(jj + x_row_offset, jj + x_col_offset);

      d_m.at(ii + d_row_offset, ii + d_col_offset) = tmp_i;
      d_m.at(jj + d_row_offset, jj + d_col_offset) = tmp_j;
      }

    if(ii < d_n_elem)
      {
      d_m.at(ii + d_row_offset, ii + d_col_offset) = x_m.at(ii + x_row_offset, ii + x_col_offset);
      }
    }
  else
    {
    const Mat<eT> tmp = x;

    (*this).operator=(tmp);
    }
  }



template<typename eT>
inline
void
diagview<eT>::operator+=(const eT val)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& t_m = const_cast< Mat<eT>& >(m);

  const uword t_n_elem     = n_elem;
  const uword t_row_offset = row_offset;
  const uword t_col_offset = col_offset;

  for(uword ii=0; ii < t_n_elem; ++ii)
    {
    t_m.at( ii + t_row_offset,  ii + t_col_offset) += val;
    }
  }



template<typename eT>
inline
void
diagview<eT>::operator-=(const eT val)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& t_m = const_cast< Mat<eT>& >(m);

  const uword t_n_elem     = n_elem;
  const uword t_row_offset = row_offset;
  const uword t_col_offset = col_offset;

  for(uword ii=0; ii < t_n_elem; ++ii)
    {
    t_m.at( ii + t_row_offset,  ii + t_col_offset) -= val;
    }
  }



template<typename eT>
inline
void
diagview<eT>::operator*=(const eT val)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& t_m = const_cast< Mat<eT>& >(m);

  const uword t_n_elem     = n_elem;
  const uword t_row_offset = row_offset;
  const uword t_col_offset = col_offset;

  for(uword ii=0; ii < t_n_elem; ++ii)
    {
    t_m.at( ii + t_row_offset,  ii + t_col_offset) *= val;
    }
  }



template<typename eT>
inline
void
diagview<eT>::operator/=(const eT val)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& t_m = const_cast< Mat<eT>& >(m);

  const uword t_n_elem     = n_elem;
  const uword t_row_offset = row_offset;
  const uword t_col_offset = col_offset;

  for(uword ii=0; ii < t_n_elem; ++ii)
    {
    t_m.at( ii + t_row_offset,  ii + t_col_offset) /= val;
    }
  }



//! set a diagonal of our matrix using data from a foreign object
template<typename eT>
template<typename T1>
inline
void
diagview<eT>::operator= (const Base<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  diagview<eT>& d = *this;

  Mat<eT>& d_m = const_cast< Mat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const Proxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "diagview: given object has incompatible size"
    );

  const bool is_alias = P.is_alias(d_m);

  if(is_alias)  { arma_extra_debug_print("aliasing detected"); }

  if( (is_Mat<typename Proxy<T1>::stored_type>::value) || (Proxy<T1>::use_at) || (is_alias) )
    {
    const unwrap_check<typename Proxy<T1>::stored_type> tmp(P.Q, is_alias);
    const Mat<eT>& x = tmp.M;

    const eT* x_mem = x.memptr();

    uword ii,jj;
    for(ii=0, jj=1; jj < d_n_elem; ii+=2, jj+=2)
      {
      const eT tmp_i = x_mem[ii];
      const eT tmp_j = x_mem[jj];

      d_m.at( ii + d_row_offset,  ii + d_col_offset) = tmp_i;
      d_m.at( jj + d_row_offset,  jj + d_col_offset) = tmp_j;
      }

    if(ii < d_n_elem)
      {
      d_m.at( ii + d_row_offset,  ii + d_col_offset) = x_mem[ii];
      }
    }
  else
    {
    typename Proxy<T1>::ea_type Pea = P.get_ea();

    uword ii,jj;
    for(ii=0, jj=1; jj < d_n_elem; ii+=2, jj+=2)
      {
      const eT tmp_i = Pea[ii];
      const eT tmp_j = Pea[jj];

      d_m.at( ii + d_row_offset,  ii + d_col_offset) = tmp_i;
      d_m.at( jj + d_row_offset,  jj + d_col_offset) = tmp_j;
      }

    if(ii < d_n_elem)
      {
      d_m.at( ii + d_row_offset,  ii + d_col_offset) = Pea[ii];
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
diagview<eT>::operator+=(const Base<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  diagview<eT>& d = *this;

  Mat<eT>& d_m = const_cast< Mat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const Proxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "diagview: given object has incompatible size"
    );

  const bool is_alias = P.is_alias(d_m);

  if(is_alias)  { arma_extra_debug_print("aliasing detected"); }

  if( (is_Mat<typename Proxy<T1>::stored_type>::value) || (Proxy<T1>::use_at) || (is_alias) )
    {
    const unwrap_check<typename Proxy<T1>::stored_type> tmp(P.Q, is_alias);
    const Mat<eT>& x = tmp.M;

    const eT* x_mem = x.memptr();

    uword ii,jj;
    for(ii=0, jj=1; jj < d_n_elem; ii+=2, jj+=2)
      {
      const eT tmp_i = x_mem[ii];
      const eT tmp_j = x_mem[jj];

      d_m.at( ii + d_row_offset,  ii + d_col_offset) += tmp_i;
      d_m.at( jj + d_row_offset,  jj + d_col_offset) += tmp_j;
      }

    if(ii < d_n_elem)
      {
      d_m.at( ii + d_row_offset,  ii + d_col_offset) += x_mem[ii];
      }
    }
  else
    {
    typename Proxy<T1>::ea_type Pea = P.get_ea();

    uword ii,jj;
    for(ii=0, jj=1; jj < d_n_elem; ii+=2, jj+=2)
      {
      const eT tmp_i = Pea[ii];
      const eT tmp_j = Pea[jj];

      d_m.at( ii + d_row_offset,  ii + d_col_offset) += tmp_i;
      d_m.at( jj + d_row_offset,  jj + d_col_offset) += tmp_j;
      }

    if(ii < d_n_elem)
      {
      d_m.at( ii + d_row_offset,  ii + d_col_offset) += Pea[ii];
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
diagview<eT>::operator-=(const Base<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  diagview<eT>& d = *this;

  Mat<eT>& d_m = const_cast< Mat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const Proxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "diagview: given object has incompatible size"
    );

  const bool is_alias = P.is_alias(d_m);

  if(is_alias)  { arma_extra_debug_print("aliasing detected"); }

  if( (is_Mat<typename Proxy<T1>::stored_type>::value) || (Proxy<T1>::use_at) || (is_alias) )
    {
    const unwrap_check<typename Proxy<T1>::stored_type> tmp(P.Q, is_alias);
    const Mat<eT>& x = tmp.M;

    const eT* x_mem = x.memptr();

    uword ii,jj;
    for(ii=0, jj=1; jj < d_n_elem; ii+=2, jj+=2)
      {
      const eT tmp_i = x_mem[ii];
      const eT tmp_j = x_mem[jj];

      d_m.at( ii + d_row_offset,  ii + d_col_offset) -= tmp_i;
      d_m.at( jj + d_row_offset,  jj + d_col_offset) -= tmp_j;
      }

    if(ii < d_n_elem)
      {
      d_m.at( ii + d_row_offset,  ii + d_col_offset) -= x_mem[ii];
      }
    }
  else
    {
    typename Proxy<T1>::ea_type Pea = P.get_ea();

    uword ii,jj;
    for(ii=0, jj=1; jj < d_n_elem; ii+=2, jj+=2)
      {
      const eT tmp_i = Pea[ii];
      const eT tmp_j = Pea[jj];

      d_m.at( ii + d_row_offset,  ii + d_col_offset) -= tmp_i;
      d_m.at( jj + d_row_offset,  jj + d_col_offset) -= tmp_j;
      }

    if(ii < d_n_elem)
      {
      d_m.at( ii + d_row_offset,  ii + d_col_offset) -= Pea[ii];
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
diagview<eT>::operator%=(const Base<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  diagview<eT>& d = *this;

  Mat<eT>& d_m = const_cast< Mat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const Proxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "diagview: given object has incompatible size"
    );

  const bool is_alias = P.is_alias(d_m);

  if(is_alias)  { arma_extra_debug_print("aliasing detected"); }

  if( (is_Mat<typename Proxy<T1>::stored_type>::value) || (Proxy<T1>::use_at) || (is_alias) )
    {
    const unwrap_check<typename Proxy<T1>::stored_type> tmp(P.Q, is_alias);
    const Mat<eT>& x = tmp.M;

    const eT* x_mem = x.memptr();

    uword ii,jj;
    for(ii=0, jj=1; jj < d_n_elem; ii+=2, jj+=2)
      {
      const eT tmp_i = x_mem[ii];
      const eT tmp_j = x_mem[jj];

      d_m.at( ii + d_row_offset,  ii + d_col_offset) *= tmp_i;
      d_m.at( jj + d_row_offset,  jj + d_col_offset) *= tmp_j;
      }

    if(ii < d_n_elem)
      {
      d_m.at( ii + d_row_offset,  ii + d_col_offset) *= x_mem[ii];
      }
    }
  else
    {
    typename Proxy<T1>::ea_type Pea = P.get_ea();

    uword ii,jj;
    for(ii=0, jj=1; jj < d_n_elem; ii+=2, jj+=2)
      {
      const eT tmp_i = Pea[ii];
      const eT tmp_j = Pea[jj];

      d_m.at( ii + d_row_offset,  ii + d_col_offset) *= tmp_i;
      d_m.at( jj + d_row_offset,  jj + d_col_offset) *= tmp_j;
      }

    if(ii < d_n_elem)
      {
      d_m.at( ii + d_row_offset,  ii + d_col_offset) *= Pea[ii];
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
diagview<eT>::operator/=(const Base<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  diagview<eT>& d = *this;

  Mat<eT>& d_m = const_cast< Mat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const Proxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "diagview: given object has incompatible size"
    );

  const bool is_alias = P.is_alias(d_m);

  if(is_alias)  { arma_extra_debug_print("aliasing detected"); }

  if( (is_Mat<typename Proxy<T1>::stored_type>::value) || (Proxy<T1>::use_at) || (is_alias) )
    {
    const unwrap_check<typename Proxy<T1>::stored_type> tmp(P.Q, is_alias);
    const Mat<eT>& x = tmp.M;

    const eT* x_mem = x.memptr();

    uword ii,jj;
    for(ii=0, jj=1; jj < d_n_elem; ii+=2, jj+=2)
      {
      const eT tmp_i = x_mem[ii];
      const eT tmp_j = x_mem[jj];

      d_m.at( ii + d_row_offset,  ii + d_col_offset) /= tmp_i;
      d_m.at( jj + d_row_offset,  jj + d_col_offset) /= tmp_j;
      }

    if(ii < d_n_elem)
      {
      d_m.at( ii + d_row_offset,  ii + d_col_offset) /= x_mem[ii];
      }
    }
  else
    {
    typename Proxy<T1>::ea_type Pea = P.get_ea();

    uword ii,jj;
    for(ii=0, jj=1; jj < d_n_elem; ii+=2, jj+=2)
      {
      const eT tmp_i = Pea[ii];
      const eT tmp_j = Pea[jj];

      d_m.at( ii + d_row_offset,  ii + d_col_offset) /= tmp_i;
      d_m.at( jj + d_row_offset,  jj + d_col_offset) /= tmp_j;
      }

    if(ii < d_n_elem)
      {
      d_m.at( ii + d_row_offset,  ii + d_col_offset) /= Pea[ii];
      }
    }
  }



//! extract a diagonal and store it as a column vector
template<typename eT>
inline
void
diagview<eT>::extract(Mat<eT>& out, const diagview<eT>& in)
  {
  arma_extra_debug_sigprint();

  // NOTE: we're assuming that the matrix has already been set to the correct size and there is no aliasing;
  // size setting and alias checking is done by either the Mat contructor or operator=()

  const Mat<eT>& in_m = in.m;

  const uword in_n_elem     = in.n_elem;
  const uword in_row_offset = in.row_offset;
  const uword in_col_offset = in.col_offset;

  eT* out_mem = out.memptr();

  uword i,j;
  for(i=0, j=1; j < in_n_elem; i+=2, j+=2)
    {
    const eT tmp_i = in_m.at( i + in_row_offset, i + in_col_offset );
    const eT tmp_j = in_m.at( j + in_row_offset, j + in_col_offset );

    out_mem[i] = tmp_i;
    out_mem[j] = tmp_j;
    }

  if(i < in_n_elem)
    {
    out_mem[i] = in_m.at( i + in_row_offset, i + in_col_offset );
    }
  }



//! X += Y.diag()
template<typename eT>
inline
void
diagview<eT>::plus_inplace(Mat<eT>& out, const diagview<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, in.n_rows, in.n_cols, "addition");

  const Mat<eT>& in_m = in.m;

  const uword in_n_elem     = in.n_elem;
  const uword in_row_offset = in.row_offset;
  const uword in_col_offset = in.col_offset;

  eT* out_mem = out.memptr();

  uword i,j;
  for(i=0, j=1; j < in_n_elem; i+=2, j+=2)
    {
    const eT tmp_i = in_m.at( i + in_row_offset, i + in_col_offset );
    const eT tmp_j = in_m.at( j + in_row_offset, j + in_col_offset );

    out_mem[i] += tmp_i;
    out_mem[j] += tmp_j;
    }

  if(i < in_n_elem)
    {
    out_mem[i] += in_m.at( i + in_row_offset, i + in_col_offset );
    }
  }



//! X -= Y.diag()
template<typename eT>
inline
void
diagview<eT>::minus_inplace(Mat<eT>& out, const diagview<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, in.n_rows, in.n_cols, "subtraction");

  const Mat<eT>& in_m = in.m;

  const uword in_n_elem     = in.n_elem;
  const uword in_row_offset = in.row_offset;
  const uword in_col_offset = in.col_offset;

  eT* out_mem = out.memptr();

  uword i,j;
  for(i=0, j=1; j < in_n_elem; i+=2, j+=2)
    {
    const eT tmp_i = in_m.at( i + in_row_offset, i + in_col_offset );
    const eT tmp_j = in_m.at( j + in_row_offset, j + in_col_offset );

    out_mem[i] -= tmp_i;
    out_mem[j] -= tmp_j;
    }

  if(i < in_n_elem)
    {
    out_mem[i] -= in_m.at( i + in_row_offset, i + in_col_offset );
    }
  }



//! X %= Y.diag()
template<typename eT>
inline
void
diagview<eT>::schur_inplace(Mat<eT>& out, const diagview<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, in.n_rows, in.n_cols, "element-wise multiplication");

  const Mat<eT>& in_m = in.m;

  const uword in_n_elem     = in.n_elem;
  const uword in_row_offset = in.row_offset;
  const uword in_col_offset = in.col_offset;

  eT* out_mem = out.memptr();

  uword i,j;
  for(i=0, j=1; j < in_n_elem; i+=2, j+=2)
    {
    const eT tmp_i = in_m.at( i + in_row_offset, i + in_col_offset );
    const eT tmp_j = in_m.at( j + in_row_offset, j + in_col_offset );

    out_mem[i] *= tmp_i;
    out_mem[j] *= tmp_j;
    }

  if(i < in_n_elem)
    {
    out_mem[i] *= in_m.at( i + in_row_offset, i + in_col_offset );
    }
  }



//! X /= Y.diag()
template<typename eT>
inline
void
diagview<eT>::div_inplace(Mat<eT>& out, const diagview<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, in.n_rows, in.n_cols, "element-wise division");

  const Mat<eT>& in_m = in.m;

  const uword in_n_elem     = in.n_elem;
  const uword in_row_offset = in.row_offset;
  const uword in_col_offset = in.col_offset;

  eT* out_mem = out.memptr();

  uword i,j;
  for(i=0, j=1; j < in_n_elem; i+=2, j+=2)
    {
    const eT tmp_i = in_m.at( i + in_row_offset, i + in_col_offset );
    const eT tmp_j = in_m.at( j + in_row_offset, j + in_col_offset );

    out_mem[i] /= tmp_i;
    out_mem[j] /= tmp_j;
    }

  if(i < in_n_elem)
    {
    out_mem[i] /= in_m.at( i + in_row_offset, i + in_col_offset );
    }
  }



template<typename eT>
arma_inline
eT
diagview<eT>::at_alt(const uword ii) const
  {
  return m.at(ii+row_offset, ii+col_offset);
  }



template<typename eT>
arma_inline
eT&
diagview<eT>::operator[](const uword ii)
  {
  return (const_cast< Mat<eT>& >(m)).at(ii+row_offset, ii+col_offset);
  }



template<typename eT>
arma_inline
eT
diagview<eT>::operator[](const uword ii) const
  {
  return m.at(ii+row_offset, ii+col_offset);
  }



template<typename eT>
arma_inline
eT&
diagview<eT>::at(const uword ii)
  {
  return (const_cast< Mat<eT>& >(m)).at(ii+row_offset, ii+col_offset);
  }



template<typename eT>
arma_inline
eT
diagview<eT>::at(const uword ii) const
  {
  return m.at(ii+row_offset, ii+col_offset);
  }



template<typename eT>
arma_inline
eT&
diagview<eT>::operator()(const uword ii)
  {
  arma_debug_check( (ii >= n_elem), "diagview::operator(): out of bounds" );

  return (const_cast< Mat<eT>& >(m)).at(ii+row_offset, ii+col_offset);
  }



template<typename eT>
arma_inline
eT
diagview<eT>::operator()(const uword ii) const
  {
  arma_debug_check( (ii >= n_elem), "diagview::operator(): out of bounds" );

  return m.at(ii+row_offset, ii+col_offset);
  }



template<typename eT>
arma_inline
eT&
diagview<eT>::at(const uword row, const uword)
  {
  return (const_cast< Mat<eT>& >(m)).at(row+row_offset, row+col_offset);
  }



template<typename eT>
arma_inline
eT
diagview<eT>::at(const uword row, const uword) const
  {
  return m.at(row+row_offset, row+col_offset);
  }



template<typename eT>
arma_inline
eT&
diagview<eT>::operator()(const uword row, const uword col)
  {
  arma_debug_check( ((row >= n_elem) || (col > 0)), "diagview::operator(): out of bounds" );

  return (const_cast< Mat<eT>& >(m)).at(row+row_offset, row+col_offset);
  }



template<typename eT>
arma_inline
eT
diagview<eT>::operator()(const uword row, const uword col) const
  {
  arma_debug_check( ((row >= n_elem) || (col > 0)), "diagview::operator(): out of bounds" );

  return m.at(row+row_offset, row+col_offset);
  }



template<typename eT>
arma_inline
const Op<diagview<eT>,op_htrans>
diagview<eT>::t() const
  {
  return Op<diagview<eT>,op_htrans>(*this);
  }



template<typename eT>
arma_inline
const Op<diagview<eT>,op_htrans>
diagview<eT>::ht() const
  {
  return Op<diagview<eT>,op_htrans>(*this);
  }



template<typename eT>
arma_inline
const Op<diagview<eT>,op_strans>
diagview<eT>::st() const
  {
  return Op<diagview<eT>,op_strans>(*this);
  }



template<typename eT>
inline
void
diagview<eT>::replace(const eT old_val, const eT new_val)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& x = const_cast< Mat<eT>& >(m);

  const uword local_n_elem = n_elem;

  if(arma_isnan(old_val))
    {
    for(uword ii=0; ii < local_n_elem; ++ii)
      {
      eT& val = x.at(ii+row_offset, ii+col_offset);

      val = (arma_isnan(val)) ? new_val : val;
      }
    }
  else
    {
    for(uword ii=0; ii < local_n_elem; ++ii)
      {
      eT& val = x.at(ii+row_offset, ii+col_offset);

      val = (val == old_val) ? new_val : val;
      }
    }
  }



template<typename eT>
inline
void
diagview<eT>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& x = const_cast< Mat<eT>& >(m);

  const uword local_n_elem = n_elem;

  for(uword ii=0; ii < local_n_elem; ++ii)
    {
    x.at(ii+row_offset, ii+col_offset) = val;
    }
  }



template<typename eT>
inline
void
diagview<eT>::zeros()
  {
  arma_extra_debug_sigprint();

  (*this).fill(eT(0));
  }



template<typename eT>
inline
void
diagview<eT>::ones()
  {
  arma_extra_debug_sigprint();

  (*this).fill(eT(1));
  }



template<typename eT>
inline
void
diagview<eT>::randu()
  {
  arma_extra_debug_sigprint();

  Mat<eT>& x = const_cast< Mat<eT>& >(m);

  const uword local_n_elem = n_elem;

  for(uword ii=0; ii < local_n_elem; ++ii)
    {
    x.at(ii+row_offset, ii+col_offset) = eT(arma_rng::randu<eT>());
    }
  }



template<typename eT>
inline
void
diagview<eT>::randn()
  {
  arma_extra_debug_sigprint();

  Mat<eT>& x = const_cast< Mat<eT>& >(m);

  const uword local_n_elem = n_elem;

  for(uword ii=0; ii < local_n_elem; ++ii)
    {
    x.at(ii+row_offset, ii+col_offset) = eT(arma_rng::randn<eT>());
    }
  }



//! @}
