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


//! \addtogroup auxlib
//! @{


//! interface functions for accessing decompositions in LAPACK and ATLAS
class auxlib
  {
  public:


  template<const uword row, const uword col>
  struct pos
    {
    static const uword n2 = row + col*2;
    static const uword n3 = row + col*3;
    static const uword n4 = row + col*4;
    };


  //
  // inv

  template<typename eT, typename T1>
  inline static bool inv(Mat<eT>& out, const Base<eT,T1>& X);

  template<typename eT>
  inline static bool inv(Mat<eT>& out, const Mat<eT>& A);

  template<typename eT>
  inline static bool inv_noalias_tinymat(Mat<eT>& out, const Mat<eT>& X, const uword N);

  template<typename eT>
  inline static bool inv_inplace_lapack(Mat<eT>& out);


  //
  // inv_tr

  template<typename eT, typename T1>
  inline static bool inv_tr(Mat<eT>& out, const Base<eT,T1>& X, const uword layout);


  //
  // inv_sym

  template<typename eT, typename T1>
  inline static bool inv_sym(Mat<eT>& out, const Base<eT,T1>& X, const uword layout);


  //
  // inv_sympd

  template<typename eT, typename T1>
  inline static bool inv_sympd(Mat<eT>& out, const Base<eT,T1>& X);


  //
  // det

  template<typename eT, typename T1>
  inline static eT det(const Base<eT,T1>& X);

  template<typename eT>
  inline static eT det_tinymat(const Mat<eT>& X, const uword N);

  template<typename eT>
  inline static eT det_lapack(const Mat<eT>& X, const bool make_copy);


  //
  // log_det

  template<typename eT, typename T1>
  inline static bool log_det(eT& out_val, typename get_pod_type<eT>::result& out_sign, const Base<eT,T1>& X);


  //
  // lu

  template<typename eT, typename T1>
  inline static bool lu(Mat<eT>& L, Mat<eT>& U, podarray<blas_int>& ipiv, const Base<eT,T1>& X);

  template<typename eT, typename T1>
  inline static bool lu(Mat<eT>& L, Mat<eT>& U, Mat<eT>& P, const Base<eT,T1>& X);

  template<typename eT, typename T1>
  inline static bool lu(Mat<eT>& L, Mat<eT>& U, const Base<eT,T1>& X);


  //
  // eig_gen

  template<typename T1>
  inline static bool eig_gen(Mat< std::complex<typename T1::pod_type> >& vals, Mat< std::complex<typename T1::pod_type> >& vecs, const bool vecs_on, const Base<typename T1::pod_type,T1>& expr);

  template<typename T1>
  inline static bool eig_gen(Mat< std::complex<typename T1::pod_type> >& vals, Mat< std::complex<typename T1::pod_type> >& vecs, const bool vecs_on, const Base< std::complex<typename T1::pod_type>, T1 >& expr);


  //
  // eig_pair

  template<typename T1, typename T2>
  inline static bool eig_pair(Mat< std::complex<typename T1::pod_type> >& vals, Mat< std::complex<typename T1::pod_type> >& vecs, const bool vecs_on, const Base<typename T1::pod_type,T1>& A_expr, const Base<typename T1::pod_type,T2>& B_expr);

  template<typename T1, typename T2>
  inline static bool eig_pair(Mat< std::complex<typename T1::pod_type> >& vals, Mat< std::complex<typename T1::pod_type> >& vecs, const bool vecs_on, const Base< std::complex<typename T1::pod_type>, T1 >& A_expr, const Base< std::complex<typename T1::pod_type>, T2 >& B_expr);


  //
  // eig_sym

  template<typename eT, typename T1>
  inline static bool eig_sym(Col<eT>& eigval, const Base<eT,T1>& X);

  template<typename T, typename T1>
  inline static bool eig_sym(Col<T>& eigval, const Base<std::complex<T>,T1>& X);

  template<typename eT, typename T1>
  inline static bool eig_sym(Col<eT>& eigval, Mat<eT>& eigvec, const Base<eT,T1>& X);

  template<typename T, typename T1>
  inline static bool eig_sym(Col<T>& eigval, Mat< std::complex<T> >& eigvec, const Base<std::complex<T>,T1>& X);

  template<typename eT, typename T1>
  inline static bool eig_sym_dc(Col<eT>& eigval, Mat<eT>& eigvec, const Base<eT,T1>& X);

  template<typename T, typename T1>
  inline static bool eig_sym_dc(Col<T>& eigval, Mat< std::complex<T> >& eigvec, const Base<std::complex<T>,T1>& X);


  //
  // chol

  template<typename eT, typename T1>
  inline static bool chol(Mat<eT>& out, const Base<eT,T1>& X, const uword layout);


  //
  // qr

  template<typename eT, typename T1>
  inline static bool qr(Mat<eT>& Q, Mat<eT>& R, const Base<eT,T1>& X);

  template<typename eT, typename T1>
  inline static bool qr_econ(Mat<eT>& Q, Mat<eT>& R, const Base<eT,T1>& X);


  //
  // svd

  template<typename eT, typename T1>
  inline static bool svd(Col<eT>& S, const Base<eT,T1>& X, uword& n_rows, uword& n_cols);

  template<typename T, typename T1>
  inline static bool svd(Col<T>& S, const Base<std::complex<T>, T1>& X, uword& n_rows, uword& n_cols);

  template<typename eT, typename T1>
  inline static bool svd(Col<eT>& S, const Base<eT,T1>& X);

  template<typename T, typename T1>
  inline static bool svd(Col<T>& S, const Base<std::complex<T>, T1>& X);

  template<typename eT, typename T1>
  inline static bool svd(Mat<eT>& U, Col<eT>& S, Mat<eT>& V, const Base<eT,T1>& X);

  template<typename T, typename T1>
  inline static bool svd(Mat< std::complex<T> >& U, Col<T>& S, Mat< std::complex<T> >& V, const Base< std::complex<T>, T1>& X);

  template<typename eT, typename T1>
  inline static bool svd_econ(Mat<eT>& U, Col<eT>& S, Mat<eT>& V, const Base<eT,T1>& X, const char mode);

  template<typename T, typename T1>
  inline static bool svd_econ(Mat< std::complex<T> >& U, Col<T>& S, Mat< std::complex<T> >& V, const Base< std::complex<T>, T1>& X, const char mode);


  template<typename eT, typename T1>
  inline static bool svd_dc(Col<eT>& S, const Base<eT,T1>& X, uword& n_rows, uword& n_cols);

  template<typename T, typename T1>
  inline static bool svd_dc(Col<T>& S, const Base<std::complex<T>, T1>& X, uword& n_rows, uword& n_cols);

  template<typename eT, typename T1>
  inline static bool svd_dc(Col<eT>& S, const Base<eT,T1>& X);

  template<typename T, typename T1>
  inline static bool svd_dc(Col<T>& S, const Base<std::complex<T>, T1>& X);


  template<typename eT, typename T1>
  inline static bool svd_dc(Mat<eT>& U, Col<eT>& S, Mat<eT>& V, const Base<eT,T1>& X);

  template<typename T, typename T1>
  inline static bool svd_dc(Mat< std::complex<T> >& U, Col<T>& S, Mat< std::complex<T> >& V, const Base< std::complex<T>, T1>& X);

  template<typename eT, typename T1>
  inline static bool svd_dc_econ(Mat<eT>& U, Col<eT>& S, Mat<eT>& V, const Base<eT,T1>& X);

  template<typename T, typename T1>
  inline static bool svd_dc_econ(Mat< std::complex<T> >& U, Col<T>& S, Mat< std::complex<T> >& V, const Base< std::complex<T>, T1>& X);


  //
  // solve

  template<typename T1>
  inline static bool solve_square_fast(Mat<typename T1::elem_type>& out, Mat<typename T1::elem_type>& A, const Base<typename T1::elem_type,T1>& B_expr);

  template<typename T1>
  inline static bool solve_square_refine(Mat<typename T1::pod_type>& out, typename T1::pod_type& out_rcond, Mat<typename T1::pod_type>& A, const Base<typename T1::pod_type,T1>& B_expr, const bool equilibrate);

  template<typename T1>
  inline static bool solve_square_refine(Mat< std::complex<typename T1::pod_type> >& out, typename T1::pod_type& out_rcond, Mat< std::complex<typename T1::pod_type> >& A, const Base<std::complex<typename T1::pod_type>,T1>& B_expr, const bool equilibrate);

  template<typename T1>
  inline static bool solve_approx_fast(Mat<typename T1::elem_type>& out, Mat<typename T1::elem_type>& A, const Base<typename T1::elem_type,T1>& B_expr);

  template<typename T1>
  inline static bool solve_approx_svd(Mat<typename T1::pod_type>& out, Mat<typename T1::pod_type>& A, const Base<typename T1::pod_type,T1>& B_expr);

  template<typename T1>
  inline static bool solve_approx_svd(Mat< std::complex<typename T1::pod_type> >& out, Mat< std::complex<typename T1::pod_type> >& A, const Base<std::complex<typename T1::pod_type>,T1>& B_expr);

  template<typename T1>
  inline static bool solve_tri(Mat<typename T1::elem_type>& out, const Mat<typename T1::elem_type>& A, const Base<typename T1::elem_type,T1>& B_expr, const uword layout);


  //
  // Schur decomposition

  template<typename eT, typename T1>
  inline static bool schur(Mat<eT>& U, Mat<eT>& S, const Base<eT,T1>& X, const bool calc_U = true);

  template<typename  T, typename T1>
  inline static bool schur(Mat<std::complex<T> >& U, Mat<std::complex<T> >& S, const Base<std::complex<T>,T1>& X, const bool calc_U = true);

  template<typename  T>
  inline static bool schur(Mat<std::complex<T> >& U, Mat<std::complex<T> >& S, const bool calc_U = true);

  //
  // syl (solution of the Sylvester equation AX + XB = C)

  template<typename eT>
  inline static bool syl(Mat<eT>& X, const Mat<eT>& A, const Mat<eT>& B, const Mat<eT>& C);


  //
  // QZ decomposition

  template<typename T, typename T1, typename T2>
  inline static bool qz(Mat<T>& A, Mat<T>& B, Mat<T>& vsl, Mat<T>& vsr, const Base<T,T1>& X_expr, const Base<T,T2>& Y_expr, const char mode);

  template<typename T, typename T1, typename T2>
  inline static bool qz(Mat< std::complex<T> >& A, Mat< std::complex<T> >& B, Mat< std::complex<T> >& vsl, Mat< std::complex<T> >& vsr, const Base< std::complex<T>, T1 >& X_expr, const Base< std::complex<T>, T2 >& Y_expr, const char mode);


  //
  // rcond

  template<typename T1>
  inline static typename T1::pod_type rcond(const Base<typename T1::pod_type,T1>& A_expr);

  template<typename T1>
  inline static typename T1::pod_type rcond(const Base<std::complex<typename T1::pod_type>,T1>& A_expr);
  };



namespace qz_helper
  {
  template<typename T> inline blas_int select_lhp(const T* x_ptr, const T* y_ptr, const T* z_ptr);
  template<typename T> inline blas_int select_rhp(const T* x_ptr, const T* y_ptr, const T* z_ptr);
  template<typename T> inline blas_int select_iuc(const T* x_ptr, const T* y_ptr, const T* z_ptr);
  template<typename T> inline blas_int select_ouc(const T* x_ptr, const T* y_ptr, const T* z_ptr);

  template<typename T> inline blas_int cx_select_lhp(const std::complex<T>* x_ptr, const std::complex<T>* y_ptr);
  template<typename T> inline blas_int cx_select_rhp(const std::complex<T>* x_ptr, const std::complex<T>* y_ptr);
  template<typename T> inline blas_int cx_select_iuc(const std::complex<T>* x_ptr, const std::complex<T>* y_ptr);
  template<typename T> inline blas_int cx_select_ouc(const std::complex<T>* x_ptr, const std::complex<T>* y_ptr);

  template<typename T> inline void_ptr ptr_cast(blas_int (*function)(const T*, const T*, const T*));
  template<typename T> inline void_ptr ptr_cast(blas_int (*function)(const std::complex<T>*, const std::complex<T>*));
  }


//! @}
