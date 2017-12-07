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


//! \addtogroup sp_auxlib
//! @{


//! wrapper for accesing external functions for sparse matrices defined in ARPACK
class sp_auxlib
  {
  public:

  enum form_type
    {
    form_none, form_lm, form_sm, form_lr, form_la, form_sr, form_li, form_si, form_sa
    };

  inline static form_type interpret_form_str(const char* form_str);

  //
  // eigs_sym()

  template<typename eT, typename T1>
  inline static bool eigs_sym(Col<eT>& eigval, Mat<eT>& eigvec, const SpBase<eT, T1>& X, const uword n_eigvals, const char* form_str, const eT default_tol);

  template<typename eT, typename T1>
  inline static bool eigs_sym_newarp(Col<eT>& eigval, Mat<eT>& eigvec, const SpBase<eT, T1>& X, const uword n_eigvals, const char* form_str, const eT default_tol);

  template<typename eT, typename T1>
  inline static bool eigs_sym_arpack(Col<eT>& eigval, Mat<eT>& eigvec, const SpBase<eT, T1>& X, const uword n_eigvals, const char* form_str, const eT default_tol);

  //
  // eigs_gen()

  template<typename T, typename T1>
  inline static bool eigs_gen(Col< std::complex<T> >& eigval, Mat< std::complex<T> >& eigvec, const SpBase<T, T1>& X, const uword n_eigvals, const char* form_str, const T default_tol);

  template<typename T, typename T1>
  inline static bool eigs_gen_newarp(Col< std::complex<T> >& eigval, Mat< std::complex<T> >& eigvec, const SpBase<T, T1>& X, const uword n_eigvals, const char* form_str, const T default_tol);

  template<typename T, typename T1>
  inline static bool eigs_gen_arpack(Col< std::complex<T> >& eigval, Mat< std::complex<T> >& eigvec, const SpBase<T, T1>& X, const uword n_eigvals, const char* form_str, const T default_tol);

  template<typename T, typename T1>
  inline static bool eigs_gen(Col< std::complex<T> >& eigval, Mat< std::complex<T> >& eigvec, const SpBase< std::complex<T>, T1>& X, const uword n_eigvals, const char* form_str, const T default_tol);


  //
  // spsolve() via SuperLU

  template<typename T1, typename T2>
  inline static bool spsolve_simple(Mat<typename T1::elem_type>& out, const SpBase<typename T1::elem_type, T1>& A, const Base<typename T1::elem_type, T2>& B, const superlu_opts& user_opts);

  template<typename T1, typename T2>
  inline static bool spsolve_refine(Mat<typename T1::elem_type>& out, typename T1::pod_type& out_rcond, const SpBase<typename T1::elem_type, T1>& A, const Base<typename T1::elem_type, T2>& B, const superlu_opts& user_opts);

  #if defined(ARMA_USE_SUPERLU)
    inline static void set_superlu_opts(superlu::superlu_options_t& options, const superlu_opts& user_opts);

    template<typename eT>
    inline static bool copy_to_supermatrix(superlu::SuperMatrix& out, const SpMat<eT>& A);

    template<typename eT>
    inline static bool wrap_to_supermatrix(superlu::SuperMatrix& out, const Mat<eT>& A);

    inline static void destroy_supermatrix(superlu::SuperMatrix& out);
  #endif



  private:

  // calls arpack saupd()/naupd() because the code is so similar for each
  // all of the extra variables are later used by seupd()/neupd(), but those
  // functions are very different and we can't combine their code

  template<typename eT, typename T, typename T1>
  inline static void run_aupd
    (
    const uword n_eigvals, char* which, const SpProxy<T1>& p, const bool sym,
    blas_int& n, eT& tol,
    podarray<T>& resid, blas_int& ncv, podarray<T>& v, blas_int& ldv,
    podarray<blas_int>& iparam, podarray<blas_int>& ipntr,
    podarray<T>& workd, podarray<T>& workl, blas_int& lworkl, podarray<eT>& rwork,
    blas_int& info
    );

  };
