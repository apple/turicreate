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


//! \addtogroup spop_var
//! @{



template<typename T1>
inline
void
spop_var::apply(SpMat<typename T1::pod_type>& out, const mtSpOp<typename T1::pod_type, T1, spop_var>& in)
  {
  arma_extra_debug_sigprint();

  //typedef typename T1::elem_type  in_eT;
  typedef typename T1::pod_type  out_eT;

  const uword norm_type = in.aux_uword_a;
  const uword dim       = in.aux_uword_b;

  arma_debug_check( (norm_type > 1), "var(): parameter 'norm_type' must be 0 or 1" );
  arma_debug_check( (dim > 1),       "var(): parameter 'dim' must be 0 or 1"       );

  const SpProxy<T1> p(in.m);

  if(p.is_alias(out) == false)
    {
    spop_var::apply_noalias(out, p, norm_type, dim);
    }
  else
    {
    SpMat<out_eT> tmp;

    spop_var::apply_noalias(tmp, p, norm_type, dim);

    out.steal_mem(tmp);
    }
  }



template<typename T1>
inline
void
spop_var::apply_noalias
  (
        SpMat<typename T1::pod_type>& out,
  const SpProxy<T1>&                  p,
  const uword                         norm_type,
  const uword                         dim
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type  in_eT;
  //typedef typename T1::pod_type  out_eT;

  const uword p_n_rows = p.get_n_rows();
  const uword p_n_cols = p.get_n_cols();

  // TODO: this is slow; rewrite based on the approach used by sparse mean()

  if(dim == 0)  // find variance in each column
    {
    arma_extra_debug_print("spop_var::apply_noalias(): dim = 0");

    out.set_size((p_n_rows > 0) ? 1 : 0, p_n_cols);

    if( (p_n_rows == 0) || (p.get_n_nonzero() == 0) )  { return; }

    for(uword col = 0; col < p_n_cols; ++col)
      {
      if(SpProxy<T1>::use_iterator)
        {
        // We must use an iterator; we can't access memory directly.
        typename SpProxy<T1>::const_iterator_type it  = p.begin_col(col);
        typename SpProxy<T1>::const_iterator_type end = p.begin_col(col + 1);

        const uword n_zero = p_n_rows - (end.pos() - it.pos());

        // in_eT is used just to get the specialization right (complex / noncomplex)
        out.at(0, col) = spop_var::iterator_var(it, end, n_zero, norm_type, in_eT(0));
        }
      else
        {
        // We can use direct memory access to calculate the variance.
        out.at(0, col) = spop_var::direct_var
          (
          &p.get_values()[p.get_col_ptrs()[col]],
          p.get_col_ptrs()[col + 1] - p.get_col_ptrs()[col],
          p_n_rows,
          norm_type
          );
        }
      }
    }
  else
  if(dim == 1)  // find variance in each row
    {
    arma_extra_debug_print("spop_var::apply_noalias(): dim = 1");

    out.set_size(p_n_rows, (p_n_cols > 0) ? 1 : 0);

    if( (p_n_cols == 0) || (p.get_n_nonzero() == 0) )  { return; }

    for(uword row = 0; row < p_n_rows; ++row)
      {
      // We have to use an iterator here regardless of whether or not we can
      // directly access memory.
      typename SpProxy<T1>::const_row_iterator_type it  = p.begin_row(row);
      typename SpProxy<T1>::const_row_iterator_type end = p.end_row(row);

      const uword n_zero = p_n_cols - (end.pos() - it.pos());

      out.at(row, 0) = spop_var::iterator_var(it, end, n_zero, norm_type, in_eT(0));
      }
    }
  }



template<typename T1>
inline
typename T1::pod_type
spop_var::var_vec
  (
  const T1& X,
  const uword norm_type
  )
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (norm_type > 1), "var(): parameter 'norm_type' must be 0 or 1" );

  // conditionally unwrap it into a temporary and then directly operate.

  const unwrap_spmat<T1> tmp(X);

  return direct_var(tmp.M.values, tmp.M.n_nonzero, tmp.M.n_elem, norm_type);
  }



template<typename eT>
inline
eT
spop_var::direct_var
  (
  const eT* const X,
  const uword length,
  const uword N,
  const uword norm_type
  )
  {
  arma_extra_debug_sigprint();

  if(length >= 2 && N >= 2)
    {
    const eT acc1 = spop_mean::direct_mean(X, length, N);

    eT acc2 = eT(0);
    eT acc3 = eT(0);

    uword i, j;

    for(i = 0, j = 1; j < length; i += 2, j += 2)
      {
      const eT Xi = X[i];
      const eT Xj = X[j];

      const eT tmpi = acc1 - Xi;
      const eT tmpj = acc1 - Xj;

      acc2 += tmpi * tmpi + tmpj * tmpj;
      acc3 += tmpi + tmpj;
      }

    if(i < length)
      {
      const eT Xi = X[i];

      const eT tmpi = acc1 - Xi;

      acc2 += tmpi * tmpi;
      acc3 += tmpi;
      }

    // Now add in all zero elements.
    acc2 += (N - length) * (acc1 * acc1);
    acc3 += (N - length) * acc1;

    const eT norm_val = (norm_type == 0) ? eT(N - 1) : eT(N);
    const eT var_val  = (acc2 - (acc3 * acc3) / eT(N)) / norm_val;

    return var_val;
    }
  else if(length == 1 && N > 1) // if N == 1, then variance is zero.
    {
    const eT mean = X[0] / eT(N);
    const eT val = mean - X[0];

    const eT acc2 = (val * val) + (N - length) * (mean * mean);
    const eT acc3 = val + (N - length) * mean;

    const eT norm_val = (norm_type == 0) ? eT(N - 1) : eT(N);
    const eT var_val  = (acc2 - (acc3 * acc3) / eT(N)) / norm_val;

    return var_val;
    }
  else
    {
    return eT(0);
    }
  }



template<typename T>
inline
T
spop_var::direct_var
  (
  const std::complex<T>* const X,
  const uword length,
  const uword N,
  const uword norm_type
  )
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT;

  if(length >= 2 && N >= 2)
    {
    const eT acc1 = spop_mean::direct_mean(X, length, N);

     T acc2 =  T(0);
    eT acc3 = eT(0);

    for (uword i = 0; i < length; ++i)
      {
      const eT tmp = acc1 - X[i];

      acc2 += std::norm(tmp);
      acc3 += tmp;
      }

    // Add zero elements to sums
    acc2 += std::norm(acc1) * T(N - length);
    acc3 += acc1 * T(N - length);

    const T norm_val = (norm_type == 0) ? T(N - 1) : T(N);
    const T var_val  = (acc2 - std::norm(acc3) / T(N)) / norm_val;

    return var_val;
    }
  else if(length == 1 && N > 1) // if N == 1, then variance is zero.
    {
    const eT mean = X[0] / T(N);
    const eT val = mean - X[0];

    const T acc2 = std::norm(val) + (N - length) * std::norm(mean);
    const eT acc3 = val + T(N - length) * mean;

    const T norm_val = (norm_type == 0) ? T(N - 1) : T(N);
    const T var_val  = (acc2 - std::norm(acc3) / T(N)) / norm_val;

    return var_val;
    }
  else
    {
    return T(0); // All elements are zero
    }
  }



template<typename T1, typename eT>
inline
eT
spop_var::iterator_var
  (
  T1& it,
  const T1& end,
  const uword n_zero,
  const uword norm_type,
  const eT junk1,
  const typename arma_not_cx<eT>::result* junk2
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  T1 new_it(it); // for mean
  // T1 backup_it(it); // in case we have to call robust iterator_var
  eT mean = spop_mean::iterator_mean(new_it, end, n_zero, eT(0));

  eT acc2 = eT(0);
  eT acc3 = eT(0);

  const uword it_begin_pos = it.pos();

  while (it != end)
    {
    const eT tmp = mean - (*it);

    acc2 += (tmp * tmp);
    acc3 += (tmp);

    ++it;
    }

  const uword n_nonzero = (it.pos() - it_begin_pos);
  if (n_nonzero == 0)
    {
    return eT(0);
    }

  if (n_nonzero + n_zero == 1)
    {
    return eT(0); // only one element
    }

  // Add in entries for zeros.
  acc2 += eT(n_zero) * (mean * mean);
  acc3 += eT(n_zero) * mean;

  const eT norm_val = (norm_type == 0) ? eT(n_zero + n_nonzero - 1) : eT(n_zero + n_nonzero);
  const eT var_val  = (acc2 - (acc3 * acc3) / eT(n_nonzero + n_zero)) / norm_val;

  return var_val;
  }



template<typename T1, typename eT>
inline
typename get_pod_type<eT>::result
spop_var::iterator_var
  (
  T1& it,
  const T1& end,
  const uword n_zero,
  const uword norm_type,
  const eT junk1,
  const typename arma_cx_only<eT>::result* junk2
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk1);
  arma_ignore(junk2);

  typedef typename get_pod_type<eT>::result T;

  T1 new_it(it); // for mean
  // T1 backup_it(it); // in case we have to call robust iterator_var
  eT mean = spop_mean::iterator_mean(new_it, end, n_zero, eT(0));

   T acc2 =  T(0);
  eT acc3 = eT(0);

  const uword it_begin_pos = it.pos();

  while (it != end)
    {
    eT tmp = mean - (*it);

    acc2 += std::norm(tmp);
    acc3 += (tmp);

    ++it;
    }

  const uword n_nonzero = (it.pos() - it_begin_pos);
  if (n_nonzero == 0)
    {
    return T(0);
    }

  if (n_nonzero + n_zero == 1)
    {
    return T(0); // only one element
    }

  // Add in entries for zero elements.
  acc2 += T(n_zero) * std::norm(mean);
  acc3 += T(n_zero) * mean;

  const T norm_val = (norm_type == 0) ? T(n_zero + n_nonzero - 1) : T(n_zero + n_nonzero);
  const T var_val  = (acc2 - std::norm(acc3) / T(n_nonzero + n_zero)) / norm_val;

  return var_val;
  }



//! @}
