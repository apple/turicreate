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


#ifdef ARMA_USE_ATLAS


//! \namespace atlas namespace for ATLAS functions (imported from the global namespace)
namespace atlas
  {

  template<typename eT>
  inline static const eT& tmp_real(const eT& X)              { return X; }

  template<typename T>
  inline static const  T  tmp_real(const std::complex<T>& X) { return X.real(); }



  template<typename eT>
  arma_inline
  eT
  cblas_asum(const int N, const eT* X)
    {
    arma_type_check((is_supported_blas_type<eT>::value == false));

    if(is_float<eT>::value)
      {
      typedef float T;
      return eT( arma_wrapper(cblas_sasum)(N, (const T*)X, 1) );
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      return eT( arma_wrapper(cblas_dasum)(N, (const T*)X, 1) );
      }
    else
      {
      return eT(0);
      }
    }



  template<typename eT>
  arma_inline
  eT
  cblas_nrm2(const int N, const eT* X)
    {
    arma_type_check((is_supported_blas_type<eT>::value == false));

    if(is_float<eT>::value)
      {
      typedef float T;
      return eT( arma_wrapper(cblas_snrm2)(N, (const T*)X, 1) );
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      return eT( arma_wrapper(cblas_dnrm2)(N, (const T*)X, 1) );
      }
    else
      {
      return eT(0);
      }
    }



  template<typename eT>
  arma_inline
  eT
  cblas_dot(const int N, const eT* X, const eT* Y)
    {
    arma_type_check((is_supported_blas_type<eT>::value == false));

    if(is_float<eT>::value)
      {
      typedef float T;
      return eT( arma_wrapper(cblas_sdot)(N, (const T*)X, 1, (const T*)Y, 1) );
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      return eT( arma_wrapper(cblas_ddot)(N, (const T*)X, 1, (const T*)Y, 1) );
      }
    else
      {
      return eT(0);
      }
    }



  template<typename eT>
  arma_inline
  eT
  cblas_cx_dot(const int N, const eT* X, const eT* Y)
    {
    arma_type_check((is_supported_blas_type<eT>::value == false));

    if(is_supported_complex_float<eT>::value)
      {
      typedef typename std::complex<float> T;

      T out;
      arma_wrapper(cblas_cdotu_sub)(N, (const T*)X, 1, (const T*)Y, 1, &out);

      return eT(out);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef typename std::complex<double> T;

      T out;
      arma_wrapper(cblas_zdotu_sub)(N, (const T*)X, 1, (const T*)Y, 1, &out);

      return eT(out);
      }
    else
      {
      return eT(0);
      }
    }



  template<typename eT>
  inline
  void
  cblas_gemv
    (
    const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE TransA,
    const int M, const int N,
    const eT alpha,
    const eT *A, const int lda,
    const eT *X, const int incX,
    const eT beta,
    eT *Y, const int incY
    )
    {
    arma_type_check((is_supported_blas_type<eT>::value == false));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_wrapper(cblas_sgemv)(Order, TransA, M, N, (const T)tmp_real(alpha), (const T*)A, lda, (const T*)X, incX, (const T)tmp_real(beta), (T*)Y, incY);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_wrapper(cblas_dgemv)(Order, TransA, M, N, (const T)tmp_real(alpha), (const T*)A, lda, (const T*)X, incX, (const T)tmp_real(beta), (T*)Y, incY);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_wrapper(cblas_cgemv)(Order, TransA, M, N, (const T*)&alpha, (const T*)A, lda, (const T*)X, incX, (const T*)&beta, (T*)Y, incY);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_wrapper(cblas_zgemv)(Order, TransA, M, N, (const T*)&alpha, (const T*)A, lda, (const T*)X, incX, (const T*)&beta, (T*)Y, incY);
      }
    }



  template<typename eT>
  inline
  void
  cblas_gemm
    (
    const enum CBLAS_ORDER Order, const enum CBLAS_TRANSPOSE TransA,
    const enum CBLAS_TRANSPOSE TransB, const int M, const int N,
    const int K, const eT alpha, const eT *A,
    const int lda, const eT *B, const int ldb,
    const eT beta, eT *C, const int ldc
    )
    {
    arma_type_check((is_supported_blas_type<eT>::value == false));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_wrapper(cblas_sgemm)(Order, TransA, TransB, M, N, K, (const T)tmp_real(alpha), (const T*)A, lda, (const T*)B, ldb, (const T)tmp_real(beta), (T*)C, ldc);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_wrapper(cblas_dgemm)(Order, TransA, TransB, M, N, K, (const T)tmp_real(alpha), (const T*)A, lda, (const T*)B, ldb, (const T)tmp_real(beta), (T*)C, ldc);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      arma_wrapper(cblas_cgemm)(Order, TransA, TransB, M, N, K, (const T*)&alpha, (const T*)A, lda, (const T*)B, ldb, (const T*)&beta, (T*)C, ldc);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      arma_wrapper(cblas_zgemm)(Order, TransA, TransB, M, N, K, (const T*)&alpha, (const T*)A, lda, (const T*)B, ldb, (const T*)&beta, (T*)C, ldc);
      }
    }



  template<typename eT>
  inline
  void
  cblas_syrk
    (
    const enum CBLAS_ORDER Order, const enum CBLAS_UPLO Uplo, const enum CBLAS_TRANSPOSE Trans,
    const int N, const int K, const eT alpha,
    const eT* A, const int lda, const eT beta, eT* C, const int ldc
    )
    {
    arma_type_check((is_supported_blas_type<eT>::value == false));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_wrapper(cblas_ssyrk)(Order, Uplo, Trans, N, K, (const T)alpha, (const T*)A, lda, (const T)beta, (T*)C, ldc);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_wrapper(cblas_dsyrk)(Order, Uplo, Trans, N, K, (const T)alpha, (const T*)A, lda, (const T)beta, (T*)C, ldc);
      }
    }



  template<typename T>
  inline
  void
  cblas_herk
    (
    const enum CBLAS_ORDER Order, const enum CBLAS_UPLO Uplo, const enum CBLAS_TRANSPOSE Trans,
    const int N, const int K, const T alpha,
    const std::complex<T>* A, const int lda, const T beta, std::complex<T>* C, const int ldc
    )
    {
    arma_type_check((is_supported_blas_type<T>::value == false));

    if(is_float<T>::value)
      {
      typedef float                  TT;
      typedef std::complex<float> cx_TT;

      arma_wrapper(cblas_cherk)(Order, Uplo, Trans, N, K, (const TT)alpha, (const cx_TT*)A, lda, (const TT)beta, (cx_TT*)C, ldc);
      }
    else
    if(is_double<T>::value)
      {
      typedef double                  TT;
      typedef std::complex<double> cx_TT;

      arma_wrapper(cblas_zherk)(Order, Uplo, Trans, N, K, (const TT)alpha, (const cx_TT*)A, lda, (const TT)beta, (cx_TT*)C, ldc);
      }
    }



  template<typename eT>
  inline
  int
  clapack_getrf
    (
    const enum CBLAS_ORDER Order, const int M, const int N,
    eT *A, const int lda, int *ipiv
    )
    {
    arma_type_check((is_supported_blas_type<eT>::value == false));

    if(is_float<eT>::value)
      {
      typedef float T;
      return arma_wrapper(clapack_sgetrf)(Order, M, N, (T*)A, lda, ipiv);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      return arma_wrapper(clapack_dgetrf)(Order, M, N, (T*)A, lda, ipiv);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      return arma_wrapper(clapack_cgetrf)(Order, M, N, (T*)A, lda, ipiv);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      return arma_wrapper(clapack_zgetrf)(Order, M, N, (T*)A, lda, ipiv);
      }
    else
      {
      return -1;
      }
    }



  template<typename eT>
  inline
  int
  clapack_getri
    (
    const enum CBLAS_ORDER Order, const int N, eT *A,
    const int lda, const int *ipiv
    )
    {
    arma_type_check((is_supported_blas_type<eT>::value == false));

    if(is_float<eT>::value)
      {
      typedef float T;
      return arma_wrapper(clapack_sgetri)(Order, N, (T*)A, lda, ipiv);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      return arma_wrapper(clapack_dgetri)(Order, N, (T*)A, lda, ipiv);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      return arma_wrapper(clapack_cgetri)(Order, N, (T*)A, lda, ipiv);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      return arma_wrapper(clapack_zgetri)(Order, N, (T*)A, lda, ipiv);
      }
    else
      {
      return -1;
      }
    }



  template<typename eT>
  inline
  int
  clapack_gesv
    (
    const enum CBLAS_ORDER Order,
    const int N, const int NRHS,
    eT* A, const int lda, int* ipiv,
    eT* B, const int ldb
    )
    {
    arma_type_check((is_supported_blas_type<eT>::value == false));

    if(is_float<eT>::value)
      {
      typedef float T;
      return arma_wrapper(clapack_sgesv)(Order, N, NRHS, (T*)A, lda, ipiv, (T*)B, ldb);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      return arma_wrapper(clapack_dgesv)(Order, N, NRHS, (T*)A, lda, ipiv, (T*)B, ldb);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      return arma_wrapper(clapack_cgesv)(Order, N, NRHS, (T*)A, lda, ipiv, (T*)B, ldb);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      return arma_wrapper(clapack_zgesv)(Order, N, NRHS, (T*)A, lda, ipiv, (T*)B, ldb);
      }
    else
      {
      return -1;
      }
    }



  }

#endif
