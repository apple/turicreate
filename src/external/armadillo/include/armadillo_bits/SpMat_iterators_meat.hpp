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


//! \addtogroup SpMat
//! @{


///////////////////////////////////////////////////////////////////////////////
// SpMat::iterator_base implementation                                       //
///////////////////////////////////////////////////////////////////////////////


template<typename eT>
inline
SpMat<eT>::iterator_base::iterator_base()
  : M(NULL)
  , internal_col(0)
  , internal_pos(0)
  {
  // Technically this iterator is invalid (it may not point to a real element)
  }



template<typename eT>
inline
SpMat<eT>::iterator_base::iterator_base(const SpMat<eT>& in_M)
  : M(&in_M)
  , internal_col(0)
  , internal_pos(0)
  {
  // Technically this iterator is invalid (it may not point to a real element)
  }



template<typename eT>
inline
SpMat<eT>::iterator_base::iterator_base(const SpMat<eT>& in_M, const uword in_col, const uword in_pos)
  : M(&in_M)
  , internal_col(in_col)
  , internal_pos(in_pos)
  {
  // Nothing to do.
  }



template<typename eT>
arma_inline
eT
SpMat<eT>::iterator_base::operator*() const
  {
  return M->values[internal_pos];
  }



///////////////////////////////////////////////////////////////////////////////
// SpMat::const_iterator implementation                                      //
///////////////////////////////////////////////////////////////////////////////

template<typename eT>
inline
SpMat<eT>::const_iterator::const_iterator()
  : iterator_base()
  {
  }

template<typename eT>
inline
SpMat<eT>::const_iterator::const_iterator(const SpMat<eT>& in_M, uword initial_pos)
  : iterator_base(in_M, 0, initial_pos)
  {
  // Corner case for empty matrices.
  if(in_M.n_nonzero == 0)
    {
    iterator_base::internal_col = in_M.n_cols;
    return;
    }

  // Determine which column we should be in.
  while(iterator_base::M->col_ptrs[iterator_base::internal_col + 1] <= iterator_base::internal_pos)
    {
    iterator_base::internal_col++;
    }
  }



template<typename eT>
inline
SpMat<eT>::const_iterator::const_iterator(const SpMat<eT>& in_M, uword in_row, uword in_col)
  : iterator_base(in_M, in_col, 0)
  {
  // So we have a position we want to be right after.  Skip to the column.
  iterator_base::internal_pos = iterator_base::M->col_ptrs[iterator_base::internal_col];

  // Now we have to make sure that is the right column.
  while(iterator_base::M->col_ptrs[iterator_base::internal_col + 1] <= iterator_base::internal_pos)
    {
    iterator_base::internal_col++;
    }

  // Now we have to get to the right row.
  while((iterator_base::M->row_indices[iterator_base::internal_pos] < in_row) && (iterator_base::internal_col == in_col))
    {
    ++(*this); // Increment iterator.
    }
  }



template<typename eT>
inline
SpMat<eT>::const_iterator::const_iterator(const SpMat<eT>& in_M, const uword /* in_row */, const uword in_col, const uword in_pos)
  : iterator_base(in_M, in_col, in_pos)
  {
  // Nothing to do.
  }



template<typename eT>
inline
SpMat<eT>::const_iterator::const_iterator(const typename SpMat<eT>::const_iterator& other)
  : iterator_base(*other.M, other.internal_col, other.internal_pos)
  {
  // Nothing to do.
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::const_iterator&
SpMat<eT>::const_iterator::operator++()
  {
  ++iterator_base::internal_pos;

  if (iterator_base::internal_pos == iterator_base::M->n_nonzero)
    {
    iterator_base::internal_col = iterator_base::M->n_cols;
    return *this;
    }

  // Check to see if we moved a column.
  while (iterator_base::M->col_ptrs[iterator_base::internal_col + 1] <= iterator_base::internal_pos)
    {
    ++iterator_base::internal_col;
    }

  return *this;
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::const_iterator
SpMat<eT>::const_iterator::operator++(int)
  {
  typename SpMat<eT>::const_iterator tmp(*this);

  ++(*this);

  return tmp;
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::const_iterator&
SpMat<eT>::const_iterator::operator--()
  {
  //iterator_base::M.print("M");

  // printf("decrement from %d, %d, %d\n", iterator_base::internal_pos, iterator_base::internal_col, iterator_base::row());

  --iterator_base::internal_pos;

  // printf("now pos %d\n", iterator_base::internal_pos);

  // First, see if we moved back a column.
  while (iterator_base::internal_pos < iterator_base::M->col_ptrs[iterator_base::internal_col])
    {
    // printf("colptr %d (col %d)\n", iterator_base::M.col_ptrs[iterator_base::internal_col], iterator_base::internal_col);

    --iterator_base::internal_col;
    }


  return *this;
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::const_iterator
SpMat<eT>::const_iterator::operator--(int)
  {
  typename SpMat<eT>::const_iterator tmp(*this);

  --(*this);

  return tmp;
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_iterator::operator==(const const_iterator& rhs) const
  {
  return (rhs.row() == (*this).row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_iterator::operator!=(const const_iterator& rhs) const
  {
  return (rhs.row() != (*this).row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_iterator::operator==(const typename SpSubview<eT>::const_iterator& rhs) const
  {
  return (rhs.row() == (*this).row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_iterator::operator!=(const typename SpSubview<eT>::const_iterator& rhs) const
  {
  return (rhs.row() != (*this).row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_iterator::operator==(const const_row_iterator& rhs) const
  {
  return (rhs.row() == (*this).row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_iterator::operator!=(const const_row_iterator& rhs) const
  {
  return (rhs.row() != (*this).row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_iterator::operator==(const typename SpSubview<eT>::const_row_iterator& rhs) const
  {
  return (rhs.row() == (*this).row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_iterator::operator!=(const typename SpSubview<eT>::const_row_iterator& rhs) const
  {
  return (rhs.row() != (*this).row()) || (rhs.col() != iterator_base::internal_col);
  }



///////////////////////////////////////////////////////////////////////////////
// SpMat::iterator implementation                                            //
///////////////////////////////////////////////////////////////////////////////

template<typename eT>
inline
arma_hot
SpValProxy<SpMat<eT> >
SpMat<eT>::iterator::operator*()
  {
  return SpValProxy<SpMat<eT> >(
    iterator_base::M->row_indices[iterator_base::internal_pos],
    iterator_base::internal_col,
    access::rw(*iterator_base::M),
    &access::rw(iterator_base::M->values[iterator_base::internal_pos]));
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::iterator&
SpMat<eT>::iterator::operator++()
  {
  const_iterator::operator++();
  return *this;
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::iterator
SpMat<eT>::iterator::operator++(int)
  {
  typename SpMat<eT>::iterator tmp(*this);

  const_iterator::operator++();

  return tmp;
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::iterator&
SpMat<eT>::iterator::operator--()
  {
  const_iterator::operator--();
  return *this;
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::iterator
SpMat<eT>::iterator::operator--(int)
  {
  typename SpMat<eT>::iterator tmp(*this);

  const_iterator::operator--();

  return tmp;
  }



///////////////////////////////////////////////////////////////////////////////
// SpMat::const_row_iterator implementation                                  //
///////////////////////////////////////////////////////////////////////////////

/**
 * Initialize the const_row_iterator.
 */

template<typename eT>
inline
SpMat<eT>::const_row_iterator::const_row_iterator()
  : iterator_base()
  , internal_row(0)
  , actual_pos(0)
  {
  }



template<typename eT>
inline
SpMat<eT>::const_row_iterator::const_row_iterator(const SpMat<eT>& in_M, uword initial_pos)
  : iterator_base(in_M, 0, initial_pos)
  , internal_row(0)
  , actual_pos(0)
  {
  // Corner case for empty matrix.
  if(in_M.n_nonzero == 0)
    {
    iterator_base::internal_col = 0;
    internal_row = in_M.n_rows;
    return;
    }

  // We don't count zeros in our position count, so we have to find the nonzero
  // value corresponding to the given initial position.  We assume initial_pos
  // is valid.

  // This is irritating because we don't know where the elements are in each
  // row.  What we will do is loop across all columns looking for elements in
  // row 0 (and add to our sum), then in row 1, and so forth, until we get to
  // the desired position.
  uword cur_pos = std::numeric_limits<uword>::max(); // Invalid value.
  uword cur_row = 0;
  uword cur_col = 0;

  while(true) // This loop is terminated from the inside.
    {
    // Is there anything in the column we are looking at?
    for (uword ind = 0; ((iterator_base::M->col_ptrs[cur_col] + ind < iterator_base::M->col_ptrs[cur_col + 1]) && (iterator_base::M->row_indices[iterator_base::M->col_ptrs[cur_col] + ind] <= cur_row)); ind++)
      {
      // There is something in this column.  Is it in the row we are looking at?
      const uword row_index = iterator_base::M->row_indices[iterator_base::M->col_ptrs[cur_col] + ind];
      if (row_index == cur_row)
        {
        // Yes, it is what we are looking for.  Increment our current position.
        if (++cur_pos == iterator_base::internal_pos)   // TODO: HACK: if cur_pos is std::numeric_limits<uword>::max(), ++cur_pos relies on a wraparound/overflow, which is not portable
          {
          actual_pos = iterator_base::M->col_ptrs[cur_col] + ind;
          internal_row = cur_row;
          iterator_base::internal_col = cur_col;

          return;
          }

        // We are done with this column.  Break to the column incrementing code (directly below).
        break;
        }
      else if(row_index > cur_row)
        {
        break; // Can't be in this column.
        }
      }

    cur_col++; // Done with the column.  Move on.
    if (cur_col == iterator_base::M->n_cols)
      {
      // We are out of columns.  Loop back to the beginning and look on the
      // next row.
      cur_col = 0;
      cur_row++;
      }
    }
  }



template<typename eT>
inline
SpMat<eT>::const_row_iterator::const_row_iterator(const SpMat<eT>& in_M, uword in_row, uword in_col)
  : iterator_base(in_M, in_col, 0)
  , internal_row(0)
  , actual_pos(0)
  {
  // This is slow.  It needs to be rewritten.
  // So we have a destination we want to be just after, but don't know what position that is.  Make another iterator to find out...
  const_row_iterator it(in_M, 0);
  while((it.row() < in_row) || ((it.row() == in_row) && (it.col() < in_col)))
    {
    it++;
    }

  // Now that it is at the right place, take its position.
  iterator_base::internal_col = it.internal_col;
  iterator_base::internal_pos = it.internal_pos;
  internal_row = it.internal_row;
  actual_pos = it.actual_pos;
  }



/**
 * Initialize the const_row_iterator from another const_row_iterator.
 */
template<typename eT>
inline
SpMat<eT>::const_row_iterator::const_row_iterator(const typename SpMat<eT>::const_row_iterator& other)
  : iterator_base(*other.M, other.internal_col, other.internal_pos)
  , internal_row(other.internal_row)
  , actual_pos(other.actual_pos)
  {
  // Nothing to do.
  }



/**
 * Increment the row_iterator.
 */
template<typename eT>
inline
arma_hot
typename SpMat<eT>::const_row_iterator&
SpMat<eT>::const_row_iterator::operator++()
  {
  // We just need to find the next nonzero element.
  iterator_base::internal_pos++;

  if(iterator_base::internal_pos == iterator_base::M->n_nonzero)
    {
    internal_row = iterator_base::M->n_rows;
    iterator_base::internal_col = 0;
    actual_pos = iterator_base::M->n_nonzero;

    return *this;
    }

  // Otherwise, we need to search.
  uword cur_col = iterator_base::internal_col;
  uword cur_row = internal_row;

  while (true) // This loop is terminated from the inside.
    {
    // Increment the current column and see if we are now on a new row.
    if (++cur_col == iterator_base::M->n_cols)
      {
      cur_col = 0;
      cur_row++;
      }

    // Is there anything in this new column?
    for (uword ind = 0; ((iterator_base::M->col_ptrs[cur_col] + ind < iterator_base::M->col_ptrs[cur_col + 1]) && (iterator_base::M->row_indices[iterator_base::M->col_ptrs[cur_col] + ind] <= cur_row)); ind++)
      {
      if (iterator_base::M->row_indices[iterator_base::M->col_ptrs[cur_col] + ind] == cur_row)
        {
        // We have successfully incremented.
        internal_row = cur_row;
        iterator_base::internal_col = cur_col;
        actual_pos = iterator_base::M->col_ptrs[cur_col] + ind;

        return *this; // Now we are done.
        }
      }
    }
  }



/**
 * Increment the row_iterator (but do not return anything.
 */
template<typename eT>
inline
arma_hot
typename SpMat<eT>::const_row_iterator
SpMat<eT>::const_row_iterator::operator++(int)
  {
  typename SpMat<eT>::const_row_iterator tmp(*this);

  ++(*this);

  return tmp;
  }



/**
 * Decrement the row_iterator.
 */
template<typename eT>
inline
arma_hot
typename SpMat<eT>::const_row_iterator&
SpMat<eT>::const_row_iterator::operator--()
  {
  iterator_base::internal_pos--;

  // We have to search backwards.
  uword cur_col = iterator_base::internal_col;
  uword cur_row = internal_row;

  while (true) // This loop is terminated from the inside.
    {
    // Decrement the current column and see if we are now on a new row.  This is a uword so a negativity check won't work.
    if (--cur_col > iterator_base::M->n_cols /* this means it underflew */)
      {
      cur_col = iterator_base::M->n_cols - 1;
      cur_row--;
      }

    // Is there anything in this new column?
    for (uword ind = 0; ((iterator_base::M->col_ptrs[cur_col] + ind < iterator_base::M->col_ptrs[cur_col + 1]) && (iterator_base::M->row_indices[iterator_base::M->col_ptrs[cur_col] + ind] <= cur_row)); ind++)
      {
      if (iterator_base::M->row_indices[iterator_base::M->col_ptrs[cur_col] + ind] == cur_row)
        {
        // We have successfully decremented.
        iterator_base::internal_col = cur_col;
        internal_row = cur_row;
        actual_pos = iterator_base::M->col_ptrs[cur_col] + ind;

        return *this; // Now we are done.
        }
      }
    }
  }



/**
 * Decrement the row_iterator.
 */
template<typename eT>
inline
arma_hot
typename SpMat<eT>::const_row_iterator
SpMat<eT>::const_row_iterator::operator--(int)
  {
  typename SpMat<eT>::const_row_iterator tmp(*this);

  --(*this);

  return tmp;
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_row_iterator::operator==(const const_iterator& rhs) const
  {
  return (rhs.row() == row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_row_iterator::operator!=(const const_iterator& rhs) const
  {
  return (rhs.row() != row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_row_iterator::operator==(const typename SpSubview<eT>::const_iterator& rhs) const
  {
  return (rhs.row() == row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_row_iterator::operator!=(const typename SpSubview<eT>::const_iterator& rhs) const
  {
  return (rhs.row() != row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_row_iterator::operator==(const const_row_iterator& rhs) const
  {
  return (rhs.row() == row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_row_iterator::operator!=(const const_row_iterator& rhs) const
  {
  return (rhs.row() != row()) || (rhs.col() != iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_row_iterator::operator==(const typename SpSubview<eT>::const_row_iterator& rhs) const
  {
  return (rhs.row() == row()) && (rhs.col() == iterator_base::internal_col);
  }



template<typename eT>
inline
arma_hot
bool
SpMat<eT>::const_row_iterator::operator!=(const typename SpSubview<eT>::const_row_iterator& rhs) const
  {
  return (rhs.row() != row()) || (rhs.col() != iterator_base::internal_col);
  }



///////////////////////////////////////////////////////////////////////////////
// SpMat::row_iterator implementation                                        //
///////////////////////////////////////////////////////////////////////////////

template<typename eT>
inline
arma_hot
SpValProxy<SpMat<eT> >
SpMat<eT>::row_iterator::operator*()
  {
  return SpValProxy<SpMat<eT> >(
    const_row_iterator::internal_row,
    iterator_base::internal_col,
    access::rw(*iterator_base::M),
    &access::rw(iterator_base::M->values[const_row_iterator::actual_pos]));
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::row_iterator&
SpMat<eT>::row_iterator::operator++()
  {
  const_row_iterator::operator++();
  return *this;
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::row_iterator
SpMat<eT>::row_iterator::operator++(int)
  {
  typename SpMat<eT>::row_iterator tmp(*this);

  const_row_iterator::operator++();

  return tmp;
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::row_iterator&
SpMat<eT>::row_iterator::operator--()
  {
  const_row_iterator::operator--();
  return *this;
  }



template<typename eT>
inline
arma_hot
typename SpMat<eT>::row_iterator
SpMat<eT>::row_iterator::operator--(int)
  {
  typename SpMat<eT>::row_iterator tmp(*this);

  const_row_iterator::operator--();

  return tmp;
  }

//! @}
