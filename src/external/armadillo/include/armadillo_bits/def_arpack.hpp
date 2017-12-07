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


#ifdef ARMA_USE_ARPACK

// I'm not sure this is necessary.
#if !defined(ARMA_BLAS_CAPITALS)

  #define arma_snaupd snaupd
  #define arma_dnaupd dnaupd
  #define arma_cnaupd cnaupd
  #define arma_znaupd znaupd

  #define arma_sneupd sneupd
  #define arma_dneupd dneupd
  #define arma_cneupd cneupd
  #define arma_zneupd zneupd

  #define arma_ssaupd ssaupd
  #define arma_dsaupd dsaupd

  #define arma_sseupd sseupd
  #define arma_dseupd dseupd

#else

  #define arma_snaupd SNAUPD
  #define arma_dnaupd DNAUPD
  #define arma_cnaupd CNAUPD
  #define arma_znaupd ZNAUPD

  #define arma_sneupd SNEUPD
  #define arma_dneupd DNEUPD
  #define arma_cneupd CNEUPD
  #define arma_zneupd ZNEUPD

  #define arma_ssaupd SSAUPD
  #define arma_dsaupd DSAUPD

  #define arma_sseupd SSEUPD
  #define arma_dseupd DSEUPD

#endif

extern "C"
  {
  // eigendecomposition of non-symmetric positive semi-definite matrices
  void arma_fortran(arma_snaupd)(blas_int* ido, char* bmat, blas_int* n, char* which, blas_int* nev,  float* tol,  float* resid, blas_int* ncv,  float* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr,  float* workd,  float* workl, blas_int* lworkl, blas_int* info);
  void arma_fortran(arma_dnaupd)(blas_int* ido, char* bmat, blas_int* n, char* which, blas_int* nev, double* tol, double* resid, blas_int* ncv, double* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr, double* workd, double* workl, blas_int* lworkl, blas_int* info);
  void arma_fortran(arma_cnaupd)(blas_int* ido, char* bmat, blas_int* n, char* which, blas_int* nev,  float* tol,   void* resid, blas_int* ncv,   void* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr,   void* workd,   void* workl, blas_int* lworkl, float* rwork, blas_int* info);
  void arma_fortran(arma_znaupd)(blas_int* ido, char* bmat, blas_int* n, char* which, blas_int* nev, double* tol,   void* resid, blas_int* ncv,   void* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr,   void* workd,   void* workl, blas_int* lworkl, double* rwork, blas_int* info);

  // eigendecomposition of symmetric positive semi-definite matrices
  void arma_fortran(arma_ssaupd)(blas_int* ido, char* bmat, blas_int* n, char* which, blas_int* nev,  float* tol,  float* resid, blas_int* ncv,  float* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr,  float* workd,  float* workl, blas_int* lworkl, blas_int* info);
  void arma_fortran(arma_dsaupd)(blas_int* ido, char* bmat, blas_int* n, char* which, blas_int* nev, double* tol, double* resid, blas_int* ncv, double* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr, double* workd, double* workl, blas_int* lworkl, blas_int* info);

  // recovery of eigenvectors after naupd(); uses blas_int for LOGICAL types
  void arma_fortran(arma_sneupd)(blas_int* rvec, char* howmny, blas_int* select,  float* dr,  float* di,  float* z, blas_int* ldz,  float* sigmar,  float* sigmai,  float* workev, char* bmat, blas_int* n, char* which, blas_int* nev,  float* tol,  float* resid, blas_int* ncv,  float* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr,  float* workd,  float* workl, blas_int* lworkl, blas_int* info);
  void arma_fortran(arma_dneupd)(blas_int* rvec, char* howmny, blas_int* select, double* dr, double* di, double* z, blas_int* ldz, double* sigmar, double* sigmai, double* workev, char* bmat, blas_int* n, char* which, blas_int* nev, double* tol, double* resid, blas_int* ncv, double* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr, double* workd, double* workl, blas_int* lworkl, blas_int* info);
  void arma_fortran(arma_cneupd)(blas_int* rvec, char* howmny, blas_int* select, void* d,   void* z, blas_int* ldz,   void* sigma,   void* workev, char* bmat, blas_int* n, char* which, blas_int* nev,  float* tol,   void* resid, blas_int* ncv,   void* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr,   void* workd, void* workl, blas_int* lworkl,  float* rwork, blas_int* info);
  void arma_fortran(arma_zneupd)(blas_int* rvec, char* howmny, blas_int* select, void* d,   void* z, blas_int* ldz,   void* sigma,   void* workev, char* bmat, blas_int* n, char* which, blas_int* nev, double* tol,   void* resid, blas_int* ncv,   void* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr,   void* workd, void* workl, blas_int* lworkl, double* rwork, blas_int* info);

  // recovery of eigenvectors after saupd(); uses blas_int for LOGICAL types
  void arma_fortran(arma_sseupd)(blas_int* rvec, char* howmny, blas_int* select,  float* d,  float* z, blas_int* ldz,  float* sigma, char* bmat, blas_int* n, char* which, blas_int* nev,  float* tol,  float* resid, blas_int* ncv,  float* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr,  float* workd,  float* workl, blas_int* lworkl, blas_int* info);
  void arma_fortran(arma_dseupd)(blas_int* rvec, char* howmny, blas_int* select, double* d, double* z, blas_int* ldz, double* sigma, char* bmat, blas_int* n, char* which, blas_int* nev, double* tol, double* resid, blas_int* ncv, double* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr, double* workd, double* workl, blas_int* lworkl, blas_int* info);
  }

#endif
