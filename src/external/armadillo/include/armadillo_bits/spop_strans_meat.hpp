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


//! \addtogroup spop_strans
//! @{



template<typename eT>
arma_hot
inline
void
spop_strans::apply_spmat(SpMat<eT>& out, const SpMat<eT>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename umat::elem_type ueT;

  const uword N = X.n_nonzero;

  if(N == uword(0))
    {
    out.zeros(X.n_cols, X.n_rows);
    return;
    }

  umat locs(2, N);

  typename SpMat<eT>::const_iterator it = X.begin();

  for(uword count = 0; count < N; ++count)
    {
    ueT* locs_ptr = locs.colptr(count);

    locs_ptr[0] = it.col();
    locs_ptr[1] = it.row();

    ++it;
    }

  const Col<eT> vals(const_cast<eT*>(X.values), N, false);

  SpMat<eT> tmp(locs, vals, X.n_cols, X.n_rows);

  out.steal_mem(tmp);
  }



template<typename T1>
arma_hot
inline
void
spop_strans::apply_proxy(SpMat<typename T1::elem_type>& out, const T1& X)
  {
  arma_extra_debug_sigprint();

  typedef typename   T1::elem_type  eT;
  typedef typename umat::elem_type ueT;

  const SpProxy<T1> p(X);

  const uword N = p.get_n_nonzero();

  if(N == uword(0))
    {
    out.zeros(p.get_n_cols(), p.get_n_rows());
    return;
    }

  umat locs(2, N);

  Col<eT> vals(N);

  eT* vals_ptr = vals.memptr();

  typename SpProxy<T1>::const_iterator_type it = p.begin();

  for(uword count = 0; count < N; ++count)
    {
    ueT* locs_ptr = locs.colptr(count);

    locs_ptr[0] = it.col();
    locs_ptr[1] = it.row();

    vals_ptr[count] = (*it);

    ++it;
    }

  SpMat<eT> tmp(locs, vals, p.get_n_cols(), p.get_n_rows());

  out.steal_mem(tmp);
  }



template<typename T1>
arma_hot
inline
void
spop_strans::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_strans>& in)
  {
  arma_extra_debug_sigprint();

  if(is_SpMat<T1>::value)
    {
    const unwrap_spmat<T1> tmp(in.m);

    spop_strans::apply_spmat(out, tmp.M);
    }
  else
    {
    spop_strans::apply_proxy(out, in.m);
    }
  }



//! for transpose of non-complex matrices, redirected from spop_htrans::apply()
template<typename T1>
arma_hot
inline
void
spop_strans::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_htrans>& in)
  {
  arma_extra_debug_sigprint();

  if(is_SpMat<T1>::value)
    {
    const unwrap_spmat<T1> tmp(in.m);

    spop_strans::apply_spmat(out, tmp.M);
    }
  else
    {
    spop_strans::apply_proxy(out, in.m);
    }
  }



//! @}
