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



#ifdef ARMA_USE_BLAS


#if !defined(ARMA_BLAS_CAPITALS)

  #define arma_sasum sasum
  #define arma_dasum dasum

  #define arma_snrm2 snrm2
  #define arma_dnrm2 dnrm2

  #define arma_sdot  sdot
  #define arma_ddot  ddot

  #define arma_sgemv sgemv
  #define arma_dgemv dgemv
  #define arma_cgemv cgemv
  #define arma_zgemv zgemv

  #define arma_sgemm sgemm
  #define arma_dgemm dgemm
  #define arma_cgemm cgemm
  #define arma_zgemm zgemm

  #define arma_ssyrk ssyrk
  #define arma_dsyrk dsyrk

  #define arma_cherk cherk
  #define arma_zherk zherk

#else

  #define arma_sasum SASUM
  #define arma_dasum DASUM

  #define arma_snrm2 SNRM2
  #define arma_dnrm2 DNRM2

  #define arma_sdot  SDOT
  #define arma_ddot  DDOT

  #define arma_sgemv SGEMV
  #define arma_dgemv DGEMV
  #define arma_cgemv CGEMV
  #define arma_zgemv ZGEMV

  #define arma_sgemm SGEMM
  #define arma_dgemm DGEMM
  #define arma_cgemm CGEMM
  #define arma_zgemm ZGEMM

  #define arma_ssyrk SSYRK
  #define arma_dsyrk DSYRK

  #define arma_cherk CHERK
  #define arma_zherk ZHERK

#endif



extern "C"
  {
  float  arma_fortran(arma_sasum)(blas_int* n, const float*  x, blas_int* incx);
  double arma_fortran(arma_dasum)(blas_int* n, const double* x, blas_int* incx);

  float  arma_fortran(arma_snrm2)(blas_int* n, const float*  x, blas_int* incx);
  double arma_fortran(arma_dnrm2)(blas_int* n, const double* x, blas_int* incx);

  float  arma_fortran(arma_sdot)(blas_int* n, const float*  x, blas_int* incx, const float*  y, blas_int* incy);
  double arma_fortran(arma_ddot)(blas_int* n, const double* x, blas_int* incx, const double* y, blas_int* incy);

  void arma_fortran(arma_sgemv)(const char* transA, const blas_int* m, const blas_int* n, const float*  alpha, const float*  A, const blas_int* ldA, const float*  x, const blas_int* incx, const float*  beta, float*  y, const blas_int* incy);
  void arma_fortran(arma_dgemv)(const char* transA, const blas_int* m, const blas_int* n, const double* alpha, const double* A, const blas_int* ldA, const double* x, const blas_int* incx, const double* beta, double* y, const blas_int* incy);
  void arma_fortran(arma_cgemv)(const char* transA, const blas_int* m, const blas_int* n, const void*   alpha, const void*   A, const blas_int* ldA, const void*   x, const blas_int* incx, const void*   beta, void*   y, const blas_int* incy);
  void arma_fortran(arma_zgemv)(const char* transA, const blas_int* m, const blas_int* n, const void*   alpha, const void*   A, const blas_int* ldA, const void*   x, const blas_int* incx, const void*   beta, void*   y, const blas_int* incy);

  void arma_fortran(arma_sgemm)(const char* transA, const char* transB, const blas_int* m, const blas_int* n, const blas_int* k, const float*  alpha, const float*  A, const blas_int* ldA, const float*  B, const blas_int* ldB, const float*  beta, float*  C, const blas_int* ldC);
  void arma_fortran(arma_dgemm)(const char* transA, const char* transB, const blas_int* m, const blas_int* n, const blas_int* k, const double* alpha, const double* A, const blas_int* ldA, const double* B, const blas_int* ldB, const double* beta, double* C, const blas_int* ldC);
  void arma_fortran(arma_cgemm)(const char* transA, const char* transB, const blas_int* m, const blas_int* n, const blas_int* k, const void*   alpha, const void*   A, const blas_int* ldA, const void*   B, const blas_int* ldB, const void*   beta, void*   C, const blas_int* ldC);
  void arma_fortran(arma_zgemm)(const char* transA, const char* transB, const blas_int* m, const blas_int* n, const blas_int* k, const void*   alpha, const void*   A, const blas_int* ldA, const void*   B, const blas_int* ldB, const void*   beta, void*   C, const blas_int* ldC);

  void arma_fortran(arma_ssyrk)(const char* uplo, const char* transA, const blas_int* n, const blas_int* k, const  float* alpha, const  float* A, const blas_int* ldA, const  float* beta,  float* C, const blas_int* ldC);
  void arma_fortran(arma_dsyrk)(const char* uplo, const char* transA, const blas_int* n, const blas_int* k, const double* alpha, const double* A, const blas_int* ldA, const double* beta, double* C, const blas_int* ldC);

  void arma_fortran(arma_cherk)(const char* uplo, const char* transA, const blas_int* n, const blas_int* k, const  float* alpha, const   void* A, const blas_int* ldA, const  float* beta,   void* C, const blas_int* ldC);
  void arma_fortran(arma_zherk)(const char* uplo, const char* transA, const blas_int* n, const blas_int* k, const double* alpha, const   void* A, const blas_int* ldA, const double* beta,   void* C, const blas_int* ldC);
  }


#endif
