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



#if !defined(ARMA_USE_LAPACK)
#define ARMA_USE_LAPACK
//// Comment out the above line if you don't have LAPACK or a high-speed replacement for LAPACK,
//// such as Intel MKL, AMD ACML, or the Accelerate framework.
//// LAPACK is required for matrix decompositions (eg. SVD) and matrix inverse.
#endif

#if !defined(ARMA_USE_BLAS)
#define ARMA_USE_BLAS
//// Comment out the above line if you don't have BLAS or a high-speed replacement for BLAS,
//// such as OpenBLAS, GotoBLAS, Intel MKL, AMD ACML, or the Accelerate framework.
//// BLAS is used for matrix multiplication.
//// Without BLAS, matrix multiplication will still work, but might be slower.
#endif

#if !defined(ARMA_USE_NEWARP)
#define ARMA_USE_NEWARP
//// Uncomment the above line to enable the built-in partial emulation of ARPACK.
//// This is used for eigen decompositions of real (non-complex) sparse matrices, eg. eigs_sym(), svds()
#endif

#if !defined(ARMA_USE_ARPACK)
// #define ARMA_USE_ARPACK
//// Uncomment the above line if you have ARPACK or a high-speed replacement for ARPACK.
//// ARPACK is required for eigen decompositions of complex sparse matrices
#endif

#if !defined(ARMA_USE_SUPERLU)
// #define ARMA_USE_SUPERLU
//// Uncomment the above line if you have SuperLU.
//// SuperLU is used for solving sparse linear systems via spsolve()
//// Caveat: only SuperLU version 5.2 can be used!
#endif

#if !defined(ARMA_SUPERLU_INCLUDE_DIR)
// #define ARMA_SUPERLU_INCLUDE_DIR /usr/include/
//// If you're using SuperLU and want to explicitly include the SuperLU headers,
//// uncomment the above define and specify the appropriate include directory.
//// Make sure the directory has a trailing /
#endif

// #define ARMA_USE_WRAPPER
//// Comment out the above line if you're getting linking errors when compiling your programs,
//// or if you prefer to directly link with LAPACK, BLAS + etc instead of the Armadillo runtime library.
//// You will then need to link your programs directly with -llapack -lblas instead of -larmadillo

// #define ARMA_BLAS_CAPITALS
//// Uncomment the above line if your BLAS and LAPACK libraries have capitalised function names (eg. ACML on 64-bit Windows)

#define ARMA_BLAS_UNDERSCORE
//// Uncomment the above line if your BLAS and LAPACK libraries have function names with a trailing underscore.
//// Conversely, comment it out if the function names don't have a trailing underscore.

// #define ARMA_BLAS_LONG
//// Uncomment the above line if your BLAS and LAPACK libraries use "long" instead of "int"

// #define ARMA_BLAS_LONG_LONG
//// Uncomment the above line if your BLAS and LAPACK libraries use "long long" instead of "int"

// #define ARMA_USE_TBB_ALLOC
//// Uncomment the above line if you want to use Intel TBB scalable_malloc() and scalable_free() instead of standard malloc() and free()

// #define ARMA_USE_MKL_ALLOC
//// Uncomment the above line if you want to use Intel MKL mkl_malloc() and mkl_free() instead of standard malloc() and free()

// #define ARMA_USE_ATLAS
// #define ARMA_ATLAS_INCLUDE_DIR /usr/include/
//// If you're using ATLAS and the compiler can't find cblas.h and/or clapack.h
//// uncomment the above define and specify the appropriate include directory.
//// Make sure the directory has a trailing /

#if !defined(ARMA_USE_CXX11)
// #define ARMA_USE_CXX11
//// Uncomment the above line to forcefully enable use of C++11 features (eg. initialiser lists).
//// Note that ARMA_USE_CXX11 is automatically enabled when a C++11 compiler is detected.
#endif

#if !defined(ARMA_USE_OPENMP)
// #define ARMA_USE_OPENMP
//// Uncomment the above line to forcefully enable use of OpenMP for parallelisation.
//// Note that ARMA_USE_OPENMP is automatically enabled when a compiler supporting OpenMP 3.0 is detected.
#endif

#if !defined(ARMA_64BIT_WORD)
// #define ARMA_64BIT_WORD
//// Uncomment the above line if you require matrices/vectors capable of holding more than 4 billion elements.
//// Your machine and compiler must have support for 64 bit integers (eg. via "long" or "long long").
//// Note that ARMA_64BIT_WORD is automatically enabled when a C++11 compiler is detected.
#endif

#if !defined(ARMA_USE_HDF5)
// #define ARMA_USE_HDF5
//// Uncomment the above line to allow the ability to save and load matrices stored in HDF5 format;
//// the hdf5.h header file must be available on your system,
//// and you will need to link with the hdf5 library (eg. -lhdf5)
#endif

// #define ARMA_USE_HDF5_ALT
#if defined(ARMA_USE_HDF5_ALT) && defined(ARMA_USE_WRAPPER)
  #undef  ARMA_USE_HDF5
  #define ARMA_USE_HDF5

  // #define ARMA_HDF5_INCLUDE_DIR /usr/include/
#endif

#if !defined(ARMA_MAT_PREALLOC)
  #define ARMA_MAT_PREALLOC 16
#endif
//// This is the number of preallocated elements used by matrices and vectors;
//// it must be an integer that is at least 1.
//// If you mainly use lots of very small vectors (eg. <= 4 elements),
//// change the number to the size of your vectors.

#if !defined(ARMA_OPENMP_THRESHOLD)
  #define ARMA_OPENMP_THRESHOLD 384
#endif
//// The minimum number of elements in a matrix to allow OpenMP based parallelisation;
//// it must be an integer that is at least 1.

#if !defined(ARMA_OPENMP_THREADS)
  #define ARMA_OPENMP_THREADS 10
#endif
//// The maximum number of threads to use for OpenMP based parallelisation;
//// it must be an integer that is at least 1.

#if !defined(ARMA_SPMAT_CHUNKSIZE)
  #define ARMA_SPMAT_CHUNKSIZE 256
#endif
//// This is the minimum increase in the amount of memory (in terms of elements) allocated by a sparse matrix;
//// it must be an integer that is at least 1.
//// The minimum recommended size is 16.

// #define ARMA_NO_DEBUG
//// Uncomment the above line if you want to disable all run-time checks.
//// This will result in faster code, but you first need to make sure that your code runs correctly!
//// We strongly recommend to have the run-time checks enabled during development,
//// as this greatly aids in finding mistakes in your code, and hence speeds up development.
//// We recommend that run-time checks be disabled _only_ for the shipped version of your program.

// #define ARMA_EXTRA_DEBUG
//// Uncomment the above line if you want to see the function traces of how Armadillo evaluates expressions.
//// This is mainly useful for debugging of the library.


#if defined(ARMA_DEFAULT_OSTREAM)
  #pragma message ("WARNING: support for ARMA_DEFAULT_OSTREAM is deprecated and will be removed;")
  #pragma message ("WARNING: use ARMA_COUT_STREAM and ARMA_CERR_STREAM instead")
#endif


#if !defined(ARMA_COUT_STREAM)
  #if defined(ARMA_DEFAULT_OSTREAM)
    // for compatibility with earlier versions of Armadillo
    #define ARMA_COUT_STREAM ARMA_DEFAULT_OSTREAM
  #else
    #define ARMA_COUT_STREAM std::cout
  #endif
#endif

#if !defined(ARMA_CERR_STREAM)
  #if defined(ARMA_DEFAULT_OSTREAM)
    // for compatibility with earlier versions of Armadillo
    #define ARMA_CERR_STREAM ARMA_DEFAULT_OSTREAM
  #else
    #define ARMA_CERR_STREAM std::cerr
  #endif
#endif


#if !defined(ARMA_PRINT_ERRORS)
//#define ARMA_PRINT_ERRORS
//// Comment out the above line if you don't want errors and warnings printed (eg. failed decompositions)
#endif

#if !defined(ARMA_PRINT_HDF5_ERRORS)
// #define ARMA_PRINT_HDF5_ERRORS
#endif

#if defined(ARMA_DONT_USE_LAPACK)
  #undef ARMA_USE_LAPACK
#endif

#if defined(ARMA_DONT_USE_BLAS)
  #undef ARMA_USE_BLAS
#endif

#if defined(ARMA_DONT_USE_NEWARP) || !defined(ARMA_USE_LAPACK)
  #undef ARMA_USE_NEWARP
#endif

#if defined(ARMA_DONT_USE_ARPACK)
  #undef ARMA_USE_ARPACK
#endif

#if defined(ARMA_DONT_USE_SUPERLU)
  #undef ARMA_USE_SUPERLU
  #undef ARMA_SUPERLU_INCLUDE_DIR
#endif

#if defined(ARMA_DONT_USE_ATLAS)
  #undef ARMA_USE_ATLAS
  #undef ARMA_ATLAS_INCLUDE_DIR
#endif

#if defined(ARMA_DONT_USE_WRAPPER)
  #undef ARMA_USE_WRAPPER
  #undef ARMA_USE_HDF5_ALT
#endif

#if defined(ARMA_DONT_USE_CXX11)
  #undef ARMA_USE_CXX11
  #undef ARMA_USE_EXTERN_CXX11_RNG
#endif

#if defined(ARMA_DONT_USE_OPENMP)
  #undef ARMA_USE_OPENMP
#endif

#if defined(ARMA_USE_WRAPPER)
  #if defined(ARMA_USE_CXX11)
    #if !defined(ARMA_USE_EXTERN_CXX11_RNG)
      // #define ARMA_USE_EXTERN_CXX11_RNG
    #endif
  #endif
#endif

#if defined(ARMA_DONT_USE_EXTERN_CXX11_RNG)
  #undef ARMA_USE_EXTERN_CXX11_RNG
#endif

#if defined(ARMA_32BIT_WORD)
  #undef ARMA_64BIT_WORD
#endif

#if defined(ARMA_DONT_USE_HDF5)
  #undef ARMA_USE_HDF5
  #undef ARMA_USE_HDF5_ALT
#endif

#if defined(ARMA_DONT_PRINT_ERRORS)
  #undef ARMA_PRINT_ERRORS
#endif

#if defined(ARMA_DONT_PRINT_HDF5_ERRORS)
  #undef ARMA_PRINT_HDF5_ERRORS
#endif


// if Armadillo was installed on this system via CMake and ARMA_USE_WRAPPER is not defined,
// ARMA_AUX_LIBS lists the libraries required by Armadillo on this system, and
// ARMA_AUX_INCDIRS lists the include directories required by Armadillo on this system.
// Do not use these unless you know what you are doing.
#define ARMA_AUX_LIBS
#define ARMA_AUX_INCDIRS
