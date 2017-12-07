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


//! \addtogroup op_diff
//! @{


template<typename eT>
inline
void
op_diff::apply_noalias(Mat<eT>& out, const Mat<eT>& X, const uword k, const uword dim)
  {
  arma_extra_debug_sigprint();

  uword n_rows = X.n_rows;
  uword n_cols = X.n_cols;

  if(dim == 0)
    {
    if(n_rows <= k)  { out.set_size(0,n_cols); return; }

    --n_rows;

    out.set_size(n_rows,n_cols);

    for(uword col=0; col < n_cols; ++col)
      {
            eT* out_colmem = out.colptr(col);
      const eT*   X_colmem =   X.colptr(col);

      for(uword row=0; row < n_rows; ++row)
        {
        const eT val0 = X_colmem[row  ];
        const eT val1 = X_colmem[row+1];

        out_colmem[row] = val1 - val0;
        }
      }

    if(k >= 2)
      {
      for(uword iter=2; iter <= k; ++iter)
        {
        --n_rows;

        for(uword col=0; col < n_cols; ++col)
          {
          eT* colmem = out.colptr(col);

          for(uword row=0; row < n_rows; ++row)
            {
            const eT val0 = colmem[row  ];
            const eT val1 = colmem[row+1];

            colmem[row] = val1 - val0;
            }
          }
        }

      out = out( span(0,n_rows-1), span::all );
      }
    }
  else
  if(dim == 1)
    {
    if(n_cols <= k)  { out.set_size(n_rows,0); return; }

    --n_cols;

    out.set_size(n_rows,n_cols);

    if(n_rows == 1)
      {
      const eT*   X_mem =   X.memptr();
            eT* out_mem = out.memptr();

      for(uword col=0; col < n_cols; ++col)
        {
        const eT val0 = X_mem[col  ];
        const eT val1 = X_mem[col+1];

        out_mem[col] = val1 - val0;
        }
      }
    else
      {
      for(uword col=0; col < n_cols; ++col)
        {
        eT* out_col_mem = out.colptr(col);

        const eT* X_col0_mem = X.colptr(col  );
        const eT* X_col1_mem = X.colptr(col+1);

        for(uword row=0; row < n_rows; ++row)
          {
          out_col_mem[row] = X_col1_mem[row] - X_col0_mem[row];
          }
        }
      }

    if(k >= 2)
      {
      for(uword iter=2; iter <= k; ++iter)
        {
        --n_cols;

        if(n_rows == 1)
          {
          eT* out_mem = out.memptr();

          for(uword col=0; col < n_cols; ++col)
            {
            const eT val0 = out_mem[col  ];
            const eT val1 = out_mem[col+1];

            out_mem[col] = val1 - val0;
            }
          }
        else
          {
          for(uword col=0; col < n_cols; ++col)
            {
                  eT* col0_mem = out.colptr(col  );
            const eT* col1_mem = out.colptr(col+1);

            for(uword row=0; row < n_rows; ++row)
              {
              col0_mem[row] = col1_mem[row] - col0_mem[row];
              }
            }
          }
        }

      out = out( span::all, span(0,n_cols-1) );
      }
    }
  }



template<typename T1>
inline
void
op_diff::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_diff>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword k   = in.aux_uword_a;
  const uword dim = in.aux_uword_b;

  arma_debug_check( (dim > 1), "diff(): parameter 'dim' must be 0 or 1" );

  if(k == 0)  { out = in.m; return; }

  const quasi_unwrap<T1> U(in.m);

  if(U.is_alias(out))
    {
    Mat<eT> tmp;

    op_diff::apply_noalias(tmp, U.M, k, dim);

    out.steal_mem(tmp);
    }
  else
    {
    op_diff::apply_noalias(out, U.M, k, dim);
    }
  }



template<typename T1>
inline
void
op_diff_default::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_diff_default>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword k = in.aux_uword_a;

  if(k == 0)  { out = in.m; return; }

  const quasi_unwrap<T1> U(in.m);

  const uword dim = (T1::is_row) ? 1 : 0;

  if(U.is_alias(out))
    {
    Mat<eT> tmp;

    op_diff::apply_noalias(tmp, U.M, k, dim);

    out.steal_mem(tmp);
    }
  else
    {
    op_diff::apply_noalias(out, U.M, k, dim);
    }
  }



//! @}
