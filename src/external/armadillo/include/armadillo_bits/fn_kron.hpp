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


//! \addtogroup fn_kron
//! @{



template<typename T1, typename T2>
arma_warn_unused
arma_inline
const Glue<T1,T2,glue_kron>
kron(const Base<typename T1::elem_type,T1>& A, const Base<typename T1::elem_type,T2>& B)
  {
  arma_extra_debug_sigprint();

  return Glue<T1, T2, glue_kron>(A.get_ref(), B.get_ref());
  }



template<typename T, typename T1, typename T2>
arma_warn_unused
inline
Mat<typename eT_promoter<T1,T2>::eT>
kron(const Base<std::complex<T>,T1>& X, const Base<T,T2>& Y)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT1;

  promote_type<eT1,T>::check();

  const unwrap<T1> tmp1(X.get_ref());
  const unwrap<T2> tmp2(Y.get_ref());

  const Mat<eT1>& A = tmp1.M;
  const Mat<T  >& B = tmp2.M;

  Mat<eT1> out;

  glue_kron::direct_kron(out, A, B);

  return out;
  }



template<typename T, typename T1, typename T2>
arma_warn_unused
inline
Mat<typename eT_promoter<T1,T2>::eT>
kron(const Base<T,T1>& X, const Base<std::complex<T>,T2>& Y)
  {
  arma_extra_debug_sigprint();

  typedef typename std::complex<T> eT2;

  promote_type<T,eT2>::check();

  const unwrap<T1> tmp1(X.get_ref());
  const unwrap<T2> tmp2(Y.get_ref());

  const Mat<T  >& A = tmp1.M;
  const Mat<eT2>& B = tmp2.M;

  Mat<eT2> out;

  glue_kron::direct_kron(out, A, B);

  return out;
  }



// template<typename T1, typename T2>
// arma_warn_unused
// inline
// SpMat<typename T1::elem_type>
// kron(const SpBase<typename T1::elem_type, T1>& A_expr, const SpBase<typename T1::elem_type, T2>& B_expr)
//   {
//   arma_extra_debug_sigprint();
//
//   typedef typename T1::elem_type eT;
//
//   const unwrap_spmat<T1> UA(A_expr.get_ref());
//   const unwrap_spmat<T2> UB(B_expr.get_ref());
//
//   const SpMat<eT>& A = UA.M;
//   const SpMat<eT>& B = UB.M;
//
//   const uword A_rows = A.n_rows;
//   const uword A_cols = A.n_cols;
//   const uword B_rows = B.n_rows;
//   const uword B_cols = B.n_cols;
//
//   SpMat<eT> out(A_rows*B_rows, A_cols*B_cols);
//
//   if(out.is_empty() == false)
//     {
//     typename SpMat<eT>::const_iterator it     = A.begin();
//     typename SpMat<eT>::const_iterator it_end = A.end();
//
//     for(; it != it_end; ++it)
//       {
//       const uword i = it.row();
//       const uword j = it.col();
//
//       const eT A_val = (*it);
//
//       out.submat(i*B_rows, j*B_cols, (i+1)*B_rows-1, (j+1)*B_cols-1) = A_val * B;
//       }
//     }
//
//   return out;
//   }



//! @}
