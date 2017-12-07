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



//! \addtogroup op_reshape
//! @{



template<typename eT>
inline
void
op_reshape::apply_unwrap(Mat<eT>& out, const Mat<eT>& A, const uword in_n_rows, const uword in_n_cols, const uword in_dim)
  {
  arma_extra_debug_sigprint();

  const bool is_alias = (&out == &A);

  const uword in_n_elem = in_n_rows * in_n_cols;

  if(A.n_elem == in_n_elem)
    {
    if(in_dim == 0)
      {
      if(is_alias == false)
        {
        out.set_size(in_n_rows, in_n_cols);
        arrayops::copy( out.memptr(), A.memptr(), out.n_elem );
        }
      else  // &out == &A, i.e. inplace resize
        {
        out.set_size(in_n_rows, in_n_cols);
        // set_size() doesn't destroy data as long as the number of elements in the matrix remains the same
        }
      }
    else
      {
      unwrap_check< Mat<eT> > B_tmp(A, is_alias);
      const Mat<eT>& B      = B_tmp.M;

      out.set_size(in_n_rows, in_n_cols);

      eT* out_mem = out.memptr();

      const uword B_n_rows = B.n_rows;
      const uword B_n_cols = B.n_cols;

      for(uword row=0; row<B_n_rows; ++row)
        {
        uword i,j;
        for(i=0, j=1; j < B_n_cols; i+=2, j+=2)
          {
          const eT tmp_i = B.at(row,i);
          const eT tmp_j = B.at(row,j);

          *out_mem = tmp_i;  out_mem++;
          *out_mem = tmp_j;  out_mem++;
          }

        if(i < B_n_cols)
          {
          *out_mem = B.at(row,i);  out_mem++;
          }
        }
      }
    }
  else
    {
    const unwrap_check< Mat<eT> > B_tmp(A, is_alias);
    const Mat<eT>& B            = B_tmp.M;

    const uword n_elem_to_copy = (std::min)(B.n_elem, in_n_elem);

    out.set_size(in_n_rows, in_n_cols);

    eT* out_mem = out.memptr();

    if(in_dim == 0)
      {
      arrayops::copy( out_mem, B.memptr(), n_elem_to_copy );
      }
    else
      {
      uword row = 0;
      uword col = 0;

      const uword B_n_cols = B.n_cols;

      for(uword i=0; i<n_elem_to_copy; ++i)
        {
        out_mem[i] = B.at(row,col);

        ++col;

        if(col >= B_n_cols)
          {
          col = 0;
          ++row;
          }
        }
      }

    for(uword i=n_elem_to_copy; i<in_n_elem; ++i)
      {
      out_mem[i] = eT(0);
      }

    }
  }



template<typename T1>
inline
void
op_reshape::apply_proxy(Mat<typename T1::elem_type>& out, const Proxy<T1>& P, const uword in_n_rows, const uword in_n_cols)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  out.set_size(in_n_rows, in_n_cols);

  eT* out_mem = out.memptr();

  const uword in_n_elem = in_n_rows * in_n_cols;

  if(P.get_n_elem() == in_n_elem)
    {
    if(Proxy<T1>::use_at == false)
      {
      typename Proxy<T1>::ea_type Pea = P.get_ea();

      for(uword i=0; i<in_n_elem; ++i)
        {
        out_mem[i] = Pea[i];
        }
      }
    else
      {
      const uword P_n_rows = P.get_n_rows();
      const uword P_n_cols = P.get_n_cols();

      for(uword col=0; col < P_n_cols; ++col)
        {
        uword i,j;

        for(i=0, j=1; j < P_n_rows; i+=2, j+=2)
          {
          const eT tmp_i = P.at(i,col);
          const eT tmp_j = P.at(j,col);

          *out_mem = tmp_i;  out_mem++;
          *out_mem = tmp_j;  out_mem++;
          }

        if(i < P_n_rows)
          {
          *out_mem = P.at(i,col);  out_mem++;
          }
        }
      }
    }
  else
    {
    const uword n_elem_to_copy = (std::min)(P.get_n_elem(), in_n_elem);

    if(Proxy<T1>::use_at == false)
      {
      typename Proxy<T1>::ea_type Pea = P.get_ea();

      for(uword i=0; i<n_elem_to_copy; ++i)
        {
        out_mem[i] = Pea[i];
        }
      }
    else
      {
      uword i = 0;

      const uword P_n_rows = P.get_n_rows();
      const uword P_n_cols = P.get_n_cols();

      for(uword col=0; col < P_n_cols; ++col)
      for(uword row=0; row < P_n_rows; ++row)
        {
        if(i >= n_elem_to_copy)  { goto nested_loop_end; }

        out_mem[i] = P.at(row,col);

        ++i;
        }

      nested_loop_end: ;
      }

    for(uword i=n_elem_to_copy; i<in_n_elem; ++i)
      {
      out_mem[i] = eT(0);
      }
    }
  }



template<typename T1>
inline
void
op_reshape::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_reshape>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> P(in.m);

  const uword in_n_rows = in.aux_uword_a;
  const uword in_n_cols = in.aux_uword_b;

  if( (is_Mat<typename Proxy<T1>::stored_type>::value == true) && (Proxy<T1>::fake_mat == false) )
    {
    // not checking for aliasing here, as this might be an inplace reshape

    const unwrap<typename Proxy<T1>::stored_type> tmp(P.Q);

    op_reshape::apply_unwrap(out, tmp.M, in_n_rows, in_n_cols, uword(0));
    }
  else
    {
    if(P.is_alias(out))
      {
      Mat<eT> tmp;

      op_reshape::apply_proxy(tmp, P, in_n_rows, in_n_cols);

      out.steal_mem(tmp);
      }
    else
      {
      op_reshape::apply_proxy(out, P, in_n_rows, in_n_cols);
      }
    }
  }



template<typename T1>
inline
void
op_reshape_ext::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_reshape_ext>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> tmp(in.m);

  const uword in_n_rows = in.aux_uword_a;
  const uword in_n_cols = in.aux_uword_b;
  const uword in_dim    = in.aux_uword_c;

  op_reshape::apply_unwrap(out, tmp.M, in_n_rows, in_n_cols, in_dim);
  }



template<typename T1>
inline
void
op_reshape_ext::apply(Cube<typename T1::elem_type>& out, const OpCube<T1,op_reshape_ext>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap_cube<T1> A_tmp(in.m);
  const Cube<eT>& A   = A_tmp.M;

  const uword in_n_rows   = in.aux_uword_a;
  const uword in_n_cols   = in.aux_uword_b;
  const uword in_n_slices = in.aux_uword_c;
  const uword in_dim      = in.aux_uword_d;

  const uword in_n_elem = in_n_rows * in_n_cols * in_n_slices;

  if(A.n_elem == in_n_elem)
    {
    if(in_dim == 0)
      {
      if(&out != &A)
        {
        out.set_size(in_n_rows, in_n_cols, in_n_slices);
        arrayops::copy( out.memptr(), A.memptr(), out.n_elem );
        }
      else  // &out == &A, i.e. inplace resize
        {
        out.set_size(in_n_rows, in_n_cols, in_n_slices);
        // set_size() doesn't destroy data as long as the number of elements in the cube remains the same
        }
      }
    else
      {
      unwrap_cube_check< Cube<eT> > B_tmp(A, out);
      const Cube<eT>& B           = B_tmp.M;

      out.set_size(in_n_rows, in_n_cols, in_n_slices);

      eT* out_mem = out.memptr();

      const uword B_n_rows   = B.n_rows;
      const uword B_n_cols   = B.n_cols;
      const uword B_n_slices = B.n_slices;

      for(uword slice = 0; slice < B_n_slices; ++slice)
      for(uword row   = 0; row   < B_n_rows;   ++row  )
      for(uword col   = 0; col   < B_n_cols;   ++col  )
        {
        *out_mem = B.at(row,col,slice);
        out_mem++;
        }
      }
    }
  else
    {
    const unwrap_cube_check< Cube<eT> > B_tmp(A, out);
    const Cube<eT>& B                 = B_tmp.M;

    const uword n_elem_to_copy = (std::min)(B.n_elem, in_n_elem);

    out.set_size(in_n_rows, in_n_cols, in_n_slices);

    eT* out_mem = out.memptr();

    if(in_dim == 0)
      {
      arrayops::copy( out_mem, B.memptr(), n_elem_to_copy );
      }
    else
      {
      uword row   = 0;
      uword col   = 0;
      uword slice = 0;

      const uword B_n_rows = B.n_rows;
      const uword B_n_cols = B.n_cols;

      for(uword i=0; i<n_elem_to_copy; ++i)
        {
        out_mem[i] = B.at(row,col,slice);

        ++col;

        if(col >= B_n_cols)
          {
          col = 0;
          ++row;

          if(row >= B_n_rows)
            {
            row = 0;
            ++slice;
            }
          }
        }
      }

    for(uword i=n_elem_to_copy; i<in_n_elem; ++i)
      {
      out_mem[i] = eT(0);
      }

    }
  }



//! @}
