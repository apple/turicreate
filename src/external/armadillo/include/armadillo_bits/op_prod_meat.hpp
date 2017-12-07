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


//! \addtogroup op_prod
//! @{


template<typename eT>
inline
void
op_prod::apply_noalias(Mat<eT>& out, const Mat<eT>& X, const uword dim)
  {
  arma_extra_debug_sigprint();

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  if(dim == 0)  // traverse across rows (i.e. find the product in each column)
    {
    out.set_size(1, X_n_cols);

    eT* out_mem = out.memptr();

    for(uword col=0; col < X_n_cols; ++col)
      {
      out_mem[col] = arrayops::product(X.colptr(col), X_n_rows);
      }
    }
  else  // traverse across columns (i.e. find the product in each row)
    {
    out.ones(X_n_rows, 1);

    eT* out_mem = out.memptr();

    for(uword col=0; col < X_n_cols; ++col)
      {
      const eT* X_col_mem = X.colptr(col);

      for(uword row=0; row < X_n_rows; ++row)
        {
        out_mem[row] *= X_col_mem[row];
        }
      }
    }
  }



template<typename T1>
inline
void
op_prod::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_prod>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;

  arma_debug_check( (dim > 1), "prod(): parameter 'dim' must be 0 or 1" );

  const quasi_unwrap<T1> U(in.m);

  if(U.is_alias(out))
    {
    Mat<eT> tmp;

    op_prod::apply_noalias(tmp, U.M, dim);

    out.steal_mem(tmp);
    }
  else
    {
    op_prod::apply_noalias(out, U.M, dim);
    }
  }



template<typename eT>
inline
eT
op_prod::prod(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  eT val = eT(1);

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  if(X_n_rows == 1)
    {
    const Mat<eT>& A = X.m;

    const uword start_row = X.aux_row1;
    const uword start_col = X.aux_col1;

    const uword end_col_p1 = start_col + X_n_cols;

    uword i,j;
    for(i=start_col, j=start_col+1; j < end_col_p1; i+=2, j+=2)
      {
      val *= A.at(start_row, i);
      val *= A.at(start_row, j);
      }

    if(i < end_col_p1)
      {
      val *= A.at(start_row, i);
      }
    }
  else
    {
    for(uword col=0; col < X_n_cols; ++col)
      {
      val *= arrayops::product( X.colptr(col), X_n_rows );
      }
    }

  return val;
  }



template<typename T1>
inline
typename T1::elem_type
op_prod::prod(const Base<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> P(X.get_ref());

  eT val = eT(1);

  if(Proxy<T1>::use_at == false)
    {
    typedef typename Proxy<T1>::ea_type ea_type;

    const ea_type A = P.get_ea();

    const uword n_elem = P.get_n_elem();

    uword i,j;
    for(i=0, j=1; j < n_elem; i+=2, j+=2)
      {
      val *= A[i];
      val *= A[j];
      }

    if(i < n_elem)
      {
      val *= A[i];
      }
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_rows == 1)
      {
      uword i,j;
      for(i=0, j=1; j < n_cols; i+=2, j+=2)
        {
        val *= P.at(0,i);
        val *= P.at(0,j);
        }

      if(i < n_cols)
        {
        val *= P.at(0,i);
        }
      }
    else
      {
      for(uword col=0; col < n_cols; ++col)
        {
        uword i,j;
        for(i=0, j=1; j < n_rows; i+=2, j+=2)
          {
          val *= P.at(i,col);
          val *= P.at(j,col);
          }

        if(i < n_rows)
          {
          val *= P.at(i,col);
          }
        }
      }
    }

  return val;
  }



//! @}
