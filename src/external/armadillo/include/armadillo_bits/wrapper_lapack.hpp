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



#ifdef ARMA_USE_LAPACK


//! \namespace lapack namespace for LAPACK functions
namespace lapack
  {


  template<typename eT>
  inline
  void
  getrf(blas_int* m, blas_int* n, eT* a, blas_int* lda, blas_int* ipiv, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgetrf)(m, n, (T*)a, lda, ipiv, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgetrf)(m, n, (T*)a, lda, ipiv, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_cgetrf)(m, n, (T*)a, lda, ipiv, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_zgetrf)(m, n, (T*)a, lda, ipiv, info);
      }
    }



  template<typename eT>
  inline
  void
  getri(blas_int* n,  eT* a, blas_int* lda, blas_int* ipiv, eT* work, blas_int* lwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgetri)(n, (T*)a, lda, ipiv, (T*)work, lwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgetri)(n, (T*)a, lda, ipiv, (T*)work, lwork, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_cgetri)(n, (T*)a, lda, ipiv, (T*)work, lwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_zgetri)(n, (T*)a, lda, ipiv, (T*)work, lwork, info);
      }
    }



  template<typename eT>
  inline
  void
  trtri(char* uplo, char* diag, blas_int* n, eT* a, blas_int* lda, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_strtri)(uplo, diag, n, (T*)a, lda, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dtrtri)(uplo, diag, n, (T*)a, lda, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_ctrtri)(uplo, diag, n, (T*)a, lda, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_ztrtri)(uplo, diag, n, (T*)a, lda, info);
      }
    }



  template<typename eT>
  inline
  void
  geev(char* jobvl, char* jobvr, blas_int* N, eT* a, blas_int* lda, eT* wr, eT* wi, eT* vl, blas_int* ldvl, eT* vr, blas_int* ldvr, eT* work, blas_int* lwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgeev)(jobvl, jobvr, N, (T*)a, lda, (T*)wr, (T*)wi, (T*)vl, ldvl, (T*)vr, ldvr, (T*)work, lwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgeev)(jobvl, jobvr, N, (T*)a, lda, (T*)wr, (T*)wi, (T*)vl, ldvl, (T*)vr, ldvr, (T*)work, lwork, info);
      }
    }



  template<typename eT>
  inline
  void
  cx_geev(char* jobvl, char* jobvr, blas_int* N, eT* a, blas_int* lda, eT* w, eT* vl, blas_int* ldvl, eT* vr, blas_int* ldvr, eT* work, blas_int* lwork, typename eT::value_type* rwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_supported_complex_float<eT>::value)
      {
      typedef float T;
      typedef typename std::complex<T> cx_T;
      arma_fortran(arma_cgeev)(jobvl, jobvr, N, (cx_T*)a, lda, (cx_T*)w, (cx_T*)vl, ldvl, (cx_T*)vr, ldvr, (cx_T*)work, lwork, (T*)rwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef double T;
      typedef typename std::complex<T> cx_T;
      arma_fortran(arma_zgeev)(jobvl, jobvr, N, (cx_T*)a, lda, (cx_T*)w, (cx_T*)vl, ldvl, (cx_T*)vr, ldvr, (cx_T*)work, lwork, (T*)rwork, info);
      }
    }



  template<typename eT>
  inline
  void
  syev(char* jobz, char* uplo, blas_int* n, eT* a, blas_int* lda, eT* w,  eT* work, blas_int* lwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_ssyev)(jobz, uplo, n, (T*)a, lda, (T*)w, (T*)work, lwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dsyev)(jobz, uplo, n, (T*)a, lda, (T*)w, (T*)work, lwork, info);
      }
    }



  template<typename eT>
  inline
  void
  syevd(char* jobz, char* uplo, blas_int* n, eT* a, blas_int* lda, eT* w,  eT* work, blas_int* lwork, blas_int* iwork, blas_int* liwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_ssyevd)(jobz, uplo, n, (T*)a, lda, (T*)w, (T*)work, lwork, iwork, liwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dsyevd)(jobz, uplo, n, (T*)a, lda, (T*)w, (T*)work, lwork, iwork, liwork, info);
      }
    }



  template<typename eT>
  inline
  void
  heev
    (
    char* jobz, char* uplo, blas_int* n,
    eT* a, blas_int* lda, typename eT::value_type* w,
    eT* work, blas_int* lwork, typename eT::value_type* rwork,
    blas_int* info
    )
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_supported_complex_float<eT>::value)
      {
      typedef float T;
      typedef typename std::complex<T> cx_T;
      arma_fortran(arma_cheev)(jobz, uplo, n, (cx_T*)a, lda, (T*)w, (cx_T*)work, lwork, (T*)rwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef double T;
      typedef typename std::complex<T> cx_T;
      arma_fortran(arma_zheev)(jobz, uplo, n, (cx_T*)a, lda, (T*)w, (cx_T*)work, lwork, (T*)rwork, info);
      }
    }



  template<typename eT>
  inline
  void
  heevd
    (
    char* jobz, char* uplo, blas_int* n,
    eT* a, blas_int* lda, typename eT::value_type* w,
    eT* work, blas_int* lwork, typename eT::value_type* rwork,
    blas_int* lrwork, blas_int* iwork, blas_int* liwork,
    blas_int* info
    )
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_supported_complex_float<eT>::value)
      {
      typedef float T;
      typedef typename std::complex<T> cx_T;
      arma_fortran(arma_cheevd)(jobz, uplo, n, (cx_T*)a, lda, (T*)w, (cx_T*)work, lwork, (T*)rwork, lrwork, iwork, liwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef double T;
      typedef typename std::complex<T> cx_T;
      arma_fortran(arma_zheevd)(jobz, uplo, n, (cx_T*)a, lda, (T*)w, (cx_T*)work, lwork, (T*)rwork, lrwork, iwork, liwork, info);
      }
    }



  template<typename eT>
  inline
  void
  ggev
    (
    char* jobvl, char* jobvr, blas_int* n,
    eT* a, blas_int* lda, eT* b, blas_int* ldb,
    eT* alphar, eT* alphai, eT* beta,
    eT* vl, blas_int* ldvl, eT* vr, blas_int* ldvr,
    eT* work, blas_int* lwork,
    blas_int* info
    )
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sggev)(jobvl, jobvr, n, (T*)a, lda, (T*)b, ldb, (T*)alphar, (T*)alphai, (T*)beta, (T*)vl, ldvl, (T*)vr, ldvr, (T*)work, lwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dggev)(jobvl, jobvr, n, (T*)a, lda, (T*)b, ldb, (T*)alphar, (T*)alphai, (T*)beta, (T*)vl, ldvl, (T*)vr, ldvr, (T*)work, lwork, info);
      }
    }



  template<typename eT>
  inline
  void
  cx_ggev
    (
    char* jobvl, char* jobvr, blas_int* n,
    eT* a, blas_int* lda, eT* b, blas_int* ldb,
    eT* alpha, eT* beta,
    eT* vl, blas_int* ldvl, eT* vr, blas_int* ldvr,
    eT* work, blas_int* lwork, typename eT::value_type* rwork,
    blas_int* info
    )
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_supported_complex_float<eT>::value)
      {
      typedef float T;
      typedef typename std::complex<T> cx_T;
      arma_fortran(arma_cggev)(jobvl, jobvr, n, (cx_T*)a, lda, (cx_T*)b, ldb, (cx_T*)alpha, (cx_T*)beta, (cx_T*)vl, ldvl, (cx_T*)vr, ldvr, (cx_T*)work, lwork, (T*)rwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef double T;
      typedef typename std::complex<T> cx_T;
      arma_fortran(arma_zggev)(jobvl, jobvr, n, (cx_T*)a, lda, (cx_T*)b, ldb, (cx_T*)alpha, (cx_T*)beta, (cx_T*)vl, ldvl, (cx_T*)vr, ldvr, (cx_T*)work, lwork, (T*)rwork, info);
      }
    }



  template<typename eT>
  inline
  void
  potrf(char* uplo, blas_int* n, eT* a, blas_int* lda, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_spotrf)(uplo, n, (T*)a, lda, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dpotrf)(uplo, n, (T*)a, lda, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_cpotrf)(uplo, n, (T*)a, lda, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_zpotrf)(uplo, n, (T*)a, lda, info);
      }

    }



  template<typename eT>
  inline
  void
  potri(char* uplo, blas_int* n, eT* a, blas_int* lda, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_spotri)(uplo, n, (T*)a, lda, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dpotri)(uplo, n, (T*)a, lda, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_cpotri)(uplo, n, (T*)a, lda, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_zpotri)(uplo, n, (T*)a, lda, info);
      }

    }



  template<typename eT>
  inline
  void
  geqrf(blas_int* m, blas_int* n, eT* a, blas_int* lda, eT* tau, eT* work, blas_int* lwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgeqrf)(m, n, (T*)a, lda, (T*)tau, (T*)work, lwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgeqrf)(m, n, (T*)a, lda, (T*)tau, (T*)work, lwork, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_cgeqrf)(m, n, (T*)a, lda, (T*)tau, (T*)work, lwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_zgeqrf)(m, n, (T*)a, lda, (T*)tau, (T*)work, lwork, info);
      }

    }



  template<typename eT>
  inline
  void
  orgqr(blas_int* m, blas_int* n, blas_int* k, eT* a, blas_int* lda, eT* tau, eT* work, blas_int* lwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sorgqr)(m, n, k, (T*)a, lda, (T*)tau, (T*)work, lwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dorgqr)(m, n, k, (T*)a, lda, (T*)tau, (T*)work, lwork, info);
      }
    }



  template<typename eT>
  inline
  void
  ungqr(blas_int* m, blas_int* n, blas_int* k, eT* a, blas_int* lda, eT* tau, eT* work, blas_int* lwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_supported_complex_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_cungqr)(m, n, k, (T*)a, lda, (T*)tau, (T*)work, lwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_zungqr)(m, n, k, (T*)a, lda, (T*)tau, (T*)work, lwork, info);
      }
    }


  template<typename eT>
  inline
  void
  gesvd
    (
    char* jobu, char* jobvt, blas_int* m, blas_int* n, eT* a, blas_int* lda,
    eT* s, eT* u, blas_int* ldu, eT* vt, blas_int* ldvt,
    eT* work, blas_int* lwork, blas_int* info
    )
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgesvd)(jobu, jobvt, m, n, (T*)a, lda, (T*)s, (T*)u, ldu, (T*)vt, ldvt, (T*)work, lwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgesvd)(jobu, jobvt, m, n, (T*)a, lda, (T*)s, (T*)u, ldu, (T*)vt, ldvt, (T*)work, lwork, info);
      }
    }



  template<typename T>
  inline
  void
  cx_gesvd
    (
    char* jobu, char* jobvt, blas_int* m, blas_int* n, std::complex<T>* a, blas_int* lda,
    T* s, std::complex<T>* u, blas_int* ldu, std::complex<T>* vt, blas_int* ldvt,
    std::complex<T>* work, blas_int* lwork, T* rwork, blas_int* info
    )
    {
    arma_type_check(( is_supported_blas_type<T>::value == false ));
    arma_type_check(( is_supported_blas_type< std::complex<T> >::value == false ));

    if(is_float<T>::value)
      {
      typedef float bT;
      arma_fortran(arma_cgesvd)
        (
        jobu, jobvt, m, n, (std::complex<bT>*)a, lda,
        (bT*)s, (std::complex<bT>*)u, ldu, (std::complex<bT>*)vt, ldvt,
        (std::complex<bT>*)work, lwork, (bT*)rwork, info
        );
      }
    else
    if(is_double<T>::value)
      {
      typedef double bT;
      arma_fortran(arma_zgesvd)
        (
        jobu, jobvt, m, n, (std::complex<bT>*)a, lda,
        (bT*)s, (std::complex<bT>*)u, ldu, (std::complex<bT>*)vt, ldvt,
        (std::complex<bT>*)work, lwork, (bT*)rwork, info
        );
      }
    }



  template<typename eT>
  inline
  void
  gesdd
    (
    char* jobz, blas_int* m, blas_int* n,
    eT* a, blas_int* lda, eT* s, eT* u, blas_int* ldu, eT* vt, blas_int* ldvt,
    eT* work, blas_int* lwork, blas_int* iwork, blas_int* info
    )
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgesdd)(jobz, m, n, (T*)a, lda, (T*)s, (T*)u, ldu, (T*)vt, ldvt, (T*)work, lwork, iwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgesdd)(jobz, m, n, (T*)a, lda, (T*)s, (T*)u, ldu, (T*)vt, ldvt, (T*)work, lwork, iwork, info);
      }
    }



  template<typename T>
  inline
  void
  cx_gesdd
    (
    char* jobz, blas_int* m, blas_int* n,
    std::complex<T>* a, blas_int* lda, T* s, std::complex<T>* u, blas_int* ldu, std::complex<T>* vt, blas_int* ldvt,
    std::complex<T>* work, blas_int* lwork, T* rwork, blas_int* iwork, blas_int* info
    )
    {
    arma_type_check(( is_supported_blas_type<T>::value == false ));
    arma_type_check(( is_supported_blas_type< std::complex<T> >::value == false ));

    if(is_float<T>::value)
      {
      typedef float bT;
      arma_fortran(arma_cgesdd)
        (
        jobz, m, n,
        (std::complex<bT>*)a, lda, (bT*)s, (std::complex<bT>*)u, ldu, (std::complex<bT>*)vt, ldvt,
        (std::complex<bT>*)work, lwork, (bT*)rwork, iwork, info
        );
      }
    else
    if(is_double<T>::value)
      {
      typedef double bT;
      arma_fortran(arma_zgesdd)
        (
        jobz, m, n,
        (std::complex<bT>*)a, lda, (bT*)s, (std::complex<bT>*)u, ldu, (std::complex<bT>*)vt, ldvt,
        (std::complex<bT>*)work, lwork, (bT*)rwork, iwork, info
        );
      }
    }



  template<typename eT>
  inline
  void
  gesv(blas_int* n, blas_int* nrhs, eT* a, blas_int* lda, blas_int* ipiv, eT* b, blas_int* ldb, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgesv)(n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgesv)(n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_cgesv)(n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_zgesv)(n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
      }
    }



  template<typename eT>
  inline
  void
  gesvx(char* fact, char* trans, blas_int* n, blas_int* nrhs, eT* a, blas_int* lda, eT* af, blas_int* ldaf, blas_int* ipiv, char* equed, eT* r, eT* c, eT* b, blas_int* ldb, eT* x, blas_int* ldx, eT* rcond, eT* ferr, eT* berr, eT* work, blas_int* iwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgesvx)(fact, trans, n, nrhs, (T*)a, lda, (T*)af, ldaf, ipiv, equed, (T*)r, (T*)c, (T*)b, ldb, (T*)x, ldx, (T*)rcond, (T*)ferr, (T*)berr, (T*)work, iwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgesvx)(fact, trans, n, nrhs, (T*)a, lda, (T*)af, ldaf, ipiv, equed, (T*)r, (T*)c, (T*)b, ldb, (T*)x, ldx, (T*)rcond, (T*)ferr, (T*)berr, (T*)work, iwork, info);
      }
    }



  template<typename T, typename eT>
  inline
  void
  cx_gesvx(char* fact, char* trans, blas_int* n, blas_int* nrhs, eT* a, blas_int* lda, eT* af, blas_int* ldaf, blas_int* ipiv, char* equed, T* r, T* c, eT* b, blas_int* ldb, eT* x, blas_int* ldx, T* rcond, T* ferr, T* berr, eT* work, T* rwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_supported_complex_float<eT>::value)
      {
      typedef float               pod_T;
      typedef std::complex<float>  cx_T;
      arma_fortran(arma_cgesvx)(fact, trans, n, nrhs, (cx_T*)a, lda, (cx_T*)af, ldaf, ipiv, equed, (pod_T*)r, (pod_T*)c, (cx_T*)b, ldb, (cx_T*)x, ldx, (pod_T*)rcond, (pod_T*)ferr, (pod_T*)berr, (cx_T*)work, (pod_T*)rwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef double               pod_T;
      typedef std::complex<double>  cx_T;
      arma_fortran(arma_zgesvx)(fact, trans, n, nrhs, (cx_T*)a, lda, (cx_T*)af, ldaf, ipiv, equed, (pod_T*)r, (pod_T*)c, (cx_T*)b, ldb, (cx_T*)x, ldx, (pod_T*)rcond, (pod_T*)ferr, (pod_T*)berr, (cx_T*)work, (pod_T*)rwork, info);
      }
    }



  template<typename eT>
  inline
  void
  gels(char* trans, blas_int* m, blas_int* n, blas_int* nrhs, eT* a, blas_int* lda, eT* b, blas_int* ldb, eT* work, blas_int* lwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgels)(trans, m, n, nrhs, (T*)a, lda, (T*)b, ldb, (T*)work, lwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgels)(trans, m, n, nrhs, (T*)a, lda, (T*)b, ldb, (T*)work, lwork, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_cgels)(trans, m, n, nrhs, (T*)a, lda, (T*)b, ldb, (T*)work, lwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_zgels)(trans, m, n, nrhs, (T*)a, lda, (T*)b, ldb, (T*)work, lwork, info);
      }
    }



  template<typename eT>
  inline
  void
  gelsd(blas_int* m, blas_int* n, blas_int* nrhs, eT* a, blas_int* lda, eT* b, blas_int* ldb, eT* S, eT* rcond, blas_int* rank, eT* work, blas_int* lwork, blas_int* iwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgelsd)(m, n, nrhs, (T*)a, lda, (T*)b, ldb, (T*)S, (T*)rcond, rank, (T*)work, lwork, iwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgelsd)(m, n, nrhs, (T*)a, lda, (T*)b, ldb, (T*)S, (T*)rcond, rank, (T*)work, lwork, iwork, info);
      }
    }



  template<typename T>
  inline
  void
  cx_gelsd(blas_int* m, blas_int* n, blas_int* nrhs, std::complex<T>* a, blas_int* lda, std::complex<T>* b, blas_int* ldb, T* S, T* rcond, blas_int* rank, std::complex<T>* work, blas_int* lwork, T* rwork, blas_int* iwork, blas_int* info)
    {
    typedef typename std::complex<T> eT;

    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_supported_complex_float<eT>::value)
      {
      typedef float               pod_T;
      typedef std::complex<float>  cx_T;
      arma_fortran(arma_cgelsd)(m, n, nrhs, (cx_T*)a, lda, (cx_T*)b, ldb, (pod_T*)S, (pod_T*)rcond, rank, (cx_T*)work, lwork, (pod_T*)rwork, iwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef double               pod_T;
      typedef std::complex<double>  cx_T;
      arma_fortran(arma_zgelsd)(m, n, nrhs, (cx_T*)a, lda, (cx_T*)b, ldb, (pod_T*)S, (pod_T*)rcond, rank, (cx_T*)work, lwork, (pod_T*)rwork, iwork, info);
      }
    }



  template<typename eT>
  inline
  void
  trtrs(char* uplo, char* trans, char* diag, blas_int* n, blas_int* nrhs, const eT* a, blas_int* lda, eT* b, blas_int* ldb, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_strtrs)(uplo, trans, diag, n, nrhs, (T*)a, lda, (T*)b, ldb, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dtrtrs)(uplo, trans, diag, n, nrhs, (T*)a, lda, (T*)b, ldb, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_ctrtrs)(uplo, trans, diag, n, nrhs, (T*)a, lda, (T*)b, ldb, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_ztrtrs)(uplo, trans, diag, n, nrhs, (T*)a, lda, (T*)b, ldb, info);
      }
    }



  template<typename eT>
  inline
  void
  gees(char* jobvs, char* sort, void* select, blas_int* n, eT* a, blas_int* lda, blas_int* sdim, eT* wr, eT* wi, eT* vs, blas_int* ldvs, eT* work, blas_int* lwork, blas_int* bwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgees)(jobvs, sort, select, n, (T*)a, lda, sdim, (T*)wr, (T*)wi, (T*)vs, ldvs, (T*)work, lwork, bwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgees)(jobvs, sort, select, n, (T*)a, lda, sdim, (T*)wr, (T*)wi, (T*)vs, ldvs, (T*)work, lwork, bwork, info);
      }
    }



  template<typename T>
  inline
  void
  cx_gees(char* jobvs, char* sort, void* select, blas_int* n, std::complex<T>* a, blas_int* lda, blas_int* sdim, std::complex<T>* w, std::complex<T>* vs, blas_int* ldvs, std::complex<T>* work, blas_int* lwork, T* rwork, blas_int* bwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<T>::value == false ));
    arma_type_check(( is_supported_blas_type< std::complex<T> >::value == false ));

    if(is_float<T>::value)
      {
      typedef float bT;
      typedef std::complex<bT> cT;
      arma_fortran(arma_cgees)(jobvs, sort, select, n, (cT*)a, lda, sdim, (cT*)w, (cT*)vs, ldvs, (cT*)work, lwork, (bT*)rwork, bwork, info);
      }
    else
    if(is_double<T>::value)
      {
      typedef double bT;
      typedef std::complex<bT> cT;
      arma_fortran(arma_zgees)(jobvs, sort, select, n, (cT*)a, lda, sdim, (cT*)w, (cT*)vs, ldvs, (cT*)work, lwork, (bT*)rwork, bwork, info);
      }
    }



  template<typename eT>
  inline
  void
  trsyl(char* transa, char* transb, blas_int* isgn, blas_int* m, blas_int* n, const eT* a, blas_int* lda, const eT* b, blas_int* ldb, eT* c, blas_int* ldc, eT* scale, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_strsyl)(transa, transb, isgn, m, n, (T*)a, lda, (T*)b, ldb, (T*)c, ldc, (T*)scale, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dtrsyl)(transa, transb, isgn, m, n, (T*)a, lda, (T*)b, ldb, (T*)c, ldc, (T*)scale, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_ctrsyl)(transa, transb, isgn, m, n, (T*)a, lda, (T*)b, ldb, (T*)c, ldc, (float*)scale, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_ztrsyl)(transa, transb, isgn, m, n, (T*)a, lda, (T*)b, ldb, (T*)c, ldc, (double*)scale, info);
      }
    }


  template<typename eT>
  inline
  void
  sytrf(char* uplo, blas_int* n, eT* a, blas_int* lda, blas_int* ipiv, eT* work, blas_int* lwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_ssytrf)(uplo, n, (T*)a, lda, ipiv, (T*)work, lwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dsytrf)(uplo, n, (T*)a, lda, ipiv, (T*)work, lwork, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_csytrf)(uplo, n, (T*)a, lda, ipiv, (T*)work, lwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_zsytrf)(uplo, n, (T*)a, lda, ipiv, (T*)work, lwork, info);
      }
    }


  template<typename eT>
  inline
  void
  sytri(char* uplo, blas_int* n, eT* a, blas_int* lda, blas_int* ipiv, eT* work, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_ssytri)(uplo, n, (T*)a, lda, ipiv, (T*)work, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dsytri)(uplo, n, (T*)a, lda, ipiv, (T*)work, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_csytri)(uplo, n, (T*)a, lda, ipiv, (T*)work, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_zsytri)(uplo, n, (T*)a, lda, ipiv, (T*)work, info);
      }
    }



  template<typename eT>
  inline
  void
  gges
    (
    char* jobvsl, char* jobvsr, char* sort, void* selctg, blas_int* n,
    eT* a, blas_int* lda, eT* b, blas_int* ldb, blas_int* sdim,
    eT* alphar, eT* alphai, eT* beta,
    eT* vsl, blas_int* ldvsl, eT* vsr, blas_int* ldvsr,
    eT* work, blas_int* lwork, eT* bwork,
    blas_int* info
    )
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgges)(jobvsl, jobvsr, sort, selctg, n, (T*)a, lda, (T*)b, ldb, sdim, (T*)alphar, (T*)alphai, (T*)beta, (T*)vsl, ldvsl, (T*)vsr, ldvsr, (T*)work, lwork, (T*)bwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgges)(jobvsl, jobvsr, sort, selctg, n, (T*)a, lda, (T*)b, ldb, sdim, (T*)alphar, (T*)alphai, (T*)beta, (T*)vsl, ldvsl, (T*)vsr, ldvsr, (T*)work, lwork, (T*)bwork, info);
      }
    }



  template<typename eT>
  inline
  void
  cx_gges
    (
    char* jobvsl, char* jobvsr, char* sort, void* selctg, blas_int* n,
    eT* a, blas_int* lda, eT* b, blas_int* ldb, blas_int* sdim,
    eT* alpha, eT* beta,
    eT* vsl, blas_int* ldvsl, eT* vsr, blas_int* ldvsr,
    eT* work, blas_int* lwork, typename eT::value_type* rwork,
    typename eT::value_type* bwork,
    blas_int* info
    )
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_supported_complex_float<eT>::value)
      {
      typedef float T;
      typedef typename std::complex<T> cx_T;
      arma_fortran(arma_cgges)(jobvsl, jobvsr, sort, selctg, n, (cx_T*)a, lda, (cx_T*)b, ldb, sdim, (cx_T*)alpha, (cx_T*)beta, (cx_T*)vsl, ldvsl, (cx_T*)vsr, ldvsr, (cx_T*)work, lwork, (T*)rwork, (T*)bwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef double T;
      typedef typename std::complex<T> cx_T;
      arma_fortran(arma_zgges)(jobvsl, jobvsr, sort, selctg, n, (cx_T*)a, lda, (cx_T*)b, ldb, sdim, (cx_T*)alpha, (cx_T*)beta, (cx_T*)vsl, ldvsl, (cx_T*)vsr, ldvsr, (cx_T*)work, lwork, (T*)rwork, (T*)bwork, info);
      }
    }



  template<typename eT>
  inline
  typename get_pod_type<eT>::result
  lange(char* norm, blas_int* m, blas_int* n, eT* a, blas_int* lda, typename get_pod_type<eT>::result* work)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    typedef typename get_pod_type<eT>::result out_T;

    if(is_float<eT>::value)
      {
      typedef float pod_T;
      typedef float     T;
      return out_T( arma_fortran(arma_slange)(norm, m, n, (T*)a, lda, (pod_T*)work) );
      }
    else
    if(is_double<eT>::value)
      {
      typedef double pod_T;
      typedef double     T;
      return out_T( arma_fortran(arma_dlange)(norm, m, n, (T*)a, lda, (pod_T*)work) );
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef float               pod_T;
      typedef std::complex<float>     T;
      return out_T( arma_fortran(arma_clange)(norm, m, n, (T*)a, lda, (pod_T*)work) );
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef double               pod_T;
      typedef std::complex<double>     T;
      return out_T( arma_fortran(arma_zlange)(norm, m, n, (T*)a, lda, (pod_T*)work) );
      }
    }



  template<typename eT>
  inline
  void
  gecon(char* norm, blas_int* n, eT* a, blas_int* lda, eT* anorm, eT* rcond, eT* work, blas_int* iwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgecon)(norm, n, (T*)a, lda, (T*)anorm, (T*)rcond, (T*)work, iwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgecon)(norm, n, (T*)a, lda, (T*)anorm, (T*)rcond, (T*)work, iwork, info);
      }
    }



  template<typename T>
  inline
  void
  cx_gecon(char* norm, blas_int* n, std::complex<T>* a, blas_int* lda, T* anorm, T* rcond, std::complex<T>* work, T* rwork, blas_int* info)
    {
    typedef typename std::complex<T> eT;

    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_supported_complex_float<eT>::value)
      {
      typedef float                    pod_T;
      typedef typename std::complex<T>  cx_T;
      arma_fortran(arma_cgecon)(norm, n, (cx_T*)a, lda, (pod_T*)anorm, (pod_T*)rcond, (cx_T*)work, (pod_T*)rwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef double                   pod_T;
      typedef typename std::complex<T>  cx_T;
      arma_fortran(arma_zgecon)(norm, n, (cx_T*)a, lda, (pod_T*)anorm, (pod_T*)rcond, (cx_T*)work, (pod_T*)rwork, info);
      }
    }



  inline
  blas_int
  laenv(blas_int* ispec, char* name, char* opts, blas_int* n1, blas_int* n2, blas_int* n3, blas_int* n4)
    {
    return arma_fortran(arma_ilaenv)(ispec, name, opts, n1, n2, n3, n4);
    }



  template<typename eT>
  inline
  void
  sytrs(char* uplo, blas_int* n, blas_int* nrhs, eT* a, blas_int* lda, blas_int* ipiv, eT* b, blas_int* ldb, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_ssytrs)(uplo, n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
        }
      else
      if(is_double<eT>::value)
        {
        typedef double T;
        arma_fortran(arma_dsytrs)(uplo, n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
        }
      else
      if(is_supported_complex_float<eT>::value)
        {
        typedef std::complex<float> T;
        arma_fortran(arma_csytrs)(uplo, n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
        }
      else
      if(is_supported_complex_double<eT>::value)
        {
        typedef std::complex<double> T;
        arma_fortran(arma_zsytrs)(uplo, n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
        }
      }



  template<typename eT>
  inline
  void
  getrs(char* trans, blas_int* n, blas_int* nrhs, eT* a, blas_int* lda, blas_int* ipiv, eT* b, blas_int* ldb, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sgetrs)(trans, n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dgetrs)(trans, n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_fortran(arma_cgetrs)(trans, n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_fortran(arma_zgetrs)(trans, n, nrhs, (T*)a, lda, ipiv, (T*)b, ldb, info);
      }
    }



  template<typename eT>
  inline
  void
  lahqr(blas_int* wantt, blas_int* wantz, blas_int* n, blas_int* ilo, blas_int* ihi, eT* h, blas_int* ldh, eT* wr, eT* wi, blas_int* iloz, blas_int* ihiz, eT* z, blas_int* ldz, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_slahqr)(wantt, wantz, n, ilo, ihi, (T*)h, ldh, (T*)wr, (T*)wi, iloz, ihiz, (T*)z, ldz, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dlahqr)(wantt, wantz, n, ilo, ihi, (T*)h, ldh, (T*)wr, (T*)wi, iloz, ihiz, (T*)z, ldz, info);
      }
    }



  template<typename eT>
  inline
  void
  stedc(char* compz, blas_int* n, eT* d, eT* e, eT* z, blas_int* ldz, eT* work, blas_int* lwork, blas_int* iwork, blas_int* liwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sstedc)(compz, n, (T*)d, (T*)e, (T*)z, ldz, (T*)work, lwork, iwork, liwork, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dstedc)(compz, n, (T*)d, (T*)e, (T*)z, ldz, (T*)work, lwork, iwork, liwork, info);
      }
    }



  template<typename eT>
  inline
  void
  trevc(char* side, char* howmny, blas_int* select, blas_int* n, eT* t, blas_int* ldt, eT* vl, blas_int* ldvl, eT* vr, blas_int* ldvr, blas_int* mm, blas_int* m, eT* work, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_strevc)(side, howmny, select, n, (T*)t, ldt, (T*)vl, ldvl, (T*)vr, ldvr, mm, m, (T*)work, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dtrevc)(side, howmny, select, n, (T*)t, ldt, (T*)vl, ldvl, (T*)vr, ldvr, mm, m, (T*)work, info);
      }
    }



  template<typename eT>
  inline
  void
  larnv(blas_int* idist, blas_int* iseed, blas_int* n, eT* x)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_slarnv)(idist, iseed, n, (T*)x);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dlarnv)(idist, iseed, n, (T*)x);
      }
    }


  }


#endif
