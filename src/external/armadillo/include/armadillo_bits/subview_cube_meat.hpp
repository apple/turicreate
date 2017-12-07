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


//! \addtogroup subview_cube
//! @{


template<typename eT>
inline
subview_cube<eT>::~subview_cube()
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
arma_inline
subview_cube<eT>::subview_cube
  (
  const Cube<eT>& in_m,
  const uword     in_row1,
  const uword     in_col1,
  const uword     in_slice1,
  const uword     in_n_rows,
  const uword     in_n_cols,
  const uword     in_n_slices
  )
  : m           (in_m)
  , aux_row1    (in_row1)
  , aux_col1    (in_col1)
  , aux_slice1  (in_slice1)
  , n_rows      (in_n_rows)
  , n_cols      (in_n_cols)
  , n_elem_slice(in_n_rows * in_n_cols)
  , n_slices    (in_n_slices)
  , n_elem      (n_elem_slice * in_n_slices)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
void
subview_cube<eT>::operator= (const eT val)
  {
  arma_extra_debug_sigprint();

  if(n_elem != 1)
    {
    arma_debug_assert_same_size(n_rows, n_cols, n_slices, 1, 1, 1, "copy into subcube");
    }

  Cube<eT>& Q = const_cast< Cube<eT>& >(m);

  Q.at(aux_row1, aux_col1, aux_slice1) = val;
  }



template<typename eT>
inline
void
subview_cube<eT>::operator+= (const eT val)
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      arrayops::inplace_plus( slice_colptr(slice,col), val, local_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview_cube<eT>::operator-= (const eT val)
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      arrayops::inplace_minus( slice_colptr(slice,col), val, local_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview_cube<eT>::operator*= (const eT val)
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      arrayops::inplace_mul( slice_colptr(slice,col), val, local_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview_cube<eT>::operator/= (const eT val)
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      arrayops::inplace_div( slice_colptr(slice,col), val, local_n_rows );
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview_cube<eT>::operator= (const BaseCube<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap_cube<T1> tmp(in.get_ref());

  const Cube<eT>&         x = tmp.M;
        subview_cube<eT>& t = *this;

  arma_debug_assert_same_size(t, x, "copy into subcube");

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  for(uword slice = 0; slice < t_n_slices; ++slice)
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::copy( t.slice_colptr(slice,col), x.slice_colptr(slice,col), t_n_rows );
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview_cube<eT>::operator+= (const BaseCube<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap_cube<T1> tmp(in.get_ref());

  const Cube<eT>&         x = tmp.M;
        subview_cube<eT>& t = *this;

  arma_debug_assert_same_size(t, x, "addition");

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  for(uword slice = 0; slice < t_n_slices; ++slice)
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_plus( t.slice_colptr(slice,col), x.slice_colptr(slice,col), t_n_rows );
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview_cube<eT>::operator-= (const BaseCube<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap_cube<T1> tmp(in.get_ref());

  const Cube<eT>&         x = tmp.M;
        subview_cube<eT>& t = *this;

  arma_debug_assert_same_size(t, x, "subtraction");

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  for(uword slice = 0; slice < t_n_slices; ++slice)
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_minus( t.slice_colptr(slice,col), x.slice_colptr(slice,col), t_n_rows );
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview_cube<eT>::operator%= (const BaseCube<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap_cube<T1> tmp(in.get_ref());

  const Cube<eT>&         x = tmp.M;
        subview_cube<eT>& t = *this;

  arma_debug_assert_same_size(t, x, "element-wise multiplication");

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  for(uword slice = 0; slice < t_n_slices; ++slice)
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_mul( t.slice_colptr(slice,col), x.slice_colptr(slice,col), t_n_rows );
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview_cube<eT>::operator/= (const BaseCube<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap_cube<T1> tmp(in.get_ref());

  const Cube<eT>&         x = tmp.M;
        subview_cube<eT>& t = *this;

  arma_debug_assert_same_size(t, x, "element-wise division");

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  for(uword slice = 0; slice < t_n_slices; ++slice)
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_div( t.slice_colptr(slice,col), x.slice_colptr(slice,col), t_n_rows );
      }
    }
  }



//! x.subcube(...) = y.subcube(...)
template<typename eT>
inline
void
subview_cube<eT>::operator= (const subview_cube<eT>& x)
  {
  arma_extra_debug_sigprint();

  if(check_overlap(x))
    {
    const Cube<eT> tmp(x);

    (*this).operator=(tmp);

    return;
    }

  subview_cube<eT>& t = *this;

  arma_debug_assert_same_size(t, x, "copy into subcube");

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  for(uword slice = 0; slice < t_n_slices; ++slice)
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::copy( t.slice_colptr(slice,col), x.slice_colptr(slice,col), t_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview_cube<eT>::operator+= (const subview_cube<eT>& x)
  {
  arma_extra_debug_sigprint();

  if(check_overlap(x))
    {
    const Cube<eT> tmp(x);

    (*this).operator+=(tmp);

    return;
    }

  subview_cube<eT>& t = *this;

  arma_debug_assert_same_size(t, x, "addition");

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  for(uword slice = 0; slice < t_n_slices; ++slice)
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_plus( t.slice_colptr(slice,col), x.slice_colptr(slice,col), t_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview_cube<eT>::operator-= (const subview_cube<eT>& x)
  {
  arma_extra_debug_sigprint();

  if(check_overlap(x))
    {
    const Cube<eT> tmp(x);

    (*this).operator-=(tmp);

    return;
    }

  subview_cube<eT>& t = *this;

  arma_debug_assert_same_size(t, x, "subtraction");

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  for(uword slice = 0; slice < t_n_slices; ++slice)
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_minus( t.slice_colptr(slice,col), x.slice_colptr(slice,col), t_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview_cube<eT>::operator%= (const subview_cube<eT>& x)
  {
  arma_extra_debug_sigprint();

  if(check_overlap(x))
    {
    const Cube<eT> tmp(x);

    (*this).operator%=(tmp);

    return;
    }

  subview_cube<eT>& t = *this;

  arma_debug_assert_same_size(t, x, "element-wise multiplication");

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  for(uword slice = 0; slice < t_n_slices; ++slice)
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_mul( t.slice_colptr(slice,col), x.slice_colptr(slice,col), t_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview_cube<eT>::operator/= (const subview_cube<eT>& x)
  {
  arma_extra_debug_sigprint();

  if(check_overlap(x))
    {
    const Cube<eT> tmp(x);

    (*this).operator/=(tmp);

    return;
    }

  subview_cube<eT>& t = *this;

  arma_debug_assert_same_size(t, x, "element-wise division");

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  for(uword slice = 0; slice < t_n_slices; ++slice)
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_div( t.slice_colptr(slice,col), x.slice_colptr(slice,col), t_n_rows );
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview_cube<eT>::operator= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> tmp(in.get_ref());

  const Mat<eT>&          x = tmp.M;
        subview_cube<eT>& t = *this;

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  const uword x_n_rows   = x.n_rows;
  const uword x_n_cols   = x.n_cols;

  if( ((x_n_rows == 1) || (x_n_cols == 1)) && (t_n_rows == 1) && (t_n_cols == 1) && (x.n_elem == t_n_slices) )
    {
    Cube<eT>& Q = const_cast< Cube<eT>& >(t.m);

    const uword t_aux_row1   = t.aux_row1;
    const uword t_aux_col1   = t.aux_col1;
    const uword t_aux_slice1 = t.aux_slice1;

    const eT* x_mem = x.memptr();

    uword i,j;
    for(i=0, j=1; j < t_n_slices; i+=2, j+=2)
      {
      const eT tmp_i = x_mem[i];
      const eT tmp_j = x_mem[j];

      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + i) = tmp_i;
      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + j) = tmp_j;
      }

    if(i < t_n_slices)
      {
      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + i) = x_mem[i];
      }
    }
  else
  if( (t_n_rows == x_n_rows) && (t_n_cols == x_n_cols) && (t_n_slices == 1) )
    {
    // interpret the matrix as a cube with one slice

    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::copy( t.slice_colptr(0, col), x.colptr(col), t_n_rows );
      }
    }
  else
  if( (t_n_rows == x_n_rows) && (t_n_cols == 1) && (t_n_slices == x_n_cols) )
    {
    for(uword i=0; i < t_n_slices; ++i)
      {
      arrayops::copy( t.slice_colptr(i, 0), x.colptr(i), t_n_rows );
      }
    }
  else
  if( (t_n_rows == 1) && (t_n_cols == x_n_rows) && (t_n_slices == x_n_cols) )
    {
    Cube<eT>& Q = const_cast< Cube<eT>& >(t.m);

    const uword t_aux_row1   = t.aux_row1;
    const uword t_aux_col1   = t.aux_col1;
    const uword t_aux_slice1 = t.aux_slice1;

    for(uword slice=0; slice < t_n_slices; ++slice)
      {
      const uword mod_slice = t_aux_slice1 + slice;

      const eT* x_colptr = x.colptr(slice);

      uword i,j;
      for(i=0, j=1; j < t_n_cols; i+=2, j+=2)
        {
        const eT tmp_i = x_colptr[i];
        const eT tmp_j = x_colptr[j];

        Q.at(t_aux_row1, t_aux_col1 + i, mod_slice) = tmp_i;
        Q.at(t_aux_row1, t_aux_col1 + j, mod_slice) = tmp_j;
        }

      if(i < t_n_cols)
        {
        Q.at(t_aux_row1, t_aux_col1 + i, mod_slice) = x_colptr[i];
        }
      }
    }
  else
    {
    if(arma_config::debug == true)
      {
      arma_stop_logic_error( arma_incompat_size_string(t, x, "copy into subcube") );
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview_cube<eT>::operator+= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> tmp(in.get_ref());

  const Mat<eT>&          x = tmp.M;
        subview_cube<eT>& t = *this;

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  const uword x_n_rows   = x.n_rows;
  const uword x_n_cols   = x.n_cols;

  if( ((x_n_rows == 1) || (x_n_cols == 1)) && (t_n_rows == 1) && (t_n_cols == 1) && (x.n_elem == t_n_slices) )
    {
    Cube<eT>& Q = const_cast< Cube<eT>& >(t.m);

    const uword t_aux_row1   = t.aux_row1;
    const uword t_aux_col1   = t.aux_col1;
    const uword t_aux_slice1 = t.aux_slice1;

    const eT* x_mem = x.memptr();

    uword i,j;
    for(i=0, j=1; j < t_n_slices; i+=2, j+=2)
      {
      const eT tmp_i = x_mem[i];
      const eT tmp_j = x_mem[j];

      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + i) += tmp_i;
      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + j) += tmp_j;
      }

    if(i < t_n_slices)
      {
      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + i) += x_mem[i];
      }
    }
  else
  if( (t_n_rows == x_n_rows) && (t_n_cols == x_n_cols) && (t_n_slices == 1) )
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_plus( t.slice_colptr(0, col), x.colptr(col), t_n_rows );
      }
    }
  else
  if( (t_n_rows == x_n_rows) && (t_n_cols == 1) && (t_n_slices == x_n_cols) )
    {
    for(uword i=0; i < t_n_slices; ++i)
      {
      arrayops::inplace_plus( t.slice_colptr(i, 0), x.colptr(i), t_n_rows );
      }
    }
  else
  if( (t_n_rows == 1) && (t_n_cols == x_n_rows) && (t_n_slices == x_n_cols) )
    {
    Cube<eT>& Q = const_cast< Cube<eT>& >(t.m);

    const uword t_aux_row1   = t.aux_row1;
    const uword t_aux_col1   = t.aux_col1;
    const uword t_aux_slice1 = t.aux_slice1;

    for(uword slice=0; slice < t_n_slices; ++slice)
      {
      const uword mod_slice = t_aux_slice1 + slice;

      const eT* x_colptr = x.colptr(slice);

      uword i,j;
      for(i=0, j=1; j < t_n_cols; i+=2, j+=2)
        {
        const eT tmp_i = x_colptr[i];
        const eT tmp_j = x_colptr[j];

        Q.at(t_aux_row1, t_aux_col1 + i, mod_slice) += tmp_i;
        Q.at(t_aux_row1, t_aux_col1 + j, mod_slice) += tmp_j;
        }

      if(i < t_n_cols)
        {
        Q.at(t_aux_row1, t_aux_col1 + i, mod_slice) += x_colptr[i];
        }
      }
    }
  else
    {
    if(arma_config::debug == true)
      {
      arma_stop_logic_error( arma_incompat_size_string(t, x, "addition") );
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview_cube<eT>::operator-= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> tmp(in.get_ref());

  const Mat<eT>&          x = tmp.M;
        subview_cube<eT>& t = *this;

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  const uword x_n_rows   = x.n_rows;
  const uword x_n_cols   = x.n_cols;

  if( ((x_n_rows == 1) || (x_n_cols == 1)) && (t_n_rows == 1) && (t_n_cols == 1) && (x.n_elem == t_n_slices) )
    {
    Cube<eT>& Q = const_cast< Cube<eT>& >(t.m);

    const uword t_aux_row1   = t.aux_row1;
    const uword t_aux_col1   = t.aux_col1;
    const uword t_aux_slice1 = t.aux_slice1;

    const eT* x_mem = x.memptr();

    uword i,j;
    for(i=0, j=1; j < t_n_slices; i+=2, j+=2)
      {
      const eT tmp_i = x_mem[i];
      const eT tmp_j = x_mem[j];

      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + i) -= tmp_i;
      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + j) -= tmp_j;
      }

    if(i < t_n_slices)
      {
      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + i) -= x_mem[i];
      }
    }
  else
  if( (t_n_rows == x_n_rows) && (t_n_cols == x_n_cols) && (t_n_slices == 1) )
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_minus( t.slice_colptr(0, col), x.colptr(col), t_n_rows );
      }
    }
  else
  if( (t_n_rows == x_n_rows) && (t_n_cols == 1) && (t_n_slices == x_n_cols) )
    {
    for(uword i=0; i < t_n_slices; ++i)
      {
      arrayops::inplace_minus( t.slice_colptr(i, 0), x.colptr(i), t_n_rows );
      }
    }
  else
  if( (t_n_rows == 1) && (t_n_cols == x_n_rows) && (t_n_slices == x_n_cols) )
    {
    Cube<eT>& Q = const_cast< Cube<eT>& >(t.m);

    const uword t_aux_row1   = t.aux_row1;
    const uword t_aux_col1   = t.aux_col1;
    const uword t_aux_slice1 = t.aux_slice1;

    for(uword slice=0; slice < t_n_slices; ++slice)
      {
      const uword mod_slice = t_aux_slice1 + slice;

      const eT* x_colptr = x.colptr(slice);

      uword i,j;
      for(i=0, j=1; j < t_n_cols; i+=2, j+=2)
        {
        const eT tmp_i = x_colptr[i];
        const eT tmp_j = x_colptr[j];

        Q.at(t_aux_row1, t_aux_col1 + i, mod_slice) -= tmp_i;
        Q.at(t_aux_row1, t_aux_col1 + j, mod_slice) -= tmp_j;
        }

      if(i < t_n_cols)
        {
        Q.at(t_aux_row1, t_aux_col1 + i, mod_slice) -= x_colptr[i];
        }
      }
    }
  else
    {
    if(arma_config::debug == true)
      {
      arma_stop_logic_error( arma_incompat_size_string(t, x, "subtraction") );
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview_cube<eT>::operator%= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> tmp(in.get_ref());

  const Mat<eT>&          x = tmp.M;
        subview_cube<eT>& t = *this;

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  const uword x_n_rows   = x.n_rows;
  const uword x_n_cols   = x.n_cols;

  if( ((x_n_rows == 1) || (x_n_cols == 1)) && (t_n_rows == 1) && (t_n_cols == 1) && (x.n_elem == t_n_slices) )
    {
    Cube<eT>& Q = const_cast< Cube<eT>& >(t.m);

    const uword t_aux_row1   = t.aux_row1;
    const uword t_aux_col1   = t.aux_col1;
    const uword t_aux_slice1 = t.aux_slice1;

    const eT* x_mem = x.memptr();

    uword i,j;
    for(i=0, j=1; j < t_n_slices; i+=2, j+=2)
      {
      const eT tmp_i = x_mem[i];
      const eT tmp_j = x_mem[j];

      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + i) *= tmp_i;
      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + j) *= tmp_j;
      }

    if(i < t_n_slices)
      {
      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + i) *= x_mem[i];
      }
    }
  else
  if( (t_n_rows == x_n_rows) && (t_n_cols == x_n_cols) && (t_n_slices == 1) )
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_mul( t.slice_colptr(0, col), x.colptr(col), t_n_rows );
      }
    }
  else
  if( (t_n_rows == x_n_rows) && (t_n_cols == 1) && (t_n_slices == x_n_cols) )
    {
    for(uword i=0; i < t_n_slices; ++i)
      {
      arrayops::inplace_mul( t.slice_colptr(i, 0), x.colptr(i), t_n_rows );
      }
    }
  else
  if( (t_n_rows == 1) && (t_n_cols == x_n_rows) && (t_n_slices == x_n_cols) )
    {
    Cube<eT>& Q = const_cast< Cube<eT>& >(t.m);

    const uword t_aux_row1   = t.aux_row1;
    const uword t_aux_col1   = t.aux_col1;
    const uword t_aux_slice1 = t.aux_slice1;

    for(uword slice=0; slice < t_n_slices; ++slice)
      {
      const uword mod_slice = t_aux_slice1 + slice;

      const eT* x_colptr = x.colptr(slice);

      uword i,j;
      for(i=0, j=1; j < t_n_cols; i+=2, j+=2)
        {
        const eT tmp_i = x_colptr[i];
        const eT tmp_j = x_colptr[j];

        Q.at(t_aux_row1, t_aux_col1 + i, mod_slice) *= tmp_i;
        Q.at(t_aux_row1, t_aux_col1 + j, mod_slice) *= tmp_j;
        }

      if(i < t_n_cols)
        {
        Q.at(t_aux_row1, t_aux_col1 + i, mod_slice) *= x_colptr[i];
        }
      }
    }
  else
    {
    if(arma_config::debug == true)
      {
      arma_stop_logic_error( arma_incompat_size_string(t, x, "element-wise multiplication") );
      }
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview_cube<eT>::operator/= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> tmp(in.get_ref());

  const Mat<eT>&          x = tmp.M;
        subview_cube<eT>& t = *this;

  const uword t_n_rows   = t.n_rows;
  const uword t_n_cols   = t.n_cols;
  const uword t_n_slices = t.n_slices;

  const uword x_n_rows   = x.n_rows;
  const uword x_n_cols   = x.n_cols;

  if( ((x_n_rows == 1) || (x_n_cols == 1)) && (t_n_rows == 1) && (t_n_cols == 1) && (x.n_elem == t_n_slices) )
    {
    Cube<eT>& Q = const_cast< Cube<eT>& >(t.m);

    const uword t_aux_row1   = t.aux_row1;
    const uword t_aux_col1   = t.aux_col1;
    const uword t_aux_slice1 = t.aux_slice1;

    const eT* x_mem = x.memptr();

    uword i,j;
    for(i=0, j=1; j < t_n_slices; i+=2, j+=2)
      {
      const eT tmp_i = x_mem[i];
      const eT tmp_j = x_mem[j];

      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + i) /= tmp_i;
      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + j) /= tmp_j;
      }

    if(i < t_n_slices)
      {
      Q.at(t_aux_row1, t_aux_col1, t_aux_slice1 + i) /= x_mem[i];
      }
    }
  else
  if( (t_n_rows == x_n_rows) && (t_n_cols == x_n_cols) && (t_n_slices == 1) )
    {
    for(uword col = 0; col < t_n_cols; ++col)
      {
      arrayops::inplace_div( t.slice_colptr(0, col), x.colptr(col), t_n_rows );
      }
    }
  else
  if( (t_n_rows == x_n_rows) && (t_n_cols == 1) && (t_n_slices == x_n_cols) )
    {
    for(uword i=0; i < t_n_slices; ++i)
      {
      arrayops::inplace_div( t.slice_colptr(i, 0), x.colptr(i), t_n_rows );
      }
    }
  else
  if( (t_n_rows == 1) && (t_n_cols == x_n_rows) && (t_n_slices == x_n_cols) )
    {
    Cube<eT>& Q = const_cast< Cube<eT>& >(t.m);

    const uword t_aux_row1   = t.aux_row1;
    const uword t_aux_col1   = t.aux_col1;
    const uword t_aux_slice1 = t.aux_slice1;

    for(uword slice=0; slice < t_n_slices; ++slice)
      {
      const uword mod_slice = t_aux_slice1 + slice;

      const eT* x_colptr = x.colptr(slice);

      uword i,j;
      for(i=0, j=1; j < t_n_cols; i+=2, j+=2)
        {
        const eT tmp_i = x_colptr[i];
        const eT tmp_j = x_colptr[j];

        Q.at(t_aux_row1, t_aux_col1 + i, mod_slice) /= tmp_i;
        Q.at(t_aux_row1, t_aux_col1 + j, mod_slice) /= tmp_j;
        }

      if(i < t_n_cols)
        {
        Q.at(t_aux_row1, t_aux_col1 + i, mod_slice) /= x_colptr[i];
        }
      }
    }
  else
    {
    if(arma_config::debug == true)
      {
      arma_stop_logic_error( arma_incompat_size_string(t, x, "element-wise division") );
      }
    }
  }



template<typename eT>
template<typename gen_type>
inline
void
subview_cube<eT>::operator= (const GenCube<eT,gen_type>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(n_rows, n_cols, n_slices, in.n_rows, in.n_cols, in.n_slices, "copy into subcube");

  in.apply(*this);
  }



//! apply a functor to each element
template<typename eT>
template<typename functor>
inline
void
subview_cube<eT>::for_each(functor F)
  {
  arma_extra_debug_sigprint();

  Cube<eT>& Q = const_cast< Cube<eT>& >(m);

  const uword start_col   = aux_col1;
  const uword start_row   = aux_row1;
  const uword start_slice = aux_slice1;

  const uword end_col_plus1   = start_col   + n_cols;
  const uword end_row_plus1   = start_row   + n_rows;
  const uword end_slice_plus1 = start_slice + n_slices;

  for(uword uslice = start_slice; uslice < end_slice_plus1; ++uslice)
  for(uword ucol   = start_col;     ucol < end_col_plus1;   ++ucol  )
  for(uword urow   = start_row;     urow < end_row_plus1;   ++urow  )
    {
    F( Q.at(urow, ucol, uslice) );
    }
  }



template<typename eT>
template<typename functor>
inline
void
subview_cube<eT>::for_each(functor F) const
  {
  arma_extra_debug_sigprint();

  const Cube<eT>& Q = m;

  const uword start_col   = aux_col1;
  const uword start_row   = aux_row1;
  const uword start_slice = aux_slice1;

  const uword end_col_plus1   = start_col   + n_cols;
  const uword end_row_plus1   = start_row   + n_rows;
  const uword end_slice_plus1 = start_slice + n_slices;

  for(uword uslice = start_slice; uslice < end_slice_plus1; ++uslice)
  for(uword ucol   = start_col;     ucol < end_col_plus1;   ++ucol  )
  for(uword urow   = start_row;     urow < end_row_plus1;   ++urow  )
    {
    F( Q.at(urow, ucol, uslice) );
    }
  }



//! transform each element in the subview using a functor
template<typename eT>
template<typename functor>
inline
void
subview_cube<eT>::transform(functor F)
  {
  arma_extra_debug_sigprint();

  Cube<eT>& Q = const_cast< Cube<eT>& >(m);

  const uword start_col   = aux_col1;
  const uword start_row   = aux_row1;
  const uword start_slice = aux_slice1;

  const uword end_col_plus1   = start_col   + n_cols;
  const uword end_row_plus1   = start_row   + n_rows;
  const uword end_slice_plus1 = start_slice + n_slices;

  for(uword uslice = start_slice; uslice < end_slice_plus1; ++uslice)
  for(uword ucol   = start_col;     ucol < end_col_plus1;   ++ucol  )
  for(uword urow   = start_row;     urow < end_row_plus1;   ++urow  )
    {
    Q.at(urow, ucol, uslice) = eT( F( Q.at(urow, ucol, uslice) ) );
    }
  }



//! imbue (fill) the subview with values provided by a functor
template<typename eT>
template<typename functor>
inline
void
subview_cube<eT>::imbue(functor F)
  {
  arma_extra_debug_sigprint();

  Cube<eT>& Q = const_cast< Cube<eT>& >(m);

  const uword start_col   = aux_col1;
  const uword start_row   = aux_row1;
  const uword start_slice = aux_slice1;

  const uword end_col_plus1   = start_col   + n_cols;
  const uword end_row_plus1   = start_row   + n_rows;
  const uword end_slice_plus1 = start_slice + n_slices;

  for(uword uslice = start_slice; uslice < end_slice_plus1; ++uslice)
  for(uword ucol   = start_col;     ucol < end_col_plus1;   ++ucol  )
  for(uword urow   = start_row;     urow < end_row_plus1;   ++urow  )
    {
    Q.at(urow, ucol, uslice) = eT( F() );
    }
  }



#if defined(ARMA_USE_CXX11)

  //! apply a lambda function to each slice, where each slice is interpreted as a matrix
  template<typename eT>
  inline
  void
  subview_cube<eT>::each_slice(const std::function< void(Mat<eT>&) >& F)
    {
    arma_extra_debug_sigprint();

    Mat<eT> tmp1(n_rows, n_cols);
    Mat<eT> tmp2('j', tmp1.memptr(), n_rows, n_cols);

    for(uword slice_id=0; slice_id < n_slices; ++slice_id)
      {
      for(uword col_id=0; col_id < n_cols; ++col_id)
        {
        arrayops::copy( tmp1.colptr(col_id), slice_colptr(slice_id, col_id), n_rows );
        }

      F(tmp2);

      for(uword col_id=0; col_id < n_cols; ++col_id)
        {
        arrayops::copy( slice_colptr(slice_id, col_id), tmp1.colptr(col_id), n_rows );
        }
      }
    }



  template<typename eT>
  inline
  void
  subview_cube<eT>::each_slice(const std::function< void(const Mat<eT>&) >& F) const
    {
    arma_extra_debug_sigprint();

          Mat<eT> tmp1(n_rows, n_cols);
    const Mat<eT> tmp2('j', tmp1.memptr(), n_rows, n_cols);

    for(uword slice_id=0; slice_id < n_slices; ++slice_id)
      {
      for(uword col_id=0; col_id < n_cols; ++col_id)
        {
        arrayops::copy( tmp1.colptr(col_id), slice_colptr(slice_id, col_id), n_rows );
        }

      F(tmp2);
      }
    }

#endif



template<typename eT>
inline
void
subview_cube<eT>::replace(const eT old_val, const eT new_val)
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      arrayops::replace(slice_colptr(slice,col), local_n_rows, old_val, new_val);
      }
    }
  }



template<typename eT>
inline
void
subview_cube<eT>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      arrayops::inplace_set( slice_colptr(slice,col), val, local_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview_cube<eT>::zeros()
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      arrayops::fill_zeros( slice_colptr(slice,col), local_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview_cube<eT>::ones()
  {
  arma_extra_debug_sigprint();

  fill(eT(1));
  }



template<typename eT>
inline
void
subview_cube<eT>::randu()
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      arma_rng::randu<eT>::fill( slice_colptr(slice,col), local_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview_cube<eT>::randn()
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      arma_rng::randn<eT>::fill( slice_colptr(slice,col), local_n_rows );
      }
    }
  }



template<typename eT>
inline
arma_warn_unused
bool
subview_cube<eT>::is_finite() const
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      if(arrayops::is_finite(slice_colptr(slice,col), local_n_rows) == false)  { return false; }
      }
    }

  return true;
  }



template<typename eT>
inline
arma_warn_unused
bool
subview_cube<eT>::has_inf() const
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      if(arrayops::has_inf(slice_colptr(slice,col), local_n_rows))  { return true; }
      }
    }

  return false;
  }



template<typename eT>
inline
arma_warn_unused
bool
subview_cube<eT>::has_nan() const
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows   = n_rows;
  const uword local_n_cols   = n_cols;
  const uword local_n_slices = n_slices;

  for(uword slice = 0; slice < local_n_slices; ++slice)
    {
    for(uword col = 0; col < local_n_cols; ++col)
      {
      if(arrayops::has_nan(slice_colptr(slice,col), local_n_rows))  { return true; }
      }
    }

  return false;
  }



template<typename eT>
inline
eT
subview_cube<eT>::at_alt(const uword i) const
  {
  return operator[](i);
  }



template<typename eT>
inline
eT&
subview_cube<eT>::operator[](const uword i)
  {
  const uword in_slice = i / n_elem_slice;
  const uword offset   = in_slice * n_elem_slice;
  const uword j        = i - offset;

  const uword in_col   = j / n_rows;
  const uword in_row   = j % n_rows;

  const uword index = (in_slice + aux_slice1)*m.n_elem_slice + (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return access::rw( (const_cast< Cube<eT>& >(m)).mem[index] );
  }



template<typename eT>
inline
eT
subview_cube<eT>::operator[](const uword i) const
  {
  const uword in_slice = i / n_elem_slice;
  const uword offset   = in_slice * n_elem_slice;
  const uword j        = i - offset;

  const uword in_col   = j / n_rows;
  const uword in_row   = j % n_rows;

  const uword index = (in_slice + aux_slice1)*m.n_elem_slice + (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return m.mem[index];
  }



template<typename eT>
inline
eT&
subview_cube<eT>::operator()(const uword i)
  {
  arma_debug_check( (i >= n_elem), "subview_cube::operator(): index out of bounds" );

  const uword in_slice = i / n_elem_slice;
  const uword offset   = in_slice * n_elem_slice;
  const uword j        = i - offset;

  const uword in_col   = j / n_rows;
  const uword in_row   = j % n_rows;

  const uword index = (in_slice + aux_slice1)*m.n_elem_slice + (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return access::rw( (const_cast< Cube<eT>& >(m)).mem[index] );
  }



template<typename eT>
inline
eT
subview_cube<eT>::operator()(const uword i) const
  {
  arma_debug_check( (i >= n_elem), "subview_cube::operator(): index out of bounds" );

  const uword in_slice = i / n_elem_slice;
  const uword offset   = in_slice * n_elem_slice;
  const uword j        = i - offset;

  const uword in_col   = j / n_rows;
  const uword in_row   = j % n_rows;

  const uword index = (in_slice + aux_slice1)*m.n_elem_slice + (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return m.mem[index];
  }



template<typename eT>
arma_inline
eT&
subview_cube<eT>::operator()(const uword in_row, const uword in_col, const uword in_slice)
  {
  arma_debug_check( ( (in_row >= n_rows) || (in_col >= n_cols) || (in_slice >= n_slices) ), "subview_cube::operator(): location out of bounds" );

  const uword index = (in_slice + aux_slice1)*m.n_elem_slice + (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return access::rw( (const_cast< Cube<eT>& >(m)).mem[index] );
  }



template<typename eT>
arma_inline
eT
subview_cube<eT>::operator()(const uword in_row, const uword in_col, const uword in_slice) const
  {
  arma_debug_check( ( (in_row >= n_rows) || (in_col >= n_cols) || (in_slice >= n_slices) ), "subview_cube::operator(): location out of bounds" );

  const uword index = (in_slice + aux_slice1)*m.n_elem_slice + (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return m.mem[index];
  }



template<typename eT>
arma_inline
eT&
subview_cube<eT>::at(const uword in_row, const uword in_col, const uword in_slice)
  {
  const uword index = (in_slice + aux_slice1)*m.n_elem_slice + (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return access::rw( (const_cast< Cube<eT>& >(m)).mem[index] );
  }



template<typename eT>
arma_inline
eT
subview_cube<eT>::at(const uword in_row, const uword in_col, const uword in_slice) const
  {
  const uword index = (in_slice + aux_slice1)*m.n_elem_slice + (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return m.mem[index];
  }



template<typename eT>
arma_inline
eT*
subview_cube<eT>::slice_colptr(const uword in_slice, const uword in_col)
  {
  return & access::rw((const_cast< Cube<eT>& >(m)).mem[  (in_slice + aux_slice1)*m.n_elem_slice + (in_col + aux_col1)*m.n_rows + aux_row1  ]);
  }



template<typename eT>
arma_inline
const eT*
subview_cube<eT>::slice_colptr(const uword in_slice, const uword in_col) const
  {
  return & m.mem[ (in_slice + aux_slice1)*m.n_elem_slice + (in_col + aux_col1)*m.n_rows + aux_row1 ];
  }



template<typename eT>
inline
bool
subview_cube<eT>::check_overlap(const subview_cube<eT>& x) const
  {
  const subview_cube<eT>& t = *this;

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

      const uword t_slice_start  = t.aux_slice1;
      const uword t_slice_end_p1 = t_slice_start + t.n_slices;


      const uword x_row_start  = x.aux_row1;
      const uword x_row_end_p1 = x_row_start + x.n_rows;

      const uword x_col_start  = x.aux_col1;
      const uword x_col_end_p1 = x_col_start + x.n_cols;

      const uword x_slice_start  = x.aux_slice1;
      const uword x_slice_end_p1 = x_slice_start + x.n_slices;


      const bool outside_rows   = ( (x_row_start   >= t_row_end_p1  ) || (t_row_start   >= x_row_end_p1  ) );
      const bool outside_cols   = ( (x_col_start   >= t_col_end_p1  ) || (t_col_start   >= x_col_end_p1  ) );
      const bool outside_slices = ( (x_slice_start >= t_slice_end_p1) || (t_slice_start >= x_slice_end_p1) );

      return ( (outside_rows == false) && (outside_cols == false) && (outside_slices == false) );
      }
    }
  }



template<typename eT>
inline
bool
subview_cube<eT>::check_overlap(const Mat<eT>& x) const
  {
  const subview_cube<eT>& t = *this;

  const uword t_aux_slice1        = t.aux_slice1;
  const uword t_aux_slice2_plus_1 = t_aux_slice1 + t.n_slices;

  for(uword slice = t_aux_slice1; slice < t_aux_slice2_plus_1; ++slice)
    {
    if(t.m.mat_ptrs[slice] != NULL)
      {
      const Mat<eT>& y = *(t.m.mat_ptrs[slice]);

      if( x.memptr() == y.memptr() )  { return true; }
      }
    }

  return false;
  }



//! cube X = Y.subcube(...)
template<typename eT>
inline
void
subview_cube<eT>::extract(Cube<eT>& out, const subview_cube<eT>& in)
  {
  arma_extra_debug_sigprint();

  // NOTE: we're assuming that the cube has already been set to the correct size and there is no aliasing;
  // size setting and alias checking is done by either the Cube contructor or operator=()

  const uword n_rows   = in.n_rows;
  const uword n_cols   = in.n_cols;
  const uword n_slices = in.n_slices;

  arma_extra_debug_print(arma_str::format("out.n_rows = %d   out.n_cols = %d    out.n_slices = %d    in.m.n_rows = %d   in.m.n_cols = %d   in.m.n_slices = %d") % out.n_rows % out.n_cols % out.n_slices % in.m.n_rows % in.m.n_cols % in.m.n_slices);


  for(uword slice = 0; slice < n_slices; ++slice)
    {
    for(uword col = 0; col < n_cols; ++col)
      {
      arrayops::copy( out.slice_colptr(slice,col), in.slice_colptr(slice,col), n_rows );
      }
    }
  }



//! cube X += Y.subcube(...)
template<typename eT>
inline
void
subview_cube<eT>::plus_inplace(Cube<eT>& out, const subview_cube<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out, in, "addition");

  const uword n_rows   = out.n_rows;
  const uword n_cols   = out.n_cols;
  const uword n_slices = out.n_slices;

  for(uword slice = 0; slice<n_slices; ++slice)
    {
    for(uword col = 0; col<n_cols; ++col)
      {
      arrayops::inplace_plus( out.slice_colptr(slice,col), in.slice_colptr(slice,col), n_rows );
      }
    }
  }



//! cube X -= Y.subcube(...)
template<typename eT>
inline
void
subview_cube<eT>::minus_inplace(Cube<eT>& out, const subview_cube<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out, in, "subtraction");

  const uword n_rows   = out.n_rows;
  const uword n_cols   = out.n_cols;
  const uword n_slices = out.n_slices;

  for(uword slice = 0; slice<n_slices; ++slice)
    {
    for(uword col = 0; col<n_cols; ++col)
      {
      arrayops::inplace_minus( out.slice_colptr(slice,col), in.slice_colptr(slice,col), n_rows );
      }
    }
  }



//! cube X %= Y.subcube(...)
template<typename eT>
inline
void
subview_cube<eT>::schur_inplace(Cube<eT>& out, const subview_cube<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out, in, "element-wise multiplication");

  const uword n_rows   = out.n_rows;
  const uword n_cols   = out.n_cols;
  const uword n_slices = out.n_slices;

  for(uword slice = 0; slice<n_slices; ++slice)
    {
    for(uword col = 0; col<n_cols; ++col)
      {
      arrayops::inplace_mul( out.slice_colptr(slice,col), in.slice_colptr(slice,col), n_rows );
      }
    }
  }



//! cube X /= Y.subcube(...)
template<typename eT>
inline
void
subview_cube<eT>::div_inplace(Cube<eT>& out, const subview_cube<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out, in, "element-wise division");

  const uword n_rows   = out.n_rows;
  const uword n_cols   = out.n_cols;
  const uword n_slices = out.n_slices;

  for(uword slice = 0; slice<n_slices; ++slice)
    {
    for(uword col = 0; col<n_cols; ++col)
      {
      arrayops::inplace_div( out.slice_colptr(slice,col), in.slice_colptr(slice,col), n_rows );
      }
    }
  }



//! mat X = Y.subcube(...)
template<typename eT>
inline
void
subview_cube<eT>::extract(Mat<eT>& out, const subview_cube<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_cube_as_mat(out, in, "copy into matrix", false);

  const uword in_n_rows   = in.n_rows;
  const uword in_n_cols   = in.n_cols;
  const uword in_n_slices = in.n_slices;

  const uword out_vec_state = out.vec_state;

  if(in_n_slices == 1)
    {
    out.set_size(in_n_rows, in_n_cols);

    for(uword col=0; col < in_n_cols; ++col)
      {
      arrayops::copy( out.colptr(col), in.slice_colptr(0, col), in_n_rows );
      }
    }
  else
    {
    if(out_vec_state == 0)
      {
      if(in_n_cols == 1)
        {
        out.set_size(in_n_rows, in_n_slices);

        for(uword i=0; i < in_n_slices; ++i)
          {
          arrayops::copy( out.colptr(i), in.slice_colptr(i, 0), in_n_rows );
          }
        }
      else
      if(in_n_rows == 1)
        {
        const Cube<eT>& Q = in.m;

        const uword in_aux_row1   = in.aux_row1;
        const uword in_aux_col1   = in.aux_col1;
        const uword in_aux_slice1 = in.aux_slice1;

        out.set_size(in_n_cols, in_n_slices);

        for(uword slice=0; slice < in_n_slices; ++slice)
          {
          const uword mod_slice = in_aux_slice1 + slice;

          eT* out_colptr = out.colptr(slice);

          uword i,j;
          for(i=0, j=1; j < in_n_cols; i+=2, j+=2)
            {
            const eT tmp_i = Q.at(in_aux_row1, in_aux_col1 + i, mod_slice);
            const eT tmp_j = Q.at(in_aux_row1, in_aux_col1 + j, mod_slice);

            out_colptr[i] = tmp_i;
            out_colptr[j] = tmp_j;
            }

          if(i < in_n_cols)
            {
            out_colptr[i] = Q.at(in_aux_row1, in_aux_col1 + i, mod_slice);
            }
          }
        }
      }
    else
      {
      out.set_size(in_n_slices);

      eT* out_mem = out.memptr();

      const Cube<eT>& Q = in.m;

      const uword in_aux_row1   = in.aux_row1;
      const uword in_aux_col1   = in.aux_col1;
      const uword in_aux_slice1 = in.aux_slice1;

      for(uword i=0; i<in_n_slices; ++i)
        {
        out_mem[i] = Q.at(in_aux_row1, in_aux_col1, in_aux_slice1 + i);
        }
      }
    }
  }



//! mat X += Y.subcube(...)
template<typename eT>
inline
void
subview_cube<eT>::plus_inplace(Mat<eT>& out, const subview_cube<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_cube_as_mat(out, in, "addition", true);

  const uword in_n_rows   = in.n_rows;
  const uword in_n_cols   = in.n_cols;
  const uword in_n_slices = in.n_slices;

  const uword out_n_rows    = out.n_rows;
  const uword out_n_cols    = out.n_cols;
  const uword out_vec_state = out.vec_state;

  if(in_n_slices == 1)
    {
    if( (arma_config::debug) && ((out_n_rows != in_n_rows) || (out_n_cols != in_n_cols)) )
      {
      std::stringstream tmp;

      tmp
        << "in-place addition: "
        << out_n_rows << 'x' << out_n_cols << " output matrix is incompatible with "
        <<  in_n_rows << 'x' <<  in_n_cols << 'x' << in_n_slices << " cube interpreted as "
        <<  in_n_rows << 'x' <<  in_n_cols << " matrix";

      arma_stop_logic_error(tmp.str());
      }

    for(uword col=0; col < in_n_cols; ++col)
      {
      arrayops::inplace_plus( out.colptr(col), in.slice_colptr(0, col), in_n_rows );
      }
    }
  else
    {
    if(out_vec_state == 0)
      {
      if( (in_n_rows == out_n_rows) && (in_n_cols == 1) && (in_n_slices == out_n_cols) )
        {
        for(uword i=0; i < in_n_slices; ++i)
          {
          arrayops::inplace_plus( out.colptr(i), in.slice_colptr(i, 0), in_n_rows );
          }
        }
      else
      if( (in_n_rows == 1) && (in_n_cols == out_n_rows) && (in_n_slices == out_n_cols) )
        {
        const Cube<eT>& Q = in.m;

        const uword in_aux_row1   = in.aux_row1;
        const uword in_aux_col1   = in.aux_col1;
        const uword in_aux_slice1 = in.aux_slice1;

        for(uword slice=0; slice < in_n_slices; ++slice)
          {
          const uword mod_slice = in_aux_slice1 + slice;

          eT* out_colptr = out.colptr(slice);

          uword i,j;
          for(i=0, j=1; j < in_n_cols; i+=2, j+=2)
            {
            const eT tmp_i = Q.at(in_aux_row1, in_aux_col1 + i, mod_slice);
            const eT tmp_j = Q.at(in_aux_row1, in_aux_col1 + j, mod_slice);

            out_colptr[i] += tmp_i;
            out_colptr[j] += tmp_j;
            }

          if(i < in_n_cols)
            {
            out_colptr[i] += Q.at(in_aux_row1, in_aux_col1 + i, mod_slice);
            }
          }
        }
      }
    else
      {
      eT* out_mem = out.memptr();

      const Cube<eT>& Q = in.m;

      const uword in_aux_row1   = in.aux_row1;
      const uword in_aux_col1   = in.aux_col1;
      const uword in_aux_slice1 = in.aux_slice1;

      for(uword i=0; i<in_n_slices; ++i)
        {
        out_mem[i] += Q.at(in_aux_row1, in_aux_col1, in_aux_slice1 + i);
        }
      }
    }
  }



//! mat X -= Y.subcube(...)
template<typename eT>
inline
void
subview_cube<eT>::minus_inplace(Mat<eT>& out, const subview_cube<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_cube_as_mat(out, in, "subtraction", true);

  const uword in_n_rows   = in.n_rows;
  const uword in_n_cols   = in.n_cols;
  const uword in_n_slices = in.n_slices;

  const uword out_n_rows    = out.n_rows;
  const uword out_n_cols    = out.n_cols;
  const uword out_vec_state = out.vec_state;

  if(in_n_slices == 1)
    {
    if( (arma_config::debug) && ((out_n_rows != in_n_rows) || (out_n_cols != in_n_cols)) )
      {
      std::stringstream tmp;

      tmp
        << "in-place subtraction: "
        << out_n_rows << 'x' << out_n_cols << " output matrix is incompatible with "
        <<  in_n_rows << 'x' <<  in_n_cols << 'x' << in_n_slices << " cube interpreted as "
        <<  in_n_rows << 'x' <<  in_n_cols << " matrix";

      arma_stop_logic_error(tmp.str());
      }

    for(uword col=0; col < in_n_cols; ++col)
      {
      arrayops::inplace_minus( out.colptr(col), in.slice_colptr(0, col), in_n_rows );
      }
    }
  else
    {
    if(out_vec_state == 0)
      {
      if( (in_n_rows == out_n_rows) && (in_n_cols == 1) && (in_n_slices == out_n_cols) )
        {
        for(uword i=0; i < in_n_slices; ++i)
          {
          arrayops::inplace_minus( out.colptr(i), in.slice_colptr(i, 0), in_n_rows );
          }
        }
      else
      if( (in_n_rows == 1) && (in_n_cols == out_n_rows) && (in_n_slices == out_n_cols) )
        {
        const Cube<eT>& Q = in.m;

        const uword in_aux_row1   = in.aux_row1;
        const uword in_aux_col1   = in.aux_col1;
        const uword in_aux_slice1 = in.aux_slice1;

        for(uword slice=0; slice < in_n_slices; ++slice)
          {
          const uword mod_slice = in_aux_slice1 + slice;

          eT* out_colptr = out.colptr(slice);

          uword i,j;
          for(i=0, j=1; j < in_n_cols; i+=2, j+=2)
            {
            const eT tmp_i = Q.at(in_aux_row1, in_aux_col1 + i, mod_slice);
            const eT tmp_j = Q.at(in_aux_row1, in_aux_col1 + j, mod_slice);

            out_colptr[i] -= tmp_i;
            out_colptr[j] -= tmp_j;
            }

          if(i < in_n_cols)
            {
            out_colptr[i] -= Q.at(in_aux_row1, in_aux_col1 + i, mod_slice);
            }
          }
        }
      }
    else
      {
      eT* out_mem = out.memptr();

      const Cube<eT>& Q = in.m;

      const uword in_aux_row1   = in.aux_row1;
      const uword in_aux_col1   = in.aux_col1;
      const uword in_aux_slice1 = in.aux_slice1;

      for(uword i=0; i<in_n_slices; ++i)
        {
        out_mem[i] -= Q.at(in_aux_row1, in_aux_col1, in_aux_slice1 + i);
        }
      }
    }
  }



//! mat X %= Y.subcube(...)
template<typename eT>
inline
void
subview_cube<eT>::schur_inplace(Mat<eT>& out, const subview_cube<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_cube_as_mat(out, in, "element-wise multiplication", true);

  const uword in_n_rows   = in.n_rows;
  const uword in_n_cols   = in.n_cols;
  const uword in_n_slices = in.n_slices;

  const uword out_n_rows    = out.n_rows;
  const uword out_n_cols    = out.n_cols;
  const uword out_vec_state = out.vec_state;

  if(in_n_slices == 1)
    {
    if( (arma_config::debug) && ((out_n_rows != in_n_rows) || (out_n_cols != in_n_cols)) )
      {
      std::stringstream tmp;

      tmp
        << "in-place element-wise multiplication: "
        << out_n_rows << 'x' << out_n_cols << " output matrix is incompatible with "
        <<  in_n_rows << 'x' <<  in_n_cols << 'x' << in_n_slices << " cube interpreted as "
        <<  in_n_rows << 'x' <<  in_n_cols << " matrix";

      arma_stop_logic_error(tmp.str());
      }

    for(uword col=0; col < in_n_cols; ++col)
      {
      arrayops::inplace_mul( out.colptr(col), in.slice_colptr(0, col), in_n_rows );
      }
    }
  else
    {
    if(out_vec_state == 0)
      {
      if( (in_n_rows == out_n_rows) && (in_n_cols == 1) && (in_n_slices == out_n_cols) )
        {
        for(uword i=0; i < in_n_slices; ++i)
          {
          arrayops::inplace_mul( out.colptr(i), in.slice_colptr(i, 0), in_n_rows );
          }
        }
      else
      if( (in_n_rows == 1) && (in_n_cols == out_n_rows) && (in_n_slices == out_n_cols) )
        {
        const Cube<eT>& Q = in.m;

        const uword in_aux_row1   = in.aux_row1;
        const uword in_aux_col1   = in.aux_col1;
        const uword in_aux_slice1 = in.aux_slice1;

        for(uword slice=0; slice < in_n_slices; ++slice)
          {
          const uword mod_slice = in_aux_slice1 + slice;

          eT* out_colptr = out.colptr(slice);

          uword i,j;
          for(i=0, j=1; j < in_n_cols; i+=2, j+=2)
            {
            const eT tmp_i = Q.at(in_aux_row1, in_aux_col1 + i, mod_slice);
            const eT tmp_j = Q.at(in_aux_row1, in_aux_col1 + j, mod_slice);

            out_colptr[i] *= tmp_i;
            out_colptr[j] *= tmp_j;
            }

          if(i < in_n_cols)
            {
            out_colptr[i] *= Q.at(in_aux_row1, in_aux_col1 + i, mod_slice);
            }
          }
        }
      }
    else
      {
      eT* out_mem = out.memptr();

      const Cube<eT>& Q = in.m;

      const uword in_aux_row1   = in.aux_row1;
      const uword in_aux_col1   = in.aux_col1;
      const uword in_aux_slice1 = in.aux_slice1;

      for(uword i=0; i<in_n_slices; ++i)
        {
        out_mem[i] *= Q.at(in_aux_row1, in_aux_col1, in_aux_slice1 + i);
        }
      }
    }
  }



//! mat X /= Y.subcube(...)
template<typename eT>
inline
void
subview_cube<eT>::div_inplace(Mat<eT>& out, const subview_cube<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_cube_as_mat(out, in, "element-wise division", true);

  const uword in_n_rows   = in.n_rows;
  const uword in_n_cols   = in.n_cols;
  const uword in_n_slices = in.n_slices;

  const uword out_n_rows    = out.n_rows;
  const uword out_n_cols    = out.n_cols;
  const uword out_vec_state = out.vec_state;

  if(in_n_slices == 1)
    {
    if( (arma_config::debug) && ((out_n_rows != in_n_rows) || (out_n_cols != in_n_cols)) )
      {
      std::stringstream tmp;

      tmp
        << "in-place element-wise division: "
        << out_n_rows << 'x' << out_n_cols << " output matrix is incompatible with "
        <<  in_n_rows << 'x' <<  in_n_cols << 'x' << in_n_slices << " cube interpreted as "
        <<  in_n_rows << 'x' <<  in_n_cols << " matrix";

      arma_stop_logic_error(tmp.str());
      }

    for(uword col=0; col < in_n_cols; ++col)
      {
      arrayops::inplace_div( out.colptr(col), in.slice_colptr(0, col), in_n_rows );
      }
    }
  else
    {
    if(out_vec_state == 0)
      {
      if( (in_n_rows == out_n_rows) && (in_n_cols == 1) && (in_n_slices == out_n_cols) )
        {
        for(uword i=0; i < in_n_slices; ++i)
          {
          arrayops::inplace_div( out.colptr(i), in.slice_colptr(i, 0), in_n_rows );
          }
        }
      else
      if( (in_n_rows == 1) && (in_n_cols == out_n_rows) && (in_n_slices == out_n_cols) )
        {
        const Cube<eT>& Q = in.m;

        const uword in_aux_row1   = in.aux_row1;
        const uword in_aux_col1   = in.aux_col1;
        const uword in_aux_slice1 = in.aux_slice1;

        for(uword slice=0; slice < in_n_slices; ++slice)
          {
          const uword mod_slice = in_aux_slice1 + slice;

          eT* out_colptr = out.colptr(slice);

          uword i,j;
          for(i=0, j=1; j < in_n_cols; i+=2, j+=2)
            {
            const eT tmp_i = Q.at(in_aux_row1, in_aux_col1 + i, mod_slice);
            const eT tmp_j = Q.at(in_aux_row1, in_aux_col1 + j, mod_slice);

            out_colptr[i] /= tmp_i;
            out_colptr[j] /= tmp_j;
            }

          if(i < in_n_cols)
            {
            out_colptr[i] /= Q.at(in_aux_row1, in_aux_col1 + i, mod_slice);
            }
          }
        }
      }
    else
      {
      eT* out_mem = out.memptr();

      const Cube<eT>& Q = in.m;

      const uword in_aux_row1   = in.aux_row1;
      const uword in_aux_col1   = in.aux_col1;
      const uword in_aux_slice1 = in.aux_slice1;

      for(uword i=0; i<in_n_slices; ++i)
        {
        out_mem[i] /= Q.at(in_aux_row1, in_aux_col1, in_aux_slice1 + i);
        }
      }
    }
  }



//! @}
