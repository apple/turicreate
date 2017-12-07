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


//! \addtogroup fn_as_scalar
//! @{



template<uword N>
struct as_scalar_redirect
  {
  template<typename T1>
  inline static typename T1::elem_type apply(const T1& X);
  };



template<>
struct as_scalar_redirect<2>
  {
  template<typename T1, typename T2>
  inline static typename T1::elem_type apply(const Glue<T1,T2,glue_times>& X);
  };


template<>
struct as_scalar_redirect<3>
  {
  template<typename T1, typename T2, typename T3>
  inline static typename T1::elem_type apply(const Glue< Glue<T1, T2, glue_times>, T3, glue_times>& X);
  };



template<uword N>
template<typename T1>
inline
typename T1::elem_type
as_scalar_redirect<N>::apply(const T1& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> P(X);

  if(P.get_n_elem() != 1)
    {
    arma_debug_check(true, "as_scalar(): expression doesn't evaluate to exactly one element");

    return Datum<eT>::nan;
    }

  return (Proxy<T1>::use_at) ? P.at(0,0) : P[0];
  }



template<typename T1, typename T2>
inline
typename T1::elem_type
as_scalar_redirect<2>::apply(const Glue<T1, T2, glue_times>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  // T1 must result in a matrix with one row
  // T2 must result in a matrix with one column

  const bool has_all_mat = (is_Mat<T1>::value || is_Mat_trans<T1>::value) && (is_Mat<T2>::value || is_Mat_trans<T2>::value);

  const bool use_at = (Proxy<T1>::use_at || Proxy<T2>::use_at);

  const bool do_partial_unwrap = (has_all_mat || use_at);

  if(do_partial_unwrap == true)
    {
    const partial_unwrap<T1> tmp1(X.A);
    const partial_unwrap<T2> tmp2(X.B);

    typedef typename partial_unwrap<T1>::stored_type TA;
    typedef typename partial_unwrap<T2>::stored_type TB;

    const TA& A = tmp1.M;
    const TB& B = tmp2.M;

    const uword A_n_rows = (tmp1.do_trans == false) ? (TA::is_row ? 1 : A.n_rows) : (TA::is_col ? 1 : A.n_cols);
    const uword A_n_cols = (tmp1.do_trans == false) ? (TA::is_col ? 1 : A.n_cols) : (TA::is_row ? 1 : A.n_rows);

    const uword B_n_rows = (tmp2.do_trans == false) ? (TB::is_row ? 1 : B.n_rows) : (TB::is_col ? 1 : B.n_cols);
    const uword B_n_cols = (tmp2.do_trans == false) ? (TB::is_col ? 1 : B.n_cols) : (TB::is_row ? 1 : B.n_rows);

    arma_debug_check( (A_n_rows != 1) || (B_n_cols != 1) || (A_n_cols != B_n_rows), "as_scalar(): incompatible dimensions" );

    const eT val = op_dot::direct_dot(A.n_elem, A.memptr(), B.memptr());

    return (tmp1.do_times || tmp2.do_times) ? (val * tmp1.get_val() * tmp2.get_val()) : val;
    }
  else
    {
    const Proxy<T1> PA(X.A);
    const Proxy<T2> PB(X.B);

    arma_debug_check
      (
      (PA.get_n_rows() != 1) || (PB.get_n_cols() != 1) || (PA.get_n_cols() != PB.get_n_rows()),
      "as_scalar(): incompatible dimensions"
      );

    return op_dot::apply_proxy(PA,PB);
    }
  }



template<typename T1, typename T2, typename T3>
inline
typename T1::elem_type
as_scalar_redirect<3>::apply(const Glue< Glue<T1, T2, glue_times>, T3, glue_times >& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  // T1 * T2 must result in a matrix with one row
  // T3 must result in a matrix with one column

  typedef typename strip_inv    <T2           >::stored_type T2_stripped_1;
  typedef typename strip_diagmat<T2_stripped_1>::stored_type T2_stripped_2;

  const strip_inv    <T2>            strip1(X.A.B);
  const strip_diagmat<T2_stripped_1> strip2(strip1.M);

  const bool tmp2_do_inv     = strip1.do_inv;
  const bool tmp2_do_diagmat = strip2.do_diagmat;

  if(tmp2_do_diagmat == false)
    {
    const Mat<eT> tmp(X);

    if(tmp.n_elem != 1)
      {
      arma_debug_check(true, "as_scalar(): expression doesn't evaluate to exactly one element");

      return Datum<eT>::nan;
      }

    return tmp[0];
    }
  else
    {
    const partial_unwrap<T1>            tmp1(X.A.A);
    const partial_unwrap<T2_stripped_2> tmp2(strip2.M);
    const partial_unwrap<T3>            tmp3(X.B);

    const Mat<eT>& A = tmp1.M;
    const Mat<eT>& B = tmp2.M;
    const Mat<eT>& C = tmp3.M;

    const uword A_n_rows = (tmp1.do_trans == false) ? A.n_rows : A.n_cols;
    const uword A_n_cols = (tmp1.do_trans == false) ? A.n_cols : A.n_rows;

    const bool B_is_vec = B.is_vec();

    const uword B_n_rows = (B_is_vec == true) ? B.n_elem : ( (tmp2.do_trans == false) ? B.n_rows : B.n_cols );
    const uword B_n_cols = (B_is_vec == true) ? B.n_elem : ( (tmp2.do_trans == false) ? B.n_cols : B.n_rows );

    const uword C_n_rows = (tmp3.do_trans == false) ? C.n_rows : C.n_cols;
    const uword C_n_cols = (tmp3.do_trans == false) ? C.n_cols : C.n_rows;

    const eT val = tmp1.get_val() * tmp2.get_val() * tmp3.get_val();

    arma_debug_check
      (
      (A_n_rows != 1)        ||
      (C_n_cols != 1)        ||
      (A_n_cols != B_n_rows) ||
      (B_n_cols != C_n_rows)
      ,
      "as_scalar(): incompatible dimensions"
      );


    if(B_is_vec == true)
      {
      if(tmp2_do_inv == true)
        {
        return val * op_dotext::direct_rowvec_invdiagvec_colvec(A.mem, B, C.mem);
        }
      else
        {
        return val * op_dot::direct_dot(A.n_elem, A.mem, B.mem, C.mem);
        }
      }
    else
      {
      if(tmp2_do_inv == true)
        {
        return val * op_dotext::direct_rowvec_invdiagmat_colvec(A.mem, B, C.mem);
        }
      else
        {
        return val * op_dotext::direct_rowvec_diagmat_colvec(A.mem, B, C.mem);
        }
      }
    }
  }



template<typename T1>
inline
typename T1::elem_type
as_scalar_diag(const Base<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T1>   tmp(X.get_ref());
  const Mat<eT>& A = tmp.M;

  if(A.n_elem != 1)
    {
    arma_debug_check(true, "as_scalar(): expression doesn't evaluate to exactly one element");

    return Datum<eT>::nan;
    }

  return A.mem[0];
  }



template<typename T1, typename T2, typename T3>
inline
typename T1::elem_type
as_scalar_diag(const Glue< Glue<T1, T2, glue_times_diag>, T3, glue_times >& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  // T1 * T2 must result in a matrix with one row
  // T3 must result in a matrix with one column

  typedef typename strip_diagmat<T2>::stored_type T2_stripped;

  const strip_diagmat<T2> strip(X.A.B);

  const partial_unwrap<T1>          tmp1(X.A.A);
  const partial_unwrap<T2_stripped> tmp2(strip.M);
  const partial_unwrap<T3>          tmp3(X.B);

  const Mat<eT>& A = tmp1.M;
  const Mat<eT>& B = tmp2.M;
  const Mat<eT>& C = tmp3.M;


  const uword A_n_rows = (tmp1.do_trans == false) ? A.n_rows : A.n_cols;
  const uword A_n_cols = (tmp1.do_trans == false) ? A.n_cols : A.n_rows;

  const bool B_is_vec = B.is_vec();

  const uword B_n_rows = (B_is_vec == true) ? B.n_elem : ( (tmp2.do_trans == false) ? B.n_rows : B.n_cols );
  const uword B_n_cols = (B_is_vec == true) ? B.n_elem : ( (tmp2.do_trans == false) ? B.n_cols : B.n_rows );

  const uword C_n_rows = (tmp3.do_trans == false) ? C.n_rows : C.n_cols;
  const uword C_n_cols = (tmp3.do_trans == false) ? C.n_cols : C.n_rows;

  const eT val = tmp1.get_val() * tmp2.get_val() * tmp3.get_val();

  arma_debug_check
    (
    (A_n_rows != 1)        ||
    (C_n_cols != 1)        ||
    (A_n_cols != B_n_rows) ||
    (B_n_cols != C_n_rows)
    ,
    "as_scalar(): incompatible dimensions"
    );


  if(B_is_vec == true)
    {
    return val * op_dot::direct_dot(A.n_elem, A.mem, B.mem, C.mem);
    }
  else
    {
    return val * op_dotext::direct_rowvec_diagmat_colvec(A.mem, B, C.mem);
    }
  }



template<typename T1, typename T2>
arma_warn_unused
arma_inline
typename T1::elem_type
as_scalar(const Glue<T1, T2, glue_times>& X, const typename arma_not_cx<typename T1::elem_type>::result* junk = 0)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  if(is_glue_times_diag<T1>::value == false)
    {
    const sword N_mat = 1 + depth_lhs< glue_times, Glue<T1,T2,glue_times> >::num;

    arma_extra_debug_print(arma_str::format("N_mat = %d") % N_mat);

    return as_scalar_redirect<N_mat>::apply(X);
    }
  else
    {
    return as_scalar_diag(X);
    }
  }



template<typename T1>
arma_warn_unused
inline
typename T1::elem_type
as_scalar(const Base<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> P(X.get_ref());

  if(P.get_n_elem() != 1)
    {
    arma_debug_check(true, "as_scalar(): expression doesn't evaluate to exactly one element");

    return Datum<eT>::nan;
    }

  return (Proxy<T1>::use_at) ? P.at(0,0) : P[0];
  }


// ensure the following two functions are aware of each other
template<typename T1,              typename   eop_type> inline arma_warn_unused typename T1::elem_type as_scalar(const   eOp<T1,       eop_type>& X);
template<typename T1, typename T2, typename eglue_type> inline arma_warn_unused typename T1::elem_type as_scalar(const eGlue<T1, T2, eglue_type>& X);



template<typename T1, typename eop_type>
arma_warn_unused
inline
typename T1::elem_type
as_scalar(const eOp<T1, eop_type>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const eT val = as_scalar(X.P.Q);

  return eop_core<eop_type>::process(val, X.aux);
  }



template<typename T1, typename T2, typename eglue_type>
inline
arma_warn_unused
typename T1::elem_type
as_scalar(const eGlue<T1, T2, eglue_type>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const eT a = as_scalar(X.P1.Q);
  const eT b = as_scalar(X.P2.Q);

  // the optimiser will keep only one return statement

       if(is_same_type<eglue_type, eglue_plus >::yes) { return a + b; }
  else if(is_same_type<eglue_type, eglue_minus>::yes) { return a - b; }
  else if(is_same_type<eglue_type, eglue_div  >::yes) { return a / b; }
  else if(is_same_type<eglue_type, eglue_schur>::yes) { return a * b; }
  }



template<typename T1>
arma_warn_unused
inline
typename T1::elem_type
as_scalar(const BaseCube<typename T1::elem_type,T1>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const ProxyCube<T1> P(X.get_ref());

  if(P.get_n_elem() != 1)
    {
    arma_debug_check(true, "as_scalar(): expression doesn't evaluate to exactly one element");

    return Datum<eT>::nan;
    }

  return (ProxyCube<T1>::use_at) ? P.at(0,0,0) : P[0];
  }



template<typename T>
arma_warn_unused
arma_inline
const typename arma_scalar_only<T>::result &
as_scalar(const T& x)
  {
  return x;
  }



template<typename T1>
arma_warn_unused
inline
typename T1::elem_type
as_scalar(const SpBase<typename T1::elem_type, T1>& X)
  {
  typedef typename T1::elem_type eT;

  const unwrap_spmat<T1>  tmp(X.get_ref());
  const SpMat<eT>& A    = tmp.M;

  if(A.n_elem != 1)
    {
    arma_debug_check(true, "as_scalar(): expression doesn't evaluate to exactly one element");

    return Datum<eT>::nan;
    }

  return A.at(0,0);
  }



//! @}
