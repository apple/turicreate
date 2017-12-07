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


//! \addtogroup spdiagview
//! @{


template<typename eT>
inline
spdiagview<eT>::~spdiagview()
  {
  arma_extra_debug_sigprint();
  }


template<typename eT>
arma_inline
spdiagview<eT>::spdiagview(const SpMat<eT>& in_m, const uword in_row_offset, const uword in_col_offset, const uword in_len)
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
spdiagview<eT>::operator= (const spdiagview<eT>& x)
  {
  arma_extra_debug_sigprint();

  spdiagview<eT>& d = *this;

  arma_debug_check( (d.n_elem != x.n_elem), "spdiagview: diagonals have incompatible lengths");

        SpMat<eT>& d_m = const_cast< SpMat<eT>& >(d.m);
  const SpMat<eT>& x_m = x.m;

  if(&d_m != &x_m)
    {
    const uword d_n_elem     = d.n_elem;
    const uword d_row_offset = d.row_offset;
    const uword d_col_offset = d.col_offset;

    const uword x_row_offset = x.row_offset;
    const uword x_col_offset = x.col_offset;

    for(uword i=0; i < d_n_elem; ++i)
      {
      d_m.at(i + d_row_offset, i + d_col_offset) = x_m.at(i + x_row_offset, i + x_col_offset);
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
spdiagview<eT>::operator+=(const eT val)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>& t_m = const_cast< SpMat<eT>& >(m);

  const uword t_n_elem     = n_elem;
  const uword t_row_offset = row_offset;
  const uword t_col_offset = col_offset;

  for(uword i=0; i < t_n_elem; ++i)
    {
    t_m.at(i + t_row_offset, i + t_col_offset) += val;
    }
  }



template<typename eT>
inline
void
spdiagview<eT>::operator-=(const eT val)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>& t_m = const_cast< SpMat<eT>& >(m);

  const uword t_n_elem     = n_elem;
  const uword t_row_offset = row_offset;
  const uword t_col_offset = col_offset;

  for(uword i=0; i < t_n_elem; ++i)
    {
    t_m.at(i + t_row_offset, i + t_col_offset) -= val;
    }
  }



template<typename eT>
inline
void
spdiagview<eT>::operator*=(const eT val)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>& t_m = const_cast< SpMat<eT>& >(m);

  const uword t_n_elem     = n_elem;
  const uword t_row_offset = row_offset;
  const uword t_col_offset = col_offset;

  for(uword i=0; i < t_n_elem; ++i)
    {
    t_m.at(i + t_row_offset, i + t_col_offset) *= val;
    }
  }



template<typename eT>
inline
void
spdiagview<eT>::operator/=(const eT val)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>& t_m = const_cast< SpMat<eT>& >(m);

  const uword t_n_elem     = n_elem;
  const uword t_row_offset = row_offset;
  const uword t_col_offset = col_offset;

  for(uword i=0; i < t_n_elem; ++i)
    {
    t_m.at(i + t_row_offset, i + t_col_offset) /= val;
    }
  }



//! set a diagonal of our matrix using data from a foreign object
template<typename eT>
template<typename T1>
inline
void
spdiagview<eT>::operator= (const Base<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  spdiagview<eT>& d = *this;

  SpMat<eT>& d_m = const_cast< SpMat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const Proxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "spdiagview: given object has incompatible size"
    );

  if( (is_Mat<typename Proxy<T1>::stored_type>::value) || (Proxy<T1>::use_at) )
    {
    const unwrap<typename Proxy<T1>::stored_type> tmp(P.Q);
    const Mat<eT>& x = tmp.M;

    const eT* x_mem = x.memptr();

    for(uword i=0; i < d_n_elem; ++i)
      {
      d_m.at(i + d_row_offset, i + d_col_offset) = x_mem[i];
      }
    }
  else
    {
    typename Proxy<T1>::ea_type Pea = P.get_ea();

    for(uword i=0; i < d_n_elem; ++i)
      {
      d_m.at(i + d_row_offset, i + d_col_offset) = Pea[i];
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
spdiagview<eT>::operator+=(const Base<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  spdiagview<eT>& d = *this;

  SpMat<eT>& d_m = const_cast< SpMat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const Proxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "spdiagview: given object has incompatible size"
    );

  if( (is_Mat<typename Proxy<T1>::stored_type>::value) || (Proxy<T1>::use_at) )
    {
    const unwrap<typename Proxy<T1>::stored_type> tmp(P.Q);
    const Mat<eT>& x = tmp.M;

    const eT* x_mem = x.memptr();

    for(uword i=0; i < d_n_elem; ++i)
      {
      d_m.at(i + d_row_offset, i + d_col_offset) += x_mem[i];
      }
    }
  else
    {
    typename Proxy<T1>::ea_type Pea = P.get_ea();

    for(uword i=0; i < d_n_elem; ++i)
      {
      d_m.at(i + d_row_offset, i + d_col_offset) += Pea[i];
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
spdiagview<eT>::operator-=(const Base<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  spdiagview<eT>& d = *this;

  SpMat<eT>& d_m = const_cast< SpMat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const Proxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "spdiagview: given object has incompatible size"
    );

  if( (is_Mat<typename Proxy<T1>::stored_type>::value) || (Proxy<T1>::use_at) )
    {
    const unwrap<typename Proxy<T1>::stored_type> tmp(P.Q);
    const Mat<eT>& x = tmp.M;

    const eT* x_mem = x.memptr();

    for(uword i=0; i < d_n_elem; ++i)
      {
      d_m.at(i + d_row_offset, i + d_col_offset) -= x_mem[i];
      }
    }
  else
    {
    typename Proxy<T1>::ea_type Pea = P.get_ea();

    for(uword i=0; i < d_n_elem; ++i)
      {
      d_m.at(i + d_row_offset, i + d_col_offset) -= Pea[i];
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
spdiagview<eT>::operator%=(const Base<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  spdiagview<eT>& d = *this;

  SpMat<eT>& d_m = const_cast< SpMat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const Proxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "spdiagview: given object has incompatible size"
    );

  if( (is_Mat<typename Proxy<T1>::stored_type>::value) || (Proxy<T1>::use_at) )
    {
    const unwrap<typename Proxy<T1>::stored_type> tmp(P.Q);
    const Mat<eT>& x = tmp.M;

    const eT* x_mem = x.memptr();

    for(uword i=0; i < d_n_elem; ++i)
      {
      d_m.at(i + d_row_offset, i + d_col_offset) *= x_mem[i];
      }
    }
  else
    {
    typename Proxy<T1>::ea_type Pea = P.get_ea();

    for(uword i=0; i < d_n_elem; ++i)
      {
      d_m.at(i + d_row_offset, i + d_col_offset) *= Pea[i];
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
spdiagview<eT>::operator/=(const Base<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  spdiagview<eT>& d = *this;

  SpMat<eT>& d_m = const_cast< SpMat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const Proxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "spdiagview: given object has incompatible size"
    );

  if( (is_Mat<typename Proxy<T1>::stored_type>::value) || (Proxy<T1>::use_at) )
    {
    const unwrap<typename Proxy<T1>::stored_type> tmp(P.Q);
    const Mat<eT>& x = tmp.M;

    const eT* x_mem = x.memptr();

    for(uword i=0; i < d_n_elem; ++i)
      {
      d_m.at(i + d_row_offset, i + d_col_offset) /= x_mem[i];
      }
    }
  else
    {
    typename Proxy<T1>::ea_type Pea = P.get_ea();

    for(uword i=0; i < d_n_elem; ++i)
      {
      d_m.at(i + d_row_offset, i + d_col_offset) /= Pea[i];
      }
    }
  }



//! set a diagonal of our matrix using data from a foreign object
template<typename eT>
template<typename T1>
inline
void
spdiagview<eT>::operator= (const SpBase<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  spdiagview<eT>& d = *this;

  SpMat<eT>& d_m = const_cast< SpMat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const SpProxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "spdiagview: given object has incompatible size"
    );

  if( SpProxy<T1>::use_iterator || P.is_alias(d_m) )
    {
    const SpMat<eT> tmp(P.Q);

    if(tmp.n_cols == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) = tmp.at(i,0); }
      }
    else
    if(tmp.n_rows == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) = tmp.at(0,i); }
      }
    }
  else
    {
    if(P.get_n_cols() == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) = P.at(i,0); }
      }
    else
    if(P.get_n_rows() == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) = P.at(0,i); }
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
spdiagview<eT>::operator+=(const SpBase<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  spdiagview<eT>& d = *this;

  SpMat<eT>& d_m = const_cast< SpMat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const SpProxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "spdiagview: given object has incompatible size"
    );

  if( SpProxy<T1>::use_iterator || P.is_alias(d_m) )
    {
    const SpMat<eT> tmp(P.Q);

    if(tmp.n_cols == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) += tmp.at(i,0); }
      }
    else
    if(tmp.n_rows == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) += tmp.at(0,i); }
      }
    }
  else
    {
    if(P.get_n_cols() == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) += P.at(i,0); }
      }
    else
    if(P.get_n_rows() == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) += P.at(0,i); }
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
spdiagview<eT>::operator-=(const SpBase<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  spdiagview<eT>& d = *this;

  SpMat<eT>& d_m = const_cast< SpMat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const SpProxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "spdiagview: given object has incompatible size"
    );

  if( SpProxy<T1>::use_iterator || P.is_alias(d_m) )
    {
    const SpMat<eT> tmp(P.Q);

    if(tmp.n_cols == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) -= tmp.at(i,0); }
      }
    else
    if(tmp.n_rows == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) -= tmp.at(0,i); }
      }
    }
  else
    {
    if(P.get_n_cols() == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) -= P.at(i,0); }
      }
    else
    if(P.get_n_rows() == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) -= P.at(0,i); }
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
spdiagview<eT>::operator%=(const SpBase<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  spdiagview<eT>& d = *this;

  SpMat<eT>& d_m = const_cast< SpMat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const SpProxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "spdiagview: given object has incompatible size"
    );

  if( SpProxy<T1>::use_iterator || P.is_alias(d_m) )
    {
    const SpMat<eT> tmp(P.Q);

    if(tmp.n_cols == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) *= tmp.at(i,0); }
      }
    else
    if(tmp.n_rows == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) *= tmp.at(0,i); }
      }
    }
  else
    {
    if(P.get_n_cols() == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) *= P.at(i,0); }
      }
    else
    if(P.get_n_rows() == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) *= P.at(0,i); }
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
spdiagview<eT>::operator/=(const SpBase<eT,T1>& o)
  {
  arma_extra_debug_sigprint();

  spdiagview<eT>& d = *this;

  SpMat<eT>& d_m = const_cast< SpMat<eT>& >(d.m);

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  const SpProxy<T1> P( o.get_ref() );

  arma_debug_check
    (
    ( (d_n_elem != P.get_n_elem()) || ((P.get_n_rows() != 1) && (P.get_n_cols() != 1)) ),
    "spdiagview: given object has incompatible size"
    );

  if( SpProxy<T1>::use_iterator || P.is_alias(d_m) )
    {
    const SpMat<eT> tmp(P.Q);

    if(tmp.n_cols == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) /= tmp.at(i,0); }
      }
    else
    if(tmp.n_rows == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) /= tmp.at(0,i); }
      }
    }
  else
    {
    if(P.get_n_cols() == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) /= P.at(i,0); }
      }
    else
    if(P.get_n_rows() == 1)
      {
      for(uword i=0; i < d_n_elem; ++i)  { d_m.at(i + d_row_offset, i + d_col_offset) /= P.at(0,i); }
      }
    }
  }



template<typename eT>
inline
void
spdiagview<eT>::extract(SpMat<eT>& out, const spdiagview<eT>& d)
  {
  arma_extra_debug_sigprint();

  const SpMat<eT>& d_m = d.m;

  const uword d_n_elem     = d.n_elem;
  const uword d_row_offset = d.row_offset;
  const uword d_col_offset = d.col_offset;

  d_m.sync();

  Col<eT> cache(d_n_elem);
  eT* cache_mem = cache.memptr();

  uword d_n_nonzero = 0;

  for(uword i=0; i < d_n_elem; ++i)
    {
    const eT val = d_m.at(i + d_row_offset, i + d_col_offset);

    cache_mem[i] = val;

    d_n_nonzero += ( val != eT(0)) ? uword(1) : uword(0);
    }

  out.set_size(d_n_elem, 1);

  out.mem_resize(d_n_nonzero);

  uword count = 0;
  for(uword i=0; i < d_n_elem; ++i)
    {
    const eT val = cache_mem[i];

    if(val != eT(0))
      {
      access::rw(out.row_indices[count]) = i;
      access::rw(out.values[count])      = val;
      ++count;
      }
    }

  access::rw(out.col_ptrs[0]) = 0;
  access::rw(out.col_ptrs[1]) = d_n_nonzero;
  }



//! extract a diagonal and store it as a dense column vector
template<typename eT>
inline
void
spdiagview<eT>::extract(Mat<eT>& out, const spdiagview<eT>& in)
  {
  arma_extra_debug_sigprint();

  // NOTE: we're assuming that the 'out' matrix has already been set to the correct size;
  // size setting is done by either the Mat contructor or Mat::operator=()

  const SpMat<eT>& in_m = in.m;

  const uword in_n_elem     = in.n_elem;
  const uword in_row_offset = in.row_offset;
  const uword in_col_offset = in.col_offset;

  in_m.sync();

  eT* out_mem = out.memptr();

  for(uword i=0; i < in_n_elem; ++i)
    {
    out_mem[i] = in_m.at(i + in_row_offset, i + in_col_offset );
    }
  }



template<typename eT>
inline
MapMat_elem<eT>
spdiagview<eT>::operator[](const uword i)
  {
  return (const_cast< SpMat<eT>& >(m)).at(i+row_offset, i+col_offset);
  }



template<typename eT>
inline
eT
spdiagview<eT>::operator[](const uword i) const
  {
  return m.at(i+row_offset, i+col_offset);
  }



template<typename eT>
inline
MapMat_elem<eT>
spdiagview<eT>::at(const uword i)
  {
  return (const_cast< SpMat<eT>& >(m)).at(i+row_offset, i+col_offset);
  }



template<typename eT>
inline
eT
spdiagview<eT>::at(const uword i) const
  {
  return m.at(i+row_offset, i+col_offset);
  }



template<typename eT>
inline
MapMat_elem<eT>
spdiagview<eT>::operator()(const uword i)
  {
  arma_debug_check( (i >= n_elem), "spdiagview::operator(): out of bounds" );

  return (const_cast< SpMat<eT>& >(m)).at(i+row_offset, i+col_offset);
  }



template<typename eT>
inline
eT
spdiagview<eT>::operator()(const uword i) const
  {
  arma_debug_check( (i >= n_elem), "spdiagview::operator(): out of bounds" );

  return m.at(i+row_offset, i+col_offset);
  }



template<typename eT>
inline
MapMat_elem<eT>
spdiagview<eT>::at(const uword row, const uword)
  {
  return (const_cast< SpMat<eT>& >(m)).at(row+row_offset, row+col_offset);
  }



template<typename eT>
inline
eT
spdiagview<eT>::at(const uword row, const uword) const
  {
  return m.at(row+row_offset, row+col_offset);
  }



template<typename eT>
inline
MapMat_elem<eT>
spdiagview<eT>::operator()(const uword row, const uword col)
  {
  arma_debug_check( ((row >= n_elem) || (col > 0)), "spdiagview::operator(): out of bounds" );

  return (const_cast< SpMat<eT>& >(m)).at(row+row_offset, row+col_offset);
  }



template<typename eT>
inline
eT
spdiagview<eT>::operator()(const uword row, const uword col) const
  {
  arma_debug_check( ((row >= n_elem) || (col > 0)), "spdiagview::operator(): out of bounds" );

  return m.at(row+row_offset, row+col_offset);
  }



template<typename eT>
inline
void
spdiagview<eT>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>& x = const_cast< SpMat<eT>& >(m);

  const uword local_n_elem = n_elem;

  for(uword i=0; i < local_n_elem; ++i)
    {
    x.at(i+row_offset, i+col_offset) = val;
    }
  }



template<typename eT>
inline
void
spdiagview<eT>::zeros()
  {
  arma_extra_debug_sigprint();

  (*this).fill(eT(0));
  }



template<typename eT>
inline
void
spdiagview<eT>::ones()
  {
  arma_extra_debug_sigprint();

  (*this).fill(eT(1));
  }



template<typename eT>
inline
void
spdiagview<eT>::randu()
  {
  arma_extra_debug_sigprint();

  SpMat<eT>& x = const_cast< SpMat<eT>& >(m);

  const uword local_n_elem = n_elem;

  for(uword i=0; i < local_n_elem; ++i)
    {
    x.at(i+row_offset, i+col_offset) = eT(arma_rng::randu<eT>());
    }
  }



template<typename eT>
inline
void
spdiagview<eT>::randn()
  {
  arma_extra_debug_sigprint();

  SpMat<eT>& x = const_cast< SpMat<eT>& >(m);

  const uword local_n_elem = n_elem;

  for(uword i=0; i < local_n_elem; ++i)
    {
    x.at(i+row_offset, i+col_offset) = eT(arma_rng::randn<eT>());
    }
  }



//! @}
