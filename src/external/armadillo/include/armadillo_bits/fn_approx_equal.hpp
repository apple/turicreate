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


//! \addtogroup fn_approx_equal
//! @{



template<typename eT>
arma_inline
bool
internal_approx_equal_abs_diff(const eT& x, const eT& y, const typename get_pod_type<eT>::result tol)
  {
  typedef typename get_pod_type<eT>::result T;

  if(x != y)
    {
    if(is_real<T>::value)  // also true for eT = std::complex<float> or eT = std::complex<double>
      {
      if( arma_isnan(x) || arma_isnan(y) || (eop_aux::arma_abs(x - y) > tol) )  { return false; }
      }
    else
      {
      if( eop_aux::arma_abs( ( cond_rel< is_not_complex<eT>::value >::gt(x, y) ) ? (x-y) : (y-x) ) > tol )  { return false; }
      }
    }

  return true;
  }



template<typename eT>
arma_inline
bool
internal_approx_equal_rel_diff(const eT& a, const eT& b, const typename get_pod_type<eT>::result tol)
  {
  typedef typename get_pod_type<eT>::result T;

  if(a != b)
    {
    if(is_real<T>::value)  // also true for eT = std::complex<float> or eT = std::complex<double>
      {
      if( arma_isnan(a) || arma_isnan(b) )  { return false; }

      const T abs_a = eop_aux::arma_abs(a);
      const T abs_b = eop_aux::arma_abs(b);

      const T max_c = (std::max)(abs_a,abs_b);

      const T abs_d = eop_aux::arma_abs(a - b);

      if(max_c >= T(1))
        {
        if( abs_d > (tol * max_c) )  { return false; }
        }
      else
        {
        if( (abs_d / max_c) > tol )  { return false; }
        }
      }
    else
      {
      const T abs_a = eop_aux::arma_abs(a);
      const T abs_b = eop_aux::arma_abs(b);

      const T max_c = (std::max)(abs_a,abs_b);

      const T abs_d = eop_aux::arma_abs( ( cond_rel< is_not_complex<eT>::value >::gt(a, b) ) ? (a-b) : (b-a) );

      if( abs_d > (tol * max_c) )  { return false; }
      }
    }

  return true;
  }



template<bool use_abs_diff, bool use_rel_diff, typename T1, typename T2>
inline
bool
internal_approx_equal_worker
  (
  const Base<typename T1::elem_type,T1>& A,
  const Base<typename T1::elem_type,T2>& B,
  const typename T1::pod_type abs_tol,
  const typename T1::pod_type rel_tol
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;
  typedef typename T1::pod_type   T;

  arma_debug_check( ((use_abs_diff == false) && (use_rel_diff == false)), "internal_approx_equal_worker(): both 'use_abs_diff' and 'use_rel_diff' are false" );

  if(use_abs_diff)  { arma_debug_check( cond_rel< is_signed<T>::value >::lt(abs_tol, T(0)), "approx_equal(): argument 'abs_tol' must be >= 0" ); }
  if(use_rel_diff)  { arma_debug_check( cond_rel< is_signed<T>::value >::lt(rel_tol, T(0)), "approx_equal(): argument 'rel_tol' must be >= 0" ); }

  const Proxy<T1> PA(A.get_ref());
  const Proxy<T2> PB(B.get_ref());

  if( (PA.get_n_rows() != PB.get_n_rows()) || (PA.get_n_cols() != PB.get_n_cols()) )  { return false; }

  if( (Proxy<T1>::use_at == false) && (Proxy<T2>::use_at == false) )
    {
    const uword N = PA.get_n_elem();

    const typename Proxy<T1>::ea_type PA_ea = PA.get_ea();
    const typename Proxy<T2>::ea_type PB_ea = PB.get_ea();

    for(uword i=0; i<N; ++i)
      {
      const eT x = PA_ea[i];
      const eT y = PB_ea[i];

      const bool state_abs_diff = (use_abs_diff) ? internal_approx_equal_abs_diff(x, y, abs_tol) : true;
      const bool state_rel_diff = (use_rel_diff) ? internal_approx_equal_rel_diff(x, y, rel_tol) : true;

      if(use_abs_diff && use_rel_diff)
        {
        if((state_abs_diff == false) && (state_rel_diff == false))  { return false; }
        }
      else
      if(use_abs_diff)
        {
        if(state_abs_diff == false)  { return false; }
        }
      else
      if(use_rel_diff)
        {
        if(state_rel_diff == false)  { return false; }
        }
      }
    }
  else
    {
    const uword n_rows = PA.get_n_rows();
    const uword n_cols = PA.get_n_cols();

    for(uword col=0; col < n_cols; ++col)
    for(uword row=0; row < n_rows; ++row)
      {
      const eT x = PA.at(row,col);
      const eT y = PB.at(row,col);

      const bool state_abs_diff = (use_abs_diff) ? internal_approx_equal_abs_diff(x, y, abs_tol) : true;
      const bool state_rel_diff = (use_rel_diff) ? internal_approx_equal_rel_diff(x, y, rel_tol) : true;

      if(use_abs_diff && use_rel_diff)
        {
        if((state_abs_diff == false) && (state_rel_diff == false))  { return false; }
        }
      else
      if(use_abs_diff)
        {
        if(state_abs_diff == false)  { return false; }
        }
      else
      if(use_rel_diff)
        {
        if(state_rel_diff == false)  { return false; }
        }
      }
    }

  return true;
  }



template<bool use_abs_diff, bool use_rel_diff, typename T1, typename T2>
inline
bool
internal_approx_equal_worker
  (
  const BaseCube<typename T1::elem_type,T1>& A,
  const BaseCube<typename T1::elem_type,T2>& B,
  const typename T1::pod_type abs_tol,
  const typename T1::pod_type rel_tol
  )
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;
  typedef typename T1::pod_type   T;

  arma_debug_check( ((use_abs_diff == false) && (use_rel_diff == false)), "internal_approx_equal_worker(): both 'use_abs_diff' and 'use_rel_diff' are false" );

  if(use_abs_diff)  { arma_debug_check( cond_rel< is_signed<T>::value >::lt(abs_tol, T(0)), "approx_equal(): argument 'abs_tol' must be >= 0" ); }
  if(use_rel_diff)  { arma_debug_check( cond_rel< is_signed<T>::value >::lt(rel_tol, T(0)), "approx_equal(): argument 'rel_tol' must be >= 0" ); }

  const ProxyCube<T1> PA(A.get_ref());
  const ProxyCube<T2> PB(B.get_ref());

  if( (PA.get_n_rows() != PB.get_n_rows()) || (PA.get_n_cols() != PB.get_n_cols()) || (PA.get_n_slices() != PB.get_n_slices()) )  { return false; }

  if( (ProxyCube<T1>::use_at == false) && (ProxyCube<T2>::use_at == false) )
    {
    const uword N = PA.get_n_elem();

    const typename ProxyCube<T1>::ea_type PA_ea = PA.get_ea();
    const typename ProxyCube<T2>::ea_type PB_ea = PB.get_ea();

    for(uword i=0; i<N; ++i)
      {
      const eT x = PA_ea[i];
      const eT y = PB_ea[i];

      const bool state_abs_diff = (use_abs_diff) ? internal_approx_equal_abs_diff(x, y, abs_tol) : true;
      const bool state_rel_diff = (use_rel_diff) ? internal_approx_equal_rel_diff(x, y, rel_tol) : true;

      if(use_abs_diff && use_rel_diff)
        {
        if((state_abs_diff == false) && (state_rel_diff == false))  { return false; }
        }
      else
      if(use_abs_diff)
        {
        if(state_abs_diff == false)  { return false; }
        }
      else
      if(use_rel_diff)
        {
        if(state_rel_diff == false)  { return false; }
        }
      }
    }
  else
    {
    const uword n_rows   = PA.get_n_rows();
    const uword n_cols   = PA.get_n_cols();
    const uword n_slices = PA.get_n_slices();

    for(uword slice=0; slice < n_slices; ++slice)
    for(uword   col=0;   col < n_cols;   ++col  )
    for(uword   row=0;   row < n_rows;   ++row  )
      {
      const eT x = PA.at(row,col,slice);
      const eT y = PB.at(row,col,slice);

      const bool state_abs_diff = (use_abs_diff) ? internal_approx_equal_abs_diff(x, y, abs_tol) : true;
      const bool state_rel_diff = (use_rel_diff) ? internal_approx_equal_rel_diff(x, y, rel_tol) : true;

      if(use_abs_diff && use_rel_diff)
        {
        if((state_abs_diff == false) && (state_rel_diff == false))  { return false; }
        }
      else
      if(use_abs_diff)
        {
        if(state_abs_diff == false)  { return false; }
        }
      else
      if(use_rel_diff)
        {
        if(state_rel_diff == false)  { return false; }
        }
      }
    }

  return true;
  }



template<typename T1, typename T2>
inline
bool
internal_approx_equal_handler(const T1& A, const T2& B, const char* method, const typename T1::pod_type abs_tol, const typename T1::pod_type rel_tol)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::pod_type T;

  const char sig = (method != NULL) ? method[0] : char(0);

  arma_debug_check( ((sig != 'a') && (sig != 'r') && (sig != 'b')), "approx_equal(): argument 'method' must be \"absdiff\" or \"reldiff\" or \"both\"" );

  bool status = false;

  if(sig == 'a')
    {
    status = internal_approx_equal_worker<true,false>(A, B, abs_tol, T(0));
    }
  else
  if(sig == 'r')
    {
    status = internal_approx_equal_worker<false,true>(A, B, T(0), rel_tol);
    }
  else
  if(sig == 'b')
    {
    status = internal_approx_equal_worker<true,true>(A, B, abs_tol, rel_tol);
    }

  return status;
  }



template<typename T1, typename T2>
inline
bool
internal_approx_equal_handler(const T1& A, const T2& B, const char* method, const typename T1::pod_type tol)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::pod_type T;

  const char sig = (method != NULL) ? method[0] : char(0);

  arma_debug_check( ((sig != 'a') && (sig != 'r') && (sig != 'b')), "approx_equal(): argument 'method' must be \"absdiff\" or \"reldiff\" or \"both\"" );

  arma_debug_check( (sig == 'b'), "approx_equal(): argument 'method' is \"both\", but only one 'tol' argument has been given" );

  bool status = false;

  if(sig == 'a')
    {
    status = internal_approx_equal_worker<true,false>(A, B, tol, T(0));
    }
  else
  if(sig == 'r')
    {
    status = internal_approx_equal_worker<false,true>(A, B, T(0), tol);
    }

  return status;
  }



template<typename T1, typename T2>
arma_warn_unused
inline
bool
approx_equal(const Base<typename T1::elem_type,T1>& A, const Base<typename T1::elem_type,T2>& B, const char* method, const typename T1::pod_type tol)
  {
  arma_extra_debug_sigprint();

  return internal_approx_equal_handler(A.get_ref(), B.get_ref(), method, tol);
  }



template<typename T1, typename T2>
arma_warn_unused
inline
bool
approx_equal(const BaseCube<typename T1::elem_type,T1>& A, const BaseCube<typename T1::elem_type,T2>& B, const char* method, const typename T1::pod_type tol)
  {
  arma_extra_debug_sigprint();

  return internal_approx_equal_handler(A.get_ref(), B.get_ref(), method, tol);
  }



template<typename T1, typename T2>
arma_warn_unused
inline
bool
approx_equal(const Base<typename T1::elem_type,T1>& A, const Base<typename T1::elem_type,T2>& B, const char* method, const typename T1::pod_type abs_tol, const typename T1::pod_type rel_tol)
  {
  arma_extra_debug_sigprint();

  return internal_approx_equal_handler(A.get_ref(), B.get_ref(), method, abs_tol, rel_tol);
  }



template<typename T1, typename T2>
arma_warn_unused
inline
bool
approx_equal(const BaseCube<typename T1::elem_type,T1>& A, const BaseCube<typename T1::elem_type,T2>& B, const char* method, const typename T1::pod_type abs_tol, const typename T1::pod_type rel_tol)
  {
  arma_extra_debug_sigprint();

  return internal_approx_equal_handler(A.get_ref(), B.get_ref(), method, abs_tol, rel_tol);
  }



template<typename T1, typename T2>
arma_warn_unused
inline
bool
approx_equal(const SpBase<typename T1::elem_type,T1>& A, const SpBase<typename T1::elem_type,T2>& B, const char* method, const typename T1::pod_type tol)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;
  typedef typename T1::pod_type   T;

  const char sig = (method != NULL) ? method[0] : char(0);

  arma_debug_check( ((sig != 'a') && (sig != 'r') && (sig != 'b')), "approx_equal(): argument 'method' must be \"absdiff\" or \"reldiff\" or \"both\"" );

  arma_debug_check( (sig == 'b'), "approx_equal(): argument 'method' is \"both\", but only one 'tol' argument has been given" );

  arma_debug_check( (sig == 'r'), "approx_equal(): only the \"absdiff\" method is currently implemented for sparse matrices" );

  arma_debug_check( cond_rel< is_signed<T>::value >::lt(tol, T(0)), "approx_equal(): argument 'tol' must be >= 0" );

  const unwrap_spmat<T1> UA(A.get_ref());
  const unwrap_spmat<T2> UB(B.get_ref());

  if( (UA.M.n_rows != UB.M.n_rows) || (UA.M.n_cols != UB.M.n_cols) )  { return false; }

  const SpMat<eT> C = UA.M - UB.M;

  typename SpMat<eT>::const_iterator it     = C.begin();
  typename SpMat<eT>::const_iterator it_end = C.end();

  while(it != it_end)
    {
    const eT val = (*it);

    if( arma_isnan(val) || (eop_aux::arma_abs(val) > tol) )  { return false; }

    ++it;
    }

  return true;
  }



template<typename T1, typename T2>
arma_warn_unused
inline
bool
approx_equal(const SpBase<typename T1::elem_type,T1>& A, const SpBase<typename T1::elem_type,T2>& B, const char* method, const typename T1::pod_type abs_tol, const typename T1::pod_type rel_tol)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::pod_type T;

  const char sig = (method != NULL) ? method[0] : char(0);

  arma_debug_check( ((sig != 'a') && (sig != 'r') && (sig != 'b')), "approx_equal(): argument 'method' must be \"absdiff\" or \"reldiff\" or \"both\"" );

  arma_debug_check( ((sig == 'r') || (sig == 'b')), "approx_equal(): only the \"absdiff\" method is currently implemented for sparse matrices" );

  arma_debug_check( cond_rel< is_signed<T>::value >::lt(abs_tol, T(0)), "approx_equal(): argument 'abs_tol' must be >= 0" );
  arma_debug_check( cond_rel< is_signed<T>::value >::lt(rel_tol, T(0)), "approx_equal(): argument 'rel_tol' must be >= 0" );

  return approx_equal(A.get_ref(), B.get_ref(), "abs", abs_tol);
  }



//! @}
