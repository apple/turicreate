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



//! \addtogroup op_hist
//! @{



template<typename eT>
inline
void
op_hist::apply_noalias(Mat<uword>& out, const Mat<eT>& A, const uword n_bins, const bool A_is_row)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ((A.is_vec() == false) && (A.is_empty() == false)), "hist(): only vectors are supported when automatically determining bin centers" );

  if(n_bins == 0)  { out.reset(); return; }

        uword A_n_elem = A.n_elem;
  const eT*   A_mem    = A.memptr();

  eT min_val = priv::most_pos<eT>();
  eT max_val = priv::most_neg<eT>();

  uword i,j;
  for(i=0, j=1; j < A_n_elem; i+=2, j+=2)
    {
    const eT val_i = A_mem[i];
    const eT val_j = A_mem[j];

    if(min_val > val_i) { min_val = val_i; }
    if(min_val > val_j) { min_val = val_j; }

    if(max_val < val_i) { max_val = val_i; }
    if(max_val < val_j) { max_val = val_j; }
    }

  if(i < A_n_elem)
    {
    const eT val_i = A_mem[i];

    if(min_val > val_i) { min_val = val_i; }
    if(max_val < val_i) { max_val = val_i; }
    }

  if(arma_isfinite(min_val) == false) { min_val = priv::most_neg<eT>(); }
  if(arma_isfinite(max_val) == false) { max_val = priv::most_pos<eT>(); }

  Col<eT> c(n_bins);
  eT* c_mem = c.memptr();

  for(uword ii=0; ii < n_bins; ++ii)
    {
    c_mem[ii] = (0.5 + ii) / double(n_bins);   // TODO: may need to be modified for integer matrices
    }

  c = ((max_val - min_val) * c) + min_val;

  const uword dim = (A_is_row) ? 1 : 0;

  glue_hist::apply_noalias(out, A, c, dim);
  }



template<typename T1>
inline
void
op_hist::apply(Mat<uword>& out, const mtOp<uword, T1, op_hist>& X)
  {
  arma_extra_debug_sigprint();

  const uword n_bins = X.aux_uword_a;

  const quasi_unwrap<T1> U(X.m);

  if(U.is_alias(out))
    {
    Mat<uword> tmp;

    op_hist::apply_noalias(tmp, U.M, n_bins, (T1::is_row));

    out.steal_mem(tmp);
    }
  else
    {
    op_hist::apply_noalias(out, U.M, n_bins, (T1::is_row));
    }
  }



//! @}
