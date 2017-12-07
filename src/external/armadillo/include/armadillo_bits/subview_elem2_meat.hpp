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


//! \addtogroup subview_elem2
//! @{


template<typename eT, typename T1, typename T2>
inline
subview_elem2<eT,T1,T2>::~subview_elem2()
  {
  arma_extra_debug_sigprint();
  }


template<typename eT, typename T1, typename T2>
arma_inline
subview_elem2<eT,T1,T2>::subview_elem2
  (
  const Mat<eT>&        in_m,
  const Base<uword,T1>& in_ri,
  const Base<uword,T2>& in_ci,
  const bool            in_all_rows,
  const bool            in_all_cols
  )
  : m        (in_m       )
  , base_ri  (in_ri      )
  , base_ci  (in_ci      )
  , all_rows (in_all_rows)
  , all_cols (in_all_cols)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT, typename T1, typename T2>
template<typename op_type>
inline
void
subview_elem2<eT,T1,T2>::inplace_op(const eT val)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& m_local = const_cast< Mat<eT>& >(m);

  const uword m_n_rows = m_local.n_rows;
  const uword m_n_cols = m_local.n_cols;

  if( (all_rows == false) && (all_cols == false) )
    {
    const unwrap_check_mixed<T1> tmp1(base_ri.get_ref(), m_local);
    const unwrap_check_mixed<T2> tmp2(base_ci.get_ref(), m_local);

    const umat& ri = tmp1.M;
    const umat& ci = tmp2.M;

    arma_debug_check
      (
      ( ((ri.is_vec() == false) && (ri.is_empty() == false)) || ((ci.is_vec() == false) && (ci.is_empty() == false)) ),
      "Mat::elem(): given object is not a vector"
      );

    const uword* ri_mem    = ri.memptr();
    const uword  ri_n_elem = ri.n_elem;

    const uword* ci_mem    = ci.memptr();
    const uword  ci_n_elem = ci.n_elem;

    for(uword ci_count=0; ci_count < ci_n_elem; ++ci_count)
      {
      const uword col = ci_mem[ci_count];

      arma_debug_check( (col >= m_n_cols), "Mat::elem(): index out of bounds" );

      for(uword ri_count=0; ri_count < ri_n_elem; ++ri_count)
        {
        const uword row = ri_mem[ri_count];

        arma_debug_check( (row >= m_n_rows), "Mat::elem(): index out of bounds" );

        if(is_same_type<op_type, op_internal_equ  >::yes) { m_local.at(row,col)  = val; }
        if(is_same_type<op_type, op_internal_plus >::yes) { m_local.at(row,col) += val; }
        if(is_same_type<op_type, op_internal_minus>::yes) { m_local.at(row,col) -= val; }
        if(is_same_type<op_type, op_internal_schur>::yes) { m_local.at(row,col) *= val; }
        if(is_same_type<op_type, op_internal_div  >::yes) { m_local.at(row,col) /= val; }
        }
      }
    }
  else
  if( (all_rows == true) && (all_cols == false) )
    {
    const unwrap_check_mixed<T2> tmp2(base_ci.get_ref(), m_local);

    const umat& ci = tmp2.M;

    arma_debug_check
      (
      ( (ci.is_vec() == false) && (ci.is_empty() == false) ),
      "Mat::elem(): given object is not a vector"
      );

    const uword* ci_mem    = ci.memptr();
    const uword  ci_n_elem = ci.n_elem;

    for(uword ci_count=0; ci_count < ci_n_elem; ++ci_count)
      {
      const uword col = ci_mem[ci_count];

      arma_debug_check( (col >= m_n_cols), "Mat::elem(): index out of bounds" );

      eT* colptr = m_local.colptr(col);

      if(is_same_type<op_type, op_internal_equ  >::yes) { arrayops::inplace_set  (colptr, val, m_n_rows); }
      if(is_same_type<op_type, op_internal_plus >::yes) { arrayops::inplace_plus (colptr, val, m_n_rows); }
      if(is_same_type<op_type, op_internal_minus>::yes) { arrayops::inplace_minus(colptr, val, m_n_rows); }
      if(is_same_type<op_type, op_internal_schur>::yes) { arrayops::inplace_mul  (colptr, val, m_n_rows); }
      if(is_same_type<op_type, op_internal_div  >::yes) { arrayops::inplace_div  (colptr, val, m_n_rows); }
      }
    }
  else
  if( (all_rows == false) && (all_cols == true) )
    {
    const unwrap_check_mixed<T1> tmp1(base_ri.get_ref(), m_local);

    const umat& ri = tmp1.M;

    arma_debug_check
      (
      ( (ri.is_vec() == false) && (ri.is_empty() == false) ),
      "Mat::elem(): given object is not a vector"
      );

    const uword* ri_mem    = ri.memptr();
    const uword  ri_n_elem = ri.n_elem;

    for(uword col=0; col < m_n_cols; ++col)
      {
      for(uword ri_count=0; ri_count < ri_n_elem; ++ri_count)
        {
        const uword row = ri_mem[ri_count];

        arma_debug_check( (row >= m_n_rows), "Mat::elem(): index out of bounds" );

        if(is_same_type<op_type, op_internal_equ  >::yes) { m_local.at(row,col)  = val; }
        if(is_same_type<op_type, op_internal_plus >::yes) { m_local.at(row,col) += val; }
        if(is_same_type<op_type, op_internal_minus>::yes) { m_local.at(row,col) -= val; }
        if(is_same_type<op_type, op_internal_schur>::yes) { m_local.at(row,col) *= val; }
        if(is_same_type<op_type, op_internal_div  >::yes) { m_local.at(row,col) /= val; }
        }
      }
    }
  }



template<typename eT, typename T1, typename T2>
template<typename op_type, typename expr>
inline
void
subview_elem2<eT,T1,T2>::inplace_op(const Base<eT,expr>& x)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& m_local = const_cast< Mat<eT>& >(m);

  const uword m_n_rows = m_local.n_rows;
  const uword m_n_cols = m_local.n_cols;

  const unwrap_check<expr> tmp(x.get_ref(), m_local);
  const Mat<eT>& X       = tmp.M;

  if( (all_rows == false) && (all_cols == false) )
    {
    const unwrap_check_mixed<T1> tmp1(base_ri.get_ref(), m_local);
    const unwrap_check_mixed<T2> tmp2(base_ci.get_ref(), m_local);

    const umat& ri = tmp1.M;
    const umat& ci = tmp2.M;

    arma_debug_check
      (
      ( ((ri.is_vec() == false) && (ri.is_empty() == false)) || ((ci.is_vec() == false) && (ci.is_empty() == false)) ),
      "Mat::elem(): given object is not a vector"
      );

    const uword* ri_mem    = ri.memptr();
    const uword  ri_n_elem = ri.n_elem;

    const uword* ci_mem    = ci.memptr();
    const uword  ci_n_elem = ci.n_elem;

    arma_debug_assert_same_size( ri_n_elem, ci_n_elem, X.n_rows, X.n_cols, "Mat::elem()" );

    for(uword ci_count=0; ci_count < ci_n_elem; ++ci_count)
      {
      const uword col = ci_mem[ci_count];

      arma_debug_check( (col >= m_n_cols), "Mat::elem(): index out of bounds" );

      for(uword ri_count=0; ri_count < ri_n_elem; ++ri_count)
        {
        const uword row = ri_mem[ri_count];

        arma_debug_check( (row >= m_n_rows), "Mat::elem(): index out of bounds" );

        if(is_same_type<op_type, op_internal_equ  >::yes) { m_local.at(row,col)  = X.at(ri_count, ci_count); }
        if(is_same_type<op_type, op_internal_plus >::yes) { m_local.at(row,col) += X.at(ri_count, ci_count); }
        if(is_same_type<op_type, op_internal_minus>::yes) { m_local.at(row,col) -= X.at(ri_count, ci_count); }
        if(is_same_type<op_type, op_internal_schur>::yes) { m_local.at(row,col) *= X.at(ri_count, ci_count); }
        if(is_same_type<op_type, op_internal_div  >::yes) { m_local.at(row,col) /= X.at(ri_count, ci_count); }
        }
      }
    }
  else
  if( (all_rows == true) && (all_cols == false) )
    {
    const unwrap_check_mixed<T2> tmp2(base_ci.get_ref(), m_local);

    const umat& ci = tmp2.M;

    arma_debug_check
      (
      ( (ci.is_vec() == false) && (ci.is_empty() == false) ),
      "Mat::elem(): given object is not a vector"
      );

    const uword* ci_mem    = ci.memptr();
    const uword  ci_n_elem = ci.n_elem;

    arma_debug_assert_same_size( m_n_rows, ci_n_elem, X.n_rows, X.n_cols, "Mat::elem()" );

    for(uword ci_count=0; ci_count < ci_n_elem; ++ci_count)
      {
      const uword col = ci_mem[ci_count];

      arma_debug_check( (col >= m_n_cols), "Mat::elem(): index out of bounds" );

            eT* m_colptr = m_local.colptr(col);
      const eT* X_colptr = X.colptr(ci_count);

      if(is_same_type<op_type, op_internal_equ  >::yes) { arrayops::copy         (m_colptr, X_colptr, m_n_rows); }
      if(is_same_type<op_type, op_internal_plus >::yes) { arrayops::inplace_plus (m_colptr, X_colptr, m_n_rows); }
      if(is_same_type<op_type, op_internal_minus>::yes) { arrayops::inplace_minus(m_colptr, X_colptr, m_n_rows); }
      if(is_same_type<op_type, op_internal_schur>::yes) { arrayops::inplace_mul  (m_colptr, X_colptr, m_n_rows); }
      if(is_same_type<op_type, op_internal_div  >::yes) { arrayops::inplace_div  (m_colptr, X_colptr, m_n_rows); }
      }
    }
  else
  if( (all_rows == false) && (all_cols == true) )
    {
    const unwrap_check_mixed<T1> tmp1(base_ri.get_ref(), m_local);

    const umat& ri = tmp1.M;

    arma_debug_check
      (
      ( (ri.is_vec() == false) && (ri.is_empty() == false) ),
      "Mat::elem(): given object is not a vector"
      );

    const uword* ri_mem    = ri.memptr();
    const uword  ri_n_elem = ri.n_elem;

    arma_debug_assert_same_size( ri_n_elem, m_n_cols, X.n_rows, X.n_cols, "Mat::elem()" );

    for(uword col=0; col < m_n_cols; ++col)
      {
      for(uword ri_count=0; ri_count < ri_n_elem; ++ri_count)
        {
        const uword row = ri_mem[ri_count];

        arma_debug_check( (row >= m_n_rows), "Mat::elem(): index out of bounds" );

        if(is_same_type<op_type, op_internal_equ  >::yes) { m_local.at(row,col)  = X.at(ri_count, col); }
        if(is_same_type<op_type, op_internal_plus >::yes) { m_local.at(row,col) += X.at(ri_count, col); }
        if(is_same_type<op_type, op_internal_minus>::yes) { m_local.at(row,col) -= X.at(ri_count, col); }
        if(is_same_type<op_type, op_internal_schur>::yes) { m_local.at(row,col) *= X.at(ri_count, col); }
        if(is_same_type<op_type, op_internal_div  >::yes) { m_local.at(row,col) /= X.at(ri_count, col); }
        }
      }
    }
  }



//
//



template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_equ>(val);
  }



template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::zeros()
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_equ>(eT(0));
  }



template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::ones()
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_equ>(eT(1));
  }



template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::operator+= (const eT val)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_plus>(val);
  }



template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::operator-= (const eT val)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_minus>(val);
  }



template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::operator*= (const eT val)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_schur>(val);
  }



template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::operator/= (const eT val)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_div>(val);
  }



//
//



template<typename eT, typename T1, typename T2>
template<typename T3, typename T4>
inline
void
subview_elem2<eT,T1,T2>::operator_equ(const subview_elem2<eT,T3,T4>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_equ>(x);
  }




template<typename eT, typename T1, typename T2>
template<typename T3, typename T4>
inline
void
subview_elem2<eT,T1,T2>::operator= (const subview_elem2<eT,T3,T4>& x)
  {
  arma_extra_debug_sigprint();

  (*this).operator_equ(x);
  }



//! work around compiler bugs
template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::operator= (const subview_elem2<eT,T1,T2>& x)
  {
  arma_extra_debug_sigprint();

  (*this).operator_equ(x);
  }



template<typename eT, typename T1, typename T2>
template<typename T3, typename T4>
inline
void
subview_elem2<eT,T1,T2>::operator+= (const subview_elem2<eT,T3,T4>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_plus>(x);
  }



template<typename eT, typename T1, typename T2>
template<typename T3, typename T4>
inline
void
subview_elem2<eT,T1,T2>::operator-= (const subview_elem2<eT,T3,T4>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_minus>(x);
  }



template<typename eT, typename T1, typename T2>
template<typename T3, typename T4>
inline
void
subview_elem2<eT,T1,T2>::operator%= (const subview_elem2<eT,T3,T4>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_schur>(x);
  }



template<typename eT, typename T1, typename T2>
template<typename T3, typename T4>
inline
void
subview_elem2<eT,T1,T2>::operator/= (const subview_elem2<eT,T3,T4>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_div>(x);
  }



template<typename eT, typename T1, typename T2>
template<typename expr>
inline
void
subview_elem2<eT,T1,T2>::operator= (const Base<eT,expr>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_equ>(x);
  }



template<typename eT, typename T1, typename T2>
template<typename expr>
inline
void
subview_elem2<eT,T1,T2>::operator+= (const Base<eT,expr>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_plus>(x);
  }



template<typename eT, typename T1, typename T2>
template<typename expr>
inline
void
subview_elem2<eT,T1,T2>::operator-= (const Base<eT,expr>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_minus>(x);
  }



template<typename eT, typename T1, typename T2>
template<typename expr>
inline
void
subview_elem2<eT,T1,T2>::operator%= (const Base<eT,expr>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_schur>(x);
  }



template<typename eT, typename T1, typename T2>
template<typename expr>
inline
void
subview_elem2<eT,T1,T2>::operator/= (const Base<eT,expr>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_div>(x);
  }



//
//



template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::extract(Mat<eT>& actual_out, const subview_elem2<eT,T1,T2>& in)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& m_local = const_cast< Mat<eT>& >(in.m);

  const uword m_n_rows = m_local.n_rows;
  const uword m_n_cols = m_local.n_cols;

  const bool alias = (&actual_out == &m_local);

  if(alias)  { arma_extra_debug_print("subview_elem2::extract(): aliasing detected"); }

  Mat<eT>* tmp_out = alias ? new Mat<eT>() : 0;
  Mat<eT>& out     = alias ? *tmp_out      : actual_out;

  if( (in.all_rows == false) && (in.all_cols == false) )
    {
    const unwrap_check_mixed<T1> tmp1(in.base_ri.get_ref(), actual_out);
    const unwrap_check_mixed<T2> tmp2(in.base_ci.get_ref(), actual_out);

    const umat& ri = tmp1.M;
    const umat& ci = tmp2.M;

    arma_debug_check
      (
      ( ((ri.is_vec() == false) && (ri.is_empty() == false)) || ((ci.is_vec() == false) && (ci.is_empty() == false)) ),
      "Mat::elem(): given object is not a vector"
      );

    const uword* ri_mem    = ri.memptr();
    const uword  ri_n_elem = ri.n_elem;

    const uword* ci_mem    = ci.memptr();
    const uword  ci_n_elem = ci.n_elem;

    out.set_size(ri_n_elem, ci_n_elem);

    eT*   out_mem   = out.memptr();
    uword out_count = 0;

    for(uword ci_count=0; ci_count < ci_n_elem; ++ci_count)
      {
      const uword col = ci_mem[ci_count];

      arma_debug_check( (col >= m_n_cols), "Mat::elem(): index out of bounds" );

      for(uword ri_count=0; ri_count < ri_n_elem; ++ri_count)
        {
        const uword row = ri_mem[ri_count];

        arma_debug_check( (row >= m_n_rows), "Mat::elem(): index out of bounds" );

        out_mem[out_count] = m_local.at(row,col);
        ++out_count;
        }
      }
    }
  else
  if( (in.all_rows == true) && (in.all_cols == false) )
    {
    const unwrap_check_mixed<T2> tmp2(in.base_ci.get_ref(), m_local);

    const umat& ci = tmp2.M;

    arma_debug_check
      (
      ( (ci.is_vec() == false) && (ci.is_empty() == false) ),
      "Mat::elem(): given object is not a vector"
      );

    const uword* ci_mem    = ci.memptr();
    const uword  ci_n_elem = ci.n_elem;

    out.set_size(m_n_rows, ci_n_elem);

    for(uword ci_count=0; ci_count < ci_n_elem; ++ci_count)
      {
      const uword col = ci_mem[ci_count];

      arma_debug_check( (col >= m_n_cols), "Mat::elem(): index out of bounds" );

      arrayops::copy( out.colptr(ci_count), m_local.colptr(col), m_n_rows );
      }
    }
  else
  if( (in.all_rows == false) && (in.all_cols == true) )
    {
    const unwrap_check_mixed<T1> tmp1(in.base_ri.get_ref(), m_local);

    const umat& ri = tmp1.M;

    arma_debug_check
      (
      ( (ri.is_vec() == false) && (ri.is_empty() == false) ),
      "Mat::elem(): given object is not a vector"
      );

    const uword* ri_mem    = ri.memptr();
    const uword  ri_n_elem = ri.n_elem;

    out.set_size(ri_n_elem, m_n_cols);

    for(uword col=0; col < m_n_cols; ++col)
      {
      for(uword ri_count=0; ri_count < ri_n_elem; ++ri_count)
        {
        const uword row = ri_mem[ri_count];

        arma_debug_check( (row >= m_n_rows), "Mat::elem(): index out of bounds" );

        out.at(ri_count,col) = m_local.at(row,col);
        }
      }
    }


  if(alias)
    {
    actual_out.steal_mem(out);

    delete tmp_out;
    }
  }



// TODO: implement a dedicated function instead of creating a temporary (but lots of potential aliasing issues)
template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::plus_inplace(Mat<eT>& out, const subview_elem2& in)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> tmp(in);

  out += tmp;
  }



template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::minus_inplace(Mat<eT>& out, const subview_elem2& in)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> tmp(in);

  out -= tmp;
  }



template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::schur_inplace(Mat<eT>& out, const subview_elem2& in)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> tmp(in);

  out %= tmp;
  }



template<typename eT, typename T1, typename T2>
inline
void
subview_elem2<eT,T1,T2>::div_inplace(Mat<eT>& out, const subview_elem2& in)
  {
  arma_extra_debug_sigprint();

  const Mat<eT> tmp(in);

  out /= tmp;
  }



//! @}
