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


//! \addtogroup spop_trimat
//! @{



template<typename T1>
inline
void
spop_trimat::apply_noalias(SpMat<typename T1::elem_type>& out, const SpProxy<T1>& P, const bool upper)
  {
  arma_extra_debug_sigprint();

  typename SpProxy<T1>::const_iterator_type it = P.begin();

  const uword old_n_nonzero = P.get_n_nonzero();
        uword new_n_nonzero = 0;

  if(upper)
    {
    // upper triangular: count elements on the diagonal and above the diagonal

    for(uword i=0; i < old_n_nonzero; ++i)
      {
      new_n_nonzero += (it.row() <= it.col()) ? uword(1) : uword(0);
      ++it;
      }
    }
  else
    {
    // lower triangular: count elements on the diagonal and below the diagonal

    for(uword i=0; i < old_n_nonzero; ++i)
      {
      new_n_nonzero += (it.row() >= it.col()) ? uword(1) : uword(0);
      ++it;
      }
    }

  const uword n_rows = P.get_n_rows();
  const uword n_cols = P.get_n_cols();

  out.set_size(n_rows, n_cols);

  out.mem_resize(new_n_nonzero);

  uword new_index = 0;

  it = P.begin();

  if(upper)
    {
    // upper triangular: copy elements on the diagonal and above the diagonal

    for(uword i=0; i < old_n_nonzero; ++i)
      {
      const uword row = it.row();
      const uword col = it.col();

      if(row <= col)
        {
        access::rw(out.values[new_index])      = (*it);
        access::rw(out.row_indices[new_index]) = row;
        access::rw(out.col_ptrs[col + 1])++;
        ++new_index;
        }

      ++it;
      }
    }
  else
    {
    // lower triangular: copy elements on the diagonal and below the diagonal

    for(uword i=0; i < old_n_nonzero; ++i)
      {
      const uword row = it.row();
      const uword col = it.col();

      if(row >= col)
        {
        access::rw(out.values[new_index])      = (*it);
        access::rw(out.row_indices[new_index]) = row;
        access::rw(out.col_ptrs[col + 1])++;
        ++new_index;
        }

      ++it;
      }
    }

  for(uword i=0; i < n_cols; ++i)
    {
    access::rw(out.col_ptrs[i + 1]) += out.col_ptrs[i];
    }
  }



template<typename T1>
inline
void
spop_trimat::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_trimat>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const SpProxy<T1> P(in.m);

  arma_debug_check( (P.get_n_rows() != P.get_n_cols()), "trimatu()/trimatl(): given matrix must be square sized" );

  const bool upper = (in.aux_uword_a == 0);

  if(P.is_alias(out))
    {
    SpMat<eT> tmp;
    spop_trimat::apply_noalias(tmp, P, upper);
    out.steal_mem(tmp);
    }
  else
    {
    spop_trimat::apply_noalias(out, P, upper);
    }
  }



//! @}
