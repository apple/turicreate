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


//! \addtogroup op_range
//! @{



template<typename T1>
inline
void
op_range::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_range>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 1), "range(): parameter 'dim' must be 0 or 1");

  const quasi_unwrap<T1> U(in.m);
  const Mat<eT>& X = U.M;

  if(U.is_alias(out) == false)
    {
    op_range::apply_noalias(out, X, dim);
    }
  else
    {
    Mat<eT> tmp;

    op_range::apply_noalias(tmp, X, dim);

    out.steal_mem(tmp);
    }
  }



template<typename eT>
inline
void
op_range::apply_noalias(Mat<eT>& out, const Mat<eT>& X, const uword dim)
  {
  arma_extra_debug_sigprint();

  // TODO: replace with dedicated implementation which finds min and max at the same time
  out = max(X,dim) - min(X,dim);
  }



template<typename T1>
inline
typename T1::elem_type
op_range::vector_range(const T1& expr)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const quasi_unwrap<T1> U(expr);
  const Mat<eT>& X = U.M;

  const eT*   X_mem = X.memptr();
  const uword N     = X.n_elem;

  if(N == 0)
    {
    arma_debug_check(true, "range(): object has no elements");

    return Datum<eT>::nan;
    }

  // TODO: replace with dedicated implementation which finds min and max at the same time
  return op_max::direct_max(X_mem, N) - op_min::direct_min(X_mem, N);
  }



//! @}
