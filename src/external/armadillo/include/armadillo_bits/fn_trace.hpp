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


//! \addtogroup fn_trace
//! @{


template<typename T1>
arma_warn_unused
arma_hot
inline
typename enable_if2<is_arma_type<T1>::value, typename T1::elem_type>::result
trace(const T1& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T1> A(X);

  const uword N = (std::min)(A.get_n_rows(), A.get_n_cols());

  eT val1 = eT(0);
  eT val2 = eT(0);

  uword i,j;
  for(i=0, j=1; j<N; i+=2, j+=2)
    {
    val1 += A.at(i,i);
    val2 += A.at(j,j);
    }

  if(i < N)
    {
    val1 += A.at(i,i);
    }

  return val1 + val2;
  }



template<typename T1>
arma_warn_unused
arma_hot
inline
typename T1::elem_type
trace(const Op<T1, op_diagmat>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const diagmat_proxy<T1> A(X.m);

  const uword N = (std::min)(A.n_rows, A.n_cols);

  eT val = eT(0);

  for(uword i=0; i<N; ++i)
    {
    val += A[i];
    }

  return val;
  }



template<typename T1, typename T2>
arma_hot
inline
typename T1::elem_type
trace_mul_unwrap(const Proxy<T1>& PA, const T2& XB)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const unwrap<T2> tmpB(XB);

  const Mat<eT>& B = tmpB.M;

  const uword A_n_rows = PA.get_n_rows();
  const uword A_n_cols = PA.get_n_cols();

  const uword B_n_rows = B.n_rows;
  const uword B_n_cols = B.n_cols;

  arma_debug_assert_mul_size(A_n_rows, A_n_cols, B_n_rows, B_n_cols, "matrix multiplication");

  const uword N = (std::min)(A_n_rows, B_n_cols);

  eT val = eT(0);

  for(uword k=0; k < N; ++k)
    {
    const eT* B_colptr = B.colptr(k);

    eT acc1 = eT(0);
    eT acc2 = eT(0);

    uword j;

    for(j=1; j < A_n_cols; j+=2)
      {
      const uword i = (j-1);

      const eT tmp_i = B_colptr[i];
      const eT tmp_j = B_colptr[j];

      acc1 += PA.at(k, i) * tmp_i;
      acc2 += PA.at(k, j) * tmp_j;
      }

    const uword i = (j-1);

    if(i < A_n_cols)
      {
      acc1 += PA.at(k, i) * B_colptr[i];
      }

    val += (acc1 + acc2);
    }

  return val;
  }



//! speedup for trace(A*B), where the result of A*B is a square sized matrix
template<typename T1, typename T2>
arma_hot
inline
typename T1::elem_type
trace_mul_proxy(const Proxy<T1>& PA, const T2& XB)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const Proxy<T2> PB(XB);

  if(is_Mat<typename Proxy<T2>::stored_type>::value)
    {
    return trace_mul_unwrap(PA, PB.Q);
    }

  const uword A_n_rows = PA.get_n_rows();
  const uword A_n_cols = PA.get_n_cols();

  const uword B_n_rows = PB.get_n_rows();
  const uword B_n_cols = PB.get_n_cols();

  arma_debug_assert_mul_size(A_n_rows, A_n_cols, B_n_rows, B_n_cols, "matrix multiplication");

  const uword N = (std::min)(A_n_rows, B_n_cols);

  eT val = eT(0);

  for(uword k=0; k < N; ++k)
    {
    eT acc1 = eT(0);
    eT acc2 = eT(0);

    uword j;

    for(j=1; j < A_n_cols; j+=2)
      {
      const uword i = (j-1);

      const eT tmp_i = PB.at(i, k);
      const eT tmp_j = PB.at(j, k);

      acc1 += PA.at(k, i) * tmp_i;
      acc2 += PA.at(k, j) * tmp_j;
      }

    const uword i = (j-1);

    if(i < A_n_cols)
      {
      acc1 += PA.at(k, i) * PB.at(i, k);
      }

    val += (acc1 + acc2);
    }

  return val;
  }



//! speedup for trace(A*B), where the result of A*B is a square sized matrix
template<typename T1, typename T2>
arma_warn_unused
arma_hot
inline
typename T1::elem_type
trace(const Glue<T1, T2, glue_times>& X)
  {
  arma_extra_debug_sigprint();

  const Proxy<T1> PA(X.A);

  return (is_Mat<T2>::value) ? trace_mul_unwrap(PA, X.B) : trace_mul_proxy(PA, X.B);
  }



//! trace of sparse object
template<typename T1>
arma_warn_unused
arma_hot
inline
typename enable_if2<is_arma_sparse_type<T1>::value, typename T1::elem_type>::result
trace(const T1& x)
  {
  arma_extra_debug_sigprint();

  const SpProxy<T1> p(x);

  typedef typename T1::elem_type eT;

  eT result = eT(0);

  typename SpProxy<T1>::const_iterator_type it     = p.begin();
  typename SpProxy<T1>::const_iterator_type it_end = p.end();

  while(it != it_end)
    {
    if(it.row() == it.col())
      {
      result += (*it);
      }

    ++it;
    }

  return result;
  }



//! @}
