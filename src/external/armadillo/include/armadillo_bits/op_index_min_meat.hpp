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


//! \addtogroup op_index_min
//! @{



template<typename T1>
inline
void
op_index_min::apply(Mat<uword>& out, const mtOp<uword,T1,op_index_min>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 1), "index_min(): parameter 'dim' must be 0 or 1");

  const quasi_unwrap<T1> U(in.m);
  const Mat<eT>& X = U.M;

  if(U.is_alias(out) == false)
    {
    op_index_min::apply_noalias(out, X, dim);
    }
  else
    {
    Mat<uword> tmp;

    op_index_min::apply_noalias(tmp, X, dim);

    out.steal_mem(tmp);
    }
  }



template<typename eT>
inline
void
op_index_min::apply_noalias(Mat<uword>& out, const Mat<eT>& X, const uword dim)
  {
  arma_extra_debug_sigprint();

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  if(dim == 0)
    {
    arma_extra_debug_print("op_index_min::apply(): dim = 0");

    out.set_size((X_n_rows > 0) ? 1 : 0, X_n_cols);

    if(X_n_rows == 0)  { return; }

    uword* out_mem = out.memptr();

    for(uword col=0; col < X_n_cols; ++col)
      {
      op_min::direct_min( X.colptr(col), X_n_rows, out_mem[col] );
      }
    }
  else
  if(dim == 1)
    {
    arma_extra_debug_print("op_index_min::apply(): dim = 1");

    out.set_size(X_n_rows, (X_n_cols > 0) ? 1 : 0);

    if(X_n_cols == 0)  { return; }

    uword* out_mem = out.memptr();

    for(uword row=0; row<X_n_rows; ++row)
      {
      out_mem[row] = X.row(row).index_min();
      }
    }
  }



template<typename T1>
inline
void
op_index_min::apply(Mat<uword>& out, const SpBase<typename T1::elem_type,T1>& expr, const uword dim)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  arma_debug_check( (dim > 1), "index_min(): parameter 'dim' must be 0 or 1" );

  const unwrap_spmat<T1> U(expr.get_ref());
  const SpMat<eT>& X   = U.M;

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  if(dim == 0)
    {
    arma_extra_debug_print("op_index_min::apply(): dim = 0");

    out.set_size((X_n_rows > 0) ? 1 : 0, X_n_cols);

    if(X_n_rows == 0)  { return; }

    uword* out_mem = out.memptr();

    for(uword col=0; col < X_n_cols; ++col)
      {
      out_mem[col] = X.col(col).index_min();
      }
    }
  else
  if(dim == 1)
    {
    arma_extra_debug_print("op_index_min::apply(): dim = 1");

    out.set_size(X_n_rows, (X_n_cols > 0) ? 1 : 0);

    if(X_n_cols == 0)  { return; }

    uword* out_mem = out.memptr();

    for(uword row=0; row<X_n_rows; ++row)
      {
      out_mem[row] = X.row(row).index_min();
      }
    }
  }



//! @}
