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


//! \addtogroup SpCol
//! @{



template<typename eT>
inline
SpCol<eT>::SpCol()
  : SpMat<eT>(arma_vec_indicator(), 1)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
SpCol<eT>::SpCol(const uword in_n_elem)
  : SpMat<eT>(arma_vec_indicator(), in_n_elem, 1, 1)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
SpCol<eT>::SpCol(const uword in_n_rows, const uword in_n_cols)
  : SpMat<eT>(arma_vec_indicator(), in_n_rows, in_n_cols, 1)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
SpCol<eT>::SpCol(const SizeMat& s)
  : SpMat<eT>(arma_vec_indicator(), 0, 0, 1)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>::init(s.n_rows, s.n_cols);
  }



template<typename eT>
inline
SpCol<eT>::SpCol(const char* text)
  : SpMat<eT>(arma_vec_indicator(), 1)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>::init(std::string(text));
  }



template<typename eT>
inline
SpCol<eT>&
SpCol<eT>::operator=(const char* text)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>::init(std::string(text));

  return *this;
  }



template<typename eT>
inline
SpCol<eT>::SpCol(const std::string& text)
  : SpMat<eT>(arma_vec_indicator(), 1)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>::init(text);
  }



template<typename eT>
inline
SpCol<eT>&
SpCol<eT>::operator=(const std::string& text)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>::init(text);

  return *this;
  }



template<typename eT>
inline
SpCol<eT>&
SpCol<eT>::operator=(const eT val)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>::operator=(val);

  return *this;
  }



template<typename eT>
template<typename T1>
inline
SpCol<eT>::SpCol(const Base<eT,T1>& X)
  : SpMat<eT>(arma_vec_indicator(), 1)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>::operator=(X.get_ref());
  }



template<typename eT>
template<typename T1>
inline
SpCol<eT>&
SpCol<eT>::operator=(const Base<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>::operator=(X.get_ref());

  return *this;
  }



template<typename eT>
template<typename T1>
inline
SpCol<eT>::SpCol(const SpBase<eT,T1>& X)
  : SpMat<eT>(arma_vec_indicator(), 1)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>::operator=(X.get_ref());
  }



template<typename eT>
template<typename T1>
inline
SpCol<eT>&
SpCol<eT>::operator=(const SpBase<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>::operator=(X.get_ref());

  return *this;
  }



template<typename eT>
template<typename T1, typename T2>
inline
SpCol<eT>::SpCol
  (
  const SpBase<typename SpCol<eT>::pod_type, T1>& A,
  const SpBase<typename SpCol<eT>::pod_type, T2>& B
  )
  : SpMat<eT>(arma_vec_indicator(), 1)
  {
  arma_extra_debug_sigprint();

  SpMat<eT>::init(A,B);
  }



//! remove specified row
template<typename eT>
inline
void
SpCol<eT>::shed_row(const uword row_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( row_num >= SpMat<eT>::n_rows, "SpCol::shed_row(): out of bounds");

  shed_rows(row_num, row_num);
  }



//! remove specified rows
template<typename eT>
inline
void
SpCol<eT>::shed_rows(const uword in_row1, const uword in_row2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_row2 >= SpMat<eT>::n_rows),
    "SpCol::shed_rows(): indices out of bounds or incorrectly used"
    );

  SpMat<eT>::sync_csc();

  const uword diff = (in_row2 - in_row1 + 1);

  // This is easy because everything is in one column.
  uword start = 0, end = 0;
  bool start_found = false, end_found = false;
  for(uword i = 0; i < SpMat<eT>::n_nonzero; ++i)
    {
    // Start position found?
    if (SpMat<eT>::row_indices[i] >= in_row1 && !start_found)
      {
      start = i;
      start_found = true;
      }

    // End position found?
    if (SpMat<eT>::row_indices[i] > in_row2)
      {
      end = i;
      end_found = true;
      break;
      }
    }

  if (!end_found)
    {
    end = SpMat<eT>::n_nonzero;
    }

  // Now we can make the copy.
  if (start != end)
    {
    const uword elem_diff = end - start;

    eT*    new_values      = memory::acquire_chunked<eT>   (SpMat<eT>::n_nonzero - elem_diff);
    uword* new_row_indices = memory::acquire_chunked<uword>(SpMat<eT>::n_nonzero - elem_diff);

    // Copy before the section we are dropping (if it exists).
    if (start > 0)
      {
      arrayops::copy(new_values, SpMat<eT>::values, start);
      arrayops::copy(new_row_indices, SpMat<eT>::row_indices, start);
      }

    // Copy after the section we are dropping (if it exists).
    if (end != SpMat<eT>::n_nonzero)
      {
      arrayops::copy(new_values + start, SpMat<eT>::values + end, (SpMat<eT>::n_nonzero - end));
      arrayops::copy(new_row_indices + start, SpMat<eT>::row_indices + end, (SpMat<eT>::n_nonzero - end));
      arrayops::inplace_minus(new_row_indices + start, diff, (SpMat<eT>::n_nonzero - end));
      }

    memory::release(SpMat<eT>::values);
    memory::release(SpMat<eT>::row_indices);

    access::rw(SpMat<eT>::values) = new_values;
    access::rw(SpMat<eT>::row_indices) = new_row_indices;

    access::rw(SpMat<eT>::n_nonzero) -= elem_diff;
    access::rw(SpMat<eT>::col_ptrs[1]) -= elem_diff;
    }

  access::rw(SpMat<eT>::n_rows) -= diff;
  access::rw(SpMat<eT>::n_elem) -= diff;

  SpMat<eT>::invalidate_cache();
  }



// //! insert N rows at the specified row position,
// //! optionally setting the elements of the inserted rows to zero
// template<typename eT>
// inline
// void
// SpCol<eT>::insert_rows(const uword row_num, const uword N, const bool set_to_zero)
//   {
//   arma_extra_debug_sigprint();
//
//   arma_debug_check(set_to_zero == false, "SpCol::insert_rows(): cannot set nonzero values");
//
//   arma_debug_check((row_num > SpMat<eT>::n_rows), "SpCol::insert_rows(): out of bounds");
//
//   for(uword row = 0; row < SpMat<eT>::n_rows; ++row)
//     {
//     if (SpMat<eT>::row_indices[row] >= row_num)
//       {
//       access::rw(SpMat<eT>::row_indices[row]) += N;
//       }
//     }
//
//   access::rw(SpMat<eT>::n_rows) += N;
//   access::rw(SpMat<eT>::n_elem) += N;
//   }



template<typename eT>
inline
typename SpCol<eT>::row_iterator
SpCol<eT>::begin_row(const uword row_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= SpMat<eT>::n_rows), "SpCol::begin_row(): index out of bounds");

  SpMat<eT>::sync_csc();

  return row_iterator(*this, row_num, 0);
  }



template<typename eT>
inline
typename SpCol<eT>::const_row_iterator
SpCol<eT>::begin_row(const uword row_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= SpMat<eT>::n_rows), "SpCol::begin_row(): index out of bounds");

  SpMat<eT>::sync_csc();

  return const_row_iterator(*this, row_num, 0);
  }



template<typename eT>
inline
typename SpCol<eT>::row_iterator
SpCol<eT>::end_row(const uword row_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= SpMat<eT>::n_rows), "SpCol::end_row(): index out of bounds");

  SpMat<eT>::sync_csc();

  return row_iterator(*this, row_num + 1, 0);
  }



template<typename eT>
inline
typename SpCol<eT>::const_row_iterator
SpCol<eT>::end_row(const uword row_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= SpMat<eT>::n_rows), "SpCol::end_row(): index out of bounds");

  SpMat<eT>::sync_csc();

  return const_row_iterator(*this, row_num + 1, 0);
  }



#ifdef ARMA_EXTRA_SPCOL_MEAT
  #include ARMA_INCFILE_WRAP(ARMA_EXTRA_SPCOL_MEAT)
#endif



//! @}
