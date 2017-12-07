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



//! \addtogroup op_normalise
//! @{



template<typename T1>
inline
void
op_normalise_vec::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_normalise_vec>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::pod_type T;

  const uword p = in.aux_uword_a;

  arma_debug_check( (p == 0), "normalise(): parameter 'p' must be greater than zero" );

  const quasi_unwrap<T1> tmp(in.m);

  const T norm_val_a = norm(tmp.M, p);
  const T norm_val_b = (norm_val_a != T(0)) ? norm_val_a : T(1);

  out = tmp.M / norm_val_b;
  }



template<typename T1>
inline
void
op_normalise_mat::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_normalise_mat>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword p   = in.aux_uword_a;
  const uword dim = in.aux_uword_b;

  arma_debug_check( (p   == 0), "normalise(): parameter 'p' must be greater than zero" );
  arma_debug_check( (dim >  1), "normalise(): parameter 'dim' must be 0 or 1"          );

  const unwrap<T1>   tmp(in.m);
  const Mat<eT>& A = tmp.M;

  const bool alias = ( (&out) == (&A) );

  if(alias)
    {
    Mat<eT> out2;

    op_normalise_mat::apply(out2, A, p, dim);

    out.steal_mem(out2);
    }
  else
    {
    op_normalise_mat::apply(out, A, p, dim);
    }
  }



template<typename eT>
inline
void
op_normalise_mat::apply(Mat<eT>& out, const Mat<eT>& A, const uword p, const uword dim)
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  out.copy_size(A);

  if(A.n_elem == 0)  { return; }

  if(dim == 0)
    {
    const uword n_cols = A.n_cols;

    for(uword i=0; i<n_cols; ++i)
      {
      const T norm_val_a = norm(A.col(i), p);
      const T norm_val_b = (norm_val_a != T(0)) ? norm_val_a : T(1);

      out.col(i) = A.col(i) / norm_val_b;
      }
    }
  else
    {
    // better-than-nothing implementation

    const uword n_rows = A.n_rows;

    for(uword i=0; i<n_rows; ++i)
      {
      const T norm_val_a = norm(A.row(i), p);
      const T norm_val_b = (norm_val_a != T(0)) ? norm_val_a : T(1);

      out.row(i) = A.row(i) / norm_val_b;
      }
    }
  }



//! @}
