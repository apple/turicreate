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


#if !defined(ARMA_BLAS_CAPITALS)

  #define arma_sgetrf sgetrf
  #define arma_dgetrf dgetrf
  #define arma_cgetrf cgetrf
  #define arma_zgetrf zgetrf

  #define arma_sgetri sgetri
  #define arma_dgetri dgetri
  #define arma_cgetri cgetri
  #define arma_zgetri zgetri

  #define arma_strtri strtri
  #define arma_dtrtri dtrtri
  #define arma_ctrtri ctrtri
  #define arma_ztrtri ztrtri

  #define arma_sgeev  sgeev
  #define arma_dgeev  dgeev
  #define arma_cgeev  cgeev
  #define arma_zgeev  zgeev

  #define arma_ssyev  ssyev
  #define arma_dsyev  dsyev

  #define arma_cheev  cheev
  #define arma_zheev  zheev

  #define arma_ssyevd ssyevd
  #define arma_dsyevd dsyevd

  #define arma_cheevd cheevd
  #define arma_zheevd zheevd

  #define arma_sggev  sggev
  #define arma_dggev  dggev

  #define arma_cggev  cggev
  #define arma_zggev  zggev

  #define arma_spotrf spotrf
  #define arma_dpotrf dpotrf
  #define arma_cpotrf cpotrf
  #define arma_zpotrf zpotrf

  #define arma_spotri spotri
  #define arma_dpotri dpotri
  #define arma_cpotri cpotri
  #define arma_zpotri zpotri

  #define arma_sgeqrf sgeqrf
  #define arma_dgeqrf dgeqrf
  #define arma_cgeqrf cgeqrf
  #define arma_zgeqrf zgeqrf

  #define arma_sorgqr sorgqr
  #define arma_dorgqr dorgqr

  #define arma_cungqr cungqr
  #define arma_zungqr zungqr

  #define arma_sgesvd sgesvd
  #define arma_dgesvd dgesvd

  #define arma_cgesvd cgesvd
  #define arma_zgesvd zgesvd

  #define arma_sgesdd sgesdd
  #define arma_dgesdd dgesdd
  #define arma_cgesdd cgesdd
  #define arma_zgesdd zgesdd

  #define arma_sgesv  sgesv
  #define arma_dgesv  dgesv
  #define arma_cgesv  cgesv
  #define arma_zgesv  zgesv

  #define arma_sgesvx sgesvx
  #define arma_dgesvx dgesvx
  #define arma_cgesvx cgesvx
  #define arma_zgesvx zgesvx

  #define arma_sgels  sgels
  #define arma_dgels  dgels
  #define arma_cgels  cgels
  #define arma_zgels  zgels

  #define arma_sgelsd sgelsd
  #define arma_dgelsd dgelsd
  #define arma_cgelsd cgelsd
  #define arma_zgelsd zgelsd

  #define arma_strtrs strtrs
  #define arma_dtrtrs dtrtrs
  #define arma_ctrtrs ctrtrs
  #define arma_ztrtrs ztrtrs

  #define arma_sgees  sgees
  #define arma_dgees  dgees
  #define arma_cgees  cgees
  #define arma_zgees  zgees

  #define arma_strsyl strsyl
  #define arma_dtrsyl dtrsyl
  #define arma_ctrsyl ctrsyl
  #define arma_ztrsyl ztrsyl

  #define arma_ssytrf ssytrf
  #define arma_dsytrf dsytrf
  #define arma_csytrf csytrf
  #define arma_zsytrf zsytrf

  #define arma_ssytri ssytri
  #define arma_dsytri dsytri
  #define arma_csytri csytri
  #define arma_zsytri zsytri

  #define arma_sgges  sgges
  #define arma_dgges  dgges
  #define arma_cgges  cgges
  #define arma_zgges  zgges

  #define arma_slange slange
  #define arma_dlange dlange
  #define arma_clange clange
  #define arma_zlange zlange

  #define arma_sgecon sgecon
  #define arma_dgecon dgecon
  #define arma_cgecon cgecon
  #define arma_zgecon zgecon

  #define arma_ilaenv ilaenv

  #define arma_ssytrs ssytrs
  #define arma_dsytrs dsytrs
  #define arma_csytrs csytrs
  #define arma_zsytrs zsytrs

  #define arma_sgetrs sgetrs
  #define arma_dgetrs dgetrs
  #define arma_cgetrs cgetrs
  #define arma_zgetrs zgetrs

  #define arma_slahqr slahqr
  #define arma_dlahqr dlahqr

  #define arma_sstedc sstedc
  #define arma_dstedc dstedc

  #define arma_strevc strevc
  #define arma_dtrevc dtrevc

  #define arma_slarnv slarnv
  #define arma_dlarnv dlarnv

#else

  #define arma_sgetrf SGETRF
  #define arma_dgetrf DGETRF
  #define arma_cgetrf CGETRF
  #define arma_zgetrf ZGETRF

  #define arma_sgetri SGETRI
  #define arma_dgetri DGETRI
  #define arma_cgetri CGETRI
  #define arma_zgetri ZGETRI

  #define arma_strtri STRTRI
  #define arma_dtrtri DTRTRI
  #define arma_ctrtri CTRTRI
  #define arma_ztrtri ZTRTRI

  #define arma_sgeev  SGEEV
  #define arma_dgeev  DGEEV
  #define arma_cgeev  CGEEV
  #define arma_zgeev  ZGEEV

  #define arma_ssyev  SSYEV
  #define arma_dsyev  DSYEV

  #define arma_cheev  CHEEV
  #define arma_zheev  ZHEEV

  #define arma_ssyevd SSYEVD
  #define arma_dsyevd DSYEVD

  #define arma_cheevd CHEEVD
  #define arma_zheevd ZHEEVD

  #define arma_sggev  SGGEV
  #define arma_dggev  DGGEV

  #define arma_cggev  CGGEV
  #define arma_zggev  ZGGEV

  #define arma_spotrf SPOTRF
  #define arma_dpotrf DPOTRF
  #define arma_cpotrf CPOTRF
  #define arma_zpotrf ZPOTRF

  #define arma_spotri SPOTRI
  #define arma_dpotri DPOTRI
  #define arma_cpotri CPOTRI
  #define arma_zpotri ZPOTRI

  #define arma_sgeqrf SGEQRF
  #define arma_dgeqrf DGEQRF
  #define arma_cgeqrf CGEQRF
  #define arma_zgeqrf ZGEQRF

  #define arma_sorgqr SORGQR
  #define arma_dorgqr DORGQR

  #define arma_cungqr CUNGQR
  #define arma_zungqr ZUNGQR

  #define arma_sgesvd SGESVD
  #define arma_dgesvd DGESVD

  #define arma_cgesvd CGESVD
  #define arma_zgesvd ZGESVD

  #define arma_sgesdd SGESDD
  #define arma_dgesdd DGESDD
  #define arma_cgesdd CGESDD
  #define arma_zgesdd ZGESDD

  #define arma_sgesv  SGESV
  #define arma_dgesv  DGESV
  #define arma_cgesv  CGESV
  #define arma_zgesv  ZGESV

  #define arma_sgesvx SGESVX
  #define arma_dgesvx DGESVX
  #define arma_cgesvx CGESVX
  #define arma_zgesvx ZGESVX

  #define arma_sgels  SGELS
  #define arma_dgels  DGELS
  #define arma_cgels  CGELS
  #define arma_zgels  ZGELS

  #define arma_sgelsd SGELSD
  #define arma_dgelsd DGELSD
  #define arma_cgelsd CGELSD
  #define arma_zgelsd ZGELSD

  #define arma_strtrs STRTRS
  #define arma_dtrtrs DTRTRS
  #define arma_ctrtrs CTRTRS
  #define arma_ztrtrs ZTRTRS

  #define arma_sgees  SGEES
  #define arma_dgees  DGEES
  #define arma_cgees  CGEES
  #define arma_zgees  ZGEES

  #define arma_strsyl STRSYL
  #define arma_dtrsyl DTRSYL
  #define arma_ctrsyl CTRSYL
  #define arma_ztrsyl ZTRSYL

  #define arma_ssytrf SSYTRF
  #define arma_dsytrf DSYTRF
  #define arma_csytrf CSYTRF
  #define arma_zsytrf ZSYTRF

  #define arma_ssytri SSYTRI
  #define arma_dsytri DSYTRI
  #define arma_csytri CSYTRI
  #define arma_zsytri ZSYTRI

  #define arma_sgges  SGGES
  #define arma_dgges  DGGES
  #define arma_cgges  CGGES
  #define arma_zgges  ZGGES

  #define arma_slange SLANGE
  #define arma_dlange DLANGE
  #define arma_clange CLANGE
  #define arma_zlange ZLANGE

  #define arma_sgecon SGECON
  #define arma_dgecon DGECON
  #define arma_cgecon CGECON
  #define arma_zgecon ZGECON

  #define arma_ilaenv ILAENV

  #define arma_ssytrs SSYTRS
  #define arma_dsytrs DSYTRS
  #define arma_csytrs CSYTRS
  #define arma_zsytrs ZSYTRS

  #define arma_sgetrs SGETRS
  #define arma_dgetrs DGETRS
  #define arma_cgetrs CGETRS
  #define arma_zgetrs ZGETRS

  #define arma_slahqr SLAHQR
  #define arma_dlahqr DLAHQR

  #define arma_sstedc SSTEDC
  #define arma_dstedc DSTEDC

  #define arma_strevc STREVC
  #define arma_dtrevc DTREVC

  #define arma_slarnv SLARNV
  #define arma_dlarnv DLARNV

#endif



extern "C"
  {
  // LU factorisation
  void arma_fortran(arma_sgetrf)(blas_int* m, blas_int* n,  float* a, blas_int* lda, blas_int* ipiv, blas_int* info);
  void arma_fortran(arma_dgetrf)(blas_int* m, blas_int* n, double* a, blas_int* lda, blas_int* ipiv, blas_int* info);
  void arma_fortran(arma_cgetrf)(blas_int* m, blas_int* n,   void* a, blas_int* lda, blas_int* ipiv, blas_int* info);
  void arma_fortran(arma_zgetrf)(blas_int* m, blas_int* n,   void* a, blas_int* lda, blas_int* ipiv, blas_int* info);

  // matrix inversion (using LU factorisation result)
  void arma_fortran(arma_sgetri)(blas_int* n,  float* a, blas_int* lda, blas_int* ipiv,  float* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_dgetri)(blas_int* n, double* a, blas_int* lda, blas_int* ipiv, double* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_cgetri)(blas_int* n,  void*  a, blas_int* lda, blas_int* ipiv,   void* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_zgetri)(blas_int* n,  void*  a, blas_int* lda, blas_int* ipiv,   void* work, blas_int* lwork, blas_int* info);

  // matrix inversion (triangular matrices)
  void arma_fortran(arma_strtri)(char* uplo, char* diag, blas_int* n,  float* a, blas_int* lda, blas_int* info);
  void arma_fortran(arma_dtrtri)(char* uplo, char* diag, blas_int* n, double* a, blas_int* lda, blas_int* info);
  void arma_fortran(arma_ctrtri)(char* uplo, char* diag, blas_int* n,   void* a, blas_int* lda, blas_int* info);
  void arma_fortran(arma_ztrtri)(char* uplo, char* diag, blas_int* n,   void* a, blas_int* lda, blas_int* info);

  // eigen decomposition of general matrix (real)
  void arma_fortran(arma_sgeev)(char* jobvl, char* jobvr, blas_int* N,  float* a, blas_int* lda,  float* wr,  float* wi,  float* vl, blas_int* ldvl,  float* vr, blas_int* ldvr,  float* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_dgeev)(char* jobvl, char* jobvr, blas_int* N, double* a, blas_int* lda, double* wr, double* wi, double* vl, blas_int* ldvl, double* vr, blas_int* ldvr, double* work, blas_int* lwork, blas_int* info);

  // eigen decomposition of general matrix (complex)
  void arma_fortran(arma_cgeev)(char* jobvl, char* jobvr, blas_int* N, void* a, blas_int* lda, void* w, void* vl, blas_int* ldvl, void* vr, blas_int* ldvr, void* work, blas_int* lwork,  float* rwork, blas_int* info);
  void arma_fortran(arma_zgeev)(char* jobvl, char* jobvr, blas_int* N, void* a, blas_int* lda, void* w, void* vl, blas_int* ldvl, void* vr, blas_int* ldvr, void* work, blas_int* lwork, double* rwork, blas_int* info);

  // eigen decomposition of symmetric real matrices
  void arma_fortran(arma_ssyev)(char* jobz, char* uplo, blas_int* n,  float* a, blas_int* lda,  float* w,  float* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_dsyev)(char* jobz, char* uplo, blas_int* n, double* a, blas_int* lda, double* w, double* work, blas_int* lwork, blas_int* info);

  // eigen decomposition of hermitian matrices (complex)
  void arma_fortran(arma_cheev)(char* jobz, char* uplo, blas_int* n,   void* a, blas_int* lda,  float* w,   void* work, blas_int* lwork,  float* rwork, blas_int* info);
  void arma_fortran(arma_zheev)(char* jobz, char* uplo, blas_int* n,   void* a, blas_int* lda, double* w,   void* work, blas_int* lwork, double* rwork, blas_int* info);

  // eigen decomposition of symmetric real matrices by divide and conquer
  void arma_fortran(arma_ssyevd)(char* jobz, char* uplo, blas_int* n,  float* a, blas_int* lda,  float* w,  float* work, blas_int* lwork, blas_int* iwork, blas_int* liwork, blas_int* info);
  void arma_fortran(arma_dsyevd)(char* jobz, char* uplo, blas_int* n, double* a, blas_int* lda, double* w, double* work, blas_int* lwork, blas_int* iwork, blas_int* liwork, blas_int* info);

  // eigen decomposition of hermitian matrices (complex) by divide and conquer
  void arma_fortran(arma_cheevd)(char* jobz, char* uplo, blas_int* n,   void* a, blas_int* lda,  float* w,   void* work, blas_int* lwork,  float* rwork, blas_int* lrwork, blas_int* iwork, blas_int* liwork, blas_int* info);
  void arma_fortran(arma_zheevd)(char* jobz, char* uplo, blas_int* n,   void* a, blas_int* lda, double* w,   void* work, blas_int* lwork, double* rwork, blas_int* lrwork, blas_int* iwork, blas_int* liwork, blas_int* info);

  // eigen decomposition of general real matrix pair
  void arma_fortran(arma_sggev)(char* jobvl, char* jobvr, blas_int* n,  float* a, blas_int* lda,  float* b, blas_int* ldb,  float* alphar,  float* alphai,  float* beta,  float* vl, blas_int* ldvl,  float* vr, blas_int* ldvr,  float* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_dggev)(char* jobvl, char* jobvr, blas_int* n, double* a, blas_int* lda, double* b, blas_int* ldb, double* alphar, double* alphai, double* beta, double* vl, blas_int* ldvl, double* vr, blas_int* ldvr, double* work, blas_int* lwork, blas_int* info);

  // eigen decomposition of general complex matrix pair
  void arma_fortran(arma_cggev)(char* jobvl, char* jobvr, blas_int* n, void* a, blas_int* lda, void* b, blas_int* ldb, void* alpha, void* beta, void* vl, blas_int* ldvl, void* vr, blas_int* ldvr, void* work, blas_int* lwork,  float* rwork, blas_int* info);
  void arma_fortran(arma_zggev)(char* jobvl, char* jobvr, blas_int* n, void* a, blas_int* lda, void* b, blas_int* ldb, void* alpha, void* beta, void* vl, blas_int* ldvl, void* vr, blas_int* ldvr, void* work, blas_int* lwork, double* rwork, blas_int* info);

  // Cholesky decomposition
  void arma_fortran(arma_spotrf)(char* uplo, blas_int* n,  float* a, blas_int* lda, blas_int* info);
  void arma_fortran(arma_dpotrf)(char* uplo, blas_int* n, double* a, blas_int* lda, blas_int* info);
  void arma_fortran(arma_cpotrf)(char* uplo, blas_int* n,   void* a, blas_int* lda, blas_int* info);
  void arma_fortran(arma_zpotrf)(char* uplo, blas_int* n,   void* a, blas_int* lda, blas_int* info);

  // matrix inversion (using Cholesky decomposition result)
  void arma_fortran(arma_spotri)(char* uplo, blas_int* n,  float* a, blas_int* lda, blas_int* info);
  void arma_fortran(arma_dpotri)(char* uplo, blas_int* n, double* a, blas_int* lda, blas_int* info);
  void arma_fortran(arma_cpotri)(char* uplo, blas_int* n,   void* a, blas_int* lda, blas_int* info);
  void arma_fortran(arma_zpotri)(char* uplo, blas_int* n,   void* a, blas_int* lda, blas_int* info);

  // QR decomposition
  void arma_fortran(arma_sgeqrf)(blas_int* m, blas_int* n,  float* a, blas_int* lda,  float* tau,  float* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_dgeqrf)(blas_int* m, blas_int* n, double* a, blas_int* lda, double* tau, double* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_cgeqrf)(blas_int* m, blas_int* n,   void* a, blas_int* lda,   void* tau,   void* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_zgeqrf)(blas_int* m, blas_int* n,   void* a, blas_int* lda,   void* tau,   void* work, blas_int* lwork, blas_int* info);

  // Q matrix calculation from QR decomposition (real matrices)
  void arma_fortran(arma_sorgqr)(blas_int* m, blas_int* n, blas_int* k,  float* a, blas_int* lda,  float* tau,  float* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_dorgqr)(blas_int* m, blas_int* n, blas_int* k, double* a, blas_int* lda, double* tau, double* work, blas_int* lwork, blas_int* info);

  // Q matrix calculation from QR decomposition (complex matrices)
  void arma_fortran(arma_cungqr)(blas_int* m, blas_int* n, blas_int* k,   void* a, blas_int* lda,   void* tau,   void* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_zungqr)(blas_int* m, blas_int* n, blas_int* k,   void* a, blas_int* lda,   void* tau,   void* work, blas_int* lwork, blas_int* info);

  // SVD (real matrices)
  void arma_fortran(arma_sgesvd)(char* jobu, char* jobvt, blas_int* m, blas_int* n, float*  a, blas_int* lda, float*  s, float*  u, blas_int* ldu, float*  vt, blas_int* ldvt, float*  work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_dgesvd)(char* jobu, char* jobvt, blas_int* m, blas_int* n, double* a, blas_int* lda, double* s, double* u, blas_int* ldu, double* vt, blas_int* ldvt, double* work, blas_int* lwork, blas_int* info);

  // SVD (complex matrices)
  void arma_fortran(arma_cgesvd)(char* jobu, char* jobvt, blas_int* m, blas_int* n, void*   a, blas_int* lda, float*  s, void*   u, blas_int* ldu, void*   vt, blas_int* ldvt, void*   work, blas_int* lwork, float*  rwork, blas_int* info);
  void arma_fortran(arma_zgesvd)(char* jobu, char* jobvt, blas_int* m, blas_int* n, void*   a, blas_int* lda, double* s, void*   u, blas_int* ldu, void*   vt, blas_int* ldvt, void*   work, blas_int* lwork, double* rwork, blas_int* info);

  // SVD (real matrices) by divide and conquer
  void arma_fortran(arma_sgesdd)(char* jobz, blas_int* m, blas_int* n, float*  a, blas_int* lda, float*  s, float*  u, blas_int* ldu, float*  vt, blas_int* ldvt, float*  work, blas_int* lwork, blas_int* iwork, blas_int* info);
  void arma_fortran(arma_dgesdd)(char* jobz, blas_int* m, blas_int* n, double* a, blas_int* lda, double* s, double* u, blas_int* ldu, double* vt, blas_int* ldvt, double* work, blas_int* lwork, blas_int* iwork, blas_int* info);

  // SVD (complex matrices) by divide and conquer
  void arma_fortran(arma_cgesdd)(char* jobz, blas_int* m, blas_int* n, void* a, blas_int* lda, float*  s, void* u, blas_int* ldu, void* vt, blas_int* ldvt, void* work, blas_int* lwork, float*  rwork, blas_int* iwork, blas_int* info);
  void arma_fortran(arma_zgesdd)(char* jobz, blas_int* m, blas_int* n, void* a, blas_int* lda, double* s, void* u, blas_int* ldu, void* vt, blas_int* ldvt, void* work, blas_int* lwork, double* rwork, blas_int* iwork, blas_int* info);

  // solve system of linear equations (general square matrix)
  void arma_fortran(arma_sgesv)(blas_int* n, blas_int* nrhs, float*  a, blas_int* lda, blas_int* ipiv, float*  b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_dgesv)(blas_int* n, blas_int* nrhs, double* a, blas_int* lda, blas_int* ipiv, double* b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_cgesv)(blas_int* n, blas_int* nrhs, void*   a, blas_int* lda, blas_int* ipiv, void*   b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_zgesv)(blas_int* n, blas_int* nrhs, void*   a, blas_int* lda, blas_int* ipiv, void*   b, blas_int* ldb, blas_int* info);

  // solve system of linear equations (general square matrix, advanced form, real matrices)
  void arma_fortran(arma_sgesvx)(char* fact, char* trans, blas_int* n, blas_int* nrhs,  float* a, blas_int* lda,  float* af, blas_int* ldaf, blas_int* ipiv, char* equed,  float* r,  float* c,  float* b, blas_int* ldb,  float* x, blas_int* ldx,  float* rcond,  float* ferr,  float* berr,  float* work, blas_int* iwork, blas_int* info);
  void arma_fortran(arma_dgesvx)(char* fact, char* trans, blas_int* n, blas_int* nrhs, double* a, blas_int* lda, double* af, blas_int* ldaf, blas_int* ipiv, char* equed, double* r, double* c, double* b, blas_int* ldb, double* x, blas_int* ldx, double* rcond, double* ferr, double* berr, double* work, blas_int* iwork, blas_int* info);

  // solve system of linear equations (general square matrix, advanced form, complex matrices)
  void arma_fortran(arma_cgesvx)(char* fact, char* trans, blas_int* n, blas_int* nrhs, void* a, blas_int* lda, void* af, blas_int* ldaf, blas_int* ipiv, char* equed,  float* r,  float* c, void* b, blas_int* ldb, void* x, blas_int* ldx,  float* rcond,  float* ferr,  float* berr, void* work,  float* rwork, blas_int* info);
  void arma_fortran(arma_zgesvx)(char* fact, char* trans, blas_int* n, blas_int* nrhs, void* a, blas_int* lda, void* af, blas_int* ldaf, blas_int* ipiv, char* equed, double* r, double* c, void* b, blas_int* ldb, void* x, blas_int* ldx, double* rcond, double* ferr, double* berr, void* work, double* rwork, blas_int* info);

  // solve over/under-determined system of linear equations
  void arma_fortran(arma_sgels)(char* trans, blas_int* m, blas_int* n, blas_int* nrhs, float*  a, blas_int* lda, float*  b, blas_int* ldb, float*  work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_dgels)(char* trans, blas_int* m, blas_int* n, blas_int* nrhs, double* a, blas_int* lda, double* b, blas_int* ldb, double* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_cgels)(char* trans, blas_int* m, blas_int* n, blas_int* nrhs, void*   a, blas_int* lda, void*   b, blas_int* ldb, void*   work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_zgels)(char* trans, blas_int* m, blas_int* n, blas_int* nrhs, void*   a, blas_int* lda, void*   b, blas_int* ldb, void*   work, blas_int* lwork, blas_int* info);

  // approximately solve system of linear equations using svd (real)
  void arma_fortran(arma_sgelsd)(blas_int* m, blas_int* n, blas_int* nrhs,  float* a, blas_int* lda,  float* b, blas_int* ldb,  float* S,  float* rcond, blas_int* rank,  float* work, blas_int* lwork, blas_int* iwork, blas_int* info);
  void arma_fortran(arma_dgelsd)(blas_int* m, blas_int* n, blas_int* nrhs, double* a, blas_int* lda, double* b, blas_int* ldb, double* S, double* rcond, blas_int* rank, double* work, blas_int* lwork, blas_int* iwork, blas_int* info);

  // approximately solve system of linear equations using svd (complex)
  void arma_fortran(arma_cgelsd)(blas_int* m, blas_int* n, blas_int* nrhs, void* a, blas_int* lda, void* b, blas_int* ldb,  float* S,  float* rcond, blas_int* rank, void* work, blas_int* lwork,  float* rwork, blas_int* iwork, blas_int* info);
  void arma_fortran(arma_zgelsd)(blas_int* m, blas_int* n, blas_int* nrhs, void* a, blas_int* lda, void* b, blas_int* ldb, double* S, double* rcond, blas_int* rank, void* work, blas_int* lwork, double* rwork, blas_int* iwork, blas_int* info);

  // solve system of linear equations (triangular matrix)
  void arma_fortran(arma_strtrs)(char* uplo, char* trans, char* diag, blas_int* n, blas_int* nrhs, const float*  a, blas_int* lda, float*  b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_dtrtrs)(char* uplo, char* trans, char* diag, blas_int* n, blas_int* nrhs, const double* a, blas_int* lda, double* b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_ctrtrs)(char* uplo, char* trans, char* diag, blas_int* n, blas_int* nrhs, const void*   a, blas_int* lda, void*   b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_ztrtrs)(char* uplo, char* trans, char* diag, blas_int* n, blas_int* nrhs, const void*   a, blas_int* lda, void*   b, blas_int* ldb, blas_int* info);

  // Schur decomposition (real matrices)
  void arma_fortran(arma_sgees)(char* jobvs, char* sort, void* select, blas_int* n, float*  a, blas_int* lda, blas_int* sdim, float*  wr, float*  wi, float*  vs, blas_int* ldvs, float*  work, blas_int* lwork, blas_int* bwork, blas_int* info);
  void arma_fortran(arma_dgees)(char* jobvs, char* sort, void* select, blas_int* n, double* a, blas_int* lda, blas_int* sdim, double* wr, double* wi, double* vs, blas_int* ldvs, double* work, blas_int* lwork, blas_int* bwork, blas_int* info);

  // Schur decomposition (complex matrices)
  void arma_fortran(arma_cgees)(char* jobvs, char* sort, void* select, blas_int* n, void* a, blas_int* lda, blas_int* sdim, void* w, void* vs, blas_int* ldvs, void* work, blas_int* lwork, float*  rwork, blas_int* bwork, blas_int* info);
  void arma_fortran(arma_zgees)(char* jobvs, char* sort, void* select, blas_int* n, void* a, blas_int* lda, blas_int* sdim, void* w, void* vs, blas_int* ldvs, void* work, blas_int* lwork, double* rwork, blas_int* bwork, blas_int* info);

  // solve a Sylvester equation ax + xb = c, with a and b assumed to be in Schur form
  void arma_fortran(arma_strsyl)(char* transa, char* transb, blas_int* isgn, blas_int* m, blas_int* n, const float*  a, blas_int* lda, const float*  b, blas_int* ldb, float*  c, blas_int* ldc, float*  scale, blas_int* info);
  void arma_fortran(arma_dtrsyl)(char* transa, char* transb, blas_int* isgn, blas_int* m, blas_int* n, const double* a, blas_int* lda, const double* b, blas_int* ldb, double* c, blas_int* ldc, double* scale, blas_int* info);
  void arma_fortran(arma_ctrsyl)(char* transa, char* transb, blas_int* isgn, blas_int* m, blas_int* n, const void*   a, blas_int* lda, const void*   b, blas_int* ldb, void*   c, blas_int* ldc, float*  scale, blas_int* info);
  void arma_fortran(arma_ztrsyl)(char* transa, char* transb, blas_int* isgn, blas_int* m, blas_int* n, const void*   a, blas_int* lda, const void*   b, blas_int* ldb, void*   c, blas_int* ldc, double* scale, blas_int* info);

  void arma_fortran(arma_ssytrf)(char* uplo, blas_int* n, float*  a, blas_int* lda, blas_int* ipiv, float*  work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_dsytrf)(char* uplo, blas_int* n, double* a, blas_int* lda, blas_int* ipiv, double* work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_csytrf)(char* uplo, blas_int* n, void*   a, blas_int* lda, blas_int* ipiv, void*   work, blas_int* lwork, blas_int* info);
  void arma_fortran(arma_zsytrf)(char* uplo, blas_int* n, void*   a, blas_int* lda, blas_int* ipiv, void*   work, blas_int* lwork, blas_int* info);

  void arma_fortran(arma_ssytri)(char* uplo, blas_int* n, float*  a, blas_int* lda, blas_int* ipiv, float*  work, blas_int* info);
  void arma_fortran(arma_dsytri)(char* uplo, blas_int* n, double* a, blas_int* lda, blas_int* ipiv, double* work, blas_int* info);
  void arma_fortran(arma_csytri)(char* uplo, blas_int* n, void*   a, blas_int* lda, blas_int* ipiv, void*   work, blas_int* info);
  void arma_fortran(arma_zsytri)(char* uplo, blas_int* n, void*   a, blas_int* lda, blas_int* ipiv, void*   work, blas_int* info);

  // QZ decomposition (real matrices)
  void arma_fortran(arma_sgges)(char* jobvsl, char* jobvsr, char* sort, void* selctg, blas_int* n,  float* a, blas_int* lda,  float* b, blas_int* ldb, blas_int* sdim,  float* alphar,  float* alphai,  float* beta,  float* vsl, blas_int* ldvsl,  float* vsr, blas_int* ldvsr,  float* work, blas_int* lwork,  float* bwork, blas_int* info);
  void arma_fortran(arma_dgges)(char* jobvsl, char* jobvsr, char* sort, void* selctg, blas_int* n, double* a, blas_int* lda, double* b, blas_int* ldb, blas_int* sdim, double* alphar, double* alphai, double* beta, double* vsl, blas_int* ldvsl, double* vsr, blas_int* ldvsr, double* work, blas_int* lwork, double* bwork, blas_int* info);

  // QZ decomposition (complex matrices)
  void arma_fortran(arma_cgges)(char* jobvsl, char* jobvsr, char* sort, void* selctg, blas_int* n, void* a, blas_int* lda, void* b, blas_int* ldb, blas_int* sdim, void* alpha, void* beta, void* vsl, blas_int* ldvsl, void* vsr, blas_int* ldvsr, void* work, blas_int* lwork,  float* rwork,  float* bwork, blas_int* info);
  void arma_fortran(arma_zgges)(char* jobvsl, char* jobvsr, char* sort, void* selctg, blas_int* n, void* a, blas_int* lda, void* b, blas_int* ldb, blas_int* sdim, void* alpha, void* beta, void* vsl, blas_int* ldvsl, void* vsr, blas_int* ldvsr, void* work, blas_int* lwork, double* rwork, double* bwork, blas_int* info);

  // 1-norm
  float  arma_fortran(arma_slange)(char* norm, blas_int* m, blas_int* n,  float* a, blas_int* lda,  float* work);
  double arma_fortran(arma_dlange)(char* norm, blas_int* m, blas_int* n, double* a, blas_int* lda, double* work);
  float  arma_fortran(arma_clange)(char* norm, blas_int* m, blas_int* n,   void* a, blas_int* lda,  float* work);
  double arma_fortran(arma_zlange)(char* norm, blas_int* m, blas_int* n,   void* a, blas_int* lda, double* work);

  // reciprocal of condition number (real)
  void arma_fortran(arma_sgecon)(char* norm, blas_int* n,  float* a, blas_int* lda,  float* anorm,  float* rcond,  float* work, blas_int* iwork, blas_int* info);
  void arma_fortran(arma_dgecon)(char* norm, blas_int* n, double* a, blas_int* lda, double* anorm, double* rcond, double* work, blas_int* iwork, blas_int* info);

  // reciprocal of condition number (complex)
  void arma_fortran(arma_cgecon)(char* norm, blas_int* n, void* a, blas_int* lda,  float* anorm,  float* rcond, void* work,  float* rwork, blas_int* info);
  void arma_fortran(arma_zgecon)(char* norm, blas_int* n, void* a, blas_int* lda, double* anorm, double* rcond, void* work, double* rwork, blas_int* info);

  // obtain parameters according to the local configuration of lapack
  blas_int arma_fortran(arma_ilaenv)(blas_int* ispec, char* name, char* opts, blas_int* n1, blas_int* n2, blas_int* n3, blas_int* n4);

  // solve linear equations using LDL decomposition
  void arma_fortran(arma_ssytrs)(char* uplo, blas_int* n, blas_int* nrhs, float*  a, blas_int* lda, blas_int* ipiv, float*  b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_dsytrs)(char* uplo, blas_int* n, blas_int* nrhs, double* a, blas_int* lda, blas_int* ipiv, double* b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_csytrs)(char* uplo, blas_int* n, blas_int* nrhs, void*   a, blas_int* lda, blas_int* ipiv, void*   b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_zsytrs)(char* uplo, blas_int* n, blas_int* nrhs, void*   a, blas_int* lda, blas_int* ipiv, void*   b, blas_int* ldb, blas_int* info);

  // solve linear equations using LU decomposition
  void arma_fortran(arma_sgetrs)(char* trans, blas_int* n, blas_int* nrhs, float*  a, blas_int* lda, blas_int* ipiv, float*  b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_dgetrs)(char* trans, blas_int* n, blas_int* nrhs, double* a, blas_int* lda, blas_int* ipiv, double* b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_cgetrs)(char* trans, blas_int* n, blas_int* nrhs, void*   a, blas_int* lda, blas_int* ipiv, void*   b, blas_int* ldb, blas_int* info);
  void arma_fortran(arma_zgetrs)(char* trans, blas_int* n, blas_int* nrhs, void*   a, blas_int* lda, blas_int* ipiv, void*   b, blas_int* ldb, blas_int* info);

  // calculate eigenvalues of an upper Hessenberg matrix
  void arma_fortran(arma_slahqr)(blas_int* wantt, blas_int* wantz, blas_int* n, blas_int* ilo, blas_int* ihi, float*  h, blas_int* ldh, float*  wr, float*  wi, blas_int* iloz, blas_int* ihiz, float*  z, blas_int* ldz, blas_int* info);
  void arma_fortran(arma_dlahqr)(blas_int* wantt, blas_int* wantz, blas_int* n, blas_int* ilo, blas_int* ihi, double* h, blas_int* ldh, double* wr, double* wi, blas_int* iloz, blas_int* ihiz, double* z, blas_int* ldz, blas_int* info);

  // calculate eigenvalues of a symmetric tridiagonal matrix
  void arma_fortran(arma_sstedc)(char* compz, blas_int* n, float*  d, float*  e, float*  z, blas_int* ldz, float*  work, blas_int* lwork, blas_int* iwork, blas_int* liwork, blas_int* info);
  void arma_fortran(arma_dstedc)(char* compz, blas_int* n, double* d, double* e, double* z, blas_int* ldz, double* work, blas_int* lwork, blas_int* iwork, blas_int* liwork, blas_int* info);

  // calculate eigenvectors of a Schur form matrix
  void arma_fortran(arma_strevc)(char* side, char* howmny, blas_int* select, blas_int* n, float*  t, blas_int* ldt, float*  vl, blas_int* ldvl, float*  vr, blas_int* ldvr, blas_int* mm, blas_int* m, float*  work, blas_int* info);
  void arma_fortran(arma_dtrevc)(char* side, char* howmny, blas_int* select, blas_int* n, double* t, blas_int* ldt, double* vl, blas_int* ldvl, double* vr, blas_int* ldvr, blas_int* mm, blas_int* m, double* work, blas_int* info);

  // generate a vector of random numbers
  void arma_fortran(arma_slarnv)(blas_int* idist, blas_int* iseed, blas_int* n, float*  x);
  void arma_fortran(arma_dlarnv)(blas_int* idist, blas_int* iseed, blas_int* n, double* x);
  }


#endif
