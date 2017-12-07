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


//! \addtogroup op_flip
//! @{



template<typename T1>
inline
void
op_flipud::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_flipud>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T1>   tmp(in.m);
  const Mat<eT>& X = tmp.M;

  const uword X_n_rows = T1::is_row ? uword(1) : X.n_rows;
  const uword X_n_cols = T1::is_col ? uword(1) : X.n_cols;

  if(&out != &X)
    {
    out.copy_size(X);

    for(uword col=0; col<X_n_cols; ++col)
      {
      const eT*   X_data =   X.colptr(col);
            eT* out_data = out.colptr(col);

      for(uword row=0; row<X_n_rows; ++row)
        {
        out_data[row] = X_data[X_n_rows-1 - row];
        }
      }
    }
  else  // in-place operation
    {
    const uword N = X_n_rows / 2;

    for(uword col=0; col<X_n_cols; ++col)
      {
      eT* out_data = out.colptr(col);

      for(uword row=0; row<N; ++row)
        {
        std::swap(out_data[row], out_data[X_n_rows-1 - row]);
        }
      }
    }
  }



template<typename T1>
inline
void
op_fliplr::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_fliplr>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T1>   tmp(in.m);
  const Mat<eT>& X = tmp.M;

  const uword X_n_cols = X.n_cols;

  if(&out != &X)
    {
    out.copy_size(X);

    if(T1::is_row || X.is_rowvec())
      {
      for(uword i=0; i<X_n_cols; ++i)  { out[i] = X[X_n_cols-1 - i]; }
      }
    else
      {
      for(uword i=0; i<X_n_cols; ++i)  { out.col(i) = X.col(X_n_cols-1 - i); }
      }
    }
  else
    {
    const uword N = X_n_cols / 2;

    if(T1::is_row || X.is_rowvec())
      {
      for(uword i=0; i<N; ++i)  { std::swap(out[i], out[X_n_cols-1 - i]); }
      }
    else
      {
      for(uword i=0; i<N; ++i)  { out.swap_cols(i, X_n_cols-1 - i); }
      }
    }
  }



//! @}
