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


//! \addtogroup spop_max
//! @{



template<typename T1>
inline
void
spop_max::apply(SpMat<typename T1::elem_type>& out, const SpOp<T1,spop_max>& in)
  {
  arma_extra_debug_sigprint();

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 1), "max(): parameter 'dim' must be 0 or 1" );

  const SpProxy<T1> p(in.m);

  const uword p_n_rows = p.get_n_rows();
  const uword p_n_cols = p.get_n_cols();

  if( (p_n_rows == 0) || (p_n_cols == 0) || (p.get_n_nonzero() == 0) )
    {
    if(dim == 0)  { out.zeros((p_n_rows > 0) ? 1 : 0, p_n_cols); }
    if(dim == 1)  { out.zeros(p_n_rows, (p_n_cols > 0) ? 1 : 0); }

    return;
    }

  spop_max::apply_proxy(out, p, dim);
  }



template<typename T1>
inline
void
spop_max::apply_proxy
  (
        SpMat<typename T1::elem_type>& out,
  const SpProxy<T1>&                   p,
  const uword                          dim,
  const typename arma_not_cx<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  typename SpProxy<T1>::const_iterator_type it     = p.begin();
  typename SpProxy<T1>::const_iterator_type it_end = p.end();

  const uword p_n_cols = p.get_n_cols();
  const uword p_n_rows = p.get_n_rows();

  if(dim == 0) // find the maximum in each column
    {
    Row<eT> value(p_n_cols, fill::zeros);
    urowvec count(p_n_cols, fill::zeros);

    while(it != it_end)
      {
      const uword col = it.col();

      value[col] = (count[col] == 0) ? (*it) : (std::max)(value[col], (*it));
      count[col]++;
      ++it;
      }

    for(uword col=0; col<p_n_cols; ++col)
      {
      if(count[col] < p_n_rows)  { value[col] = (std::max)(value[col], eT(0)); }
      }

    out = value;
    }
  else
  if(dim == 1)  // find the maximum in each row
    {
    Col<eT> value(p_n_rows, fill::zeros);
    ucolvec count(p_n_rows, fill::zeros);

    while(it != it_end)
      {
      const uword row = it.row();

      value[row] = (count[row] == 0) ? (*it) : (std::max)(value[row], (*it));
      count[row]++;
      ++it;
      }

    for(uword row=0; row<p_n_rows; ++row)
      {
      if(count[row] < p_n_cols)  { value[row] = (std::max)(value[row], eT(0)); }
      }

    out = value;
    }
  }



template<typename T1>
inline
typename T1::elem_type
spop_max::vector_max
  (
  const T1& x,
  const typename arma_not_cx<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  const SpProxy<T1> p(x);

  if(p.get_n_elem() == 0)
    {
    arma_debug_check(true, "max(): object has no elements");

    return Datum<eT>::nan;
    }

  if(p.get_n_nonzero() == 0)  { return eT(0); }

  if(SpProxy<T1>::use_iterator == false)
    {
    // direct access of values
    if(p.get_n_nonzero() == p.get_n_elem())
      {
      return op_max::direct_max(p.get_values(), p.get_n_nonzero());
      }
    else
      {
      return std::max(eT(0), op_max::direct_max(p.get_values(), p.get_n_nonzero()));
      }
    }
  else
    {
    // use iterator
    typename SpProxy<T1>::const_iterator_type it     = p.begin();
    typename SpProxy<T1>::const_iterator_type it_end = p.end();

    eT result = (*it);
    ++it;

    while(it != it_end)
      {
      if((*it) > result)  { result = (*it); }

      ++it;
      }

    if(p.get_n_nonzero() == p.get_n_elem())
      {
      return result;
      }
    else
      {
      return std::max(eT(0), result);
      }
    }
  }



template<typename T1>
inline
typename arma_not_cx<typename T1::elem_type>::result
spop_max::max(const SpBase<typename T1::elem_type, T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const SpProxy<T1> P(X.get_ref());

  const uword n_elem    = P.get_n_elem();
  const uword n_nonzero = P.get_n_nonzero();

  if(n_elem == 0)
    {
    arma_debug_check(true, "max(): object has no elements");

    return Datum<eT>::nan;
    }

  eT max_val = priv::most_neg<eT>();

  if(SpProxy<T1>::use_iterator)
    {
    // We have to iterate over the elements.
    typedef typename SpProxy<T1>::const_iterator_type it_type;

    it_type it     = P.begin();
    it_type it_end = P.end();

    while (it != it_end)
      {
      if ((*it) > max_val)  { max_val = *it; }

      ++it;
      }
    }
  else
    {
    // We can do direct access of the values, row_indices, and col_ptrs.
    // We don't need the location of the max value, so we can just call out to
    // other functions...
    max_val = op_max::direct_max(P.get_values(), n_nonzero);
    }

  if(n_elem == n_nonzero)
    {
    return max_val;
    }
  else
    {
    return std::max(eT(0), max_val);
    }
  }



template<typename T1>
inline
typename arma_not_cx<typename T1::elem_type>::result
spop_max::max_with_index(const SpProxy<T1>& P, uword& index_of_max_val)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword n_elem    = P.get_n_elem();
  const uword n_nonzero = P.get_n_nonzero();
  const uword n_rows    = P.get_n_rows();

  if(n_elem == 0)
    {
    arma_debug_check(true, "max(): object has no elements");

    index_of_max_val = uword(0);

    return Datum<eT>::nan;
    }

  eT max_val = priv::most_neg<eT>();

  if(SpProxy<T1>::use_iterator)
    {
    // We have to iterate over the elements.
    typedef typename SpProxy<T1>::const_iterator_type it_type;

    it_type it     = P.begin();
    it_type it_end = P.end();

    while (it != it_end)
      {
      if ((*it) > max_val)
        {
        max_val = *it;
        index_of_max_val = it.row() + it.col() * n_rows;
        }

      ++it;
      }
    }
  else
    {
    // We can do direct access.
    max_val = op_max::direct_max(P.get_values(), n_nonzero, index_of_max_val);

    // Convert to actual position in matrix.
    const uword row = P.get_row_indices()[index_of_max_val];
    uword col = 0;
    while (P.get_col_ptrs()[++col] <= index_of_max_val) { }
    index_of_max_val = (col - 1) * n_rows + row;
    }


  if(n_elem != n_nonzero)
    {
    max_val = std::max(eT(0), max_val);

    // If the max_val is a nonzero element, we need its actual position in the matrix.
    if(max_val == eT(0))
      {
      // Find first zero element.
      uword last_row = 0;
      uword last_col = 0;

      typedef typename SpProxy<T1>::const_iterator_type it_type;

      it_type it     = P.begin();
      it_type it_end = P.end();

      while (it != it_end)
        {
        // Have we moved more than one position from the last place?
        if ((it.col() == last_col) && (it.row() - last_row > 1))
          {
          index_of_max_val = it.col() * n_rows + last_row + 1;
          break;
          }
        else if ((it.col() >= last_col + 1) && (last_row < n_rows - 1))
          {
          index_of_max_val = last_col * n_rows + last_row + 1;
          break;
          }
        else if ((it.col() == last_col + 1) && (it.row() > 0))
          {
          index_of_max_val = it.col() * n_rows;
          break;
          }
        else if (it.col() > last_col + 1)
          {
          index_of_max_val = (last_col + 1) * n_rows;
          break;
          }

        last_row = it.row();
        last_col = it.col();
        ++it;
        }
      }
    }

  return max_val;
  }



template<typename T1>
inline
void
spop_max::apply_proxy
  (
        SpMat<typename T1::elem_type>& out,
  const SpProxy<T1>&                   p,
  const uword                          dim,
  const typename arma_cx_only<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result  T;

  typename SpProxy<T1>::const_iterator_type it     = p.begin();
  typename SpProxy<T1>::const_iterator_type it_end = p.end();

  const uword p_n_cols = p.get_n_cols();
  const uword p_n_rows = p.get_n_rows();

  if(dim == 0) // find the maximum in each column
    {
    Row<eT> rawval(p_n_cols, fill::zeros);
    Row< T> absval(p_n_cols, fill::zeros);

    while(it != it_end)
      {
      const uword col = it.col();

      const eT& v = (*it);
      const  T  a = std::abs(v);

      if(a > absval[col])
        {
        absval[col] = a;
        rawval[col] = v;
        }

      ++it;
      }

    out = rawval;
    }
  else
  if(dim == 1)  // find the maximum in each row
    {
    Col<eT> rawval(p_n_rows, fill::zeros);
    Col< T> absval(p_n_rows, fill::zeros);

    while(it != it_end)
      {
      const uword row = it.row();

      const eT& v = (*it);
      const  T  a = std::abs(v);

      if(a > absval[row])
        {
        absval[row] = a;
        rawval[row] = v;
        }

      ++it;
      }

    out = rawval;
    }
  }



template<typename T1>
inline
typename T1::elem_type
spop_max::vector_max
  (
  const T1& x,
  const typename arma_cx_only<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result  T;

  const SpProxy<T1> p(x);

  if(p.get_n_elem() == 0)
    {
    arma_debug_check(true, "max(): object has no elements");

    return Datum<eT>::nan;
    }

  if(p.get_n_nonzero() == 0)  { return eT(0); }

  if(SpProxy<T1>::use_iterator == false)
    {
    // direct access of values
    if(p.get_n_nonzero() == p.get_n_elem())
      {
      return op_max::direct_max(p.get_values(), p.get_n_nonzero());
      }
    else
      {
      const eT val1 = eT(0);
      const eT val2 = op_max::direct_max(p.get_values(), p.get_n_nonzero());

      return ( std::abs(val1) >= std::abs(val2) ) ? val1 : val2;
      }
    }
  else
    {
    // use iterator
    typename SpProxy<T1>::const_iterator_type it     = p.begin();
    typename SpProxy<T1>::const_iterator_type it_end = p.end();

    eT best_val_orig = *it;
     T best_val_abs  = std::abs(best_val_orig);

    ++it;

    while(it != it_end)
      {
      eT val_orig = *it;
       T val_abs  = std::abs(val_orig);

      if(val_abs > best_val_abs)
        {
        best_val_abs  = val_abs;
        best_val_orig = val_orig;
        }

      ++it;
      }

    if(p.get_n_nonzero() == p.get_n_elem())
      {
      return best_val_orig;
      }
    else
      {
      const eT val1 = eT(0);

      return ( std::abs(val1) >= best_val_abs ) ? val1 : best_val_orig;
      }
    }
  }



template<typename T1>
inline
typename arma_cx_only<typename T1::elem_type>::result
spop_max::max(const SpBase<typename T1::elem_type, T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result  T;

  const SpProxy<T1> P(X.get_ref());

  const uword n_elem    = P.get_n_elem();
  const uword n_nonzero = P.get_n_nonzero();

  if(n_elem == 0)
    {
    arma_debug_check(true, "max(): object has no elements");

    return Datum<eT>::nan;
    }

   T max_val = priv::most_neg<T>();
  eT ret_val;

  if(SpProxy<T1>::use_iterator)
    {
    // We have to iterate over the elements.
    typedef typename SpProxy<T1>::const_iterator_type it_type;

    it_type it     = P.begin();
    it_type it_end = P.end();

    while (it != it_end)
      {
      const T tmp_val = std::abs(*it);

      if (tmp_val > max_val)
        {
        max_val = tmp_val;
        ret_val = *it;
        }

      ++it;
      }
    }
  else
    {
    // We can do direct access of the values, row_indices, and col_ptrs.
    // We don't need the location of the max value, so we can just call out to
    // other functions...
    ret_val = op_max::direct_max(P.get_values(), n_nonzero);
    max_val = std::abs(ret_val);
    }

  if(n_elem == n_nonzero)
    {
    return max_val;
    }
  else
    {
    return (T(0) > max_val) ? eT(0) : ret_val;
    }
  }



template<typename T1>
inline
typename arma_cx_only<typename T1::elem_type>::result
spop_max::max_with_index(const SpProxy<T1>& P, uword& index_of_max_val)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type            eT;
  typedef typename get_pod_type<eT>::result  T;

  const uword n_elem    = P.get_n_elem();
  const uword n_nonzero = P.get_n_nonzero();
  const uword n_rows    = P.get_n_rows();

  if(n_elem == 0)
    {
    arma_debug_check(true, "max(): object has no elements");

    index_of_max_val = uword(0);

    return Datum<eT>::nan;
    }

  T max_val = priv::most_neg<T>();

  if(SpProxy<T1>::use_iterator)
    {
    // We have to iterate over the elements.
    typedef typename SpProxy<T1>::const_iterator_type it_type;

    it_type it     = P.begin();
    it_type it_end = P.end();

    while (it != it_end)
      {
      const T tmp_val = std::abs(*it);

      if (tmp_val > max_val)
        {
                 max_val = tmp_val;
        index_of_max_val = it.row() + it.col() * n_rows;
        }

      ++it;
      }
    }
  else
    {
    // We can do direct access.
    max_val = std::abs(op_max::direct_max(P.get_values(), n_nonzero, index_of_max_val));

    // Convert to actual position in matrix.
    const uword row = P.get_row_indices()[index_of_max_val];
    uword col = 0;
    while (P.get_col_ptrs()[++col] <= index_of_max_val) { }
    index_of_max_val = (col - 1) * n_rows + row;
    }


  if(n_elem != n_nonzero)
    {
    max_val = std::max(T(0), max_val);

    // If the max_val is a nonzero element, we need its actual position in the matrix.
    if(max_val == T(0))
      {
      // Find first zero element.
      uword last_row = 0;
      uword last_col = 0;

      typedef typename SpProxy<T1>::const_iterator_type it_type;

      it_type it     = P.begin();
      it_type it_end = P.end();

      while (it != it_end)
        {
        // Have we moved more than one position from the last place?
        if ((it.col() == last_col) && (it.row() - last_row > 1))
          {
          index_of_max_val = it.col() * n_rows + last_row + 1;
          break;
          }
        else if ((it.col() >= last_col + 1) && (last_row < n_rows - 1))
          {
          index_of_max_val = last_col * n_rows + last_row + 1;
          break;
          }
        else if ((it.col() == last_col + 1) && (it.row() > 0))
          {
          index_of_max_val = it.col() * n_rows;
          break;
          }
        else if (it.col() > last_col + 1)
          {
          index_of_max_val = (last_col + 1) * n_rows;
          break;
          }

        last_row = it.row();
        last_col = it.col();
        ++it;
        }
      }
    }

  return P[index_of_max_val];
  }



//! @}
