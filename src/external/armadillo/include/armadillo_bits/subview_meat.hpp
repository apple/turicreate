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


//! \addtogroup subview
//! @{


template<typename eT>
inline
subview<eT>::~subview()
  {
  arma_extra_debug_sigprint();
  }


template<typename eT>
inline
subview<eT>::subview(const Mat<eT>& in_m, const uword in_row1, const uword in_col1, const uword in_n_rows, const uword in_n_cols)
  : m(in_m)
  , aux_row1(in_row1)
  , aux_col1(in_col1)
  , n_rows(in_n_rows)
  , n_cols(in_n_cols)
  , n_elem(in_n_rows*in_n_cols)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
void
subview<eT>::operator= (const eT val)
  {
  arma_extra_debug_sigprint();

  if(n_elem != 1)
    {
    arma_debug_assert_same_size(n_rows, n_cols, 1, 1, "copy into submatrix");
    }

  Mat<eT>& X = const_cast< Mat<eT>& >(m);

  X.at(aux_row1, aux_col1) = val;
  }



template<typename eT>
template<typename op_type>
inline
void
subview<eT>::inplace_op(const eT val)
  {
  arma_extra_debug_sigprint();

  subview<eT>& s = *this;

  const uword s_n_rows = s.n_rows;
  const uword s_n_cols = s.n_cols;

  if(s_n_rows == 1)
    {
    Mat<eT>& A = const_cast< Mat<eT>& >(s.m);

    const uword A_n_rows = A.n_rows;

    eT* Aptr = &(A.at(s.aux_row1,s.aux_col1));

    uword jj;
    for(jj=1; jj < s_n_cols; jj+=2)
      {
      if(is_same_type<op_type, op_internal_plus >::yes)  { (*Aptr) += val; Aptr += A_n_rows;  (*Aptr) += val; Aptr += A_n_rows; }
      if(is_same_type<op_type, op_internal_minus>::yes)  { (*Aptr) -= val; Aptr += A_n_rows;  (*Aptr) -= val; Aptr += A_n_rows; }
      if(is_same_type<op_type, op_internal_schur>::yes)  { (*Aptr) *= val; Aptr += A_n_rows;  (*Aptr) *= val; Aptr += A_n_rows; }
      if(is_same_type<op_type, op_internal_div  >::yes)  { (*Aptr) /= val; Aptr += A_n_rows;  (*Aptr) /= val; Aptr += A_n_rows; }
      }

    if((jj-1) < s_n_cols)
      {
      if(is_same_type<op_type, op_internal_plus >::yes)  { (*Aptr) += val; }
      if(is_same_type<op_type, op_internal_minus>::yes)  { (*Aptr) -= val; }
      if(is_same_type<op_type, op_internal_schur>::yes)  { (*Aptr) *= val; }
      if(is_same_type<op_type, op_internal_div  >::yes)  { (*Aptr) /= val; }
      }
    }
  else
    {
    for(uword ucol=0; ucol < s_n_cols; ++ucol)
      {
      if(is_same_type<op_type, op_internal_plus >::yes)  { arrayops::inplace_plus ( colptr(ucol), val, s_n_rows ); }
      if(is_same_type<op_type, op_internal_minus>::yes)  { arrayops::inplace_minus( colptr(ucol), val, s_n_rows ); }
      if(is_same_type<op_type, op_internal_schur>::yes)  { arrayops::inplace_mul  ( colptr(ucol), val, s_n_rows ); }
      if(is_same_type<op_type, op_internal_div  >::yes)  { arrayops::inplace_div  ( colptr(ucol), val, s_n_rows ); }
      }
    }
  }



template<typename eT>
template<typename op_type, typename T1>
inline
void
subview<eT>::inplace_op(const Base<eT,T1>& in, const char* identifier)
  {
  arma_extra_debug_sigprint();

  const Proxy<T1> P(in.get_ref());

  subview<eT>& s = *this;

  const uword s_n_rows = s.n_rows;
  const uword s_n_cols = s.n_cols;

  arma_debug_assert_same_size(s, P, identifier);

  const bool use_mp   = arma_config::cxx11 && arma_config::openmp && Proxy<T1>::use_mp && mp_gate<eT>::eval(s.n_elem);
  const bool is_alias = P.is_alias(s.m);

  if(is_alias)  { arma_extra_debug_print("aliasing detected"); }

  if( (is_Mat<typename Proxy<T1>::stored_type>::value) || (use_mp) || (is_alias) )
    {
    const unwrap_check<typename Proxy<T1>::stored_type> tmp(P.Q, is_alias);
    const Mat<eT>& B = tmp.M;

    if(s_n_rows == 1)
      {
      Mat<eT>& A = const_cast< Mat<eT>& >(m);

      const uword A_n_rows = A.n_rows;

            eT* Aptr = &(A.at(aux_row1,aux_col1));
      const eT* Bptr = B.memptr();

      uword jj;
      for(jj=1; jj < s_n_cols; jj+=2)
        {
        const eT tmp1 = (*Bptr);  Bptr++;
        const eT tmp2 = (*Bptr);  Bptr++;

        if(is_same_type<op_type, op_internal_equ  >::yes)  { (*Aptr) =  tmp1; Aptr += A_n_rows;  (*Aptr) =  tmp2; Aptr += A_n_rows; }
        if(is_same_type<op_type, op_internal_plus >::yes)  { (*Aptr) += tmp1; Aptr += A_n_rows;  (*Aptr) += tmp2; Aptr += A_n_rows; }
        if(is_same_type<op_type, op_internal_minus>::yes)  { (*Aptr) -= tmp1; Aptr += A_n_rows;  (*Aptr) -= tmp2; Aptr += A_n_rows; }
        if(is_same_type<op_type, op_internal_schur>::yes)  { (*Aptr) *= tmp1; Aptr += A_n_rows;  (*Aptr) *= tmp2; Aptr += A_n_rows; }
        if(is_same_type<op_type, op_internal_div  >::yes)  { (*Aptr) /= tmp1; Aptr += A_n_rows;  (*Aptr) /= tmp2; Aptr += A_n_rows; }
        }

      if((jj-1) < s_n_cols)
        {
        if(is_same_type<op_type, op_internal_equ  >::yes)  { (*Aptr) =  (*Bptr); }
        if(is_same_type<op_type, op_internal_plus >::yes)  { (*Aptr) += (*Bptr); }
        if(is_same_type<op_type, op_internal_minus>::yes)  { (*Aptr) -= (*Bptr); }
        if(is_same_type<op_type, op_internal_schur>::yes)  { (*Aptr) *= (*Bptr); }
        if(is_same_type<op_type, op_internal_div  >::yes)  { (*Aptr) /= (*Bptr); }
        }
      }
    else  // not a row vector
      {
      for(uword ucol=0; ucol < s_n_cols; ++ucol)
        {
        if(is_same_type<op_type, op_internal_equ  >::yes)  { arrayops::copy         ( s.colptr(ucol), B.colptr(ucol), s_n_rows ); }
        if(is_same_type<op_type, op_internal_plus >::yes)  { arrayops::inplace_plus ( s.colptr(ucol), B.colptr(ucol), s_n_rows ); }
        if(is_same_type<op_type, op_internal_minus>::yes)  { arrayops::inplace_minus( s.colptr(ucol), B.colptr(ucol), s_n_rows ); }
        if(is_same_type<op_type, op_internal_schur>::yes)  { arrayops::inplace_mul  ( s.colptr(ucol), B.colptr(ucol), s_n_rows ); }
        if(is_same_type<op_type, op_internal_div  >::yes)  { arrayops::inplace_div  ( s.colptr(ucol), B.colptr(ucol), s_n_rows ); }
        }
      }
    }
  else  // use the Proxy
    {
    if(s_n_rows == 1)
      {
      Mat<eT>& A = const_cast< Mat<eT>& >(m);

      const uword A_n_rows = A.n_rows;

      eT* Aptr = &(A.at(aux_row1,aux_col1));

      uword jj;
      for(jj=1; jj < s_n_cols; jj+=2)
        {
        const uword ii = (jj-1);

        const eT tmp1 = (Proxy<T1>::use_at) ? P.at(0,ii) : P[ii];
        const eT tmp2 = (Proxy<T1>::use_at) ? P.at(0,jj) : P[jj];

        if(is_same_type<op_type, op_internal_equ  >::yes)  { (*Aptr) =  tmp1; Aptr += A_n_rows;  (*Aptr) =  tmp2; Aptr += A_n_rows; }
        if(is_same_type<op_type, op_internal_plus >::yes)  { (*Aptr) += tmp1; Aptr += A_n_rows;  (*Aptr) += tmp2; Aptr += A_n_rows; }
        if(is_same_type<op_type, op_internal_minus>::yes)  { (*Aptr) -= tmp1; Aptr += A_n_rows;  (*Aptr) -= tmp2; Aptr += A_n_rows; }
        if(is_same_type<op_type, op_internal_schur>::yes)  { (*Aptr) *= tmp1; Aptr += A_n_rows;  (*Aptr) *= tmp2; Aptr += A_n_rows; }
        if(is_same_type<op_type, op_internal_div  >::yes)  { (*Aptr) /= tmp1; Aptr += A_n_rows;  (*Aptr) /= tmp2; Aptr += A_n_rows; }
        }

      const uword ii = (jj-1);
      if(ii < s_n_cols)
        {
        if(is_same_type<op_type, op_internal_equ  >::yes)  { (*Aptr) =  (Proxy<T1>::use_at) ? P.at(0,ii) : P[ii]; }
        if(is_same_type<op_type, op_internal_plus >::yes)  { (*Aptr) += (Proxy<T1>::use_at) ? P.at(0,ii) : P[ii]; }
        if(is_same_type<op_type, op_internal_minus>::yes)  { (*Aptr) -= (Proxy<T1>::use_at) ? P.at(0,ii) : P[ii]; }
        if(is_same_type<op_type, op_internal_schur>::yes)  { (*Aptr) *= (Proxy<T1>::use_at) ? P.at(0,ii) : P[ii]; }
        if(is_same_type<op_type, op_internal_div  >::yes)  { (*Aptr) /= (Proxy<T1>::use_at) ? P.at(0,ii) : P[ii]; }
        }
      }
    else  // not a row vector
      {
      if(Proxy<T1>::use_at)
        {
        for(uword ucol=0; ucol < s_n_cols; ++ucol)
          {
          eT* s_col_data = s.colptr(ucol);

          uword jj;
          for(jj=1; jj < s_n_rows; jj+=2)
            {
            const uword ii = (jj-1);

            const eT tmp1 = P.at(ii,ucol);
            const eT tmp2 = P.at(jj,ucol);

            if(is_same_type<op_type, op_internal_equ  >::yes)  { (*s_col_data) =  tmp1; s_col_data++;  (*s_col_data) =  tmp2; s_col_data++; }
            if(is_same_type<op_type, op_internal_plus >::yes)  { (*s_col_data) += tmp1; s_col_data++;  (*s_col_data) += tmp2; s_col_data++; }
            if(is_same_type<op_type, op_internal_minus>::yes)  { (*s_col_data) -= tmp1; s_col_data++;  (*s_col_data) -= tmp2; s_col_data++; }
            if(is_same_type<op_type, op_internal_schur>::yes)  { (*s_col_data) *= tmp1; s_col_data++;  (*s_col_data) *= tmp2; s_col_data++; }
            if(is_same_type<op_type, op_internal_div  >::yes)  { (*s_col_data) /= tmp1; s_col_data++;  (*s_col_data) /= tmp2; s_col_data++; }
            }

          const uword ii = (jj-1);
          if(ii < s_n_rows)
            {
            if(is_same_type<op_type, op_internal_equ  >::yes)  { (*s_col_data) =  P.at(ii,ucol); }
            if(is_same_type<op_type, op_internal_plus >::yes)  { (*s_col_data) += P.at(ii,ucol); }
            if(is_same_type<op_type, op_internal_minus>::yes)  { (*s_col_data) -= P.at(ii,ucol); }
            if(is_same_type<op_type, op_internal_schur>::yes)  { (*s_col_data) *= P.at(ii,ucol); }
            if(is_same_type<op_type, op_internal_div  >::yes)  { (*s_col_data) /= P.at(ii,ucol); }
            }
          }
        }
      else
        {
        typename Proxy<T1>::ea_type Pea = P.get_ea();

        uword count = 0;

        for(uword ucol=0; ucol < s_n_cols; ++ucol)
          {
          eT* s_col_data = s.colptr(ucol);

          uword jj;
          for(jj=1; jj < s_n_rows; jj+=2)
            {
            const eT tmp1 = Pea[count];  count++;
            const eT tmp2 = Pea[count];  count++;

            if(is_same_type<op_type, op_internal_equ  >::yes)  { (*s_col_data) =  tmp1; s_col_data++;  (*s_col_data) =  tmp2; s_col_data++; }
            if(is_same_type<op_type, op_internal_plus >::yes)  { (*s_col_data) += tmp1; s_col_data++;  (*s_col_data) += tmp2; s_col_data++; }
            if(is_same_type<op_type, op_internal_minus>::yes)  { (*s_col_data) -= tmp1; s_col_data++;  (*s_col_data) -= tmp2; s_col_data++; }
            if(is_same_type<op_type, op_internal_schur>::yes)  { (*s_col_data) *= tmp1; s_col_data++;  (*s_col_data) *= tmp2; s_col_data++; }
            if(is_same_type<op_type, op_internal_div  >::yes)  { (*s_col_data) /= tmp1; s_col_data++;  (*s_col_data) /= tmp2; s_col_data++; }
            }

          if((jj-1) < s_n_rows)
            {
            if(is_same_type<op_type, op_internal_equ  >::yes)  { (*s_col_data) =  Pea[count];  count++; }
            if(is_same_type<op_type, op_internal_plus >::yes)  { (*s_col_data) += Pea[count];  count++; }
            if(is_same_type<op_type, op_internal_minus>::yes)  { (*s_col_data) -= Pea[count];  count++; }
            if(is_same_type<op_type, op_internal_schur>::yes)  { (*s_col_data) *= Pea[count];  count++; }
            if(is_same_type<op_type, op_internal_div  >::yes)  { (*s_col_data) /= Pea[count];  count++; }
            }
          }
        }
      }
    }
  }



template<typename eT>
template<typename op_type>
inline
void
subview<eT>::inplace_op(const subview<eT>& x, const char* identifier)
  {
  arma_extra_debug_sigprint();

  if(check_overlap(x))
    {
    const Mat<eT> tmp(x);

    if(is_same_type<op_type, op_internal_equ  >::yes)  { (*this).operator= (tmp); }
    if(is_same_type<op_type, op_internal_plus >::yes)  { (*this).operator+=(tmp); }
    if(is_same_type<op_type, op_internal_minus>::yes)  { (*this).operator-=(tmp); }
    if(is_same_type<op_type, op_internal_schur>::yes)  { (*this).operator%=(tmp); }
    if(is_same_type<op_type, op_internal_div  >::yes)  { (*this).operator/=(tmp); }

    return;
    }

  subview<eT>& s = *this;

  arma_debug_assert_same_size(s, x, identifier);

  const uword s_n_cols = s.n_cols;
  const uword s_n_rows = s.n_rows;

  if(s_n_rows == 1)
    {
          Mat<eT>& A = const_cast< Mat<eT>& >(s.m);
    const Mat<eT>& B = x.m;

    const uword A_n_rows = A.n_rows;
    const uword B_n_rows = B.n_rows;

          eT* Aptr = &(A.at(s.aux_row1,s.aux_col1));
    const eT* Bptr = &(B.at(x.aux_row1,x.aux_col1));

    uword jj;
    for(jj=1; jj < s_n_cols; jj+=2)
      {
      const eT tmp1 = (*Bptr);  Bptr += B_n_rows;
      const eT tmp2 = (*Bptr);  Bptr += B_n_rows;

      if(is_same_type<op_type, op_internal_equ  >::yes)  { (*Aptr) =  tmp1; Aptr += A_n_rows;  (*Aptr) =  tmp2; Aptr += A_n_rows; }
      if(is_same_type<op_type, op_internal_plus >::yes)  { (*Aptr) += tmp1; Aptr += A_n_rows;  (*Aptr) += tmp2; Aptr += A_n_rows; }
      if(is_same_type<op_type, op_internal_minus>::yes)  { (*Aptr) -= tmp1; Aptr += A_n_rows;  (*Aptr) -= tmp2; Aptr += A_n_rows; }
      if(is_same_type<op_type, op_internal_schur>::yes)  { (*Aptr) *= tmp1; Aptr += A_n_rows;  (*Aptr) *= tmp2; Aptr += A_n_rows; }
      if(is_same_type<op_type, op_internal_div  >::yes)  { (*Aptr) /= tmp1; Aptr += A_n_rows;  (*Aptr) /= tmp2; Aptr += A_n_rows; }
      }

    if((jj-1) < s_n_cols)
      {
      if(is_same_type<op_type, op_internal_equ  >::yes)  { (*Aptr) =  (*Bptr); }
      if(is_same_type<op_type, op_internal_plus >::yes)  { (*Aptr) += (*Bptr); }
      if(is_same_type<op_type, op_internal_minus>::yes)  { (*Aptr) -= (*Bptr); }
      if(is_same_type<op_type, op_internal_schur>::yes)  { (*Aptr) *= (*Bptr); }
      if(is_same_type<op_type, op_internal_div  >::yes)  { (*Aptr) /= (*Bptr); }
      }
    }
  else
    {
    for(uword ucol=0; ucol < s_n_cols; ++ucol)
      {
      if(is_same_type<op_type, op_internal_equ  >::yes)  { arrayops::copy         ( s.colptr(ucol), x.colptr(ucol), s_n_rows ); }
      if(is_same_type<op_type, op_internal_plus >::yes)  { arrayops::inplace_plus ( s.colptr(ucol), x.colptr(ucol), s_n_rows ); }
      if(is_same_type<op_type, op_internal_minus>::yes)  { arrayops::inplace_minus( s.colptr(ucol), x.colptr(ucol), s_n_rows ); }
      if(is_same_type<op_type, op_internal_schur>::yes)  { arrayops::inplace_mul  ( s.colptr(ucol), x.colptr(ucol), s_n_rows ); }
      if(is_same_type<op_type, op_internal_div  >::yes)  { arrayops::inplace_div  ( s.colptr(ucol), x.colptr(ucol), s_n_rows ); }
      }
    }
  }



template<typename eT>
inline
void
subview<eT>::operator+= (const eT val)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_plus>(val);
  }



template<typename eT>
inline
void
subview<eT>::operator-= (const eT val)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_minus>(val);
  }



template<typename eT>
inline
void
subview<eT>::operator*= (const eT val)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_schur>(val);
  }



template<typename eT>
inline
void
subview<eT>::operator/= (const eT val)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_div>(val);
  }



template<typename eT>
inline
void
subview<eT>::operator= (const subview<eT>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_equ>(x, "copy into submatrix");
  }



template<typename eT>
inline
void
subview<eT>::operator+= (const subview<eT>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_plus>(x, "addition");
  }



template<typename eT>
inline
void
subview<eT>::operator-= (const subview<eT>& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_minus>(x, "subtraction");
  }



template<typename eT>
inline
void
subview<eT>::operator%= (const subview& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_schur>(x, "element-wise multiplication");
  }



template<typename eT>
inline
void
subview<eT>::operator/= (const subview& x)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_div>(x, "element-wise division");
  }



template<typename eT>
template<typename T1>
inline
void
subview<eT>::operator= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_equ>(in, "copy into submatrix");
  }



template<typename eT>
template<typename T1>
inline
void
subview<eT>::operator+= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_plus>(in, "addition");
  }



template<typename eT>
template<typename T1>
inline
void
subview<eT>::operator-= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_minus>(in, "subtraction");
  }



template<typename eT>
template<typename T1>
inline
void
subview<eT>::operator%= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_schur>(in, "element-wise multiplication");
  }



template<typename eT>
template<typename T1>
inline
void
subview<eT>::operator/= (const Base<eT,T1>& in)
  {
  arma_extra_debug_sigprint();

  inplace_op<op_internal_div>(in, "element-wise division");
  }



template<typename eT>
template<typename T1>
inline
void
subview<eT>::operator=(const SpBase<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> p(x.get_ref());

  arma_debug_assert_same_size(n_rows, n_cols, p.get_n_rows(), p.get_n_cols(), "copy into submatrix");

  // Clear the subview.
  zeros();

  // Iterate through the sparse subview and set the nonzero values appropriately.
  typename SpProxy<T1>::const_iterator_type cit     = p.begin();
  typename SpProxy<T1>::const_iterator_type cit_end = p.end();

  while(cit != cit_end)
    {
    at(cit.row(), cit.col()) = *cit;
    ++cit;
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview<eT>::operator+=(const SpBase<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> p(x.get_ref());

  arma_debug_assert_same_size(n_rows, n_cols, p.get_n_rows(), p.get_n_cols(), "addition");

  // Iterate through the sparse subview and add its values.
  typename SpProxy<T1>::const_iterator_type cit     = p.begin();
  typename SpProxy<T1>::const_iterator_type cit_end = p.end();

  while(cit != cit_end)
    {
    at(cit.row(), cit.col()) += *cit;
    ++cit;
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview<eT>::operator-=(const SpBase<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> p(x.get_ref());

  arma_debug_assert_same_size(n_rows, n_cols, p.get_n_rows(), p.get_n_cols(), "subtraction");

  // Iterate through the sparse subview and subtract its values.
  typename SpProxy<T1>::const_iterator_type cit     = p.begin();
  typename SpProxy<T1>::const_iterator_type cit_end = p.end();

  while(cit != cit_end)
    {
    at(cit.row(), cit.col()) -= *cit;
    ++cit;
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview<eT>::operator%=(const SpBase<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  const uword s_n_rows = (*this).n_rows;
  const uword s_n_cols = (*this).n_cols;

  const SpProxy<T1> p(x.get_ref());

  arma_debug_assert_same_size(s_n_rows, s_n_cols, p.get_n_rows(), p.get_n_cols(), "element-wise multiplication");

  if(n_elem == 0)  { return; }

  if(p.get_n_nonzero() == 0)  { (*this).zeros(); return; }

  // Iterate over nonzero values.
  // Any zero values in the sparse expression will result in a zero in our subview.
  typename SpProxy<T1>::const_iterator_type cit     = p.begin();
  typename SpProxy<T1>::const_iterator_type cit_end = p.end();

  uword r = 0;
  uword c = 0;

  while(cit != cit_end)
    {
    const uword cit_row = cit.row();
    const uword cit_col = cit.col();

    while( ((r == cit_row) && (c == cit_col)) == false )
      {
      at(r,c) = eT(0);

      r++;  if(r >= s_n_rows)  { r = 0; c++; }
      }

    at(r, c) *= (*cit);

    ++cit;
    r++;  if(r >= s_n_rows)  { r = 0; c++; }
    }
  }



template<typename eT>
template<typename T1>
inline
void
subview<eT>::operator/=(const SpBase<eT, T1>& x)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> p(x.get_ref());

  arma_debug_assert_same_size(n_rows, n_cols, p.get_n_rows(), p.get_n_cols(), "element-wise division");

  // This is probably going to fill your subview with a bunch of NaNs,
  // so I'm not going to bother to implement it fast.
  // You can have slow NaNs.  They're fine too.
  for (uword c = 0; c < n_cols; ++c)
  for (uword r = 0; r < n_rows; ++r)
    {
    at(r, c) /= p.at(r, c);
    }
  }



template<typename eT>
template<typename T1, typename gen_type>
inline
typename enable_if2< is_same_type<typename T1::elem_type, eT>::value, void>::result
subview<eT>::operator= (const Gen<T1,gen_type>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(n_rows, n_cols, in.n_rows, in.n_cols, "copy into submatrix");

  in.apply(*this);
  }



//! apply a functor to each element
template<typename eT>
template<typename functor>
inline
void
subview<eT>::for_each(functor F)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& X = const_cast< Mat<eT>& >(m);

  if(n_rows == 1)
    {
    const uword urow          = aux_row1;
    const uword start_col     = aux_col1;
    const uword end_col_plus1 = start_col + n_cols;

    for(uword ucol = start_col; ucol < end_col_plus1; ++ucol)
      {
      F( X.at(urow, ucol) );
      }
    }
  else
    {
    const uword start_col = aux_col1;
    const uword start_row = aux_row1;

    const uword end_col_plus1 = start_col + n_cols;
    const uword end_row_plus1 = start_row + n_rows;

    for(uword ucol = start_col; ucol < end_col_plus1; ++ucol)
    for(uword urow = start_row; urow < end_row_plus1; ++urow)
      {
      F( X.at(urow, ucol) );
      }
    }
  }



template<typename eT>
template<typename functor>
inline
void
subview<eT>::for_each(functor F) const
  {
  arma_extra_debug_sigprint();

  const Mat<eT>& X = m;

  if(n_rows == 1)
    {
    const uword urow          = aux_row1;
    const uword start_col     = aux_col1;
    const uword end_col_plus1 = start_col + n_cols;

    for(uword ucol = start_col; ucol < end_col_plus1; ++ucol)
      {
      F( X.at(urow, ucol) );
      }
    }
  else
    {
    const uword start_col = aux_col1;
    const uword start_row = aux_row1;

    const uword end_col_plus1 = start_col + n_cols;
    const uword end_row_plus1 = start_row + n_rows;

    for(uword ucol = start_col; ucol < end_col_plus1; ++ucol)
    for(uword urow = start_row; urow < end_row_plus1; ++urow)
      {
      F( X.at(urow, ucol) );
      }
    }
  }



//! transform each element in the subview using a functor
template<typename eT>
template<typename functor>
inline
void
subview<eT>::transform(functor F)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& X = const_cast< Mat<eT>& >(m);

  if(n_rows == 1)
    {
    const uword urow          = aux_row1;
    const uword start_col     = aux_col1;
    const uword end_col_plus1 = start_col + n_cols;

    for(uword ucol = start_col; ucol < end_col_plus1; ++ucol)
      {
      X.at(urow, ucol) = eT( F( X.at(urow, ucol) ) );
      }
    }
  else
    {
    const uword start_col = aux_col1;
    const uword start_row = aux_row1;

    const uword end_col_plus1 = start_col + n_cols;
    const uword end_row_plus1 = start_row + n_rows;

    for(uword ucol = start_col; ucol < end_col_plus1; ++ucol)
    for(uword urow = start_row; urow < end_row_plus1; ++urow)
      {
      X.at(urow, ucol) = eT( F( X.at(urow, ucol) ) );
      }
    }
  }



//! imbue (fill) the subview with values provided by a functor
template<typename eT>
template<typename functor>
inline
void
subview<eT>::imbue(functor F)
  {
  arma_extra_debug_sigprint();

  Mat<eT>& X = const_cast< Mat<eT>& >(m);

  if(n_rows == 1)
    {
    const uword urow          = aux_row1;
    const uword start_col     = aux_col1;
    const uword end_col_plus1 = start_col + n_cols;

    for(uword ucol = start_col; ucol < end_col_plus1; ++ucol)
      {
      X.at(urow, ucol) = eT( F() );
      }
    }
  else
    {
    const uword start_col = aux_col1;
    const uword start_row = aux_row1;

    const uword end_col_plus1 = start_col + n_cols;
    const uword end_row_plus1 = start_row + n_rows;

    for(uword ucol = start_col; ucol < end_col_plus1; ++ucol)
    for(uword urow = start_row; urow < end_row_plus1; ++urow)
      {
      X.at(urow, ucol) = eT( F() );
      }
    }
  }



template<typename eT>
inline
void
subview<eT>::replace(const eT old_val, const eT new_val)
  {
  arma_extra_debug_sigprint();

  subview<eT>& s = *this;

  const uword s_n_cols = s.n_cols;
  const uword s_n_rows = s.n_rows;

  if(s_n_rows == 1)
    {
    Mat<eT>& A = const_cast< Mat<eT>& >(s.m);

    const uword A_n_rows = A.n_rows;

    eT* Aptr = &(A.at(s.aux_row1,s.aux_col1));

    if(arma_isnan(old_val))
      {
      for(uword ucol=0; ucol < s_n_cols; ++ucol)
        {
        (*Aptr) = (arma_isnan(*Aptr)) ? new_val : (*Aptr);

        Aptr += A_n_rows;
        }
      }
    else
      {
      for(uword ucol=0; ucol < s_n_cols; ++ucol)
        {
        (*Aptr) = ((*Aptr) == old_val) ? new_val : (*Aptr);

        Aptr += A_n_rows;
        }
      }
    }
  else
    {
    for(uword ucol=0; ucol < s_n_cols; ++ucol)
      {
      arrayops::replace(s.colptr(ucol), s_n_rows, old_val, new_val);
      }
    }
  }



template<typename eT>
inline
void
subview<eT>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  subview<eT>& s = *this;

  const uword s_n_cols = s.n_cols;
  const uword s_n_rows = s.n_rows;

  if(s_n_rows == 1)
    {
    Mat<eT>& A = const_cast< Mat<eT>& >(s.m);

    const uword A_n_rows = A.n_rows;

    eT* Aptr = &(A.at(s.aux_row1,s.aux_col1));

    uword jj;
    for(jj=1; jj < s_n_cols; jj+=2)
      {
      (*Aptr) = val;  Aptr += A_n_rows;
      (*Aptr) = val;  Aptr += A_n_rows;
      }

    if((jj-1) < s_n_cols)
      {
      (*Aptr) = val;
      }
    }
  else
    {
    for(uword ucol=0; ucol < s_n_cols; ++ucol)
      {
      arrayops::inplace_set( s.colptr(ucol), val, s_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview<eT>::zeros()
  {
  arma_extra_debug_sigprint();

  (*this).fill(eT(0));
  }



template<typename eT>
inline
void
subview<eT>::ones()
  {
  arma_extra_debug_sigprint();

  (*this).fill(eT(1));
  }



template<typename eT>
inline
void
subview<eT>::eye()
  {
  arma_extra_debug_sigprint();

  (*this).zeros();

  const uword N = (std::min)(n_rows, n_cols);

  for(uword ii=0; ii < N; ++ii)
    {
    at(ii,ii) = eT(1);
    }
  }



template<typename eT>
inline
void
subview<eT>::randu()
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows = n_rows;
  const uword local_n_cols = n_cols;

  if(local_n_rows == 1)
    {
    for(uword ii=0; ii < local_n_cols; ++ii)
      {
      at(0,ii) = eT(arma_rng::randu<eT>());
      }
    }
  else
    {
    for(uword ii=0; ii < local_n_cols; ++ii)
      {
      arma_rng::randu<eT>::fill( colptr(ii), local_n_rows );
      }
    }
  }



template<typename eT>
inline
void
subview<eT>::randn()
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows = n_rows;
  const uword local_n_cols = n_cols;

  if(local_n_rows == 1)
    {
    for(uword ii=0; ii < local_n_cols; ++ii)
      {
      at(0,ii) = eT(arma_rng::randn<eT>());
      }
    }
  else
    {
    for(uword ii=0; ii < local_n_cols; ++ii)
      {
      arma_rng::randn<eT>::fill( colptr(ii), local_n_rows );
      }
    }
  }



template<typename eT>
inline
eT
subview<eT>::at_alt(const uword ii) const
  {
  return operator[](ii);
  }



template<typename eT>
inline
eT&
subview<eT>::operator[](const uword ii)
  {
  const uword in_col = ii / n_rows;
  const uword in_row = ii % n_rows;

  const uword index = (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return access::rw( (const_cast< Mat<eT>& >(m)).mem[index] );
  }



template<typename eT>
inline
eT
subview<eT>::operator[](const uword ii) const
  {
  const uword in_col = ii / n_rows;
  const uword in_row = ii % n_rows;

  const uword index = (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return m.mem[index];
  }



template<typename eT>
inline
eT&
subview<eT>::operator()(const uword ii)
  {
  arma_debug_check( (ii >= n_elem), "subview::operator(): index out of bounds");

  const uword in_col = ii / n_rows;
  const uword in_row = ii % n_rows;

  const uword index = (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return access::rw( (const_cast< Mat<eT>& >(m)).mem[index] );
  }



template<typename eT>
inline
eT
subview<eT>::operator()(const uword ii) const
  {
  arma_debug_check( (ii >= n_elem), "subview::operator(): index out of bounds");

  const uword in_col = ii / n_rows;
  const uword in_row = ii % n_rows;

  const uword index = (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return m.mem[index];
  }



template<typename eT>
inline
eT&
subview<eT>::operator()(const uword in_row, const uword in_col)
  {
  arma_debug_check( ((in_row >= n_rows) || (in_col >= n_cols)), "subview::operator(): index out of bounds");

  const uword index = (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return access::rw( (const_cast< Mat<eT>& >(m)).mem[index] );
  }



template<typename eT>
inline
eT
subview<eT>::operator()(const uword in_row, const uword in_col) const
  {
  arma_debug_check( ((in_row >= n_rows) || (in_col >= n_cols)), "subview::operator(): index out of bounds");

  const uword index = (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return m.mem[index];
  }



template<typename eT>
inline
eT&
subview<eT>::at(const uword in_row, const uword in_col)
  {
  const uword index = (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return access::rw( (const_cast< Mat<eT>& >(m)).mem[index] );
  }



template<typename eT>
inline
eT
subview<eT>::at(const uword in_row, const uword in_col) const
  {
  const uword index = (in_col + aux_col1)*m.n_rows + aux_row1 + in_row;

  return m.mem[index];
  }



template<typename eT>
arma_inline
eT*
subview<eT>::colptr(const uword in_col)
  {
  return & access::rw((const_cast< Mat<eT>& >(m)).mem[ (in_col + aux_col1)*m.n_rows + aux_row1 ]);
  }



template<typename eT>
arma_inline
const eT*
subview<eT>::colptr(const uword in_col) const
  {
  return & m.mem[ (in_col + aux_col1)*m.n_rows + aux_row1 ];
  }



template<typename eT>
inline
bool
subview<eT>::check_overlap(const subview<eT>& x) const
  {
  const subview<eT>& s = *this;

  if(&s.m != &x.m)
    {
    return false;
    }
  else
    {
    if( (s.n_elem == 0) || (x.n_elem == 0) )
      {
      return false;
      }
    else
      {
      const uword s_row_start  = s.aux_row1;
      const uword s_row_end_p1 = s_row_start + s.n_rows;

      const uword s_col_start  = s.aux_col1;
      const uword s_col_end_p1 = s_col_start + s.n_cols;


      const uword x_row_start  = x.aux_row1;
      const uword x_row_end_p1 = x_row_start + x.n_rows;

      const uword x_col_start  = x.aux_col1;
      const uword x_col_end_p1 = x_col_start + x.n_cols;


      const bool outside_rows = ( (x_row_start >= s_row_end_p1) || (s_row_start >= x_row_end_p1) );
      const bool outside_cols = ( (x_col_start >= s_col_end_p1) || (s_col_start >= x_col_end_p1) );

      return ( (outside_rows == false) && (outside_cols == false) );
      }
    }
  }



template<typename eT>
inline
arma_warn_unused
bool
subview<eT>::is_vec() const
  {
  return ( (n_rows == 1) || (n_cols == 1) );
  }



template<typename eT>
inline
arma_warn_unused
bool
subview<eT>::is_finite() const
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows = n_rows;
  const uword local_n_cols = n_cols;

  for(uword ii=0; ii<local_n_cols; ++ii)
    {
    if(arrayops::is_finite(colptr(ii), local_n_rows) == false)  { return false; }
    }

  return true;
  }



template<typename eT>
inline
arma_warn_unused
bool
subview<eT>::has_inf() const
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows = n_rows;
  const uword local_n_cols = n_cols;

  for(uword ii=0; ii<local_n_cols; ++ii)
    {
    if(arrayops::has_inf(colptr(ii), local_n_rows))  { return true; }
    }

  return false;
  }



template<typename eT>
inline
arma_warn_unused
bool
subview<eT>::has_nan() const
  {
  arma_extra_debug_sigprint();

  const uword local_n_rows = n_rows;
  const uword local_n_cols = n_cols;

  for(uword ii=0; ii<local_n_cols; ++ii)
    {
    if(arrayops::has_nan(colptr(ii), local_n_rows))  { return true; }
    }

  return false;
  }



//! X = Y.submat(...)
template<typename eT>
inline
void
subview<eT>::extract(Mat<eT>& out, const subview<eT>& in)
  {
  arma_extra_debug_sigprint();

  // NOTE: we're assuming that the matrix has already been set to the correct size and there is no aliasing;
  // size setting and alias checking is done by either the Mat contructor or operator=()

  const uword n_rows = in.n_rows;  // number of rows in the subview
  const uword n_cols = in.n_cols;  // number of columns in the subview

  arma_extra_debug_print(arma_str::format("out.n_rows = %d   out.n_cols = %d    in.m.n_rows = %d  in.m.n_cols = %d") % out.n_rows % out.n_cols % in.m.n_rows % in.m.n_cols );


  if(in.is_vec() == true)
    {
    if(n_cols == 1)   // a column vector
      {
      arma_extra_debug_print("subview::extract(): copying col (going across rows)");

      // in.colptr(0) the first column of the subview, taking into account any row offset
      arrayops::copy( out.memptr(), in.colptr(0), n_rows );
      }
    else   // a row vector (possibly empty)
      {
      arma_extra_debug_print("subview::extract(): copying row (going across columns)");

      eT* out_mem = out.memptr();

      const uword X_n_rows = in.m.n_rows;

      const eT* Xptr = &(in.m.at(in.aux_row1,in.aux_col1));

      uword j;

      for(j=1; j < n_cols; j+=2)
        {
        const eT tmp1 = (*Xptr);  Xptr += X_n_rows;
        const eT tmp2 = (*Xptr);  Xptr += X_n_rows;

        (*out_mem) = tmp1;  out_mem++;
        (*out_mem) = tmp2;  out_mem++;
        }

      if((j-1) < n_cols)
        {
        (*out_mem) = (*Xptr);
        }
      }
    }
  else   // general submatrix
    {
    arma_extra_debug_print("subview::extract(): general submatrix");

    for(uword col=0; col < n_cols; ++col)
      {
      arrayops::copy( out.colptr(col), in.colptr(col), n_rows );
      }
    }
  }



//! X += Y.submat(...)
template<typename eT>
inline
void
subview<eT>::plus_inplace(Mat<eT>& out, const subview<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out, in, "addition");

  const uword n_rows = in.n_rows;
  const uword n_cols = in.n_cols;

  if(n_rows == 1)
    {
    eT* out_mem = out.memptr();

    const Mat<eT>& X = in.m;

    const uword row       = in.aux_row1;
    const uword start_col = in.aux_col1;

    uword i,j;
    for(i=0, j=1; j < n_cols; i+=2, j+=2)
      {
      const eT tmp1 = X.at(row, start_col+i);
      const eT tmp2 = X.at(row, start_col+j);

      out_mem[i] += tmp1;
      out_mem[j] += tmp2;
      }

    if(i < n_cols)
      {
      out_mem[i] += X.at(row, start_col+i);
      }
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)
      {
      arrayops::inplace_plus(out.colptr(col), in.colptr(col), n_rows);
      }
    }
  }



//! X -= Y.submat(...)
template<typename eT>
inline
void
subview<eT>::minus_inplace(Mat<eT>& out, const subview<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out, in, "subtraction");

  const uword n_rows = in.n_rows;
  const uword n_cols = in.n_cols;

  if(n_rows == 1)
    {
    eT* out_mem = out.memptr();

    const Mat<eT>& X = in.m;

    const uword row       = in.aux_row1;
    const uword start_col = in.aux_col1;

    uword i,j;
    for(i=0, j=1; j < n_cols; i+=2, j+=2)
      {
      const eT tmp1 = X.at(row, start_col+i);
      const eT tmp2 = X.at(row, start_col+j);

      out_mem[i] -= tmp1;
      out_mem[j] -= tmp2;
      }

    if(i < n_cols)
      {
      out_mem[i] -= X.at(row, start_col+i);
      }
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)
      {
      arrayops::inplace_minus(out.colptr(col), in.colptr(col), n_rows);
      }
    }
  }



//! X %= Y.submat(...)
template<typename eT>
inline
void
subview<eT>::schur_inplace(Mat<eT>& out, const subview<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out, in, "element-wise multiplication");

  const uword n_rows = in.n_rows;
  const uword n_cols = in.n_cols;

  if(n_rows == 1)
    {
    eT* out_mem = out.memptr();

    const Mat<eT>& X = in.m;

    const uword row       = in.aux_row1;
    const uword start_col = in.aux_col1;

    uword i,j;
    for(i=0, j=1; j < n_cols; i+=2, j+=2)
      {
      const eT tmp1 = X.at(row, start_col+i);
      const eT tmp2 = X.at(row, start_col+j);

      out_mem[i] *= tmp1;
      out_mem[j] *= tmp2;
      }

    if(i < n_cols)
      {
      out_mem[i] *= X.at(row, start_col+i);
      }
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)
      {
      arrayops::inplace_mul(out.colptr(col), in.colptr(col), n_rows);
      }
    }
  }



//! X /= Y.submat(...)
template<typename eT>
inline
void
subview<eT>::div_inplace(Mat<eT>& out, const subview<eT>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out, in, "element-wise division");

  const uword n_rows = in.n_rows;
  const uword n_cols = in.n_cols;

  if(n_rows == 1)
    {
    eT* out_mem = out.memptr();

    const Mat<eT>& X = in.m;

    const uword row       = in.aux_row1;
    const uword start_col = in.aux_col1;

    uword i,j;
    for(i=0, j=1; j < n_cols; i+=2, j+=2)
      {
      const eT tmp1 = X.at(row, start_col+i);
      const eT tmp2 = X.at(row, start_col+j);

      out_mem[i] /= tmp1;
      out_mem[j] /= tmp2;
      }

    if(i < n_cols)
      {
      out_mem[i] /= X.at(row, start_col+i);
      }
    }
  else
    {
    for(uword col=0; col < n_cols; ++col)
      {
      arrayops::inplace_div(out.colptr(col), in.colptr(col), n_rows);
      }
    }
  }



//! creation of subview (row vector)
template<typename eT>
inline
subview_row<eT>
subview<eT>::row(const uword row_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( row_num >= n_rows, "subview::row(): out of bounds" );

  const uword base_row = aux_row1 + row_num;

  return subview_row<eT>(m, base_row, aux_col1, n_cols);
  }



//! creation of subview (row vector)
template<typename eT>
inline
const subview_row<eT>
subview<eT>::row(const uword row_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( row_num >= n_rows, "subview::row(): out of bounds" );

  const uword base_row = aux_row1 + row_num;

  return subview_row<eT>(m, base_row, aux_col1, n_cols);
  }



template<typename eT>
inline
subview_row<eT>
subview<eT>::operator()(const uword row_num, const span& col_span)
  {
  arma_extra_debug_sigprint();

  const bool col_all = col_span.whole;

  const uword local_n_cols = n_cols;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword submat_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  const uword base_col1     = aux_col1 + in_col1;
  const uword base_row      = aux_row1 + row_num;

  arma_debug_check
    (
    (row_num >= n_rows)
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "subview::operator(): indices out of bounds or incorrectly used"
    );

  return subview_row<eT>(m, base_row, base_col1, submat_n_cols);
  }



template<typename eT>
inline
const subview_row<eT>
subview<eT>::operator()(const uword row_num, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  const bool col_all = col_span.whole;

  const uword local_n_cols = n_cols;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword submat_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  const uword base_col1     = aux_col1 + in_col1;
  const uword base_row      = aux_row1 + row_num;

  arma_debug_check
    (
    (row_num >= n_rows)
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "subview::operator(): indices out of bounds or incorrectly used"
    );

  return subview_row<eT>(m, base_row, base_col1, submat_n_cols);
  }



//! creation of subview (column vector)
template<typename eT>
inline
subview_col<eT>
subview<eT>::col(const uword col_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( col_num >= n_cols, "subview::col(): out of bounds");

  const uword base_col = aux_col1 + col_num;

  return subview_col<eT>(m, base_col, aux_row1, n_rows);
  }



//! creation of subview (column vector)
template<typename eT>
inline
const subview_col<eT>
subview<eT>::col(const uword col_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( col_num >= n_cols, "subview::col(): out of bounds");

  const uword base_col = aux_col1 + col_num;

  return subview_col<eT>(m, base_col, aux_row1, n_rows);
  }



template<typename eT>
inline
subview_col<eT>
subview<eT>::operator()(const span& row_span, const uword col_num)
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;

  const uword local_n_rows = n_rows;

  const uword in_row1       = row_all ? 0            : row_span.a;
  const uword in_row2       =                          row_span.b;
  const uword submat_n_rows = row_all ? local_n_rows : in_row2 - in_row1 + 1;

  const uword base_row1       = aux_row1 + in_row1;
  const uword base_col        = aux_col1 + col_num;

  arma_debug_check
    (
    (col_num >= n_cols)
    ||
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ,
    "subview::operator(): indices out of bounds or incorrectly used"
    );

  return subview_col<eT>(m, base_col, base_row1, submat_n_rows);
  }



template<typename eT>
inline
const subview_col<eT>
subview<eT>::operator()(const span& row_span, const uword col_num) const
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;

  const uword local_n_rows = n_rows;

  const uword in_row1       = row_all ? 0            : row_span.a;
  const uword in_row2       =                          row_span.b;
  const uword submat_n_rows = row_all ? local_n_rows : in_row2 - in_row1 + 1;

  const uword base_row1       = aux_row1 + in_row1;
  const uword base_col        = aux_col1 + col_num;

  arma_debug_check
    (
    (col_num >= n_cols)
    ||
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ,
    "subview::operator(): indices out of bounds or incorrectly used"
    );

  return subview_col<eT>(m, base_col, base_row1, submat_n_rows);
  }



//! create a Col object which uses memory from an existing matrix object.
//! this approach is currently not alias safe
//! and does not take into account that the parent matrix object could be deleted.
//! if deleted memory is accessed by the created Col object,
//! it will cause memory corruption and/or a crash
template<typename eT>
inline
Col<eT>
subview<eT>::unsafe_col(const uword col_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( col_num >= n_cols, "subview::unsafe_col(): out of bounds");

  return Col<eT>(colptr(col_num), n_rows, false, true);
  }



//! create a Col object which uses memory from an existing matrix object.
//! this approach is currently not alias safe
//! and does not take into account that the parent matrix object could be deleted.
//! if deleted memory is accessed by the created Col object,
//! it will cause memory corruption and/or a crash
template<typename eT>
inline
const Col<eT>
subview<eT>::unsafe_col(const uword col_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( col_num >= n_cols, "subview::unsafe_col(): out of bounds");

  return Col<eT>(const_cast<eT*>(colptr(col_num)), n_rows, false, true);
  }



//! creation of subview (submatrix comprised of specified row vectors)
template<typename eT>
inline
subview<eT>
subview<eT>::rows(const uword in_row1, const uword in_row2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_row2 >= n_rows),
    "subview::rows(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_rows = in_row2 - in_row1 + 1;
  const uword base_row1 = aux_row1 + in_row1;

  return subview<eT>(m, base_row1, aux_col1, subview_n_rows, n_cols );
  }



//! creation of subview (submatrix comprised of specified row vectors)
template<typename eT>
inline
const subview<eT>
subview<eT>::rows(const uword in_row1, const uword in_row2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_row2 >= n_rows),
    "subview::rows(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_rows = in_row2 - in_row1 + 1;
  const uword base_row1 = aux_row1 + in_row1;

  return subview<eT>(m, base_row1, aux_col1, subview_n_rows, n_cols );
  }



//! creation of subview (submatrix comprised of specified column vectors)
template<typename eT>
inline
subview<eT>
subview<eT>::cols(const uword in_col1, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_col1 > in_col2) || (in_col2 >= n_cols),
    "subview::cols(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_cols = in_col2 - in_col1 + 1;
  const uword base_col1 = aux_col1 + in_col1;

  return subview<eT>(m, aux_row1, base_col1, n_rows, subview_n_cols);
  }



//! creation of subview (submatrix comprised of specified column vectors)
template<typename eT>
inline
const subview<eT>
subview<eT>::cols(const uword in_col1, const uword in_col2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_col1 > in_col2) || (in_col2 >= n_cols),
    "subview::cols(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_cols = in_col2 - in_col1 + 1;
  const uword base_col1 = aux_col1 + in_col1;

  return subview<eT>(m, aux_row1, base_col1, n_rows, subview_n_cols);
  }



//! creation of subview (submatrix)
template<typename eT>
inline
subview<eT>
subview<eT>::submat(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_col1 >  in_col2) || (in_row2 >= n_rows) || (in_col2 >= n_cols),
    "subview::submat(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_rows = in_row2 - in_row1 + 1;
  const uword subview_n_cols = in_col2 - in_col1 + 1;

  const uword base_row1 = aux_row1 + in_row1;
  const uword base_col1 = aux_col1 + in_col1;

  return subview<eT>(m, base_row1, base_col1, subview_n_rows, subview_n_cols);
  }



//! creation of subview (generic submatrix)
template<typename eT>
inline
const subview<eT>
subview<eT>::submat(const uword in_row1, const uword in_col1, const uword in_row2, const uword in_col2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 > in_row2) || (in_col1 >  in_col2) || (in_row2 >= n_rows) || (in_col2 >= n_cols),
    "subview::submat(): indices out of bounds or incorrectly used"
    );

  const uword subview_n_rows = in_row2 - in_row1 + 1;
  const uword subview_n_cols = in_col2 - in_col1 + 1;

  const uword base_row1 = aux_row1 + in_row1;
  const uword base_col1 = aux_col1 + in_col1;

  return subview<eT>(m, base_row1, base_col1, subview_n_rows, subview_n_cols);
  }



//! creation of subview (submatrix)
template<typename eT>
inline
subview<eT>
subview<eT>::submat(const span& row_span, const span& col_span)
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;
  const bool col_all = col_span.whole;

  const uword local_n_rows = n_rows;
  const uword local_n_cols = n_cols;

  const uword in_row1       = row_all ? 0            : row_span.a;
  const uword in_row2       =                          row_span.b;
  const uword submat_n_rows = row_all ? local_n_rows : in_row2 - in_row1 + 1;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword submat_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check
    (
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "subview::submat(): indices out of bounds or incorrectly used"
    );

  const uword base_row1 = aux_row1 + in_row1;
  const uword base_col1 = aux_col1 + in_col1;

  return subview<eT>(m, base_row1, base_col1, submat_n_rows, submat_n_cols);
  }



//! creation of subview (generic submatrix)
template<typename eT>
inline
const subview<eT>
subview<eT>::submat(const span& row_span, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  const bool row_all = row_span.whole;
  const bool col_all = col_span.whole;

  const uword local_n_rows = n_rows;
  const uword local_n_cols = n_cols;

  const uword in_row1       = row_all ? 0            : row_span.a;
  const uword in_row2       =                          row_span.b;
  const uword submat_n_rows = row_all ? local_n_rows : in_row2 - in_row1 + 1;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword submat_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check
    (
    ( row_all ? false : ((in_row1 > in_row2) || (in_row2 >= local_n_rows)) )
    ||
    ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) )
    ,
    "subview::submat(): indices out of bounds or incorrectly used"
    );

  const uword base_row1 = aux_row1 + in_row1;
  const uword base_col1 = aux_col1 + in_col1;

  return subview<eT>(m, base_row1, base_col1, submat_n_rows, submat_n_cols);
  }



template<typename eT>
inline
subview<eT>
subview<eT>::operator()(const span& row_span, const span& col_span)
  {
  arma_extra_debug_sigprint();

  return (*this).submat(row_span, col_span);
  }



template<typename eT>
inline
const subview<eT>
subview<eT>::operator()(const span& row_span, const span& col_span) const
  {
  arma_extra_debug_sigprint();

  return (*this).submat(row_span, col_span);
  }



template<typename eT>
inline
subview_each1< subview<eT>, 0 >
subview<eT>::each_col()
  {
  arma_extra_debug_sigprint();

  return subview_each1< subview<eT>, 0 >(*this);
  }



template<typename eT>
inline
subview_each1< subview<eT>, 1 >
subview<eT>::each_row()
  {
  arma_extra_debug_sigprint();

  return subview_each1< subview<eT>, 1 >(*this);
  }



template<typename eT>
template<typename T1>
inline
subview_each2< subview<eT>, 0, T1 >
subview<eT>::each_col(const Base<uword,T1>& indices)
  {
  arma_extra_debug_sigprint();

  return subview_each2< subview<eT>, 0, T1 >(*this, indices);
  }



template<typename eT>
template<typename T1>
inline
subview_each2< subview<eT>, 1, T1 >
subview<eT>::each_row(const Base<uword,T1>& indices)
  {
  arma_extra_debug_sigprint();

  return subview_each2< subview<eT>, 1, T1 >(*this, indices);
  }



#if defined(ARMA_USE_CXX11)

  //! apply a lambda function to each column, where each column is interpreted as a column vector
  template<typename eT>
  inline
  void
  subview<eT>::each_col(const std::function< void(Col<eT>&) >& F)
    {
    arma_extra_debug_sigprint();

    for(uword ii=0; ii < n_cols; ++ii)
      {
      Col<eT> tmp(colptr(ii), n_rows, false, true);
      F(tmp);
      }
    }



  template<typename eT>
  inline
  void
  subview<eT>::each_col(const std::function< void(const Col<eT>&) >& F) const
    {
    arma_extra_debug_sigprint();

    for(uword ii=0; ii < n_cols; ++ii)
      {
      const Col<eT> tmp(colptr(ii), n_rows, false, true);
      F(tmp);
      }
    }



  //! apply a lambda function to each row, where each row is interpreted as a row vector
  template<typename eT>
  inline
  void
  subview<eT>::each_row(const std::function< void(Row<eT>&) >& F)
    {
    arma_extra_debug_sigprint();

    podarray<eT> array1(n_cols);
    podarray<eT> array2(n_cols);

    Row<eT> tmp1( array1.memptr(), n_cols, false, true );
    Row<eT> tmp2( array2.memptr(), n_cols, false, true );

    eT* tmp1_mem = tmp1.memptr();
    eT* tmp2_mem = tmp2.memptr();

    uword ii, jj;

    for(ii=0, jj=1; jj < n_rows; ii+=2, jj+=2)
      {
      for(uword col_id = 0; col_id < n_cols; ++col_id)
        {
        const eT* col_mem = colptr(col_id);

        tmp1_mem[col_id] = col_mem[ii];
        tmp2_mem[col_id] = col_mem[jj];
        }

      F(tmp1);
      F(tmp2);

      for(uword col_id = 0; col_id < n_cols; ++col_id)
        {
        eT* col_mem = colptr(col_id);

        col_mem[ii] = tmp1_mem[col_id];
        col_mem[jj] = tmp2_mem[col_id];
        }
      }

    if(ii < n_rows)
      {
      tmp1 = (*this).row(ii);

      F(tmp1);

      (*this).row(ii) = tmp1;
      }
    }



  template<typename eT>
  inline
  void
  subview<eT>::each_row(const std::function< void(const Row<eT>&) >& F) const
    {
    arma_extra_debug_sigprint();

    podarray<eT> array1(n_cols);
    podarray<eT> array2(n_cols);

    Row<eT> tmp1( array1.memptr(), n_cols, false, true );
    Row<eT> tmp2( array2.memptr(), n_cols, false, true );

    eT* tmp1_mem = tmp1.memptr();
    eT* tmp2_mem = tmp2.memptr();

    uword ii, jj;

    for(ii=0, jj=1; jj < n_rows; ii+=2, jj+=2)
      {
      for(uword col_id = 0; col_id < n_cols; ++col_id)
        {
        const eT* col_mem = colptr(col_id);

        tmp1_mem[col_id] = col_mem[ii];
        tmp2_mem[col_id] = col_mem[jj];
        }

      F(tmp1);
      F(tmp2);
      }

    if(ii < n_rows)
      {
      tmp1 = (*this).row(ii);

      F(tmp1);
      }
    }

#endif



//! creation of diagview (diagonal)
template<typename eT>
inline
diagview<eT>
subview<eT>::diag(const sword in_id)
  {
  arma_extra_debug_sigprint();

  const uword row_offset = (in_id < 0) ? uword(-in_id) : 0;
  const uword col_offset = (in_id > 0) ? uword( in_id) : 0;

  arma_debug_check
    (
    ((row_offset > 0) && (row_offset >= n_rows)) || ((col_offset > 0) && (col_offset >= n_cols)),
    "subview::diag(): requested diagonal out of bounds"
    );

  const uword len = (std::min)(n_rows - row_offset, n_cols - col_offset);

  const uword base_row_offset = aux_row1 + row_offset;
  const uword base_col_offset = aux_col1 + col_offset;

  return diagview<eT>(m, base_row_offset, base_col_offset, len);
  }



//! creation of diagview (diagonal)
template<typename eT>
inline
const diagview<eT>
subview<eT>::diag(const sword in_id) const
  {
  arma_extra_debug_sigprint();

  const uword row_offset = uword( (in_id < 0) ? -in_id : 0 );
  const uword col_offset = uword( (in_id > 0) ?  in_id : 0 );

  arma_debug_check
    (
    ((row_offset > 0) && (row_offset >= n_rows)) || ((col_offset > 0) && (col_offset >= n_cols)),
    "subview::diag(): requested diagonal out of bounds"
    );

  const uword len = (std::min)(n_rows - row_offset, n_cols - col_offset);

  const uword base_row_offset = aux_row1 + row_offset;
  const uword base_col_offset = aux_col1 + col_offset;

  return diagview<eT>(m, base_row_offset, base_col_offset, len);
  }



template<typename eT>
inline
void
subview<eT>::swap_rows(const uword in_row1, const uword in_row2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_row1 >= n_rows) || (in_row2 >= n_rows),
    "subview::swap_rows(): out of bounds"
    );

  eT* mem = (const_cast< Mat<eT>& >(m)).memptr();

  if(n_elem > 0)
    {
    const uword m_n_rows = m.n_rows;

    for(uword ucol=0; ucol < n_cols; ++ucol)
      {
      const uword offset = (aux_col1 + ucol) * m_n_rows;
      const uword pos1   = aux_row1 + in_row1 + offset;
      const uword pos2   = aux_row1 + in_row2 + offset;

      std::swap( access::rw(mem[pos1]), access::rw(mem[pos2]) );
      }
    }
  }



template<typename eT>
inline
void
subview<eT>::swap_cols(const uword in_col1, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_col1 >= n_cols) || (in_col2 >= n_cols),
    "subview::swap_cols(): out of bounds"
    );

  if(n_elem > 0)
    {
    eT* ptr1 = colptr(in_col1);
    eT* ptr2 = colptr(in_col2);

    for(uword urow=0; urow < n_rows; ++urow)
      {
      std::swap( ptr1[urow], ptr2[urow] );
      }
    }
  }



// template<typename eT>
// inline
// subview<eT>::iter::iter(const subview<eT>& S)
//   : mem       (S.m.mem)
//   , n_rows    (S.m.n_rows)
//   , row_start (S.aux_row1)
//   , row_end_p1(row_start + S.n_rows)
//   , row       (row_start)
//   , col       (S.aux_col1)
//   , i         (row + col*n_rows)
//   {
//   arma_extra_debug_sigprint();
//   }
//
//
//
// template<typename eT>
// arma_inline
// eT
// subview<eT>::iter::operator*() const
//   {
//   return mem[i];
//   }
//
//
//
// template<typename eT>
// inline
// void
// subview<eT>::iter::operator++()
//   {
//   ++row;
//
//   if(row < row_end_p1)
//     {
//     ++i;
//     }
//   else
//     {
//     row = row_start;
//     ++col;
//
//     i = row + col*n_rows;
//     }
//   }
//
//
//
// template<typename eT>
// inline
// void
// subview<eT>::iter::operator++(int)
//   {
//   operator++();
//   }



//
//
//



template<typename eT>
inline
subview_col<eT>::subview_col(const Mat<eT>& in_m, const uword in_col)
  : subview<eT>(in_m, 0, in_col, in_m.n_rows, 1)
  , colmem(subview<eT>::colptr(0))
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
subview_col<eT>::subview_col(const Mat<eT>& in_m, const uword in_col, const uword in_row1, const uword in_n_rows)
  : subview<eT>(in_m, in_row1, in_col, in_n_rows, 1)
  , colmem(subview<eT>::colptr(0))
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
void
subview_col<eT>::operator=(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview<eT>::operator=(X);
  }



template<typename eT>
inline
void
subview_col<eT>::operator=(const subview_col<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview<eT>::operator=(X); // interprets 'subview_col' as 'subview'
  }



template<typename eT>
inline
void
subview_col<eT>::operator=(const eT val)
  {
  arma_extra_debug_sigprint();

  if(subview<eT>::n_elem != 1)
    {
    arma_debug_assert_same_size(subview<eT>::n_rows, subview<eT>::n_cols, 1, 1, "copy into submatrix");
    }

  access::rw( colmem[0] ) = val;
  }



template<typename eT>
template<typename T1>
inline
void
subview_col<eT>::operator=(const Base<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  subview<eT>::operator=(X);
  }



template<typename eT>
template<typename T1, typename gen_type>
inline
typename enable_if2< is_same_type<typename T1::elem_type, eT>::value, void>::result
subview_col<eT>::operator= (const Gen<T1,gen_type>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(subview<eT>::n_rows, uword(1), in.n_rows, (in.is_col ? uword(1) : in.n_cols), "copy into submatrix");

  in.apply(*this);
  }



template<typename eT>
arma_inline
const Op<subview_col<eT>,op_htrans>
subview_col<eT>::t() const
  {
  return Op<subview_col<eT>,op_htrans>(*this);
  }



template<typename eT>
arma_inline
const Op<subview_col<eT>,op_htrans>
subview_col<eT>::ht() const
  {
  return Op<subview_col<eT>,op_htrans>(*this);
  }



template<typename eT>
arma_inline
const Op<subview_col<eT>,op_strans>
subview_col<eT>::st() const
  {
  return Op<subview_col<eT>,op_strans>(*this);
  }



template<typename eT>
inline
void
subview_col<eT>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_set( access::rwp(colmem), val, subview<eT>::n_rows );
  }



template<typename eT>
inline
void
subview_col<eT>::zeros()
  {
  arma_extra_debug_sigprint();

  arrayops::fill_zeros( access::rwp(colmem), subview<eT>::n_rows );
  }



template<typename eT>
inline
void
subview_col<eT>::ones()
  {
  arma_extra_debug_sigprint();

  arrayops::inplace_set( access::rwp(colmem), eT(1), subview<eT>::n_rows );
  }



template<typename eT>
arma_inline
eT
subview_col<eT>::at_alt(const uword ii) const
  {
  const eT* colmem_aligned = colmem;
  memory::mark_as_aligned(colmem_aligned);

  return colmem_aligned[ii];
  }



template<typename eT>
arma_inline
eT&
subview_col<eT>::operator[](const uword ii)
  {
  return access::rw( colmem[ii] );
  }



template<typename eT>
arma_inline
eT
subview_col<eT>::operator[](const uword ii) const
  {
  return colmem[ii];
  }



template<typename eT>
inline
eT&
subview_col<eT>::operator()(const uword ii)
  {
  arma_debug_check( (ii >= subview<eT>::n_elem), "subview::operator(): index out of bounds");

  return access::rw( colmem[ii] );
  }



template<typename eT>
inline
eT
subview_col<eT>::operator()(const uword ii) const
  {
  arma_debug_check( (ii >= subview<eT>::n_elem), "subview::operator(): index out of bounds");

  return colmem[ii];
  }



template<typename eT>
inline
eT&
subview_col<eT>::operator()(const uword in_row, const uword in_col)
  {
  arma_debug_check( ((in_row >= subview<eT>::n_rows) || (in_col > 0)), "subview::operator(): index out of bounds");

  return access::rw( colmem[in_row] );
  }



template<typename eT>
inline
eT
subview_col<eT>::operator()(const uword in_row, const uword in_col) const
  {
  arma_debug_check( ((in_row >= subview<eT>::n_rows) || (in_col > 0)), "subview::operator(): index out of bounds");

  return colmem[in_row];
  }



template<typename eT>
inline
eT&
subview_col<eT>::at(const uword in_row, const uword)
  {
  return access::rw( colmem[in_row] );
  }



template<typename eT>
inline
eT
subview_col<eT>::at(const uword in_row, const uword) const
  {
  return colmem[in_row];
  }



template<typename eT>
arma_inline
eT*
subview_col<eT>::colptr(const uword)
  {
  return const_cast<eT*>(colmem);
  }


template<typename eT>
arma_inline
const eT*
subview_col<eT>::colptr(const uword) const
  {
  return colmem;
  }


template<typename eT>
inline
subview_col<eT>
subview_col<eT>::rows(const uword in_row1, const uword in_row2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_row1 > in_row2) || (in_row2 >= subview<eT>::n_rows) ), "subview_col::rows(): indices out of bounds or incorrectly used");

  const uword subview_n_rows = in_row2 - in_row1 + 1;

  const uword base_row1 = this->aux_row1 + in_row1;

  return subview_col<eT>(this->m, this->aux_col1, base_row1, subview_n_rows);
  }



template<typename eT>
inline
const subview_col<eT>
subview_col<eT>::rows(const uword in_row1, const uword in_row2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_row1 > in_row2) || (in_row2 >= subview<eT>::n_rows) ), "subview_col::rows(): indices out of bounds or incorrectly used");

  const uword subview_n_rows = in_row2 - in_row1 + 1;

  const uword base_row1 = this->aux_row1 + in_row1;

  return subview_col<eT>(this->m, this->aux_col1, base_row1, subview_n_rows);
  }



template<typename eT>
inline
subview_col<eT>
subview_col<eT>::subvec(const uword in_row1, const uword in_row2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_row1 > in_row2) || (in_row2 >= subview<eT>::n_rows) ), "subview_col::subvec(): indices out of bounds or incorrectly used");

  const uword subview_n_rows = in_row2 - in_row1 + 1;

  const uword base_row1 = this->aux_row1 + in_row1;

  return subview_col<eT>(this->m, this->aux_col1, base_row1, subview_n_rows);
  }



template<typename eT>
inline
const subview_col<eT>
subview_col<eT>::subvec(const uword in_row1, const uword in_row2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_row1 > in_row2) || (in_row2 >= subview<eT>::n_rows) ), "subview_col::subvec(): indices out of bounds or incorrectly used");

  const uword subview_n_rows = in_row2 - in_row1 + 1;

  const uword base_row1 = this->aux_row1 + in_row1;

  return subview_col<eT>(this->m, this->aux_col1, base_row1, subview_n_rows);
  }



template<typename eT>
inline
subview_col<eT>
subview_col<eT>::subvec(const uword start_row, const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (s.n_cols != 1), "subview_col::subvec(): given size does not specify a column vector" );

  arma_debug_check( ( (start_row >= subview<eT>::n_rows) || ((start_row + s.n_rows) > subview<eT>::n_rows) ), "subview_col::subvec(): size out of bounds" );

  const uword base_row1 = this->aux_row1 + start_row;

  return subview_col<eT>(this->m, this->aux_col1, base_row1, s.n_rows);
  }



template<typename eT>
inline
const subview_col<eT>
subview_col<eT>::subvec(const uword start_row, const SizeMat& s) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (s.n_cols != 1), "subview_col::subvec(): given size does not specify a column vector" );

  arma_debug_check( ( (start_row >= subview<eT>::n_rows) || ((start_row + s.n_rows) > subview<eT>::n_rows) ), "subview_col::subvec(): size out of bounds" );

  const uword base_row1 = this->aux_row1 + start_row;

  return subview_col<eT>(this->m, this->aux_col1, base_row1, s.n_rows);
  }



template<typename eT>
inline
subview_col<eT>
subview_col<eT>::head(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > subview<eT>::n_rows), "subview_col::head(): size out of bounds");

  return subview_col<eT>(this->m, this->aux_col1, this->aux_row1, N);
  }



template<typename eT>
inline
const subview_col<eT>
subview_col<eT>::head(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > subview<eT>::n_rows), "subview_col::head(): size out of bounds");

  return subview_col<eT>(this->m, this->aux_col1, this->aux_row1, N);
  }



template<typename eT>
inline
subview_col<eT>
subview_col<eT>::tail(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > subview<eT>::n_rows), "subview_col::tail(): size out of bounds");

  const uword start_row = subview<eT>::aux_row1 + subview<eT>::n_rows - N;

  return subview_col<eT>(this->m, this->aux_col1, start_row, N);
  }



template<typename eT>
inline
const subview_col<eT>
subview_col<eT>::tail(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > subview<eT>::n_rows), "subview_col::tail(): size out of bounds");

  const uword start_row = subview<eT>::aux_row1 + subview<eT>::n_rows - N;

  return subview_col<eT>(this->m, this->aux_col1, start_row, N);
  }



template<typename eT>
inline
arma_warn_unused
eT
subview_col<eT>::min() const
  {
  arma_extra_debug_sigprint();

  if(subview<eT>::n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    return Datum<eT>::nan;
    }

  return op_min::direct_min(colmem, subview<eT>::n_elem);
  }



template<typename eT>
inline
arma_warn_unused
eT
subview_col<eT>::max() const
  {
  arma_extra_debug_sigprint();

  if(subview<eT>::n_elem == 0)
    {
    arma_debug_check(true, "max(): object has no elements");

    return Datum<eT>::nan;
    }

  return op_max::direct_max(colmem, subview<eT>::n_elem);
  }



template<typename eT>
inline
eT
subview_col<eT>::min(uword& index_of_min_val) const
  {
  arma_extra_debug_sigprint();

  if(subview<eT>::n_elem == 0)
    {
    arma_debug_check(true, "min(): object has no elements");

    index_of_min_val = uword(0);

    return Datum<eT>::nan;
    }
  else
    {
    return op_min::direct_min(colmem, subview<eT>::n_elem, index_of_min_val);
    }
  }



template<typename eT>
inline
eT
subview_col<eT>::max(uword& index_of_max_val) const
  {
  arma_extra_debug_sigprint();

  if(subview<eT>::n_elem == 0)
    {
    arma_debug_check(true, "max(): object has no elements");

    index_of_max_val = uword(0);

    return Datum<eT>::nan;
    }
  else
    {
    return op_max::direct_max(colmem, subview<eT>::n_elem, index_of_max_val);
    }
  }



template<typename eT>
inline
arma_warn_unused
uword
subview_col<eT>::index_min() const
  {
  arma_extra_debug_sigprint();

  uword index = 0;

  if(subview<eT>::n_elem == 0)
    {
    arma_debug_check(true, "index_min(): object has no elements");
    }
  else
    {
    op_min::direct_min(colmem, subview<eT>::n_elem, index);
    }

  return index;
  }



template<typename eT>
inline
arma_warn_unused
uword
subview_col<eT>::index_max() const
  {
  arma_extra_debug_sigprint();

  uword index = 0;

  if(subview<eT>::n_elem == 0)
    {
    arma_debug_check(true, "index_max(): object has no elements");
    }
  else
    {
    op_max::direct_max(colmem, subview<eT>::n_elem, index);
    }

  return index;
  }



//
//
//



template<typename eT>
inline
subview_row<eT>::subview_row(const Mat<eT>& in_m, const uword in_row)
  : subview<eT>(in_m, in_row, 0, 1, in_m.n_cols)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
subview_row<eT>::subview_row(const Mat<eT>& in_m, const uword in_row, const uword in_col1, const uword in_n_cols)
  : subview<eT>(in_m, in_row, in_col1, 1, in_n_cols)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
void
subview_row<eT>::operator=(const subview<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview<eT>::operator=(X);
  }



template<typename eT>
inline
void
subview_row<eT>::operator=(const subview_row<eT>& X)
  {
  arma_extra_debug_sigprint();

  subview<eT>::operator=(X); // interprets 'subview_row' as 'subview'
  }



template<typename eT>
inline
void
subview_row<eT>::operator=(const eT val)
  {
  arma_extra_debug_sigprint();

  subview<eT>::operator=(val); // interprets 'subview_row' as 'subview'
  }



template<typename eT>
template<typename T1>
inline
void
subview_row<eT>::operator=(const Base<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  subview<eT>::operator=(X);
  }



template<typename eT>
template<typename T1, typename gen_type>
inline
typename enable_if2< is_same_type<typename T1::elem_type, eT>::value, void>::result
subview_row<eT>::operator= (const Gen<T1,gen_type>& in)
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(uword(1), subview<eT>::n_cols, (in.is_row ? uword(1) : in.n_rows), in.n_cols, "copy into submatrix");

  in.apply(*this);
  }



template<typename eT>
arma_inline
const Op<subview_row<eT>,op_htrans>
subview_row<eT>::t() const
  {
  return Op<subview_row<eT>,op_htrans>(*this);
  }



template<typename eT>
arma_inline
const Op<subview_row<eT>,op_htrans>
subview_row<eT>::ht() const
  {
  return Op<subview_row<eT>,op_htrans>(*this);
  }



template<typename eT>
arma_inline
const Op<subview_row<eT>,op_strans>
subview_row<eT>::st() const
  {
  return Op<subview_row<eT>,op_strans>(*this);
  }



template<typename eT>
inline
eT
subview_row<eT>::at_alt(const uword ii) const
  {
  const uword index = (ii + (subview<eT>::aux_col1))*(subview<eT>::m).n_rows + (subview<eT>::aux_row1);

  return subview<eT>::m.mem[index];
  }



template<typename eT>
inline
eT&
subview_row<eT>::operator[](const uword ii)
  {
  const uword index = (ii + (subview<eT>::aux_col1))*(subview<eT>::m).n_rows + (subview<eT>::aux_row1);

  return access::rw( (const_cast< Mat<eT>& >(subview<eT>::m)).mem[index] );
  }



template<typename eT>
inline
eT
subview_row<eT>::operator[](const uword ii) const
  {
  const uword index = (ii + (subview<eT>::aux_col1))*(subview<eT>::m).n_rows + (subview<eT>::aux_row1);

  return subview<eT>::m.mem[index];
  }



template<typename eT>
inline
eT&
subview_row<eT>::operator()(const uword ii)
  {
  arma_debug_check( (ii >= subview<eT>::n_elem), "subview::operator(): index out of bounds");

  const uword index = (ii + (subview<eT>::aux_col1))*(subview<eT>::m).n_rows + (subview<eT>::aux_row1);

  return access::rw( (const_cast< Mat<eT>& >(subview<eT>::m)).mem[index] );
  }



template<typename eT>
inline
eT
subview_row<eT>::operator()(const uword ii) const
  {
  arma_debug_check( (ii >= subview<eT>::n_elem), "subview::operator(): index out of bounds");

  const uword index = (ii + (subview<eT>::aux_col1))*(subview<eT>::m).n_rows + (subview<eT>::aux_row1);

  return subview<eT>::m.mem[index];
  }



template<typename eT>
inline
eT&
subview_row<eT>::operator()(const uword in_row, const uword in_col)
  {
  arma_debug_check( ((in_row > 0) || (in_col >= subview<eT>::n_cols)), "subview::operator(): index out of bounds");

  const uword index = (in_col + (subview<eT>::aux_col1))*(subview<eT>::m).n_rows + (subview<eT>::aux_row1);

  return access::rw( (const_cast< Mat<eT>& >(subview<eT>::m)).mem[index] );
  }



template<typename eT>
inline
eT
subview_row<eT>::operator()(const uword in_row, const uword in_col) const
  {
  arma_debug_check( ((in_row > 0) || (in_col >= subview<eT>::n_cols)), "subview::operator(): index out of bounds");

  const uword index = (in_col + (subview<eT>::aux_col1))*(subview<eT>::m).n_rows + (subview<eT>::aux_row1);

  return subview<eT>::m.mem[index];
  }



template<typename eT>
inline
eT&
subview_row<eT>::at(const uword, const uword in_col)
  {
  const uword index = (in_col + (subview<eT>::aux_col1))*(subview<eT>::m).n_rows + (subview<eT>::aux_row1);

  return access::rw( (const_cast< Mat<eT>& >(subview<eT>::m)).mem[index] );
  }



template<typename eT>
inline
eT
subview_row<eT>::at(const uword, const uword in_col) const
  {
  const uword index = (in_col + (subview<eT>::aux_col1))*(subview<eT>::m).n_rows + (subview<eT>::aux_row1);

  return subview<eT>::m.mem[index];
  }



template<typename eT>
inline
subview_row<eT>
subview_row<eT>::cols(const uword in_col1, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_col1 > in_col2) || (in_col2 >= subview<eT>::n_cols) ), "subview_row::cols(): indices out of bounds or incorrectly used" );

  const uword subview_n_cols = in_col2 - in_col1 + 1;

  const uword base_col1 = this->aux_col1 + in_col1;

  return subview_row<eT>(this->m, this->aux_row1, base_col1, subview_n_cols);
  }



template<typename eT>
inline
const subview_row<eT>
subview_row<eT>::cols(const uword in_col1, const uword in_col2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_col1 > in_col2) || (in_col2 >= subview<eT>::n_cols) ), "subview_row::cols(): indices out of bounds or incorrectly used");

  const uword subview_n_cols = in_col2 - in_col1 + 1;

  const uword base_col1 = this->aux_col1 + in_col1;

  return subview_row<eT>(this->m, this->aux_row1, base_col1, subview_n_cols);
  }



template<typename eT>
inline
subview_row<eT>
subview_row<eT>::subvec(const uword in_col1, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_col1 > in_col2) || (in_col2 >= subview<eT>::n_cols) ), "subview_row::subvec(): indices out of bounds or incorrectly used");

  const uword subview_n_cols = in_col2 - in_col1 + 1;

  const uword base_col1 = this->aux_col1 + in_col1;

  return subview_row<eT>(this->m, this->aux_row1, base_col1, subview_n_cols);
  }



template<typename eT>
inline
const subview_row<eT>
subview_row<eT>::subvec(const uword in_col1, const uword in_col2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_col1 > in_col2) || (in_col2 >= subview<eT>::n_cols) ), "subview_row::subvec(): indices out of bounds or incorrectly used");

  const uword subview_n_cols = in_col2 - in_col1 + 1;

  const uword base_col1 = this->aux_col1 + in_col1;

  return subview_row<eT>(this->m, this->aux_row1, base_col1, subview_n_cols);
  }



template<typename eT>
inline
subview_row<eT>
subview_row<eT>::subvec(const uword start_col, const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (s.n_rows != 1), "subview_row::subvec(): given size does not specify a row vector" );

  arma_debug_check( ( (start_col >= subview<eT>::n_cols) || ((start_col + s.n_cols) > subview<eT>::n_cols) ), "subview_row::subvec(): size out of bounds" );

  const uword base_col1 = this->aux_col1 + start_col;

  return subview_row<eT>(this->m, this->aux_row1, base_col1, s.n_cols);
  }



template<typename eT>
inline
const subview_row<eT>
subview_row<eT>::subvec(const uword start_col, const SizeMat& s) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (s.n_rows != 1), "subview_row::subvec(): given size does not specify a row vector" );

  arma_debug_check( ( (start_col >= subview<eT>::n_cols) || ((start_col + s.n_cols) > subview<eT>::n_cols) ), "subview_row::subvec(): size out of bounds" );

  const uword base_col1 = this->aux_col1 + start_col;

  return subview_row<eT>(this->m, this->aux_row1, base_col1, s.n_cols);
  }



template<typename eT>
inline
subview_row<eT>
subview_row<eT>::head(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > subview<eT>::n_cols), "subview_row::head(): size out of bounds");

  return subview_row<eT>(this->m, this->aux_row1, this->aux_col1, N);
  }



template<typename eT>
inline
const subview_row<eT>
subview_row<eT>::head(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > subview<eT>::n_cols), "subview_row::head(): size out of bounds");

  return subview_row<eT>(this->m, this->aux_row1, this->aux_col1, N);
  }



template<typename eT>
inline
subview_row<eT>
subview_row<eT>::tail(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > subview<eT>::n_cols), "subview_row::tail(): size out of bounds");

  const uword start_col = subview<eT>::aux_col1 + subview<eT>::n_cols - N;

  return subview_row<eT>(this->m, this->aux_row1, start_col, N);
  }



template<typename eT>
inline
const subview_row<eT>
subview_row<eT>::tail(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > subview<eT>::n_cols), "subview_row::tail(): size out of bounds");

  const uword start_col = subview<eT>::aux_col1 + subview<eT>::n_cols - N;

  return subview_row<eT>(this->m, this->aux_row1, start_col, N);
  }



template<typename eT>
inline
arma_warn_unused
uword
subview_row<eT>::index_min() const
  {
  const Proxy< subview_row<eT> > P(*this);

  uword index = 0;

  if(P.get_n_elem() == 0)
    {
    arma_debug_check(true, "index_min(): object has no elements");
    }
  else
    {
    op_min::min_with_index(P, index);
    }

  return index;
  }



template<typename eT>
inline
arma_warn_unused
uword
subview_row<eT>::index_max() const
  {
  const Proxy< subview_row<eT> > P(*this);

  uword index = 0;

  if(P.get_n_elem() == 0)
    {
    arma_debug_check(true, "index_max(): object has no elements");
    }
  else
    {
    op_max::max_with_index(P, index);
    }

  return index;
  }



//
//
//



template<typename eT>
inline
subview_row_strans<eT>::subview_row_strans(const subview_row<eT>& in_sv_row)
  : sv_row(in_sv_row       )
  , n_rows(in_sv_row.n_cols)
  , n_elem(in_sv_row.n_elem)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
void
subview_row_strans<eT>::extract(Mat<eT>& out) const
  {
  arma_extra_debug_sigprint();

  // NOTE: this function assumes that matrix 'out' has already been set to the correct size

  const Mat<eT>& X = sv_row.m;

  eT* out_mem = out.memptr();

  const uword row           = sv_row.aux_row1;
  const uword start_col     = sv_row.aux_col1;
  const uword sv_row_n_cols = sv_row.n_cols;

  uword ii,jj;

  for(ii=0, jj=1; jj < sv_row_n_cols; ii+=2, jj+=2)
    {
    const eT tmp1 = X.at(row, start_col+ii);
    const eT tmp2 = X.at(row, start_col+jj);

    out_mem[ii] = tmp1;
    out_mem[jj] = tmp2;
    }

  if(ii < sv_row_n_cols)
    {
    out_mem[ii] = X.at(row, start_col+ii);
    }
  }



template<typename eT>
inline
eT
subview_row_strans<eT>::at_alt(const uword ii) const
  {
  return sv_row[ii];
  }



template<typename eT>
inline
eT
subview_row_strans<eT>::operator[](const uword ii) const
  {
  return sv_row[ii];
  }



template<typename eT>
inline
eT
subview_row_strans<eT>::operator()(const uword ii) const
  {
  return sv_row(ii);
  }



template<typename eT>
inline
eT
subview_row_strans<eT>::operator()(const uword in_row, const uword in_col) const
  {
  return sv_row(in_col, in_row);  // deliberately swapped
  }



template<typename eT>
inline
eT
subview_row_strans<eT>::at(const uword in_row, const uword) const
  {
  return sv_row.at(0, in_row);  // deliberately swapped
  }



//
//
//



template<typename eT>
inline
subview_row_htrans<eT>::subview_row_htrans(const subview_row<eT>& in_sv_row)
  : sv_row(in_sv_row       )
  , n_rows(in_sv_row.n_cols)
  , n_elem(in_sv_row.n_elem)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
void
subview_row_htrans<eT>::extract(Mat<eT>& out) const
  {
  arma_extra_debug_sigprint();

  // NOTE: this function assumes that matrix 'out' has already been set to the correct size

  const Mat<eT>& X = sv_row.m;

  eT* out_mem = out.memptr();

  const uword row           = sv_row.aux_row1;
  const uword start_col     = sv_row.aux_col1;
  const uword sv_row_n_cols = sv_row.n_cols;

  for(uword ii=0; ii < sv_row_n_cols; ++ii)
    {
    out_mem[ii] = access::alt_conj( X.at(row, start_col+ii) );
    }
  }



template<typename eT>
inline
eT
subview_row_htrans<eT>::at_alt(const uword ii) const
  {
  return access::alt_conj( sv_row[ii] );
  }



template<typename eT>
inline
eT
subview_row_htrans<eT>::operator[](const uword ii) const
  {
  return access::alt_conj( sv_row[ii] );
  }



template<typename eT>
inline
eT
subview_row_htrans<eT>::operator()(const uword ii) const
  {
  return access::alt_conj( sv_row(ii) );
  }



template<typename eT>
inline
eT
subview_row_htrans<eT>::operator()(const uword in_row, const uword in_col) const
  {
  return access::alt_conj( sv_row(in_col, in_row) );  // deliberately swapped
  }



template<typename eT>
inline
eT
subview_row_htrans<eT>::at(const uword in_row, const uword) const
  {
  return access::alt_conj( sv_row.at(0, in_row) );  // deliberately swapped
  }



//! @}
