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



//! \addtogroup op_shift
//! @{



template<typename T1>
inline
void
op_shift_default::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_shift_default>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> U(in.m);

  const uword len = in.aux_uword_a;
  const uword neg = in.aux_uword_b;
  const uword dim = (T1::is_row) ? 1 : 0;

  op_shift::apply_direct(out, U.M, len, neg, dim);
  }



template<typename T1>
inline
void
op_shift::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_shift>& in)
  {
  arma_extra_debug_sigprint();

  const unwrap<T1> U(in.m);

  const uword len = in.aux_uword_a;
  const uword neg = in.aux_uword_b;
  const uword dim = in.aux_uword_c;

  arma_debug_check( (dim > 1), "shift(): parameter 'dim' must be 0 or 1" );

  op_shift::apply_direct(out, U.M, len, neg, dim);
  }



template<typename eT>
inline
void
op_shift::apply_direct(Mat<eT>& out, const Mat<eT>& X, const uword len, const uword neg, const uword dim)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ((dim == 0) && (len >= X.n_rows)), "shift(): shift amount out of bounds" );
  arma_debug_check( ((dim == 1) && (len >= X.n_cols)), "shift(): shift amount out of bounds" );

  if(&out == &X)
    {
    op_shift::apply_alias(out, len, neg, dim);
    }
  else
    {
    op_shift::apply_noalias(out, X, len, neg, dim);
    }
  }



template<typename eT>
inline
void
op_shift::apply_noalias(Mat<eT>& out, const Mat<eT>& X, const uword len, const uword neg, const uword dim)
  {
  arma_extra_debug_sigprint();

  out.copy_size(X);

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  if(dim == 0)
    {
    if(neg == 0)
      {
      for(uword col=0; col < X_n_cols; ++col)
        {
              eT* out_ptr = out.colptr(col);
        const eT*   X_ptr =   X.colptr(col);

        for(uword out_row=len, row=0; row < (X_n_rows - len); ++row, ++out_row)
          {
          out_ptr[out_row] = X_ptr[row];
          }

        for(uword out_row=0, row=(X_n_rows - len); row < X_n_rows; ++row, ++out_row)
          {
          out_ptr[out_row] = X_ptr[row];
          }
        }
      }
    else
    if(neg == 1)
      {
      for(uword col=0; col < X_n_cols; ++col)
        {
              eT* out_ptr = out.colptr(col);
        const eT*   X_ptr =   X.colptr(col);

        for(uword out_row=0, row=len; row < X_n_rows; ++row, ++out_row)
          {
          out_ptr[out_row] = X_ptr[row];
          }

        for(uword out_row=(X_n_rows-len), row=0; row < len; ++row, ++out_row)
          {
          out_ptr[out_row] = X_ptr[row];
          }
        }
      }
    }
  else
  if(dim == 1)
    {
    if(neg == 0)
      {
      if(X_n_rows == 1)
        {
              eT* out_ptr = out.memptr();
        const eT*   X_ptr =   X.memptr();

        for(uword out_col=len, col=0; col < (X_n_cols - len); ++col, ++out_col)
          {
          out_ptr[out_col] = X_ptr[col];
          }

        for(uword out_col=0, col=(X_n_cols - len); col < X_n_cols; ++col, ++out_col)
          {
          out_ptr[out_col] = X_ptr[col];
          }
        }
      else
        {
        for(uword out_col=len, col=0; col < (X_n_cols - len); ++col, ++out_col)
          {
          arrayops::copy( out.colptr(out_col), X.colptr(col), X_n_rows );
          }

        for(uword out_col=0, col=(X_n_cols - len); col < X_n_cols; ++col, ++out_col)
          {
          arrayops::copy( out.colptr(out_col), X.colptr(col), X_n_rows );
          }
        }
      }
    else
    if(neg == 1)
      {
      if(X_n_rows == 1)
        {
              eT* out_ptr = out.memptr();
        const eT*   X_ptr =   X.memptr();

        for(uword out_col=0, col=len; col < X_n_cols; ++col, ++out_col)
          {
          out_ptr[out_col] = X_ptr[col];
          }

        for(uword out_col=(X_n_cols-len), col=0; col < len; ++col, ++out_col)
          {
          out_ptr[out_col] = X_ptr[col];
          }
        }
      else
        {
        for(uword out_col=0, col=len; col < X_n_cols; ++col, ++out_col)
          {
          arrayops::copy( out.colptr(out_col), X.colptr(col), X_n_rows );
          }

        for(uword out_col=(X_n_cols-len), col=0; col < len; ++col, ++out_col)
          {
          arrayops::copy( out.colptr(out_col), X.colptr(col), X_n_rows );
          }
        }
      }
    }
  }



template<typename eT>
inline
void
op_shift::apply_alias(Mat<eT>& X, const uword len, const uword neg, const uword dim)
  {
  arma_extra_debug_sigprint();

  // TODO: replace with better implementation

  Mat<eT> tmp;

  op_shift::apply_noalias(tmp, X, len, neg, dim);

  X.steal_mem(tmp);
  }



//! @}
