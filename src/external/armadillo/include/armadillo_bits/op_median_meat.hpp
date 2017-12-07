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


//! \addtogroup op_median
//! @{



//! \brief
//! For each row or for each column, find the median value.
//! The result is stored in a dense matrix that has either one column or one row.
//! The dimension, for which the medians are found, is set via the median() function.
template<typename T1>
inline
void
op_median::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_median>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 1), "median(): parameter 'dim' must be 0 or 1" );

  const Proxy<T1> P(in.m);

  typedef typename Proxy<T1>::stored_type P_stored_type;

  const bool is_alias = P.is_alias(out);

  if( (is_Mat<P_stored_type>::value == true) || is_alias )
    {
    const unwrap_check<P_stored_type> tmp(P.Q, is_alias);

    const typename unwrap_check<P_stored_type>::stored_type& X = tmp.M;

    const uword X_n_rows = X.n_rows;
    const uword X_n_cols = X.n_cols;

    if(dim == 0)  // in each column
      {
      arma_extra_debug_print("op_median::apply(): dim = 0");

      out.set_size((X_n_rows > 0) ? 1 : 0, X_n_cols);

      if(X_n_rows > 0)
        {
        std::vector<eT> tmp_vec(X_n_rows);

        for(uword col=0; col < X_n_cols; ++col)
          {
          arrayops::copy( &(tmp_vec[0]), X.colptr(col), X_n_rows );

          out[col] = op_median::direct_median(tmp_vec);
          }
        }
      }
    else  // in each row
      {
      arma_extra_debug_print("op_median::apply(): dim = 1");

      out.set_size(X_n_rows, (X_n_cols > 0) ? 1 : 0);

      if(X_n_cols > 0)
        {
        std::vector<eT> tmp_vec(X_n_cols);

        for(uword row=0; row < X_n_rows; ++row)
          {
          for(uword col=0; col < X_n_cols; ++col)  { tmp_vec[col] = X.at(row,col); }

          out[row] = op_median::direct_median(tmp_vec);
          }
        }
      }
    }
  else
    {
    const uword P_n_rows = P.get_n_rows();
    const uword P_n_cols = P.get_n_cols();

    if(dim == 0)  // in each column
      {
      arma_extra_debug_print("op_median::apply(): dim = 0");

      out.set_size((P_n_rows > 0) ? 1 : 0, P_n_cols);

      if(P_n_rows > 0)
        {
        std::vector<eT> tmp_vec(P_n_rows);

        for(uword col=0; col < P_n_cols; ++col)
          {
          for(uword row=0; row < P_n_rows; ++row)  { tmp_vec[row] = P.at(row,col); }

          out[col] = op_median::direct_median(tmp_vec);
          }
        }
      }
    else  // in each row
      {
      arma_extra_debug_print("op_median::apply(): dim = 1");

      out.set_size(P_n_rows, (P_n_cols > 0) ? 1 : 0);

      if(P_n_cols > 0)
        {
        std::vector<eT> tmp_vec(P_n_cols);

        for(uword row=0; row < P_n_rows; ++row)
          {
          for(uword col=0; col < P_n_cols; ++col)  { tmp_vec[col] = P.at(row,col); }

          out[row] = op_median::direct_median(tmp_vec);
          }
        }
      }
    }
  }



//! Implementation for complex numbers
template<typename T, typename T1>
inline
void
op_median::apply(Mat< std::complex<T> >& out, const Op<T1,op_median>& in)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT;

  arma_type_check(( is_same_type<eT, typename T1::elem_type>::no ));

  const unwrap_check<T1> tmp(in.m, out);
  const Mat<eT>&     X = tmp.M;

  const uword X_n_rows = X.n_rows;
  const uword X_n_cols = X.n_cols;

  const uword dim = in.aux_uword_a;
  arma_debug_check( (dim > 1), "median(): parameter 'dim' must be 0 or 1" );

  if(dim == 0)  // in each column
    {
    arma_extra_debug_print("op_median::apply(): dim = 0");

    out.set_size((X_n_rows > 0) ? 1 : 0, X_n_cols);

    if(X_n_rows > 0)
      {
      std::vector< arma_cx_median_packet<T> > tmp_vec(X_n_rows);

      for(uword col=0; col<X_n_cols; ++col)
        {
        const eT* colmem = X.colptr(col);

        for(uword row=0; row<X_n_rows; ++row)
          {
          tmp_vec[row].val   = std::abs(colmem[row]);
          tmp_vec[row].index = row;
          }

        uword index1;
        uword index2;
        op_median::direct_cx_median_index(index1, index2, tmp_vec);

        out[col] = op_mean::robust_mean(colmem[index1], colmem[index2]);
        }
      }
    }
  else
  if(dim == 1)  // in each row
    {
    arma_extra_debug_print("op_median::apply(): dim = 1");

    out.set_size(X_n_rows, (X_n_cols > 0) ? 1 : 0);

    if(X_n_cols > 0)
      {
      std::vector< arma_cx_median_packet<T> > tmp_vec(X_n_cols);

      for(uword row=0; row<X_n_rows; ++row)
        {
        for(uword col=0; col<X_n_cols; ++col)
          {
          tmp_vec[col].val   = std::abs(X.at(row,col));
          tmp_vec[col].index = col;
          }

        uword index1;
        uword index2;
        op_median::direct_cx_median_index(index1, index2, tmp_vec);

        out[row] = op_mean::robust_mean( X.at(row,index1), X.at(row,index2) );
        }
      }
    }
  }



template<typename T1>
inline
typename T1::elem_type
op_median::median_vec
  (
  const T1& X,
  const typename arma_not_cx<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;

  typedef typename Proxy<T1>::stored_type P_stored_type;

  const Proxy<T1> P(X);

  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)
    {
    arma_debug_check(true, "median(): object has no elements");

    return Datum<eT>::nan;
    }

  std::vector<eT> tmp_vec(n_elem);

  if(is_Mat<P_stored_type>::value == true)
    {
    const unwrap<P_stored_type> tmp(P.Q);

    const typename unwrap<P_stored_type>::stored_type& Y = tmp.M;

    arrayops::copy( &(tmp_vec[0]), Y.memptr(), n_elem );
    }
  else
    {
    if(Proxy<T1>::use_at == false)
      {
      typedef typename Proxy<T1>::ea_type ea_type;

      ea_type A = P.get_ea();

      for(uword i=0; i<n_elem; ++i)  { tmp_vec[i] = A[i]; }
      }
    else
      {
      const uword n_rows = P.get_n_rows();
      const uword n_cols = P.get_n_cols();

      if(n_cols == 1)
        {
        for(uword row=0; row < n_rows; ++row)  { tmp_vec[row] = P.at(row,0); }
        }
      else
      if(n_rows == 1)
        {
        for(uword col=0; col < n_cols; ++col)  { tmp_vec[col] = P.at(0,col); }
        }
      else
        {
        arma_stop_logic_error("op_median::median_vec(): expected a vector" );
        }
      }
    }

  return op_median::direct_median(tmp_vec);
  }



template<typename T1>
inline
typename T1::elem_type
op_median::median_vec
  (
  const T1& X,
  const typename arma_cx_only<typename T1::elem_type>::result* junk
  )
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  typedef typename T1::elem_type eT;
  typedef typename T1::pod_type   T;

  const Proxy<T1> P(X);

  const uword n_elem = P.get_n_elem();

  if(n_elem == 0)
    {
    arma_debug_check(true, "median(): object has no elements");

    return Datum<eT>::nan;
    }

  std::vector< arma_cx_median_packet<T> > tmp_vec(n_elem);

  if(Proxy<T1>::use_at == false)
    {
    typedef typename Proxy<T1>::ea_type ea_type;

    ea_type A = P.get_ea();

    for(uword i=0; i<n_elem; ++i)
      {
      tmp_vec[i].val   = std::abs( A[i] );
      tmp_vec[i].index = i;
      }

    uword index1;
    uword index2;
    op_median::direct_cx_median_index(index1, index2, tmp_vec);

    return op_mean::robust_mean( A[index1], A[index2] );
    }
  else
    {
    const uword n_rows = P.get_n_rows();
    const uword n_cols = P.get_n_cols();

    if(n_cols == 1)
      {
      for(uword row=0; row < n_rows; ++row)
        {
        tmp_vec[row].val   = std::abs( P.at(row,0) );
        tmp_vec[row].index = row;
        }

      uword index1;
      uword index2;
      op_median::direct_cx_median_index(index1, index2, tmp_vec);

      return op_mean::robust_mean( P.at(index1,0), P.at(index2,0) );
      }
    else
    if(n_rows == 1)
      {
      for(uword col=0; col < n_cols; ++col)
        {
        tmp_vec[col].val   = std::abs( P.at(0,col) );
        tmp_vec[col].index = col;
        }

      uword index1;
      uword index2;
      op_median::direct_cx_median_index(index1, index2, tmp_vec);

      return op_mean::robust_mean( P.at(0,index1), P.at(0,index2) );
      }
    else
      {
      arma_stop_logic_error("op_median::median_vec(): expected a vector" );

      return eT(0);
      }
    }
  }



//! find the median value of a std::vector (contents is modified)
template<typename eT>
inline
eT
op_median::direct_median(std::vector<eT>& X)
  {
  arma_extra_debug_sigprint();

  const uword n_elem = uword(X.size());
  const uword half   = n_elem/2;

  typename std::vector<eT>::iterator first    = X.begin();
  typename std::vector<eT>::iterator nth      = first + half;
  typename std::vector<eT>::iterator pastlast = X.end();

  std::nth_element(first, nth, pastlast);

  if((n_elem % 2) == 0)  // even number of elements
    {
    typename std::vector<eT>::iterator start   = X.begin();
    typename std::vector<eT>::iterator pastend = start + half;

    const eT val1 = (*nth);
    const eT val2 = (*(std::max_element(start, pastend)));

    return op_mean::robust_mean(val1, val2);
    }
  else  // odd number of elements
    {
    return (*nth);
    }
  }



template<typename T>
inline
void
op_median::direct_cx_median_index
  (
  uword& out_index1,
  uword& out_index2,
  std::vector< arma_cx_median_packet<T> >& X
  )
  {
  arma_extra_debug_sigprint();

  typedef arma_cx_median_packet<T> eT;

  const uword n_elem = uword(X.size());
  const uword half   = n_elem/2;

  typename std::vector<eT>::iterator first    = X.begin();
  typename std::vector<eT>::iterator nth      = first + half;
  typename std::vector<eT>::iterator pastlast = X.end();

  std::nth_element(first, nth, pastlast);

  out_index1 = (*nth).index;

  if((n_elem % 2) == 0)  // even number of elements
    {
    typename std::vector<eT>::iterator start   = X.begin();
    typename std::vector<eT>::iterator pastend = start + half;

    out_index2 = (*(std::max_element(start, pastend))).index;
    }
  else  // odd number of elements
    {
    out_index2 = out_index1;
    }
  }



//! @}
