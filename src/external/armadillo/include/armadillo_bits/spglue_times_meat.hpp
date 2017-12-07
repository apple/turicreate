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


//! \addtogroup spglue_times
//! @{



template<typename T1, typename T2>
inline
void
spglue_times::apply(SpMat<typename T1::elem_type>& out, const SpGlue<T1,T2,spglue_times>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  // unconditionally unwrapping, as the column iterator in SpSubview is slow

  const unwrap_spmat<T1> tmp1(X.A);
  const unwrap_spmat<T2> tmp2(X.B);

  const SpProxy<typename unwrap_spmat<T1>::stored_type> pa(tmp1.M);
  const SpProxy<typename unwrap_spmat<T2>::stored_type> pb(tmp2.M);

  const bool is_alias = pa.is_alias(out) || pb.is_alias(out);

  if(is_alias == false)
    {
    spglue_times::apply_noalias(out, pa, pb);
    }
  else
    {
    SpMat<eT> tmp;
    spglue_times::apply_noalias(tmp, pa, pb);

    out.steal_mem(tmp);
    }
  }



template<typename eT, typename T1, typename T2>
arma_hot
inline
void
spglue_times::apply_noalias(SpMat<eT>& c, const SpProxy<T1>& pa, const SpProxy<T2>& pb)
  {
  arma_extra_debug_sigprint();

  const uword x_n_rows = pa.get_n_rows();
  const uword x_n_cols = pa.get_n_cols();
  const uword y_n_rows = pb.get_n_rows();
  const uword y_n_cols = pb.get_n_cols();

  arma_debug_assert_mul_size(x_n_rows, x_n_cols, y_n_rows, y_n_cols, "matrix multiplication");

  // First we must determine the structure of the new matrix (column pointers).
  // This follows the algorithm described in 'Sparse Matrix Multiplication
  // Package (SMMP)' (R.E. Bank and C.C. Douglas, 2001).  Their description of
  // "SYMBMM" does not include anything about memory allocation.  In addition it
  // does not consider that there may be elements which space may be allocated
  // for but which evaluate to zero anyway.  So we have to modify the algorithm
  // to work that way.  For the "SYMBMM" implementation we will not determine
  // the row indices but instead just the column pointers.

  //SpMat<typename T1::elem_type> c(x_n_rows, y_n_cols); // Initializes col_ptrs to 0.
  c.zeros(x_n_rows, y_n_cols);

  //if( (pa.get_n_elem() == 0) || (pb.get_n_elem() == 0) )
  if( (pa.get_n_nonzero() == 0) || (pb.get_n_nonzero() == 0) )
    {
    return;
    }

  // Auxiliary storage which denotes when items have been found.
  podarray<uword> index(x_n_rows);
  index.fill(x_n_rows); // Fill with invalid links.

  typename SpProxy<T2>::const_iterator_type y_it  = pb.begin();
  typename SpProxy<T2>::const_iterator_type y_end = pb.end();

  // SYMBMM: calculate column pointers for resultant matrix to obtain a good
  // upper bound on the number of nonzero elements.
  uword cur_col_length = 0;
  uword last_ind = x_n_rows + 1;
  do
    {
    const uword y_it_row = y_it.row();

    // Look through the column that this point (*y_it) could affect.
    typename SpProxy<T1>::const_iterator_type x_it = pa.begin_col(y_it_row);

    while(x_it.col() == y_it_row)
      {
      // A point at x(i, j) and y(j, k) implies a point at c(i, k).
      if(index[x_it.row()] == x_n_rows)
        {
        index[x_it.row()] = last_ind;
        last_ind = x_it.row();
        ++cur_col_length;
        }

      ++x_it;
      }

    const uword old_col = y_it.col();
    ++y_it;

    // See if column incremented.
    if(old_col != y_it.col())
      {
      // Set column pointer (this is not a cumulative count; that is done later).
      access::rw(c.col_ptrs[old_col + 1]) = cur_col_length;
      cur_col_length = 0;

      // Return index markers to zero.  Use last_ind for traversal.
      while(last_ind != x_n_rows + 1)
        {
        const uword tmp = index[last_ind];
        index[last_ind] = x_n_rows;
        last_ind = tmp;
        }
      }
    }
  while(y_it != y_end);

  // Accumulate column pointers.
  for(uword i = 0; i < c.n_cols; ++i)
    {
    access::rw(c.col_ptrs[i + 1]) += c.col_ptrs[i];
    }

  // Now that we know a decent bound on the number of nonzero elements, allocate
  // the memory and fill it.
  c.mem_resize(c.col_ptrs[c.n_cols]);

  // Now the implementation of the NUMBMM algorithm.
  uword cur_pos = 0; // Current position in c matrix.
  podarray<eT> sums(x_n_rows); // Partial sums.
  sums.zeros();

  // setting the size of 'sorted_indices' to x_n_rows is a better-than-nothing guess;
  // the correct minimum size is determined later
  podarray<uword> sorted_indices(x_n_rows);

  // last_ind is already set to x_n_rows, and cur_col_length is already set to 0.
  // We will loop through all columns as necessary.
  uword cur_col = 0;
  while(cur_col < c.n_cols)
    {
    // Skip to next column with elements in it.
    while((cur_col < c.n_cols) && (c.col_ptrs[cur_col] == c.col_ptrs[cur_col + 1]))
      {
      // Update current column pointer to actual number of nonzero elements up
      // to this point.
      access::rw(c.col_ptrs[cur_col]) = cur_pos;
      ++cur_col;
      }

    if(cur_col == c.n_cols)
      {
      break;
      }

    // Update current column pointer.
    access::rw(c.col_ptrs[cur_col]) = cur_pos;

    // Check all elements in this column.
    typename SpProxy<T2>::const_iterator_type y_col_it = pb.begin_col(cur_col);

    while(y_col_it.col() == cur_col)
      {
      // Check all elements in the column of the other matrix corresponding to
      // the row of this column.
      typename SpProxy<T1>::const_iterator_type x_col_it = pa.begin_col(y_col_it.row());

      const eT y_value = (*y_col_it);

      while(x_col_it.col() == y_col_it.row())
        {
        // A point at x(i, j) and y(j, k) implies a point at c(i, k).
        // Add to partial sum.
        const eT x_value = (*x_col_it);
        sums[x_col_it.row()] += (x_value * y_value);

        // Add point if it hasn't already been marked.
        if(index[x_col_it.row()] == x_n_rows)
          {
          index[x_col_it.row()] = last_ind;
          last_ind = x_col_it.row();
          }

        ++x_col_it;
        }

      ++y_col_it;
      }

    // Now sort the indices that were used in this column.
    //podarray<uword> sorted_indices(c.col_ptrs[cur_col + 1] - c.col_ptrs[cur_col]);
    sorted_indices.set_min_size(c.col_ptrs[cur_col + 1] - c.col_ptrs[cur_col]);

    // .set_min_size() can only enlarge the array to the specified size,
    // hence if we request a smaller size than already allocated,
    // no new memory allocation is done


    uword cur_index = 0;
    while(last_ind != x_n_rows + 1)
      {
      const uword tmp = last_ind;

      // Check that it wasn't a "fake" nonzero element.
      if(sums[tmp] != eT(0))
        {
        // Assign to next open position.
        sorted_indices[cur_index] = tmp;
        ++cur_index;
        }

      last_ind = index[tmp];
      index[tmp] = x_n_rows;
      }

    // Now sort the indices.
    if (cur_index != 0)
      {
      op_sort::direct_sort_ascending(sorted_indices.memptr(), cur_index);

      for(uword k = 0; k < cur_index; ++k)
        {
        const uword row = sorted_indices[k];
        access::rw(c.row_indices[cur_pos]) = row;
        access::rw(c.values[cur_pos]) = sums[row];
        sums[row] = eT(0);
        ++cur_pos;
        }
      }

    // Move to next column.
    ++cur_col;
    }

  // Update last column pointer and resize to actual memory size.
  access::rw(c.col_ptrs[c.n_cols]) = cur_pos;
  c.mem_resize(cur_pos);
  }



//
//
// spglue_times2: scalar*(A * B)



template<typename T1, typename T2>
inline
void
spglue_times2::apply(SpMat<typename T1::elem_type>& out, const SpGlue<T1,T2,spglue_times2>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const SpProxy<T1> pa(X.A);
  const SpProxy<T2> pb(X.B);

  const bool is_alias = pa.is_alias(out) || pb.is_alias(out);

  if(is_alias == false)
    {
    spglue_times::apply_noalias(out, pa, pb);
    }
  else
    {
    SpMat<eT> tmp;
    spglue_times::apply_noalias(tmp, pa, pb);

    out.steal_mem(tmp);
    }

  out *= X.aux;
  }



//! @}
