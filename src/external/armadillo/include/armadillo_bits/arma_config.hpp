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


//! \addtogroup arma_config
//! @{



struct arma_config
  {
  #if defined(ARMA_MAT_PREALLOC)
    static const uword mat_prealloc = (sword(ARMA_MAT_PREALLOC) > 0) ? uword(ARMA_MAT_PREALLOC) : 1;
  #else
    static const uword mat_prealloc = 16;
  #endif


  #if defined(ARMA_OPENMP_THRESHOLD)
    static const uword mp_threshold = (sword(ARMA_OPENMP_THRESHOLD) > 0) ? uword(ARMA_OPENMP_THRESHOLD) : 384;
  #else
    static const uword mp_threshold = 384;
  #endif


  #if defined(ARMA_OPENMP_THREADS)
    static const uword mp_threads = (sword(ARMA_OPENMP_THREADS) > 0) ? uword(ARMA_OPENMP_THREADS) : 10;
  #else
    static const uword mp_threads = 10;
  #endif


  #if defined(ARMA_SPMAT_CHUNKSIZE)
    static const uword spmat_chunksize = (sword(ARMA_SPMAT_CHUNKSIZE) > 0) ? uword(ARMA_SPMAT_CHUNKSIZE) : 256;
  #else
    static const uword spmat_chunksize = 256;
  #endif


  #if defined(ARMA_USE_ATLAS)
    static const bool atlas = true;
  #else
    static const bool atlas = false;
  #endif


  #if defined(ARMA_USE_LAPACK)
    static const bool lapack = true;
  #else
    static const bool lapack = false;
  #endif


  #if defined(ARMA_USE_BLAS)
    static const bool blas = true;
  #else
    static const bool blas = false;
  #endif


  #if defined(ARMA_USE_NEWARP)
    static const bool newarp = true;
  #else
    static const bool newarp = false;
  #endif


  #if defined(ARMA_USE_ARPACK)
    static const bool arpack = true;
  #else
    static const bool arpack = false;
  #endif


  #if defined(ARMA_USE_SUPERLU)
    static const bool superlu = true;
  #else
    static const bool superlu = false;
  #endif


  #if defined(ARMA_USE_HDF5)
    static const bool hdf5 = true;
  #else
    static const bool hdf5 = false;
  #endif


  #if defined(ARMA_NO_DEBUG)
    static const bool debug = false;
  #else
    static const bool debug = true;
  #endif


  #if defined(ARMA_EXTRA_DEBUG)
    static const bool extra_debug = true;
  #else
    static const bool extra_debug = false;
  #endif


  #if defined(ARMA_GOOD_COMPILER)
    static const bool good_comp = true;
  #else
    static const bool good_comp = false;
  #endif


  #if (  \
         defined(ARMA_EXTRA_MAT_PROTO)   || defined(ARMA_EXTRA_MAT_MEAT)   \
      || defined(ARMA_EXTRA_COL_PROTO)   || defined(ARMA_EXTRA_COL_MEAT)   \
      || defined(ARMA_EXTRA_ROW_PROTO)   || defined(ARMA_EXTRA_ROW_MEAT)   \
      || defined(ARMA_EXTRA_CUBE_PROTO)  || defined(ARMA_EXTRA_CUBE_MEAT)  \
      || defined(ARMA_EXTRA_FIELD_PROTO) || defined(ARMA_EXTRA_FIELD_MEAT) \
      || defined(ARMA_EXTRA_SPMAT_PROTO) || defined(ARMA_EXTRA_SPMAT_MEAT) \
      || defined(ARMA_EXTRA_SPCOL_PROTO) || defined(ARMA_EXTRA_SPCOL_MEAT) \
      || defined(ARMA_EXTRA_SPROW_PROTO) || defined(ARMA_EXTRA_SPROW_MEAT) \
      )
    static const bool extra_code = true;
  #else
    static const bool extra_code = false;
  #endif


  #if defined(ARMA_USE_CXX11)
    static const bool cxx11 = true;
  #else
    static const bool cxx11 = false;
  #endif


  #if defined(ARMA_USE_WRAPPER)
    static const bool wrapper = true;
  #else
    static const bool wrapper = false;
  #endif


  #if defined(ARMA_USE_OPENMP)
    static const bool openmp = true;
  #else
    static const bool openmp = false;
  #endif
  };



//! @}
