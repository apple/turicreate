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

//! \namespace arpack namespace for ARPACK functions
namespace arpack
  {

  // If real, then eT == eeT; otherwise, eT == std::complex<eeT>.
  // For real calls, rwork is ignored; it's only necessary in the complex case.
  template<typename eT, typename eeT>
  inline
  void
  naupd(blas_int* ido, char* bmat, blas_int* n, char* which, blas_int* nev, eeT* tol, eT* resid, blas_int* ncv, eT* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr, eT* workd, eT* workl, blas_int* lworkl, eeT* rwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_snaupd)(ido, bmat, n, which, nev, (T*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dnaupd)(ido, bmat, n, which, nev, (T*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef std::complex<float> T;
      typedef float xT;
      arma_fortran(arma_cnaupd)(ido, bmat, n, which, nev, (xT*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, (xT*) rwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef std::complex<double> T;
      typedef double xT;
      arma_fortran(arma_znaupd)(ido, bmat, n, which, nev, (xT*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, (xT*) rwork, info);
      }
    }


  //! The use of two template types is necessary here because the compiler will
  //! instantiate this method for complex types (where eT != eeT) but that in
  //! practice that is never actually used.
  template<typename eT, typename eeT>
  inline
  void
  saupd(blas_int* ido, char* bmat, blas_int* n, char* which, blas_int* nev, eeT* tol, eT* resid, blas_int* ncv, eT* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr, eT* workd, eT* workl, blas_int* lworkl, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_ssaupd)(ido, bmat, n, which, nev, (T*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dsaupd)(ido, bmat, n, which, nev, (T*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, info);
      }
    }



  template<typename eT>
  inline
  void
  seupd(blas_int* rvec, char* howmny, blas_int* select, eT* d, eT* z, blas_int* ldz, eT* sigma, char* bmat, blas_int* n, char* which, blas_int* nev, eT* tol, eT* resid, blas_int* ncv, eT* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr, eT* workd, eT* workl, blas_int* lworkl, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sseupd)(rvec, howmny, select, (T*) d, (T*) z, ldz, (T*) sigma, bmat, n, which, nev, (T*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dseupd)(rvec, howmny, select, (T*) d, (T*) z, ldz, (T*) sigma, bmat, n, which, nev, (T*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, info);
      }
    }



  // for complex versions, pass d for dr, and null for di; pass sigma for
  // sigmar, and null for sigmai; rwork isn't used for non-complex versions
  template<typename eT, typename eeT>
  inline
  void
  neupd(blas_int* rvec, char* howmny, blas_int* select, eT* dr, eT* di, eT* z, blas_int* ldz, eT* sigmar, eT* sigmai, eT* workev, char* bmat, blas_int* n, char* which, blas_int* nev, eeT* tol, eT* resid, blas_int* ncv, eT* v, blas_int* ldv, blas_int* iparam, blas_int* ipntr, eT* workd, eT* workl, blas_int* lworkl, eeT* rwork, blas_int* info)
    {
    arma_type_check(( is_supported_blas_type<eT>::value == false ));

    if(is_float<eT>::value)
      {
      typedef float T;
      arma_fortran(arma_sneupd)(rvec, howmny, select, (T*) dr, (T*) di, (T*) z, ldz, (T*) sigmar, (T*) sigmai, (T*) workev, bmat, n, which, nev, (T*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, info);
      }
    else
    if(is_double<eT>::value)
      {
      typedef double T;
      arma_fortran(arma_dneupd)(rvec, howmny, select, (T*) dr, (T*) di, (T*) z, ldz, (T*) sigmar, (T*) sigmai, (T*) workev, bmat, n, which, nev, (T*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, info);
      }
    else
    if(is_supported_complex_float<eT>::value)
      {
      typedef float xT; // eT is taken
      typedef std::complex<float> T;
      arma_fortran(arma_cneupd)(rvec, howmny, select, (T*) dr, (T*) z, ldz, (T*) sigmar, (T*) workev, bmat, n, which, nev, (xT*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, (xT*) rwork, info);
      }
    else
    if(is_supported_complex_double<eT>::value)
      {
      typedef double xT; // eT is taken
      typedef std::complex<double> T;
      arma_fortran(arma_zneupd)(rvec, howmny, select, (T*) dr, (T*) z, ldz, (T*) sigmar, (T*) workev, bmat, n, which, nev, (xT*) tol, (T*) resid, ncv, (T*) v, ldv, iparam, ipntr, (T*) workd, (T*) workl, lworkl, (xT*) rwork, info);
      }
    }


  } // namespace arpack


#endif
