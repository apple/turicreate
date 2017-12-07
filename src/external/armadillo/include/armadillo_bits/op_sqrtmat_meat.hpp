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


//! \addtogroup op_sqrtmat
//! @{


//! implementation partly based on:
//! N. J. Higham.
//! A New sqrtm for Matlab.
//! Numerical Analysis Report No. 336, January 1999.
//! Department of Mathematics, University of Manchester.
//! ISSN 1360-1725
//! http://www.maths.manchester.ac.uk/~higham/narep/narep336.ps.gz


template<typename T1>
inline
void
op_sqrtmat::apply(Mat< std::complex<typename T1::elem_type> >& out, const mtOp<std::complex<typename T1::elem_type>,T1,op_sqrtmat>& in)
  {
  arma_extra_debug_sigprint();

  const bool status = op_sqrtmat::apply_direct(out, in.m);

  if(status == false)
    {
    arma_debug_warn("sqrtmat(): given matrix seems singular; may not have a square root");
    }
  }



template<typename T1>
inline
bool
op_sqrtmat::apply_direct(Mat< std::complex<typename T1::elem_type> >& out, const Op<T1,op_diagmat>& expr)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type T;

  const diagmat_proxy<T1> P(expr.m);

  arma_debug_check( (P.n_rows != P.n_cols), "sqrtmat(): given matrix must be square sized" );

  const uword N = P.n_rows;

  out.zeros(N,N);

  bool singular = false;

  for(uword i=0; i<N; ++i)
    {
    const T val = P[i];

    if(val >= T(0))
      {
      singular = (singular || (val == T(0)));

      out.at(i,i) = std::sqrt(val);
      }
    else
      {
      out.at(i,i) = std::sqrt( std::complex<T>(val) );
      }
    }

  return (singular) ? false : true;
  }



template<typename T1>
inline
bool
op_sqrtmat::apply_direct(Mat< std::complex<typename T1::elem_type> >& out, const Base<typename T1::elem_type,T1>& expr)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type       in_T;
  typedef typename std::complex<in_T> out_T;

  const Proxy<T1> P(expr.get_ref());

  arma_debug_check( (P.get_n_rows() != P.get_n_cols()), "sqrtmat(): given matrix must be square sized" );

  if(P.get_n_elem() == 0)
    {
    out.reset();
    return true;
    }

  typename Proxy<T1>::ea_type Pea = P.get_ea();

  Mat<out_T> U;
  Mat<out_T> S(P.get_n_rows(), P.get_n_cols());

  out_T* Smem = S.memptr();

  const uword N = P.get_n_elem();

  for(uword i=0; i<N; ++i)
    {
    Smem[i] = std::complex<in_T>( Pea[i] );
    }

  const bool schur_ok = auxlib::schur(U,S);

  if(schur_ok == false)
    {
    arma_extra_debug_print("sqrtmat(): schur decomposition failed");
    out.soft_reset();
    return false;
    }

  const bool status = op_sqrtmat_cx::helper(S);

  const Mat<out_T> X = U*S;

  S.reset();

  out = X*U.t();

  return status;
  }



template<typename T1>
inline
void
op_sqrtmat_cx::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_sqrtmat_cx>& in)
  {
  arma_extra_debug_sigprint();

  const bool status = op_sqrtmat_cx::apply_direct(out, in.m);

  if(status == false)
    {
    arma_debug_warn("sqrtmat(): given matrix seems singular; may not have a square root");
    }
  }



template<typename T1>
inline
bool
op_sqrtmat_cx::apply_direct(Mat<typename T1::elem_type>& out, const Op<T1,op_diagmat>& expr)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const diagmat_proxy<T1> P(expr.m);

  bool status = false;

  if(P.is_alias(out))
    {
    Mat<eT> tmp;

    status = op_sqrtmat_cx::apply_direct_noalias(tmp, P);

    out.steal_mem(tmp);
    }
  else
    {
    status = op_sqrtmat_cx::apply_direct_noalias(out, P);
    }

  return status;
  }



template<typename T1>
inline
bool
op_sqrtmat_cx::apply_direct_noalias(Mat<typename T1::elem_type>& out, const diagmat_proxy<T1>& P)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  arma_debug_check( (P.n_rows != P.n_cols), "sqrtmat(): given matrix must be square sized" );

  const uword N = P.n_rows;

  out.zeros(N,N);

  const eT zero = eT(0);

  bool singular = false;

  for(uword i=0; i<N; ++i)
    {
    const eT val = P[i];

    singular = (singular || (val == zero));

    out.at(i,i) = std::sqrt(val);
    }

  return (singular) ? false : true;
  }



template<typename T1>
inline
bool
op_sqrtmat_cx::apply_direct(Mat<typename T1::elem_type>& out, const Base<typename T1::elem_type,T1>& expr)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  Mat<eT> U;
  Mat<eT> S = expr.get_ref();

  if(S.is_empty())
    {
    out.reset();
    return true;
    }

  arma_debug_check( (S.n_rows != S.n_cols), "sqrtmat(): given matrix must be square sized" );

  const bool schur_ok = auxlib::schur(U, S);

  if(schur_ok == false)
    {
    arma_extra_debug_print("sqrtmat(): schur decomposition failed");
    out.soft_reset();
    return false;
    }

  const bool status = op_sqrtmat_cx::helper(S);

  const Mat<eT> X = U*S;

  S.reset();

  out = X*U.t();

  return status;
  }



template<typename T>
inline
bool
op_sqrtmat_cx::helper(Mat< std::complex<T> >& S)
  {
  typedef typename std::complex<T> eT;

  if(S.is_empty())  { return true; }

  const uword N = S.n_rows;

  const eT zero = eT(0);

  eT& S_00 = S[0];

  bool singular = (S_00 == zero);

  S_00 = std::sqrt(S_00);

  for(uword j=1; j < N; ++j)
    {
    eT* S_j = S.colptr(j);

    eT& S_jj = S_j[j];

    singular = (singular || (S_jj == zero));

    S_jj = std::sqrt(S_jj);

    for(uword ii=0; ii <= (j-1); ++ii)
      {
      const uword i = (j-1) - ii;

      const eT* S_i = S.colptr(i);

      //S_j[i] /= (S_i[i] + S_j[j]);
      S_j[i] /= (S_i[i] + S_jj);

      for(uword k=0; k < i; ++k)
        {
        S_j[k] -= S_i[k] * S_j[i];
        }
      }
    }

  return (singular) ? false : true;
  }



template<typename T1>
inline
void
op_sqrtmat_sympd::apply(Mat<typename T1::elem_type>& out, const Op<T1,op_sqrtmat_sympd>& in)
  {
  arma_extra_debug_sigprint();

  const bool status = op_sqrtmat_sympd::apply_direct(out, in.m);

  if(status == false)
    {
    out.soft_reset();
    arma_stop_runtime_error("sqrtmat_sympd(): transformation failed");
    }
  }



template<typename T1>
inline
bool
op_sqrtmat_sympd::apply_direct(Mat<typename T1::elem_type>& out, const Base<typename T1::elem_type,T1>& expr)
  {
  arma_extra_debug_sigprint();

  #if defined(ARMA_USE_LAPACK)
    {
    typedef typename T1::pod_type   T;
    typedef typename T1::elem_type eT;

    const unwrap<T1>   U(expr.get_ref());
    const Mat<eT>& X = U.M;

    arma_debug_check( (X.is_square() == false), "sqrtmat_sympd(): given matrix must be square sized" );

    Col< T> eigval;
    Mat<eT> eigvec;

    const bool status = auxlib::eig_sym_dc(eigval, eigvec, X);

    if(status == false)  { return false; }

    const uword N          = eigval.n_elem;
    const T*    eigval_mem = eigval.memptr();

    bool all_pos = true;

    for(uword i=0; i<N; ++i)  { all_pos = (eigval_mem[i] < T(0)) ? false : all_pos; }

    if(all_pos == false)  { return false; }

    eigval = sqrt(eigval);

    out = eigvec * diagmat(eigval) * eigvec.t();

    return true;
    }
  #else
    {
    arma_ignore(out);
    arma_ignore(expr);
    arma_stop_logic_error("sqrtmat_sympd(): use of LAPACK must be enabled");
    return false;
    }
  #endif
  }



//! @}
