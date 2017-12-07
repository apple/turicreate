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


///////////////////////////////////////////////////////////////////////////////
// SpSubview::iterator_base implementation                                   //
///////////////////////////////////////////////////////////////////////////////

template<typename eT>
inline
SpSubview<eT>::iterator_base::iterator_base(const SpSubview<eT>& in_M)
  : M(in_M)
  , internal_col(0)
  , internal_pos(0)
  , skip_pos(0)
  {
  // Technically this iterator is invalid (it may not point to a real element).
  }



template<typename eT>
inline
SpSubview<eT>::iterator_base::iterator_base(const SpSubview<eT>& in_M, const uword in_col, const uword in_pos, const uword in_skip_pos)
  : M(in_M)
  , internal_col(in_col)
  , internal_pos(in_pos)
  , skip_pos    (in_skip_pos)
  {
  // Nothing to do.
  }



template<typename eT>
arma_inline
eT
SpSubview<eT>::iterator_base::operator*() const
  {
  return M.m.values[internal_pos + skip_pos];
  }



///////////////////////////////////////////////////////////////////////////////
// SpSubview::const_iterator implementation                                  //
///////////////////////////////////////////////////////////////////////////////

template<typename eT>
inline
SpSubview<eT>::const_iterator::const_iterator(const SpSubview<eT>& in_M, const uword initial_pos)
  : iterator_base(in_M, 0, initial_pos, 0)
  {
  // Corner case for empty subviews.
  if(in_M.n_nonzero == 0)
    {
    iterator_base::internal_col = in_M.n_cols;
    iterator_base::skip_pos     = in_M.m.n_nonzero;
    return;
    }

  // Figure out the row and column of the position.
  // lskip_pos holds the number of values which aren't part of this subview.
  const uword aux_col = iterator_base::M.aux_col1;
  const uword aux_row = iterator_base::M.aux_row1;
  const uword ln_rows = iterator_base::M.n_rows;
  const uword ln_cols = iterator_base::M.n_cols;

  uword cur_pos   = 0; // off by one because we might be searching for pos 0
  uword lskip_pos = iterator_base::M.m.col_ptrs[aux_col];
  uword cur_col   = 0;

  while(cur_pos < (iterator_base::internal_pos + 1))
    {
    // Have we stepped forward a column (or multiple columns)?
    while(((lskip_pos + cur_pos) >= iterator_base::M.m.col_ptrs[cur_col + aux_col + 1]) && (cur_col < ln_cols))
      {
      ++cur_col;
      }

    // See if the current position is in the subview.
    const uword row_index = iterator_base::M.m.row_indices[cur_pos + lskip_pos];
    if(row_index < aux_row)
      {
      ++lskip_pos; // not valid
      }
    else if(row_index < (aux_row + ln_rows))
      {
      ++cur_pos; // valid, in the subview
      }
    else
      {
      // skip to end of column
      const uword next_colptr = iterator_base::M.m.col_ptrs[cur_col + aux_col + 1];
      lskip_pos += (next_colptr - (cur_pos + lskip_pos));
      }
    }

  iterator_base::internal_col = cur_col;
  iterator_base::skip_pos     = lskip_pos;
  }



template<typename eT>
inline
SpSubview<eT>::const_iterator::const_iterator(const SpSubview<eT>& in_M, const uword in_row, const uword in_col)
  : iterator_base(in_M, in_col, 0, 0)
  {
  // Corner case for empty subviews.
  if(in_M.n_nonzero == 0)
    {
    // We must be at the last position.
    iterator_base::internal_col = in_M.n_cols;
    iterator_base::skip_pos = in_M.m.n_nonzero;
    return;
    }

  // We have a destination we want to be just after, but don't know what position that is.
  // Because we have to count the points in this subview and not in this subview, this becomes a little difficult and slow.
  const uword aux_col = iterator_base::M.aux_col1;
  const uword aux_row = iterator_base::M.aux_row1;
  const uword ln_rows = iterator_base::M.n_rows;
  const uword ln_cols = iterator_base::M.n_cols;

  uword cur_pos = 0;
  uword skip_pos = iterator_base::M.m.col_ptrs[aux_col];
  uword cur_col = 0;

  // Skip any empty columns.
  while(((skip_pos + cur_pos) >= iterator_base::M.m.col_ptrs[cur_col + aux_col + 1]) && (cur_col < ln_cols))
    {
    ++cur_col;
    }

  while(cur_col < in_col)
    {
    // See if the current position is in the subview.
    const uword row_index = iterator_base::M.m.row_indices[cur_pos + skip_pos];
    if(row_index < aux_row)
      {
      ++skip_pos;
      }
    else if(row_index < (aux_row + ln_rows))
      {
      ++cur_pos;
      }
    else
      {
      // skip to end of column
      const uword next_colptr = iterator_base::M.m.col_ptrs[cur_col + aux_col + 1];
      skip_pos += (next_colptr - (cur_pos + skip_pos));
      }

    // Have we stepped forward a column (or multiple columns)?
    while(((skip_pos + cur_pos) >= iterator_base::M.m.col_ptrs[cur_col + aux_col + 1]) && (cur_col < ln_cols))
      {
      ++cur_col;
      }
    }

  // Now we are either on the right column or ahead of it.
  if(cur_col == in_col)
    {
    // We have to find the right row index.
    uword row_index = iterator_base::M.m.row_indices[cur_pos + skip_pos];
    while((row_index < (in_row + aux_row)))
      {
      if(row_index < aux_row)
        {
        ++skip_pos;
        }
      else
        {
        ++cur_pos;
        }

      // Ensure we didn't step forward a column; if we did, we need to stop.
      while(((skip_pos + cur_pos) >= iterator_base::M.m.col_ptrs[cur_col + aux_col + 1]) && (cur_col < ln_cols))
        {
        ++cur_col;
        }

      if(cur_col != in_col)
        {
        break;
        }

      row_index = iterator_base::M.m.row_indices[cur_pos + skip_pos];
      }
    }

  // Now we need to find the next valid position in the subview.
  uword row_index;
  while(true)
    {
    const uword next_colptr = iterator_base::M.m.col_ptrs[cur_col + aux_col + 1];
    row_index = iterator_base::M.m.row_indices[cur_pos + skip_pos];

    // Are we at the last position?
    if(cur_col >= ln_cols)
      {
      cur_col = ln_cols;
      // Make sure we will be pointing at the last element in the parent matrix.
      skip_pos = iterator_base::M.m.n_nonzero - iterator_base::M.n_nonzero;
      break;
      }

    if(row_index < aux_row)
      {
      ++skip_pos;
      }
    else if(row_index < (aux_row + ln_rows))
      {
      break; // found
      }
    else
      {
      skip_pos += (next_colptr - (cur_pos + skip_pos));
      }

    // Did we move any columns?
    while(((skip_pos + cur_pos) >= iterator_base::M.m.col_ptrs[cur_col + aux_col + 1]) && (cur_col < ln_cols))
      {
      ++cur_col;
      }
    }

  // It is possible we have moved another column.
  while(((skip_pos + cur_pos) >= iterator_base::M.m.col_ptrs[cur_col + aux_col + 1]) && (cur_col < ln_cols))
    {
    ++cur_col;
    }

  iterator_base::internal_pos = cur_pos;
  iterator_base::skip_pos     = skip_pos;
  iterator_base::internal_col = cur_col;
  }



template<typename eT>
inline
SpSubview<eT>::const_iterator::const_iterator(const SpSubview<eT>& in_M, uword in_row, uword in_col, uword in_pos, uword in_skip_pos)
  : iterator_base(in_M, in_col, in_pos, in_skip_pos)
  {
  arma_ignore(in_row);

  // Nothing to do.
  }



template<typename eT>
inline
SpSubview<eT>::const_iterator::const_iterator(const const_iterator& other)
  : iterator_base(other.M, other.internal_col, other.internal_pos, other.skip_pos)
  {
  // Nothing to do.
  }



template<typename eT>
inline
typename SpSubview<eT>::const_iterator&
SpSubview<eT>::const_iterator::operator++()
  {
  const uword aux_col = iterator_base::M.aux_col1;
  const uword aux_row = iterator_base::M.aux_row1;
  const uword ln_rows = iterator_base::M.n_rows;
  const uword ln_cols = iterator_base::M.n_cols;

  uword cur_col   = iterator_base::internal_col;
  uword cur_pos   = iterator_base::internal_pos + 1;
  uword lskip_pos = iterator_base::skip_pos;
  uword row_index;

  while(true)
    {
    const uword next_colptr = iterator_base::M.m.col_ptrs[cur_col + aux_col + 1];
    row_index = iterator_base::M.m.row_indices[cur_pos + lskip_pos];

    // Did we move any columns?
    while((cur_col < ln_cols) && ((lskip_pos + cur_pos) >= iterator_base::M.m.col_ptrs[cur_col + aux_col + 1]))
      {
      ++cur_col;
      }

    // Are we at the last position?
    if(cur_col >= ln_cols)
      {
      cur_col = ln_cols;
      // Make sure we will be pointing at the last element in the parent matrix.
      lskip_pos = iterator_base::M.m.n_nonzero - iterator_base::M.n_nonzero;
      break;
      }

    if(row_index < aux_row)
      {
      ++lskip_pos;
      }
    else if(row_index < (aux_row + ln_rows))
      {
      break; // found
      }
    else
      {
      lskip_pos += (next_colptr - (cur_pos + lskip_pos));
      }
    }

  iterator_base::internal_pos = cur_pos;
  iterator_base::internal_col = cur_col;
  iterator_base::skip_pos     = lskip_pos;

  return *this;
  }



template<typename eT>
inline
typename SpSubview<eT>::const_iterator
SpSubview<eT>::const_iterator::operator++(int)
  {
  typename SpSubview<eT>::const_iterator tmp(*this);

  ++(*this);

  return tmp;
  }



template<typename eT>
inline
typename SpSubview<eT>::const_iterator&
SpSubview<eT>::const_iterator::operator--()
  {
  const uword aux_col = iterator_base::M.aux_col1;
  const uword aux_row = iterator_base::M.aux_row1;
  const uword ln_rows = iterator_base::M.n_rows;

  uword cur_col  = iterator_base::internal_col;
  uword cur_pos  = iterator_base::internal_pos - 1;
  uword skip_pos = iterator_base::skip_pos;

  // Special condition for end of iterator.
  if((skip_pos + cur_pos + 1) == iterator_base::M.m.n_nonzero)
    {
    // We are at the last element.  So we need to set skip_pos back to what it
    // would be if we didn't manually modify it back in operator++().
    skip_pos = iterator_base::M.m.col_ptrs[cur_col + aux_col] - iterator_base::internal_pos;
    }

  uword row_index;

  while(true)
    {
    const uword colptr = iterator_base::M.m.col_ptrs[cur_col + aux_col];
    row_index = iterator_base::M.m.row_indices[cur_pos + skip_pos];

    // Did we move back any columns?
    while((skip_pos + cur_pos) < iterator_base::M.m.col_ptrs[cur_col + aux_col])
      {
      --cur_col;
      }

    if(row_index < aux_row)
      {
      skip_pos -= (colptr - (cur_pos + skip_pos) + 1);
      }
    else if(row_index < (aux_row + ln_rows))
      {
      break; // found
      }
    else
      {
      --skip_pos;
      }
    }

  iterator_base::internal_pos = cur_pos;
  iterator_base::skip_pos     = skip_pos;
  iterator_base::internal_col = cur_col;

  return *this;
  }



template<typename eT>
inline
typename SpSubview<eT>::const_iterator
SpSubview<eT>::const_iterator::operator--(int)
  {
  typename SpSubview<eT>::const_iterator tmp(*this);

  --(*this);

  return tmp;
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_iterator::operator==(const const_iterator& rhs) const
  {
  return (rhs.row() == (*this).row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_iterator::operator!=(const const_iterator& rhs) const
  {
  return (rhs.row() != (*this).row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_iterator::operator==(const typename SpMat<eT>::const_iterator& rhs) const
  {
  return (rhs.row() == (*this).row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_iterator::operator!=(const typename SpMat<eT>::const_iterator& rhs) const
  {
  return (rhs.row() != (*this).row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_iterator::operator==(const const_row_iterator& rhs) const
  {
  return (rhs.row() == (*this).row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_iterator::operator!=(const const_row_iterator& rhs) const
  {
  return (rhs.row() != (*this).row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_iterator::operator==(const typename SpMat<eT>::const_row_iterator& rhs) const
  {
  return (rhs.row() == (*this).row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_iterator::operator!=(const typename SpMat<eT>::const_row_iterator& rhs) const
  {
  return (rhs.row() != (*this).row()) || (rhs.col() != iterator_base::internal_col);
  }



///////////////////////////////////////////////////////////////////////////////
// SpSubview<eT>::iterator implementation                                    //
///////////////////////////////////////////////////////////////////////////////

template<typename eT>
inline
SpValProxy<SpSubview<eT> >
SpSubview<eT>::iterator::operator*()
  {
  return SpValProxy<SpSubview<eT> >(
    iterator_base::row(),
    iterator_base::col(),
    access::rw(iterator_base::M),
    &(access::rw(iterator_base::M.m.values[iterator_base::internal_pos + iterator_base::skip_pos])));
  }



template<typename eT>
inline
typename SpSubview<eT>::iterator&
SpSubview<eT>::iterator::operator++()
  {
  const_iterator::operator++();
  return *this;
  }



template<typename eT>
inline
typename SpSubview<eT>::iterator
SpSubview<eT>::iterator::operator++(int)
  {
  typename SpSubview<eT>::iterator tmp(*this);

  const_iterator::operator++();

  return tmp;
  }



template<typename eT>
inline
typename SpSubview<eT>::iterator&
SpSubview<eT>::iterator::operator--()
  {
  const_iterator::operator--();
  return *this;
  }



template<typename eT>
inline
typename SpSubview<eT>::iterator
SpSubview<eT>::iterator::operator--(int)
  {
  typename SpSubview<eT>::iterator tmp(*this);

  const_iterator::operator--();

  return tmp;
  }



///////////////////////////////////////////////////////////////////////////////
// SpSubview<eT>::const_row_iterator implementation                          //
///////////////////////////////////////////////////////////////////////////////

template<typename eT>
inline
SpSubview<eT>::const_row_iterator::const_row_iterator(const SpSubview<eT>& in_M, uword initial_pos)
  : iterator_base(in_M, 0, initial_pos, 0)
  , internal_row(0)
  , actual_pos(0)
  {
  // Corner case for empty subviews.
  if(in_M.n_nonzero == 0)
    {
    iterator_base::internal_col = 0;
    internal_row = in_M.n_rows;
    iterator_base::skip_pos = in_M.m.n_nonzero;
    return;
    }

  const uword aux_col = iterator_base::M.aux_col1;
  const uword aux_row = iterator_base::M.aux_row1;
  const uword ln_cols = iterator_base::M.n_cols;

  // We don't know where the elements are in each row.  What we will do is
  // loop across all valid columns looking for elements in row 0 (and add to
  // our sum), then in row 1, and so forth, until we get to the desired
  // position.
  uword cur_pos = -1;  // TODO: HACK: -1 is not a valid unsigned integer; using -1 is relying on wraparound/overflow, which is not portable
  uword cur_row = 0;
  uword cur_col = 0;

  while(true)
    {
    // Is there anything in the column we are looking at?
    const uword colptr = iterator_base::M.m.col_ptrs[cur_col + aux_col];
    const uword next_colptr = iterator_base::M.m.col_ptrs[cur_col + aux_col + 1];

    for(uword ind = colptr; (ind < next_colptr) && (iterator_base::M.m.row_indices[ind] <= (cur_row + aux_row)); ++ind)
      {
      // There is something in this column.  Is it in the row we are looking at?
      const uword row_index = iterator_base::M.m.row_indices[ind];
      if(row_index == (cur_row + aux_row))
        {
        // Yes, it is in the right row.
        if(++cur_pos == iterator_base::internal_pos)   // TODO: HACK: if cur_pos is std::numeric_limits<uword>::max(), ++cur_pos relies on a wraparound/overflow, which is not portable
          {
          iterator_base::internal_col = cur_col;
          internal_row = cur_row;
          actual_pos = ind;

          return;
          }

        // We are done with this column.  Break to the column incrementing code (directly below).
        break;
        }
      else if(row_index > (cur_row + aux_row))
        {
        break; // Can't be in this column.
        }
      }

    cur_col++; // Done with the column.  Move on.
    if(cur_col == ln_cols)
      {
      // Out of columns.  Loop back to the beginning and look on the next row.
      cur_col = 0;
      cur_row++;
      }
    }
  }



template<typename eT>
inline
SpSubview<eT>::const_row_iterator::const_row_iterator(const SpSubview<eT>& in_M, uword in_row, uword in_col)
  : iterator_base(in_M, in_col, 0, 0)
  , internal_row(0)
  , actual_pos(0)
  {
  // We have a destination we want to be just after, but don't know what that
  // position is.  Because we will have to loop over everything anyway, create
  // another iterator and loop it until it is at the right place, then take its
  // information.
  const_row_iterator it(in_M, 0);
  while((it.row() < in_row) || ((it.row() == in_row) && (it.col() < in_col)))
    {
    ++it;
    }

  iterator_base::internal_col = it.col();
  iterator_base::internal_pos = it.pos();
  iterator_base::skip_pos = it.skip_pos;
  internal_row = it.internal_row;
  actual_pos = it.actual_pos;
  }



template<typename eT>
inline
SpSubview<eT>::const_row_iterator::const_row_iterator(const const_row_iterator& other)
  : iterator_base(other.M, other.internal_col, other.internal_pos, other.skip_pos)
  , internal_row(other.internal_row)
  , actual_pos(other.actual_pos)
  {
  // Nothing to do.
  }



template<typename eT>
inline
typename SpSubview<eT>::const_row_iterator&
SpSubview<eT>::const_row_iterator::operator++()
  {
  // We just need to find the next nonzero element.
  ++iterator_base::internal_pos;

  // If we have exceeded the bounds, update accordingly.
  if(iterator_base::internal_pos >= iterator_base::M.n_nonzero)
    {
    internal_row = iterator_base::M.n_rows;
    iterator_base::internal_col = 0;
    actual_pos = iterator_base::M.m.n_nonzero;

    return *this;
    }

  // Otherwise, we need to search.
  uword cur_col = iterator_base::internal_col;
  uword cur_row = internal_row;

  const uword aux_col = iterator_base::M.aux_col1;
  const uword aux_row = iterator_base::M.aux_row1;
  const uword ln_cols = iterator_base::M.n_cols;

  while(true)
    {
    // Increment the current column and see if we are on a new row.
    if(++cur_col == ln_cols)
      {
      cur_col = 0;
      ++cur_row;
      }

    // Is there anything in this new column?
    const uword colptr = iterator_base::M.m.col_ptrs[cur_col + aux_col];
    const uword next_colptr = iterator_base::M.m.col_ptrs[cur_col + aux_col + 1];

    for(uword ind = colptr; (ind < next_colptr) && (iterator_base::M.m.row_indices[ind] <= (cur_row + aux_row)); ++ind)
      {
      const uword row_index = iterator_base::M.m.row_indices[ind];

      if((row_index - aux_row) == cur_row)
        {
        // We have successfully incremented.
        internal_row = cur_row;
        actual_pos = ind;
        iterator_base::internal_col = cur_col;

        return *this;
        }
      }
    }
  }



template<typename eT>
inline
typename SpSubview<eT>::const_row_iterator
SpSubview<eT>::const_row_iterator::operator++(int)
  {
  typename SpSubview<eT>::const_row_iterator tmp(*this);

  ++(*this);

  return tmp;
  }



template<typename eT>
inline
typename SpSubview<eT>::const_row_iterator&
SpSubview<eT>::const_row_iterator::operator--()
  {
  // We just need to find the previous element.
//  if(iterator_base::pos == 0)
//    {
//    // We cannot decrement.
//    return *this;
//    }
//  else if(iterator_base::pos == iterator_base::M.n_nonzero)
//    {
//    // We will be coming off the last element.  We need to reset the row correctly, because we set row = 0 in the last matrix position.
//    iterator_base::row = iterator_base::M.n_rows - 1;
//    }
//  else if(iterator_base::pos > iterator_base::M.n_nonzero)
//    {
//    // This shouldn't happen...
//    iterator_base::pos--;
//    return *this;
//    }

  iterator_base::internal_pos--;

  // We have to search backwards.
  uword cur_col = iterator_base::internal_col;
  uword cur_row = internal_row;

  const uword aux_col = iterator_base::M.aux_col1;
  const uword aux_row = iterator_base::M.aux_row1;
  const uword ln_cols = iterator_base::M.n_cols;

  while(true)
    {
    // Decrement the current column and see if we are on a new row.
    if(--cur_col > ln_cols)
      {
      cur_col = ln_cols - 1;
      cur_row--;
      }

    // Is there anything in this new column?
    const uword colptr = iterator_base::M.m.col_ptrs[cur_col + aux_col];
    const uword next_colptr = iterator_base::M.m.col_ptrs[cur_col + aux_col + 1];

    for(uword ind = colptr; (ind < next_colptr) && (iterator_base::M.m.row_indices[ind] <= (cur_row + aux_row)); ++ind)
      {
      const uword row_index = iterator_base::M.m.row_indices[ind];

      if((row_index - aux_row) == cur_row)
        {
        iterator_base::internal_col = cur_col;
        internal_row = cur_row;
        actual_pos = ind;

        return *this;
        }
      }
    }
  }



template<typename eT>
inline
typename SpSubview<eT>::const_row_iterator
SpSubview<eT>::const_row_iterator::operator--(int)
  {
  typename SpSubview<eT>::const_row_iterator tmp(*this);

  --(*this);

  return tmp;
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_row_iterator::operator==(const const_iterator& rhs) const
  {
  return (rhs.row() == row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_row_iterator::operator!=(const const_iterator& rhs) const
  {
  return (rhs.row() != row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_row_iterator::operator==(const typename SpMat<eT>::const_iterator& rhs) const
  {
  return (rhs.row() == row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_row_iterator::operator!=(const typename SpMat<eT>::const_iterator& rhs) const
  {
  return (rhs.row() != row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_row_iterator::operator==(const const_row_iterator& rhs) const
  {
  return (rhs.row() == row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_row_iterator::operator!=(const const_row_iterator& rhs) const
  {
  return (rhs.row() != row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_row_iterator::operator==(const typename SpMat<eT>::const_row_iterator& rhs) const
  {
  return (rhs.row() == row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
bool
SpSubview<eT>::const_row_iterator::operator!=(const typename SpMat<eT>::const_row_iterator& rhs) const
  {
  return (rhs.row() != row()) || (rhs.col() != iterator_base::internal_col);
  }



///////////////////////////////////////////////////////////////////////////////
// SpSubview<eT>::row_iterator implementation                                //
///////////////////////////////////////////////////////////////////////////////

template<typename eT>
inline
SpValProxy<SpSubview<eT> >
SpSubview<eT>::row_iterator::operator*()
  {
  return SpValProxy<SpSubview<eT> >(
    const_row_iterator::internal_row,
    iterator_base::internal_col,
    access::rw(iterator_base::M),
    &access::rw(iterator_base::M.m.values[const_row_iterator::actual_pos]));
  }



template<typename eT>
inline
typename SpSubview<eT>::row_iterator&
SpSubview<eT>::row_iterator::operator++()
  {
  const_row_iterator::operator++();
  return *this;
  }



template<typename eT>
inline
typename SpSubview<eT>::row_iterator
SpSubview<eT>::row_iterator::operator++(int)
  {
  typename SpSubview<eT>::row_iterator tmp(*this);

  ++(*this);

  return tmp;
  }



template<typename eT>
inline
typename SpSubview<eT>::row_iterator&
SpSubview<eT>::row_iterator::operator--()
  {
  const_row_iterator::operator--();
  return *this;
  }



template<typename eT>
inline
typename SpSubview<eT>::row_iterator
SpSubview<eT>::row_iterator::operator--(int)
  {
  typename SpSubview<eT>::row_iterator tmp(*this);

  --(*this);

  return tmp;
  }

//! @}
