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


//! \addtogroup spop_mean
//! @{



template<typename T1>
inline
void
spop_mean::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1, spop_mean>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 1), "mean(): parameter 'dim' must be 0 or 1" );

  const SpProxy<T1> p(in.m);

  if(p.is_alias(out) == false)
    {
    spop_mean::apply_noalias_fast(out, p, dim);
    }
  else
    {
    SpMat<eT> tmp;

    spop_mean::apply_noalias_fast(tmp, p, dim);

    out.steal_mem(tmp);
    }
  }



template<typename T1>
inline
void
spop_mean::apply_noalias_fast
  (
        SpMat<typename T1::elem_type>& out,
  const SpProxy<T1>&                   p,
  const uword                          dim
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;
  typedef typename T1::pod_type   T;

  const uword p_n_rows = p.get_n_rows();
  const uword p_n_cols = p.get_n_cols();

  if( (p_n_rows == 0) || (p_n_cols == 0) || (p.get_n_nonzero() == 0) )
    {
    if(dim == 0)  { out.zeros((p_n_rows > 0) ? 1 : 0, p_n_cols); }
    if(dim == 1)  { out.zeros(p_n_rows, (p_n_cols > 0) ? 1 : 0); }

    return;
    }

  if(dim == 0) // find the mean in each column
    {
    Row<eT> acc(p_n_cols, fill::zeros);

    if(SpProxy<T1>::use_iterator)
      {
      typename SpProxy<T1>::const_iterator_type it     = p.begin();
      typename SpProxy<T1>::const_iterator_type it_end = p.end();

      while(it != it_end)  { acc[it.col()] += (*it);  ++it; }

      acc /= T(p_n_rows);
      }
    else
      {
      for(uword col = 0; col < p_n_cols; ++col)
        {
        acc[col] = arrayops::accumulate
          (
          &p.get_values()[p.get_col_ptrs()[col]],
          p.get_col_ptrs()[col + 1] - p.get_col_ptrs()[col]
          ) / T(p_n_rows);
        }
      }

    out = acc;
    }
  else
  if(dim == 1)  // find the mean in each row
    {
    Col<eT> acc(p_n_rows, fill::zeros);

    typename SpProxy<T1>::const_iterator_type it     = p.begin();
    typename SpProxy<T1>::const_iterator_type it_end = p.end();

    while(it != it_end)  { acc[it.row()] += (*it);  ++it; }

    acc /= T(p_n_cols);

    out = acc;
    }

  if(out.is_finite() == false)
    {
    spop_mean::apply_noalias_slow(out, p, dim);
    }
  }



template<typename T1>
inline
void
spop_mean::apply_noalias_slow
  (
        SpMat<typename T1::elem_type>& out,
  const SpProxy<T1>&                   p,
  const uword                          dim
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword p_n_rows = p.get_n_rows();
  const uword p_n_cols = p.get_n_cols();

  if(dim == 0)  // find the mean in each column
    {
    arma_extra_debug_print("spop_mean::apply_noalias(): dim = 0");

    out.set_size((p_n_rows > 0) ? 1 : 0, p_n_cols);

    if( (p_n_rows == 0) || (p.get_n_nonzero() == 0) )  { return; }

    for(uword col = 0; col < p_n_cols; ++col)
      {
      // Do we have to use an iterator or can we use memory directly?
      if(SpProxy<T1>::use_iterator)
        {
        typename SpProxy<T1>::const_iterator_type it  = p.begin_col(col);
        typename SpProxy<T1>::const_iterator_type end = p.begin_col(col + 1);

        const uword n_zero = p_n_rows - (end.pos() - it.pos());

        out.at(0,col) = spop_mean::iterator_mean(it, end, n_zero, eT(0));
        }
      else
        {
        out.at(0,col) = spop_mean::direct_mean
          (
          &p.get_values()[p.get_col_ptrs()[col]],
          p.get_col_ptrs()[col + 1] - p.get_col_ptrs()[col],
          p_n_rows
          );
        }
      }
    }
  else
  if(dim == 1)  // find the mean in each row
    {
    arma_extra_debug_print("spop_mean::apply_noalias(): dim = 1");

    out.set_size(p_n_rows, (p_n_cols > 0) ? 1 : 0);

    if( (p_n_cols == 0) || (p.get_n_nonzero() == 0) )  { return; }

    for(uword row = 0; row < p_n_rows; ++row)
      {
      // We must use an iterator regardless of how it is stored.
      typename SpProxy<T1>::const_row_iterator_type it  = p.begin_row(row);
      typename SpProxy<T1>::const_row_iterator_type end = p.end_row(row);

      const uword n_zero = p_n_cols - (end.pos() - it.pos());

      out.at(row,0) = spop_mean::iterator_mean(it, end, n_zero, eT(0));
      }
    }
  }



template<typename eT>
inline
eT
spop_mean::direct_mean
  (
  const eT* const X,
  const uword length,
  const uword N
  )
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  const eT result = ((length > 0) && (N > 0)) ? eT(arrayops::accumulate(X, length) / T(N)) : eT(0);

  return arma_isfinite(result) ? result : spop_mean::direct_mean_robust(X, length, N);
  }



template<typename eT>
inline
eT
spop_mean::direct_mean_robust
  (
  const eT* const X,
  const uword length,
  const uword N
  )
  {
  arma_extra_debug_sigprint();

  typedef typename get_pod_type<eT>::result T;

  uword i, j;

  eT r_mean = eT(0);

  const uword diff = (N - length); // number of zeros

  for(i = 0, j = 1; j < length; i += 2, j += 2)
    {
    const eT Xi = X[i];
    const eT Xj = X[j];

    r_mean += (Xi - r_mean) / T(diff + j);
    r_mean += (Xj - r_mean) / T(diff + j + 1);
    }

  if(i < length)
    {
    const eT Xi = X[i];

    r_mean += (Xi - r_mean) / T(diff + i + 1);
    }

  return r_mean;
  }



template<typename T1>
inline
typename T1::elem_type
spop_mean::mean_all(const SpBase<typename T1::elem_type, T1>& X)
  {
  arma_extra_debug_sigprint();

  SpProxy<T1> p(X.get_ref());

  if(SpProxy<T1>::use_iterator)
    {
    typename SpProxy<T1>::const_iterator_type it  = p.begin();
    typename SpProxy<T1>::const_iterator_type end = p.end();

    return spop_mean::iterator_mean(it, end, p.get_n_elem() - p.get_n_nonzero(), typename T1::elem_type(0));
    }
  else // use_iterator == false; that is, we can directly access the values array
    {
    return spop_mean::direct_mean(p.get_values(), p.get_n_nonzero(), p.get_n_elem());
    }
  }



template<typename T1, typename eT>
inline
eT
spop_mean::iterator_mean(T1& it, const T1& end, const uword n_zero, const eT junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename get_pod_type<eT>::result T;

  eT acc = eT(0);

  T1 backup_it(it); // in case we have to use robust iterator_mean

  const uword it_begin_pos = it.pos();

  while (it != end)
    {
    acc += (*it);
    ++it;
    }

  const uword count = n_zero + (it.pos() - it_begin_pos);

  const eT result = (count > 0) ? eT(acc / T(count)) : eT(0);

  return arma_isfinite(result) ? result : spop_mean::iterator_mean_robust(backup_it, end, n_zero, eT(0));
  }



template<typename T1, typename eT>
inline
eT
spop_mean::iterator_mean_robust(T1& it, const T1& end, const uword n_zero, const eT junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename get_pod_type<eT>::result T;

  eT r_mean = eT(0);

  const uword it_begin_pos = it.pos();

  while (it != end)
    {
    r_mean += ((*it - r_mean) / T(n_zero + (it.pos() - it_begin_pos) + 1));
    ++it;
    }

  return r_mean;
  }



//! @}
