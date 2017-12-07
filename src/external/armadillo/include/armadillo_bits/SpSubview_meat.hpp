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


//! \addtogroup SpSubview
//! @{


template<typename eT>
arma_inline
SpSubview<eT>::SpSubview(const SpMat<eT>& in_m, const uword in_row1, const uword in_col1, const uword in_n_rows, const uword in_n_cols)
  : m(in_m)
  , aux_row1(in_row1)
  , aux_col1(in_col1)
  , n_rows(in_n_rows)
  , n_cols(in_n_cols)
  , n_elem(in_n_rows * in_n_cols)
  , n_nonzero(0)
  {
  arma_extra_debug_sigprint();

  m.sync_csc();

  // There must be a O(1) way to do this
  uword lend     = m.col_ptrs[in_col1 + in_n_cols];
  uword lend_row = in_row1 + in_n_rows;
  uword count   = 0;

  for(uword i = m.col_ptrs[in_col1]; i < lend; ++i)
    {
    const uword m_row_indices_i = m.row_indices[i];

    const bool condition = (m_row_indices_i >= in_row1) && (m_row_indices_i < lend_row);

    count += condition ? uword(1) : uword(0);
    }

  access::rw(n_nonzero) = count;
  }



template<typename eT>
arma_inline
SpSubview<eT>::SpSubview(SpMat<eT>& in_m, const uword in_row1, const uword in_col1, const uword in_n_rows, const uword in_n_cols)
  : m(in_m)
  , aux_row1(in_row1)
  , aux_col1(in_col1)
  , n_rows(in_n_rows)
  , n_cols(in_n_cols)
  , n_elem(in_n_rows * in_n_cols)
  , n_nonzero(0)
  {
  arma_extra_debug_sigprint();

  m.sync_csc();

  // There must be a O(1) way to do this
  uword lend     = m.col_ptrs[in_col1 + in_n_cols];
  uword lend_row = in_row1 + in_n_rows;
  uword count    = 0;

  for(uword i = m.col_ptrs[in_col1]; i < lend; ++i)
    {
    const uword m_row_indices_i = m.row_indices[i];

    const bool condition = (m_row_indices_i >= in_row1) && (m_row_indices_i < lend_row);

    count += condition ? uword(1) : uword(0);
    }

  access::rw(n_nonzero) = count;
  }



template<typename eT>
inline
SpSubview<eT>::~SpSubview()
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
const SpSubview<eT>&
SpSubview<eT>::operator+=(const eT val)
  {
  arma_extra_debug_sigprint();

  if(val == eT(0))
    {
    return *this;
    }

  Mat<eT> tmp( (*this).n_rows, (*this).n_cols );

  tmp.fill(val);

  return (*this).operator=( (*this) + tmp );
  }



template<typename eT>
inline
const SpSubview<eT>&
SpSubview<eT>::operator-=(const eT val)
  {
  arma_extra_debug_sigprint();

  if(val == eT(0))
    {
    return *this;
    }

  Mat<eT> tmp( (*this).n_rows, (*this).n_cols );

  tmp.fill(val);

  return (*this).operator=( (*this) - tmp );
  }



template<typename eT>
inline
const SpSubview<eT>&
SpSubview<eT>::operator*=(const eT val)
  {
  arma_extra_debug_sigprint();

  m.sync_csc();
  m.invalidate_cache();

  const uword lstart_row = aux_row1;
  const uword lend_row   = aux_row1 + n_rows;

  const uword lstart_col = aux_col1;
  const uword lend_col   = aux_col1 + n_cols;

  const uword* m_row_indices = m.row_indices;
        eT*    m_values      = access::rwp(m.values);

  bool has_zero = false;

  for(uword c = lstart_col; c < lend_col; ++c)
    {
    const uword r_start = m.col_ptrs[c    ];
    const uword r_end   = m.col_ptrs[c + 1];

    for(uword r = r_start; r < r_end; ++r)
      {
      const uword m_row_indices_r = m_row_indices[r];

      if( (m_row_indices_r >= lstart_row) && (m_row_indices_r < lend_row) )
        {
        eT& m_values_r = m_values[r];

        m_values_r *= val;

        if(m_values_r == eT(0))  { has_zero = true; }
        }
      }
    }

  if(has_zero)
    {
    const uword old_m_n_nonzero = m.n_nonzero;

    access::rw(m).remove_zeros();

    if(m.n_nonzero != old_m_n_nonzero)
      {
      access::rw(n_nonzero) = n_nonzero - (old_m_n_nonzero - m.n_nonzero);
      }
    }

  return *this;
  }



template<typename eT>
inline
const SpSubview<eT>&
SpSubview<eT>::operator/=(const eT val)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (val == eT(0)), "element-wise division: division by zero" );

  m.sync_csc();
  m.invalidate_cache();

  const uword lstart_row = aux_row1;
  const uword lend_row   = aux_row1 + n_rows;

  const uword lstart_col = aux_col1;
  const uword lend_col   = aux_col1 + n_cols;

  const uword* m_row_indices = m.row_indices;
        eT*    m_values      = access::rwp(m.values);

  bool has_zero = false;

  for(uword c = lstart_col; c < lend_col; ++c)
    {
    const uword r_start = m.col_ptrs[c    ];
    const uword r_end   = m.col_ptrs[c + 1];

    for(uword r = r_start; r < r_end; ++r)
      {
      const uword m_row_indices_r = m_row_indices[r];

      if( (m_row_indices_r >= lstart_row) && (m_row_indices_r < lend_row) )
        {
        eT& m_values_r = m_values[r];

        m_values_r /= val;

        if(m_values_r == eT(0))  { has_zero = true; }
        }
      }
    }

  if(has_zero)
    {
    const uword old_m_n_nonzero = m.n_nonzero;

    access::rw(m).remove_zeros();

    if(m.n_nonzero != old_m_n_nonzero)
      {
      access::rw(n_nonzero) = n_nonzero - (old_m_n_nonzero - m.n_nonzero);
      }
    }

  return *this;
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator=(const Base<eT, T1>& in)
  {
  arma_extra_debug_sigprint();

  // this is a modified version of SpSubview::operator_equ_common(const SpBase)

  const SpProxy< SpMat<eT> > pa((*this).m);

  const unwrap<T1>     b_tmp(in.get_ref());
  const Mat<eT>&   b = b_tmp.M;

  arma_debug_assert_same_size(n_rows, n_cols, b.n_rows, b.n_cols, "insertion into sparse submatrix");

  const uword pa_start_row = (*this).aux_row1;
  const uword pa_start_col = (*this).aux_col1;

  const uword pa_end_row = pa_start_row + (*this).n_rows - 1;
  const uword pa_end_col = pa_start_col + (*this).n_cols - 1;

  const uword pa_n_rows = pa.get_n_rows();

  const uword b_n_elem = b.n_elem;
  const eT*   b_mem    = b.memptr();

  uword box_count = 0;

  for(uword i=0; i<b_n_elem; ++i)
    {
    box_count += (b_mem[i] != eT(0)) ? uword(1) : uword(0);
    }

  SpMat<eT> out(pa.get_n_rows(), pa.get_n_cols());

  const uword alt_count = pa.get_n_nonzero() - (*this).n_nonzero + box_count;

  // Resize memory to correct size.
  out.mem_resize(alt_count);

  typename SpProxy< SpMat<eT> >::const_iterator_type x_it  = pa.begin();
  typename SpProxy< SpMat<eT> >::const_iterator_type x_end = pa.end();

  uword b_row = 0;
  uword b_col = 0;

  bool x_it_ok = (x_it != x_end);
  bool y_it_ok = ( (b_row < b.n_rows) && (b_col < b.n_cols) );

  uword x_it_row = (x_it_ok) ? x_it.row() : 0;
  uword x_it_col = (x_it_ok) ? x_it.col() : 0;

  uword y_it_row = (y_it_ok) ? b_row + pa_start_row : 0;
  uword y_it_col = (y_it_ok) ? b_col + pa_start_col : 0;

  uword cur_val = 0;
  while(x_it_ok || y_it_ok)
    {
    const bool x_inside_box = (x_it_row >= pa_start_row) && (x_it_row <= pa_end_row) && (x_it_col >= pa_start_col) && (x_it_col <= pa_end_col);
    const bool y_inside_box = (y_it_row >= pa_start_row) && (y_it_row <= pa_end_row) && (y_it_col >= pa_start_col) && (y_it_col <= pa_end_col);

    const eT x_val = x_inside_box ? eT(0) : ( x_it_ok ? (*x_it) : eT(0) );

    const eT y_val = y_inside_box ? ( y_it_ok ? b.at(b_row,b_col) : eT(0) ) : eT(0);

    if( (x_it_row == y_it_row) && (x_it_col == y_it_col) )
      {
      if( (x_val != eT(0)) || (y_val != eT(0)) )
        {
        access::rw(out.values[cur_val]) = (x_val != eT(0)) ? x_val : y_val;
        access::rw(out.row_indices[cur_val]) = x_it_row;
        ++access::rw(out.col_ptrs[x_it_col + 1]);
        ++cur_val;
        }

      if(x_it_ok)
        {
        ++x_it;

        if(x_it == x_end)  { x_it_ok = false; }
        }

      if(x_it_ok)
        {
        x_it_row = x_it.row();
        x_it_col = x_it.col();
        }
      else
        {
        x_it_row++;

        if(x_it_row >= pa_n_rows)  { x_it_row = 0; x_it_col++; }
        }

      if(y_it_ok)
        {
        b_row++;

        if(b_row >= b.n_rows)  { b_row = 0; b_col++; }

        if( (b_row > b.n_rows) || (b_col > b.n_cols) )  { y_it_ok = false; }
        }

      if(y_it_ok)
        {
        y_it_row = b_row + pa_start_row;
        y_it_col = b_col + pa_start_col;
        }
      else
        {
        y_it_row++;

        if(y_it_row >= pa_n_rows)  { y_it_row = 0; y_it_col++; }
        }
      }
    else
      {
      if((x_it_col < y_it_col) || ((x_it_col == y_it_col) && (x_it_row < y_it_row))) // if y is closer to the end
        {
        if(x_val != eT(0))
          {
          access::rw(out.values[cur_val]) = x_val;
          access::rw(out.row_indices[cur_val]) = x_it_row;
          ++access::rw(out.col_ptrs[x_it_col + 1]);
          ++cur_val;
          }

        if(x_it_ok)
          {
          ++x_it;

          if(x_it == x_end)  { x_it_ok = false; }
          }

        if(x_it_ok)
          {
          x_it_row = x_it.row();
          x_it_col = x_it.col();
          }
        else
          {
          x_it_row++;

          if(x_it_row >= pa_n_rows)  { x_it_row = 0; x_it_col++; }
          }
        }
      else
        {
        if(y_val != eT(0))
          {
          access::rw(out.values[cur_val]) = y_val;
          access::rw(out.row_indices[cur_val]) = y_it_row;
          ++access::rw(out.col_ptrs[y_it_col + 1]);
          ++cur_val;
          }

        if(y_it_ok)
          {
          b_row++;

          if(b_row >= b.n_rows)  { b_row = 0; b_col++; }

          if( (b_row > b.n_rows) || (b_col > b.n_cols) )  { y_it_ok = false; }
          }

        if(y_it_ok)
          {
          y_it_row = b_row + pa_start_row;
          y_it_col = b_col + pa_start_col;
          }
        else
          {
          y_it_row++;

          if(y_it_row >= pa_n_rows)  { y_it_row = 0; y_it_col++; }
          }
        }
      }
    }

  const uword out_n_cols = out.n_cols;

  uword* col_ptrs = access::rwp(out.col_ptrs);

  // Fix column pointers to be cumulative.
  for(uword c = 1; c <= out_n_cols; ++c)
    {
    col_ptrs[c] += col_ptrs[c - 1];
    }

  access::rw((*this).m).steal_mem(out);

  access::rw(n_nonzero) = box_count;

  return *this;
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator+=(const Base<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  return (*this).operator=( (*this) + x.get_ref() );
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator-=(const Base<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  return (*this).operator=( (*this) - x.get_ref() );
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator*=(const Base<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  SpMat<eT> tmp(*this);

  tmp *= x.get_ref();

  return (*this).operator=(tmp);
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator%=(const Base<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  return (*this).operator=( (*this) % x.get_ref() );
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator/=(const Base<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  return (*this).operator=( (*this) / x.get_ref() );
  }



template<typename eT>
inline
const SpSubview<eT>&
SpSubview<eT>::operator=(const SpSubview<eT>& x)
  {
  arma_extra_debug_sigprint();

  return (*this).operator_equ_common(x);
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator=(const SpBase<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  return (*this).operator_equ_common( x.get_ref() );
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator_equ_common(const SpBase<eT, T1>& in)
  {
  arma_extra_debug_sigprint();

  // algorithm:
  // instead of directly inserting values into the matrix underlying the subview,
  // create a new matrix by merging the underlying matrix with the input object,
  // and then replacing the underlying matrix with the created matrix.
  //
  // the merging process requires pretending that the input object
  // has the same size as the underlying matrix.
  // while iterating through the elements of the input object,
  // this requires adjusting the row and column locations of each element,
  // as well as providing fake zero elements.
  // in effect there is a proxy for a proxy.


  const SpProxy< SpMat<eT> > pa((*this).m   );
  const SpProxy< T1        > pb(in.get_ref());

  arma_debug_assert_same_size(n_rows, n_cols, pb.get_n_rows(), pb.get_n_cols(), "insertion into sparse submatrix");

  const uword pa_start_row = (*this).aux_row1;
  const uword pa_start_col = (*this).aux_col1;

  const uword pa_end_row = pa_start_row + (*this).n_rows - 1;
  const uword pa_end_col = pa_start_col + (*this).n_cols - 1;

  const uword pa_n_rows = pa.get_n_rows();

  SpMat<eT> out(pa.get_n_rows(), pa.get_n_cols());

  const uword alt_count = pa.get_n_nonzero() - (*this).n_nonzero + pb.get_n_nonzero();

  // Resize memory to correct size.
  out.mem_resize(alt_count);

  typename SpProxy< SpMat<eT> >::const_iterator_type x_it  = pa.begin();
  typename SpProxy< SpMat<eT> >::const_iterator_type x_end = pa.end();

  typename SpProxy<T1>::const_iterator_type y_it  = pb.begin();
  typename SpProxy<T1>::const_iterator_type y_end = pb.end();

  bool x_it_ok = (x_it != x_end);
  bool y_it_ok = (y_it != y_end);

  uword x_it_row = (x_it_ok) ? x_it.row() : 0;
  uword x_it_col = (x_it_ok) ? x_it.col() : 0;

  uword y_it_row = (y_it_ok) ? y_it.row() + pa_start_row : 0;
  uword y_it_col = (y_it_ok) ? y_it.col() + pa_start_col : 0;

  uword cur_val = 0;
  while(x_it_ok || y_it_ok)
    {
    const bool x_inside_box = (x_it_row >= pa_start_row) && (x_it_row <= pa_end_row) && (x_it_col >= pa_start_col) && (x_it_col <= pa_end_col);
    const bool y_inside_box = (y_it_row >= pa_start_row) && (y_it_row <= pa_end_row) && (y_it_col >= pa_start_col) && (y_it_col <= pa_end_col);

    const eT x_val = x_inside_box ? eT(0) : ( x_it_ok ? (*x_it) : eT(0) );

    const eT y_val = y_inside_box ? ( y_it_ok ? (*y_it) : eT(0) ) : eT(0);

    if( (x_it_row == y_it_row) && (x_it_col == y_it_col) )
      {
      if( (x_val != eT(0)) || (y_val != eT(0)) )
        {
        access::rw(out.values[cur_val]) = (x_val != eT(0)) ? x_val : y_val;
        access::rw(out.row_indices[cur_val]) = x_it_row;
        ++access::rw(out.col_ptrs[x_it_col + 1]);
        ++cur_val;
        }

      if(x_it_ok)
        {
        ++x_it;

        if(x_it == x_end)  { x_it_ok = false; }
        }

      if(x_it_ok)
        {
        x_it_row = x_it.row();
        x_it_col = x_it.col();
        }
      else
        {
        x_it_row++;

        if(x_it_row >= pa_n_rows)  { x_it_row = 0; x_it_col++; }
        }

      if(y_it_ok)
        {
        ++y_it;

        if(y_it == y_end)  { y_it_ok = false; }
        }

      if(y_it_ok)
        {
        y_it_row = y_it.row() + pa_start_row;
        y_it_col = y_it.col() + pa_start_col;
        }
      else
        {
        y_it_row++;

        if(y_it_row >= pa_n_rows)  { y_it_row = 0; y_it_col++; }
        }
      }
    else
      {
      if((x_it_col < y_it_col) || ((x_it_col == y_it_col) && (x_it_row < y_it_row))) // if y is closer to the end
        {
        if(x_val != eT(0))
          {
          access::rw(out.values[cur_val]) = x_val;
          access::rw(out.row_indices[cur_val]) = x_it_row;
          ++access::rw(out.col_ptrs[x_it_col + 1]);
          ++cur_val;
          }

        if(x_it_ok)
          {
          ++x_it;

          if(x_it == x_end)  { x_it_ok = false; }
          }

        if(x_it_ok)
          {
          x_it_row = x_it.row();
          x_it_col = x_it.col();
          }
        else
          {
          x_it_row++;

          if(x_it_row >= pa_n_rows)  { x_it_row = 0; x_it_col++; }
          }
        }
      else
        {
        if(y_val != eT(0))
          {
          access::rw(out.values[cur_val]) = y_val;
          access::rw(out.row_indices[cur_val]) = y_it_row;
          ++access::rw(out.col_ptrs[y_it_col + 1]);
          ++cur_val;
          }

        if(y_it_ok)
          {
          ++y_it;

          if(y_it == y_end)  { y_it_ok = false; }
          }

        if(y_it_ok)
          {
          y_it_row = y_it.row() + pa_start_row;
          y_it_col = y_it.col() + pa_start_col;
          }
        else
          {
          y_it_row++;

          if(y_it_row >= pa_n_rows)  { y_it_row = 0; y_it_col++; }
          }
        }
      }
    }

  const uword out_n_cols = out.n_cols;

  uword* col_ptrs = access::rwp(out.col_ptrs);

  // Fix column pointers to be cumulative.
  for(uword c = 1; c <= out_n_cols; ++c)
    {
    col_ptrs[c] += col_ptrs[c - 1];
    }

  access::rw((*this).m).steal_mem(out);

  access::rw(n_nonzero) = pb.get_n_nonzero();

  return *this;
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator+=(const SpBase<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  // TODO: implement dedicated machinery
  return (*this).operator=( (*this) + x.get_ref() );
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator-=(const SpBase<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  // TODO: implement dedicated machinery
  return (*this).operator=( (*this) - x.get_ref() );
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator*=(const SpBase<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  return (*this).operator=( (*this) * x.get_ref() );
  }



template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator%=(const SpBase<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  // TODO: implement dedicated machinery
  return (*this).operator=( (*this) % x.get_ref() );
  }



//! If you are using this function, you are probably misguided.
template<typename eT>
template<typename T1>
inline
const SpSubview<eT>&
SpSubview<eT>::operator/=(const SpBase<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  SpProxy<T1> p(x.get_ref());

  arma_debug_assert_same_size(n_rows, n_cols, p.get_n_rows(), p.get_n_cols(), "element-wise division");

  if(p.is_alias(m) == false)
    {
    for(uword lcol = 0; lcol < n_cols; ++lcol)
    for(uword lrow = 0; lrow < n_rows; ++lrow)
      {
      at(lrow,lcol) /= p.at(lrow,lcol);
      }
    }
  else
    {
    const SpMat<eT> tmp(p.Q);

    (*this).operator/=(tmp);
    }

  return *this;
  }



template<typename eT>
inline
void
SpSubview<eT>::replace(const eT old_val, const eT new_val)
  {
  arma_extra_debug_sigprint();

  if(old_val == eT(0))
    {
    if(new_val != eT(0))
      {
      Mat<eT> tmp(*this);

      tmp.replace(old_val, new_val);

      (*this).operator=(tmp);
      }

    return;
    }

  m.sync_csc();
  m.invalidate_cache();

  const uword lstart_row = aux_row1;
  const uword lend_row   = aux_row1 + n_rows;

  const uword lstart_col = aux_col1;
  const uword lend_col   = aux_col1 + n_cols;

  const uword* m_row_indices = m.row_indices;
        eT*    m_values      = access::rwp(m.values);

  if(arma_isnan(old_val))
    {
    for(uword c = lstart_col; c < lend_col; ++c)
      {
      const uword r_start = m.col_ptrs[c    ];
      const uword r_end   = m.col_ptrs[c + 1];

      for(uword r = r_start; r < r_end; ++r)
        {
        const uword m_row_indices_r = m_row_indices[r];

        if( (m_row_indices_r >= lstart_row) && (m_row_indices_r < lend_row) )
          {
          eT& val = m_values[r];

          val = (arma_isnan(val)) ? new_val : val;
          }
        }
      }
    }
  else
    {
    for(uword c = lstart_col; c < lend_col; ++c)
      {
      const uword r_start = m.col_ptrs[c    ];
      const uword r_end   = m.col_ptrs[c + 1];

      for(uword r = r_start; r < r_end; ++r)
        {
        const uword m_row_indices_r = m_row_indices[r];

        if( (m_row_indices_r >= lstart_row) && (m_row_indices_r < lend_row) )
          {
          eT& val = m_values[r];

          val = (val == old_val) ? new_val : val;
          }
        }
      }
    }

  if(new_val == eT(0))  { access::rw(m).remove_zeros(); }
  }



template<typename eT>
inline
void
SpSubview<eT>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  if(val != eT(0))
    {
    Mat<eT> tmp( (*this).n_rows, (*this).n_cols );

    tmp.fill(val);

    (*this).operator=(tmp);
    }
  else
    {
    (*this).zeros();
    }
  }



template<typename eT>
inline
void
SpSubview<eT>::zeros()
  {
  arma_extra_debug_sigprint();

  (*this).operator*=(eT(0));
  }



template<typename eT>
inline
void
SpSubview<eT>::ones()
  {
  arma_extra_debug_sigprint();

  (*this).fill(eT(1));
  }



template<typename eT>
inline
void
SpSubview<eT>::eye()
  {
  arma_extra_debug_sigprint();

  SpMat<eT> tmp;

  tmp.eye( (*this).n_rows, (*this).n_cols );

  (*this).operator=(tmp);
  }



template<typename eT>
arma_hot
inline
MapMat_svel<eT>
SpSubview<eT>::operator[](const uword i)
  {
  const uword lrow = i % n_rows;
  const uword lcol = i / n_rows;

  return (*this).at(lrow, lcol);
  }



template<typename eT>
arma_hot
inline
eT
SpSubview<eT>::operator[](const uword i) const
  {
  const uword lrow = i % n_rows;
  const uword lcol = i / n_rows;

  return (*this).at(lrow, lcol);
  }



template<typename eT>
arma_hot
inline
MapMat_svel<eT>
SpSubview<eT>::operator()(const uword i)
  {
  arma_debug_check( (i >= n_elem), "SpSubview::operator(): index out of bounds");

  const uword lrow = i % n_rows;
  const uword lcol = i / n_rows;

  return (*this).at(lrow, lcol);
  }



template<typename eT>
arma_hot
inline
eT
SpSubview<eT>::operator()(const uword i) const
  {
  arma_debug_check( (i >= n_elem), "SpSubview::operator(): index out of bounds");

  const uword lrow = i % n_rows;
  const uword lcol = i / n_rows;

  return (*this).at(lrow, lcol);
  }



template<typename eT>
arma_hot
inline
MapMat_svel<eT>
SpSubview<eT>::operator()(const uword in_row, const uword in_col)
  {
  arma_debug_check( (in_row >= n_rows) || (in_col >= n_cols), "SpSubview::operator(): index out of bounds");

  return (*this).at(in_row, in_col);
  }



template<typename eT>
arma_hot
inline
eT
SpSubview<eT>::operator()(const uword in_row, const uword in_col) const
  {
  arma_debug_check( (in_row >= n_rows) || (in_col >= n_cols), "SpSubview::operator(): index out of bounds");

  return (*this).at(in_row, in_col);
  }



template<typename eT>
arma_hot
inline
MapMat_svel<eT>
SpSubview<eT>::at(const uword i)
  {
  const uword lrow = i % n_rows;
  const uword lcol = i / n_cols;

  return (*this).at(lrow, lcol);
  }



template<typename eT>
arma_hot
inline
eT
SpSubview<eT>::at(const uword i) const
  {
  const uword lrow = i % n_rows;
  const uword lcol = i / n_cols;

  return (*this).at(lrow, lcol);
  }



template<typename eT>
arma_hot
inline
MapMat_svel<eT>
SpSubview<eT>::at(const uword in_row, const uword in_col)
  {
  m.sync_cache();

  return m.cache.svel(aux_row1 + in_row, aux_col1 + in_col, m.sync_state, access::rw(m.n_nonzero), access::rw(n_nonzero));
  }



template<typename eT>
arma_hot
inline
eT
SpSubview<eT>::at(const uword in_row, const uword in_col) const
  {
  return m.at(aux_row1 + in_row, aux_col1 + in_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::check_overlap(const SpSubview<eT>& x) const
  {
  const subview<eT>& t = *this;

  if(&t.m != &x.m)
    {
    return false;
    }
  else
    {
    if( (t.n_elem == 0) || (x.n_elem == 0) )
      {
      return false;
      }
    else
      {
      const uword t_row_start  = t.aux_row1;
      const uword t_row_end_p1 = t_row_start + t.n_rows;

      const uword t_col_start  = t.aux_col1;
      const uword t_col_end_p1 = t_col_start + t.n_cols;

      const uword x_row_start  = x.aux_row1;
      const uword x_row_end_p1 = x_row_start + x.n_rows;

      const uword x_col_start  = x.aux_col1;
      const uword x_col_end_p1 = x_col_start + x.n_cols;

      const bool outside_rows = ( (x_row_start >= t_row_end_p1) || (t_row_start >= x_row_end_p1) );
      const bool outside_cols = ( (x_col_start >= t_col_end_p1) || (t_col_start >= x_col_end_p1) );

      return ( (outside_rows == false) && (outside_cols == false) );
      }
    }
  }



template<typename eT>
inline
bool
SpSubview<eT>::is_vec() const
  {
  return ( (n_rows == 1) || (n_cols == 1) );
  }



template<typename eT>
inline
SpSubview<eT>
SpSubview<eT>::row(const uword row_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check(row_num >= n_rows, "SpSubview::row(): out of bounds");

  return submat(row_num, 0, row_num, n_cols - 1);
  }



template<typename eT>
inline
const SpSubview<eT>
SpSubview<eT>::row(const uword row_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check(row_num >= n_rows, "SpSubview::row(): out of bounds");

  return submat(row_num, 0, row_num, n_cols - 1);
  }



template<typename eT>
inline
SpSubview<eT>
SpSubview<eT>::col(const uword col_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check(col_num >= n_cols, "SpSubview::col(): out of bounds");

  return submat(0, col_num, n_rows - 1, col_num);
  }



template<typename eT>
inline
const SpSubview<eT>
SpSubview<eT>::col(const uword col_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check(col_num >= n_cols, "SpSubview::col(): out of bounds");

  return submat(0, col_num, n_rows - 1, col_num);
  }



template<typename eT>
inline
SpSubview<eT>
SpSubview<eT>::rows(const uword in_row1, const uword in_row2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_row2 >= n_rows),
    "SpSubview::rows(): indices out of bounds or incorrectly used"
    );

  return submat(in_row1, 0, in_row2, n_cols - 1);
  }



template<typename eT>
inline
const SpSubview<eT>
SpSubview<eT>::rows(const uword in_row1, const uword in_row2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_row2 >= n_rows),
    "SpSubview::rows(): indices out of bounds or incorrectly used"
    );

  return submat(in_row1, 0, in_row2, n_cols - 1);
  }



template<typename eT>
inline
SpSubview<eT>
SpSubview<eT>::cols(const uword in_col1, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_col1 > in_col2) || (in_col2 >= n_cols),
    "SpSubview::cols(): indices out of bounds or incorrectly used"
    );

  return submat(0, in_col1, n_rows - 1, in_col2);
  }



template<typename eT>
inline
const SpSubview<eT>
SpSubview<eT>::cols(const uword in_col1, const uword in_col2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_col1 > in_col2) || (in_col2 >= n_cols),
    "SpSubview::cols(): indices out of bounds or incorrectly used"
    );

  return submat(0, in_col1, n_rows - 1, in_col2);
  }



template<typename eT>
inline
SpSubview<eT>
SpSubview<eT>::submat(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_col1 > in_col2) || (in_row2 >= n_rows) || (in_col2 >= n_cols),
    "SpSubview::submat(): indices out of bounds or incorrectly used"
    );

  return access::rw(m).submat(in_row1 + aux_row1, in_col1 + aux_col1, in_row2 + aux_row1, in_col2 + aux_col1);
  }



template<typename eT>
inline
const SpSubview<eT>
SpSubview<eT>::submat(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_col1 > in_col2) || (in_row2 >= n_rows) || (in_col2 >= n_cols),
    "SpSubview::submat(): indices out of bounds or incorrectly used"
    );

  return m.submat(in_row1 + aux_row1, in_col1 + aux_col1, in_row2 + aux_row1, in_col2 + aux_col1);
  }



template<typename eT>
inline
SpSubview<eT>
SpSubview<eT>::submat(const span& row_span, const span& col_span)
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;
  const bool col_all = row_span.whole;

  const uword in_row1 = row_all ? 0      : row_span.a;
  const uword in_row2 = row_all ? n_rows : row_span.b;

  const uword in_col1 = col_all ? 0      : col_span.a;
  const uword in_col2 = col_all ? n_cols : col_span.b;

  arma_debug_check
    (
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= n_rows)))
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= n_cols))),
    "SpSubview::submat(): indices out of bounds or incorrectly used"
    );

  return submat(in_row1, in_col1, in_row2, in_col2);
  }



template<typename eT>
inline
const SpSubview<eT>
SpSubview<eT>::submat(const span& row_span, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;
  const bool col_all = row_span.whole;

  const uword in_row1 = row_all ? 0          : row_span.a;
  const uword in_row2 = row_all ? n_rows - 1 : row_span.b;

  const uword in_col1 = col_all ? 0          : col_span.a;
  const uword in_col2 = col_all ? n_cols - 1 : col_span.b;

  arma_debug_check
    (
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= n_rows)))
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= n_cols))),
    "SpSubview::submat(): indices out of bounds or incorrectly used"
    );

  return submat(in_row1, in_col1, in_row2, in_col2);
  }



template<typename eT>
inline
SpSubview<eT>
SpSubview<eT>::operator()(const uword row_num, const span& col_span)
  {
  arma_extra_debug_sigprint();

  return submat(span(row_num, row_num), col_span);
  }



template<typename eT>
inline
const SpSubview<eT>
SpSubview<eT>::operator()(const uword row_num, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  return submat(span(row_num, row_num), col_span);
  }



template<typename eT>
inline
SpSubview<eT>
SpSubview<eT>::operator()(const span& row_span, const uword col_num)
  {
  arma_extra_debug_sigprint();

  return submat(row_span, span(col_num, col_num));
  }



template<typename eT>
inline
const SpSubview<eT>
SpSubview<eT>::operator()(const span& row_span, const uword col_num) const
  {
  arma_extra_debug_sigprint();

  return submat(row_span, span(col_num, col_num));
  }



template<typename eT>
inline
SpSubview<eT>
SpSubview<eT>::operator()(const span& row_span, const span& col_span)
  {
  arma_extra_debug_sigprint();

  return submat(row_span, col_span);
  }



template<typename eT>
inline
const SpSubview<eT>
SpSubview<eT>::operator()(const span& row_span, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  return submat(row_span, col_span);
  }



template<typename eT>
inline
void
SpSubview<eT>::swap_rows(const uword in_row1, const uword in_row2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check((in_row1 >= n_rows) || (in_row2 >= n_rows), "SpSubview::swap_rows(): invalid row index");

  const uword lstart_col = aux_col1;
  const uword lend_col   = aux_col1 + n_cols;

  for(uword c = lstart_col; c < lend_col; ++c)
    {
    const eT val = access::rw(m).at(in_row1 + aux_row1, c);
    access::rw(m).at(in_row2 + aux_row1, c) = eT( access::rw(m).at(in_row1 + aux_row1, c) );
    access::rw(m).at(in_row1 + aux_row1, c) = val;
    }
  }



template<typename eT>
inline
void
SpSubview<eT>::swap_cols(const uword in_col1, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check((in_col1 >= n_cols) || (in_col2 >= n_cols), "SpSubview::swap_cols(): invalid column index");

  const uword lstart_row = aux_row1;
  const uword lend_row   = aux_row1 + n_rows;

  for(uword r = lstart_row; r < lend_row; ++r)
    {
    const eT val = access::rw(m).at(r, in_col1 + aux_col1);
    access::rw(m).at(r, in_col1 + aux_col1) = eT( access::rw(m).at(r, in_col2 + aux_col1) );
    access::rw(m).at(r, in_col2 + aux_col1) = val;
    }
  }



template<typename eT>
inline
typename SpSubview<eT>::iterator
SpSubview<eT>::begin()
  {
  return iterator(*this);
  }



template<typename eT>
inline
typename SpSubview<eT>::const_iterator
SpSubview<eT>::begin() const
  {
  m.sync_csc();

  return const_iterator(*this);
  }



template<typename eT>
inline
typename SpSubview<eT>::iterator
SpSubview<eT>::begin_col(const uword col_num)
  {
  m.sync_csc();

  return iterator(*this, 0, col_num);
  }


template<typename eT>
inline
typename SpSubview<eT>::const_iterator
SpSubview<eT>::begin_col(const uword col_num) const
  {
  m.sync_csc();

  return const_iterator(*this, 0, col_num);
  }



template<typename eT>
inline
typename SpSubview<eT>::row_iterator
SpSubview<eT>::begin_row(const uword row_num)
  {
  m.sync_csc();

  return row_iterator(*this, row_num, 0);
  }



template<typename eT>
inline
typename SpSubview<eT>::const_row_iterator
SpSubview<eT>::begin_row(const uword row_num) const
  {
  m.sync_csc();

  return const_row_iterator(*this, row_num, 0);
  }



template<typename eT>
inline
typename SpSubview<eT>::iterator
SpSubview<eT>::end()
  {
  m.sync_csc();

  return iterator(*this, 0, n_cols, n_nonzero, m.n_nonzero - n_nonzero);
  }



template<typename eT>
inline
typename SpSubview<eT>::const_iterator
SpSubview<eT>::end() const
  {
  m.sync_csc();

  return const_iterator(*this, 0, n_cols, n_nonzero, m.n_nonzero - n_nonzero);
  }



template<typename eT>
inline
typename SpSubview<eT>::row_iterator
SpSubview<eT>::end_row()
  {
  m.sync_csc();

  return row_iterator(*this, n_nonzero);
  }



template<typename eT>
inline
typename SpSubview<eT>::const_row_iterator
SpSubview<eT>::end_row() const
  {
  m.sync_csc();

  return const_row_iterator(*this, n_nonzero);
  }



template<typename eT>
inline
typename SpSubview<eT>::row_iterator
SpSubview<eT>::end_row(const uword row_num)
  {
  m.sync_csc();

  return row_iterator(*this, row_num + 1, 0);
  }



template<typename eT>
inline
typename SpSubview<eT>::const_row_iterator
SpSubview<eT>::end_row(const uword row_num) const
  {
  m.sync_csc();

  return const_row_iterator(*this, row_num + 1, 0);
  }



template<typename eT>
inline
arma_hot
arma_warn_unused
eT&
SpSubview<eT>::add_element(const uword in_row, const uword in_col, const eT in_val)
  {
  arma_extra_debug_sigprint();

  // This may not actually add an element.
  const uword old_n_nonzero = m.n_nonzero;
  eT& retval = access::rw(m).add_element(in_row + aux_row1, in_col + aux_col1, in_val);
  // Update n_nonzero (if necessary).
  access::rw(n_nonzero) += (m.n_nonzero - old_n_nonzero);

  return retval;
  }



template<typename eT>
inline
void
SpSubview<eT>::delete_element(const uword in_row, const uword in_col)
  {
  arma_extra_debug_sigprint();

  // This may not actually delete an element.
  const uword old_n_nonzero = m.n_nonzero;
  access::rw(m).delete_element(in_row + aux_row1, in_col + aux_col1);
  access::rw(n_nonzero) -= (old_n_nonzero - m.n_nonzero);
  }



/**
 * Sparse subview col
 *
template<typename eT>
inline
SpSubview_col<eT>::SpSubview_col(const Mat<eT>& in_m, const uword in_col)
  {
  arma_extra_debug_sigprint();
  }

template<typename eT>
inline
SpSubview_col<eT>::SpSubview_col(Mat<eT>& in_m, const uword in_col)
  {
  arma_extra_debug_sigprint();
  }

template<typename eT>
inline
SpSubview_col<eT>::SpSubview_col(const Mat<eT>& in_m, const uword in_col, const uword in_row1, const uword in_n_rows)
  {
  arma_extra_debug_sigprint();
  }

template<typename eT>
inline
SpSubview_col<eT>::SpSubview_col(Mat<eT>& in_m, const uword in_col, const uword in_row1, const uword in_n_rows)
  {
  arma_extra_debug_sigprint();
  }
*/

/**
 * Sparse subview row
 *
template<typename eT>
inline
SpSubview_row<eT>::SpSubview_row(const Mat<eT>& in_m, const uword in_row)
  {
  arma_extra_debug_sigprint();
  }

template<typename eT>
inline
SpSubview_row<eT>::SpSubview_row(Mat<eT>& in_m, const uword in_row)
  {
  arma_extra_debug_sigprint();
  }

template<typename eT>
inline
SpSubview_row<eT>::SpSubview_row(const Mat<eT>& in_m, const uword in_row, const uword in_col1, const uword in_n_cols)
  {
  arma_extra_debug_sigprint();
  }

template<typename eT>
inline
SpSubview_row<eT>::SpSubview_row(Mat<eT>& in_m, const uword in_row, const uword in_col1, const uword in_n_cols)
  {
  arma_extra_debug_sigprint();
  }
*/


//! @}
