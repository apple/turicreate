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


//! \addtogroup op_diagmat
//! @{



template<typename T1>
inline
void
op_diagmat::apply(Mat<typename T1::elem_type>& out, const Op<T1, op_diagmat>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> P(X.m);

  const uword n_rows = P.get_n_rows();
  const uword n_cols = P.get_n_cols();

  const bool P_is_vec = (n_rows == 1) || (n_cols == 1);


  if(P.is_alias(out) == false)
    {
    if(P_is_vec)    // generate a diagonal matrix out of a vector
      {
      const uword N = (n_rows == 1) ? n_cols : n_rows;

      out.zeros(N, N);

      if(Proxy<T1>::use_at == false)
        {
        typename Proxy<T1>::ea_type P_ea = P.get_ea();

        for(uword i=0; i < N; ++i) { out.at(i,i) = P_ea[i]; }
        }
      else
        {
        if(n_rows == 1)
          {
          for(uword i=0; i < N; ++i) { out.at(i,i) = P.at(0,i); }
          }
        else
          {
          for(uword i=0; i < N; ++i) { out.at(i,i) = P.at(i,0); }
          }
        }
      }
    else   // generate a diagonal matrix out of a matrix
      {
      out.zeros(n_rows, n_cols);

      const uword N = (std::min)(n_rows, n_cols);

      for(uword i=0; i < N; ++i) { out.at(i,i) = P.at(i,i); }
      }
    }
  else   // we have aliasing
    {
    if(P_is_vec)   // generate a diagonal matrix out of a vector
      {
      const uword N = (n_rows == 1) ? n_cols : n_rows;

      podarray<eT> tmp(N);
      eT* tmp_mem = tmp.memptr();

      if(Proxy<T1>::use_at == false)
        {
        typename Proxy<T1>::ea_type P_ea = P.get_ea();

        for(uword i=0; i < N; ++i) { tmp_mem[i] = P_ea[i]; }
        }
      else
        {
        if(n_rows == 1)
          {
          for(uword i=0; i < N; ++i) { tmp_mem[i] = P.at(0,i); }
          }
        else
          {
          for(uword i=0; i < N; ++i) { tmp_mem[i] = P.at(i,0); }
          }
        }

      out.zeros(N, N);

      for(uword i=0; i < N; ++i) { out.at(i,i) = tmp_mem[i]; }
      }
    else   // generate a diagonal matrix out of a matrix
      {
      const uword N = (std::min)(n_rows, n_cols);

      if( (Proxy<T1>::has_subview == false) && (Proxy<T1>::fake_mat == false) )
        {
        // NOTE: we have aliasing and it's not due to a subview, hence we're assuming that the output matrix already has the correct size

        for(uword i=0; i < n_cols; ++i)
          {
          if(i < N)
            {
            const eT val = P.at(i,i);

            arrayops::fill_zeros(out.colptr(i), n_rows);

            out.at(i,i) = val;
            }
          else
            {
            arrayops::fill_zeros(out.colptr(i), n_rows);
            }
          }
        }
      else
        {
        podarray<eT> tmp(N);
        eT* tmp_mem = tmp.memptr();

        for(uword i=0; i < N; ++i)  { tmp_mem[i] = P.at(i,i); }

        out.zeros(n_rows, n_cols);

        for(uword i=0; i < N; ++i)  { out.at(i,i) = tmp_mem[i]; }
        }
      }
    }
  }



template<typename T1>
inline
void
op_diagmat2::apply(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword row_offset, const uword col_offset)
  {
  arma_extra_debug_sigprint();

  const uword n_rows = P.get_n_rows();
  const uword n_cols = P.get_n_cols();
  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)  { out.reset(); return; }

  const bool P_is_vec = (T1::is_row) || (T1::is_col) || (n_rows == 1) || (n_cols == 1);

  if(P_is_vec)
    {
    const uword n_pad = (std::max)(row_offset, col_offset);

    out.zeros(n_elem + n_pad, n_elem + n_pad);

    if(Proxy<T1>::use_at == false)
      {
      typename Proxy<T1>::ea_type Pea = P.get_ea();

      for(uword i=0; i < n_elem; ++i)
        {
        out.at(row_offset + i, col_offset + i) = Pea[i];
        }
      }
    else
      {
      const unwrap<typename Proxy<T1>::stored_type> U(P.Q);

      const Proxy<typename unwrap<typename Proxy<T1>::stored_type>::stored_type> PP(U.M);

      op_diagmat2::apply(out, PP, row_offset, col_offset);
      }
    }
  else  // P represents a matrix
    {
    arma_debug_check
      (
      ((row_offset > 0) && (row_offset >= n_rows)) || ((col_offset > 0) && (col_offset >= n_cols)),
      "diagmat(): requested diagonal out of bounds"
      );

    out.zeros(n_rows, n_cols);

    const uword N = (std::min)(n_rows - row_offset, n_cols - col_offset);

    for(uword i=0; i<N; ++i)
      {
      const uword row = i + row_offset;
      const uword col = i + col_offset;

      out.at(row,col) = P.at(row,col);
      }
    }
  }



template<typename T1>
inline
void
op_diagmat2::apply(Mat<typename T1::elem_type>& out, const Op<T1, op_diagmat2>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword row_offset = X.aux_uword_a;
  const uword col_offset = X.aux_uword_b;

  const Proxy<T1> P(X.m);

  if(P.is_alias(out))
    {
    Mat<eT> tmp;

    op_diagmat2::apply(tmp, P, row_offset, col_offset);

    out.steal_mem(tmp);
    }
  else
    {
    op_diagmat2::apply(out, P, row_offset, col_offset);
    }
  }



//! @}
