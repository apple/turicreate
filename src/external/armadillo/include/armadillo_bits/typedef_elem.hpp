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


//! \addtogroup typedef_elem
//! @{


#if (defined(ARMA_U8_TYPE) && defined(ARMA_S8_TYPE))
    typedef ARMA_U8_TYPE     u8;
    typedef ARMA_S8_TYPE     s8;
#else
  #if   UCHAR_MAX >= 0xff
    typedef unsigned char    u8;
    typedef          char    s8;
  #elif defined(UINT8_MAX)
    typedef          uint8_t u8;
    typedef           int8_t s8;
  #else
    #error "don't know how to typedef 'u8' on this system"
  #endif
#endif

// NOTE:
// "char" is not guaranteed to be the same as "signed char"
// https://en.wikipedia.org/wiki/C_data_types


#if   USHRT_MAX >= 0xffff
  typedef unsigned short    u16;
  typedef          short    s16;
#elif defined(UINT16_MAX)
  typedef          uint16_t u16;
  typedef           int16_t s16;
#else
  #error "don't know how to typedef 'u16' on this system"
#endif


#if   UINT_MAX  >= 0xffffffff
  typedef unsigned int      u32;
  typedef          int      s32;
#elif defined(UINT32_MAX)
  typedef          uint32_t u32;
  typedef           int32_t s32;
#else
  #error "don't know how to typedef 'u32' on this system"
#endif


#if defined(ARMA_USE_U64S64)
  #if   ULLONG_MAX >= 0xffffffffffffffff
    typedef unsigned long long u64;
    typedef          long long s64;
  #elif ULONG_MAX  >= 0xffffffffffffffff
    typedef unsigned long      u64;
    typedef          long      s64;
    #define ARMA_U64_IS_LONG
  #elif defined(UINT64_MAX)
    typedef          uint64_t  u64;
    typedef           int64_t  s64;
  #else
      #error "don't know how to typedef 'u64' on this system; please disable ARMA_64BIT_WORD"
  #endif
#endif


#if !defined(ARMA_USE_U64S64) || (defined(ARMA_USE_U64S64) && !defined(ARMA_U64_IS_LONG))
  #define ARMA_ALLOW_LONG
#endif


typedef unsigned long ulng_t;
typedef          long slng_t;


#if defined(ARMA_64BIT_WORD)
  typedef u64 uword;
  typedef s64 sword;

  typedef u32 uhword;
  typedef s32 shword;

  #define ARMA_MAX_UWORD  0xffffffffffffffff
  #define ARMA_MAX_UHWORD 0xffffffff
#else
  typedef u32 uword;
  typedef s32 sword;

  typedef u16 uhword;
  typedef s16 shword;

  #define ARMA_MAX_UWORD  0xffffffff
  #define ARMA_MAX_UHWORD 0xffff
#endif


#if   defined(ARMA_BLAS_LONG_LONG)
  typedef long long blas_int;
  #define ARMA_MAX_BLAS_INT 0x7fffffffffffffffULL
#elif defined(ARMA_BLAS_LONG)
  typedef long      blas_int;
  #define ARMA_MAX_BLAS_INT 0x7fffffffffffffffUL
#else
  typedef int       blas_int;
  #define ARMA_MAX_BLAS_INT 0x7fffffffU
#endif


typedef std::complex<float>  cx_float;
typedef std::complex<double> cx_double;

typedef void* void_ptr;


//! @}
