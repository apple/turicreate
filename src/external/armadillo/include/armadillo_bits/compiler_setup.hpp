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



#undef arma_hot
#undef arma_cold
#undef arma_aligned
#undef arma_align_mem
#undef arma_warn_unused
#undef arma_deprecated
#undef arma_malloc
#undef arma_inline
#undef arma_noinline
#undef arma_ignore

#define arma_hot
#define arma_cold
#define arma_aligned
#define arma_align_mem
#define arma_warn_unused
#define arma_deprecated
#define arma_malloc
#define arma_inline            inline
#define arma_noinline
#define arma_ignore(variable)  ((void)(variable))

#undef arma_fortran_noprefix
#undef arma_fortran_prefix

#undef arma_fortran2_noprefix
#undef arma_fortran2_prefix

#if defined(ARMA_BLAS_UNDERSCORE)
  #define arma_fortran2_noprefix(function) function##_
  #define arma_fortran2_prefix(function)   wrapper_##function##_
#else
  #define arma_fortran2_noprefix(function) function
  #define arma_fortran2_prefix(function)   wrapper_##function
#endif

#if defined(ARMA_USE_WRAPPER)
  #define arma_fortran(function) arma_fortran2_prefix(function)
  #define arma_wrapper(function) wrapper_##function
#else
  #define arma_fortran(function) arma_fortran2_noprefix(function)
  #define arma_wrapper(function) function
#endif

#define arma_fortran_prefix(function)   arma_fortran2_prefix(function)
#define arma_fortran_noprefix(function) arma_fortran2_noprefix(function)

#undef  ARMA_INCFILE_WRAP
#define ARMA_INCFILE_WRAP(x) <x>


#if defined(ARMA_USE_CXX11)

  #undef  ARMA_USE_U64S64
  #define ARMA_USE_U64S64

  #if !defined(ARMA_32BIT_WORD)
    #undef  ARMA_64BIT_WORD
    #define ARMA_64BIT_WORD
  #endif

  #if defined(ARMA_64BIT_WORD) && defined(SIZE_MAX)
    #if (SIZE_MAX < 0xFFFFFFFFFFFFFFFFull)
      // #pragma message ("WARNING: disabled use of 64 bit integers, as std::size_t is smaller than 64 bits")
      #undef ARMA_64BIT_WORD
    #endif
  #endif

#endif


#if defined(ARMA_64BIT_WORD)
  #undef  ARMA_USE_U64S64
  #define ARMA_USE_U64S64
#endif


// most compilers can't vectorise slightly elaborate loops;
// for example clang: http://llvm.org/bugs/show_bug.cgi?id=16358
#undef  ARMA_SIMPLE_LOOPS
#define ARMA_SIMPLE_LOOPS

#undef ARMA_GOOD_COMPILER

#undef ARMA_HAVE_TR1
#undef ARMA_HAVE_GETTIMEOFDAY
#undef ARMA_HAVE_SNPRINTF
#undef ARMA_HAVE_ISFINITE
#undef ARMA_HAVE_LOG1P
#undef ARMA_HAVE_ISINF
#undef ARMA_HAVE_ISNAN


#if (defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200112L))
  #define ARMA_HAVE_GETTIMEOFDAY
#endif


// posix_memalign() is part of IEEE standard 1003.1
// http://pubs.opengroup.org/onlinepubs/009696899/functions/posix_memalign.html
// http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html
// http://sourceforge.net/p/predef/wiki/Standards/
#if ( defined(_POSIX_ADVISORY_INFO) && (_POSIX_ADVISORY_INFO >= 200112L) )
  #undef  ARMA_HAVE_POSIX_MEMALIGN
  #define ARMA_HAVE_POSIX_MEMALIGN
#endif


#if defined(__APPLE__) || defined(__apple_build_version__)
  #undef  ARMA_BLAS_SDOT_BUG
  #define ARMA_BLAS_SDOT_BUG

  #undef  ARMA_HAVE_POSIX_MEMALIGN
  #undef  ARMA_USE_EXTERN_CXX11_RNG
  // TODO: thread local storage (TLS) (eg. "extern thread_local") appears currently broken on Mac OS X
#endif


#if defined(__MINGW32__) || defined(__CYGWIN__) || defined(_MSC_VER)
  #undef ARMA_HAVE_POSIX_MEMALIGN
#endif


#undef ARMA_FNSIG

#if defined (__GNUG__)
  #define ARMA_FNSIG  __PRETTY_FUNCTION__
#elif defined (_MSC_VER)
  #define ARMA_FNSIG  __FUNCSIG__
#elif defined(__INTEL_COMPILER)
  #define ARMA_FNSIG  __FUNCTION__
#elif defined(ARMA_USE_CXX11)
  #define ARMA_FNSIG  __func__
#else
  #define ARMA_FNSIG  "(unknown)"
#endif


#if (defined(__GNUG__) || defined(__GNUC__)) && (defined(__clang__) || defined(__INTEL_COMPILER) || defined(__NVCC__) || defined(__CUDACC__) || defined(__PGI) || defined(__PATHSCALE__) || defined(__ARMCC_VERSION) || defined(__IBMCPP__))
  #undef  ARMA_FAKE_GCC
  #define ARMA_FAKE_GCC
#endif


#if defined(__GNUG__) && !defined(ARMA_FAKE_GCC)

  #undef  ARMA_GCC_VERSION
  #define ARMA_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

  #if (ARMA_GCC_VERSION < 40400)
    #error "*** Need a newer compiler ***"
  #endif

  #if (ARMA_GCC_VERSION < 40600)
    #undef  ARMA_PRINT_CXX98_WARNING
    #define ARMA_PRINT_CXX98_WARNING
  #endif

  #if ( (ARMA_GCC_VERSION >= 40700) && (ARMA_GCC_VERSION <= 40701) )
    #error "gcc versions 4.7.0 and 4.7.1 are unsupported; use 4.7.2 or later"
    // due to http://gcc.gnu.org/bugzilla/show_bug.cgi?id=53549
  #endif

  #define ARMA_GOOD_COMPILER

  #undef  arma_hot
  #undef  arma_cold
  #undef  arma_aligned
  #undef  arma_align_mem
  #undef  arma_warn_unused
  #undef  arma_deprecated
  #undef  arma_malloc
  #undef  arma_inline
  #undef  arma_noinline

  #define arma_hot                __attribute__((__hot__))
  #define arma_cold               __attribute__((__cold__))
  #define arma_aligned            __attribute__((__aligned__))
  #define arma_align_mem          __attribute__((__aligned__(16)))
  #define arma_warn_unused        __attribute__((__warn_unused_result__))
  #define arma_deprecated         __attribute__((__deprecated__))
  #define arma_malloc             __attribute__((__malloc__))
  #define arma_inline      inline __attribute__((__always_inline__))
  #define arma_noinline           __attribute__((__noinline__))

  #undef  ARMA_HAVE_ALIGNED_ATTRIBUTE
  #define ARMA_HAVE_ALIGNED_ATTRIBUTE

  #if defined(ARMA_USE_CXX11)
    #if (ARMA_GCC_VERSION < 40800)
      #undef  ARMA_PRINT_CXX11_WARNING
      #define ARMA_PRINT_CXX11_WARNING
    #endif
  #endif

  #if !defined(ARMA_USE_CXX11) && !defined(__GXX_EXPERIMENTAL_CXX0X__) && (__cplusplus < 201103L)
    #if defined(_GLIBCXX_USE_C99_MATH_TR1) && defined(_GLIBCXX_USE_C99_COMPLEX_TR1)
      #define ARMA_HAVE_TR1
    #endif
  #endif

  #if (ARMA_GCC_VERSION >= 40700)
    #define ARMA_HAVE_GCC_ASSUME_ALIGNED
  #endif

  // gcc's vectoriser can handle elaborate loops
  #undef ARMA_SIMPLE_LOOPS

  #if defined(__OPTIMIZE_SIZE__)
    #define ARMA_SIMPLE_LOOPS
  #endif

  #if !defined(ARMA_USE_CXX11) && (defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200112L))
    #define ARMA_HAVE_SNPRINTF
    #define ARMA_HAVE_ISFINITE
    #define ARMA_HAVE_LOG1P
    #define ARMA_HAVE_ISINF
    #define ARMA_HAVE_ISNAN
  #endif

#endif


#if defined(__clang__) && (defined(__INTEL_COMPILER) || defined(__NVCC__) || defined(__CUDACC__) || defined(__PGI) || defined(__PATHSCALE__) || defined(__ARMCC_VERSION) || defined(__IBMCPP__))
  #undef  ARMA_FAKE_CLANG
  #define ARMA_FAKE_CLANG
#endif


#if defined(__clang__) && !defined(ARMA_FAKE_CLANG)

  #define ARMA_GOOD_COMPILER

  #if !defined(__has_attribute)
    #define __has_attribute(x) 0
  #endif

  #if __has_attribute(__aligned__)
    #undef  arma_aligned
    #undef  arma_align_mem

    #define arma_aligned   __attribute__((__aligned__))
    #define arma_align_mem __attribute__((__aligned__(16)))

    #undef  ARMA_HAVE_ALIGNED_ATTRIBUTE
    #define ARMA_HAVE_ALIGNED_ATTRIBUTE
  #endif

  #if __has_attribute(__warn_unused_result__)
    #undef  arma_warn_unused
    #define arma_warn_unused __attribute__((__warn_unused_result__))
  #endif

  #if __has_attribute(__deprecated__)
    #undef  arma_deprecated
    #define arma_deprecated __attribute__((__deprecated__))
  #endif

  #if __has_attribute(__malloc__)
    #undef  arma_malloc
    #define arma_malloc __attribute__((__malloc__))
  #endif

  #if __has_attribute(__always_inline__)
    #undef  arma_inline
    #define arma_inline inline __attribute__((__always_inline__))
  #endif

  #if __has_attribute(__noinline__)
    #undef  arma_noinline
    #define arma_noinline __attribute__((__noinline__))
  #endif

  #if __has_attribute(__hot__)
    #undef  arma_hot
    #define arma_hot __attribute__((__hot__))
  #endif

  #if __has_attribute(__cold__)
    #undef  arma_cold
    #define arma_cold __attribute__((__cold__))
  #endif

  #if defined(__has_builtin) && __has_builtin(__builtin_assume_aligned)
    #undef  ARMA_HAVE_GCC_ASSUME_ALIGNED
    #define ARMA_HAVE_GCC_ASSUME_ALIGNED
  #endif

  #if !defined(ARMA_USE_CXX11) && (defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200112L))
    #define ARMA_HAVE_SNPRINTF
    #define ARMA_HAVE_ISFINITE
    #define ARMA_HAVE_LOG1P
    #define ARMA_HAVE_ISINF
    #define ARMA_HAVE_ISNAN
  #endif

#endif


#if defined(__INTEL_COMPILER)

  #if (__INTEL_COMPILER == 9999)
    #error "*** Need a newer compiler ***"
  #endif

  #if (__INTEL_COMPILER < 1300)
    #error "*** Need a newer compiler ***"
  #endif

  #undef  ARMA_HAVE_GCC_ASSUME_ALIGNED
  #undef  ARMA_HAVE_ICC_ASSUME_ALIGNED
  #define ARMA_HAVE_ICC_ASSUME_ALIGNED

  #if defined(ARMA_USE_CXX11)
    #if (__INTEL_COMPILER < 1500)
      #undef  ARMA_PRINT_CXX11_WARNING
      #define ARMA_PRINT_CXX11_WARNING
    #endif
  #endif

#endif


#if defined(_MSC_VER)

  #if (_MSC_VER < 1700)
    #error "*** Need a newer compiler ***"
  #endif

  #if (_MSC_VER < 1800)
    #undef  ARMA_PRINT_CXX98_WARNING
    #define ARMA_PRINT_CXX98_WARNING
  #endif

  #if defined(ARMA_USE_CXX11)
    #if (_MSC_VER < 1900)
      #undef  ARMA_PRINT_CXX11_WARNING
      #define ARMA_PRINT_CXX11_WARNING
    #endif
  #endif

  #undef  arma_deprecated
  #define arma_deprecated __declspec(deprecated)
  // #undef  arma_inline
  // #define arma_inline inline __forceinline

  #pragma warning(push)

  #pragma warning(disable: 4127)  // conditional expression is constant
  #pragma warning(disable: 4180)  // qualifier has no meaning
  #pragma warning(disable: 4244)  // possible loss of data when converting types
  #pragma warning(disable: 4510)  // default constructor could not be generated
  #pragma warning(disable: 4511)  // copy constructor can't be generated
  #pragma warning(disable: 4512)  // assignment operator can't be generated
  #pragma warning(disable: 4513)  // destructor can't be generated
  #pragma warning(disable: 4514)  // unreferenced inline function has been removed
  #pragma warning(disable: 4522)  // multiple assignment operators specified
  #pragma warning(disable: 4623)  // default constructor can't be generated
  #pragma warning(disable: 4624)  // destructor can't be generated
  #pragma warning(disable: 4625)  // copy constructor can't be generated
  #pragma warning(disable: 4626)  // assignment operator can't be generated
  #pragma warning(disable: 4702)  // unreachable code
  #pragma warning(disable: 4710)  // function not inlined
  #pragma warning(disable: 4711)  // call was inlined
  #pragma warning(disable: 4714)  // __forceinline can't be inlined
  #pragma warning(disable: 4800)  // value forced to bool

  // #if (_MANAGED == 1) || (_M_CEE == 1)
  //
  //   // don't do any alignment when compiling in "managed code" mode
  //
  //   #undef  arma_aligned
  //   #define arma_aligned
  //
  //   #undef  arma_align_mem
  //   #define arma_align_mem
  //
  // #elif (_MSC_VER >= 1700)
  //
  //   #undef  arma_align_mem
  //   #define arma_align_mem __declspec(align(16))
  //
  //   #define ARMA_HAVE_ALIGNED_ATTRIBUTE
  //
  //   // disable warnings: "structure was padded due to __declspec(align(16))"
  //   #pragma warning(disable: 4324)
  //
  // #endif

#endif


#if defined(__SUNPRO_CC)

  // http://www.oracle.com/technetwork/server-storage/solarisstudio/training/index-jsp-141991.html
  // http://www.oracle.com/technetwork/server-storage/solarisstudio/documentation/cplusplus-faq-355066.html

  #if (__SUNPRO_CC < 0x5100)
    #error "*** Need a newer compiler ***"
  #endif

  #if defined(ARMA_USE_CXX11)
    #if (__SUNPRO_CC < 0x5130)
      #undef  ARMA_PRINT_CXX11_WARNING
      #define ARMA_PRINT_CXX11_WARNING
    #endif
  #endif

#endif


#if defined(ARMA_USE_CXX11) && defined(__CYGWIN__) && !defined(ARMA_DONT_PRINT_CXX11_WARNING)
  #pragma message ("WARNING: Cygwin may have incomplete support for C++11 features.")
#endif


#if defined(ARMA_USE_CXX11) && (__cplusplus < 201103L)
  #undef  ARMA_PRINT_CXX11_WARNING
  #define ARMA_PRINT_CXX11_WARNING
#endif


#if defined(ARMA_PRINT_CXX98_WARNING) && !defined(ARMA_DONT_PRINT_CXX98_WARNING)
  #pragma message ("WARNING: this compiler is OUTDATED and has INCOMPLETE support for the C++ standard;")
  #pragma message ("WARNING: if something breaks, you get to keep all the pieces.")
#endif


#if defined(ARMA_PRINT_CXX11_WARNING) && !defined(ARMA_DONT_PRINT_CXX11_WARNING)
  #pragma message ("WARNING: use of C++11 features has been enabled,")
  #pragma message ("WARNING: but this compiler has INCOMPLETE support for C++11;")
  #pragma message ("WARNING: if something breaks, you get to keep all the pieces.")
  #pragma message ("WARNING: to forcefully prevent Armadillo from using C++11 features,")
  #pragma message ("WARNING: #define ARMA_DONT_USE_CXX11 before #include <numerics/armadillo.hpp>")
#endif


#if ( defined(ARMA_USE_OPENMP) && (!defined(_OPENMP) || (defined(_OPENMP) && (_OPENMP < 200805))) )
  // we require OpenMP 3.0 to enable parallelisation of for loops with unsigned integers;
  // earlier versions of OpenMP can only handle signed integers
  #undef  ARMA_USE_OPENMP
  #undef  ARMA_PRINT_OPENMP_WARNING
  #define ARMA_PRINT_OPENMP_WARNING
#endif


#if ( (defined(_OPENMP) && (_OPENMP < 200805)) && !defined(ARMA_DONT_USE_OPENMP) )
  // if the compiler has an ancient version of OpenMP and use of OpenMP hasn't been explicitly disabled,
  // print a warning to ensure there is no confusion about OpenMP support
  #undef  ARMA_USE_OPENMP
  #undef  ARMA_PRINT_OPENMP_WARNING
  #define ARMA_PRINT_OPENMP_WARNING
#endif


#if defined(ARMA_PRINT_OPENMP_WARNING) && !defined(ARMA_DONT_PRINT_OPENMP_WARNING)
  #pragma message ("WARNING: use of OpenMP disabled; this compiler doesn't support OpenMP 3.0+")
#endif


#if defined(ARMA_USE_OPENMP) && !defined(ARMA_USE_CXX11)
  #if (defined(ARMA_GCC_VERSION) && (ARMA_GCC_VERSION >= 40803)) || (defined(__clang__) && !defined(ARMA_FAKE_CLANG))
    #undef  ARMA_PRINT_OPENMP_CXX11_WARNING
    #define ARMA_PRINT_OPENMP_CXX11_WARNING
  #endif
#endif


#if defined(ARMA_PRINT_OPENMP_CXX11_WARNING) && !defined(ARMA_DONT_PRINT_OPENMP_WARNING)
  #pragma message ("WARNING: support for OpenMP requires C++11/C++14; add -std=c++11 or -std=c++14 to compiler flags")
#endif



// cleanup

#undef ARMA_FAKE_GCC
#undef ARMA_FAKE_CLANG
#undef ARMA_GCC_VERSION
#undef ARMA_PRINT_CXX98_WARNING
#undef ARMA_PRINT_CXX11_WARNING
#undef ARMA_PRINT_OPENMP_WARNING
#undef ARMA_PRINT_OPENMP_CXX11_WARNING



#if defined(log2)
  #undef log2
  #pragma message ("WARNING: detected 'log2' macro and undefined it")
#endif



//
// whoever defined macros with the names "min" and "max" should be permanently removed from the gene pool

#if defined(min) || defined(max)
  #undef min
  #undef max
  #pragma message ("WARNING: detected 'min' and/or 'max' macros and undefined them;")
  #pragma message ("WARNING: you may wish to define NOMINMAX before including any windows header")
#endif
