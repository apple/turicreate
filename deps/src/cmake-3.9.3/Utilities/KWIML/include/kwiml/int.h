/*============================================================================
  Kitware Information Macro Library
  Copyright 2010-2016 Kitware, Inc.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

  * Neither the name of Kitware, Inc. nor the names of its contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================*/
/*
This header defines macros with information about sized integer types.
Only information that can be determined using the preprocessor at
compilation time is available.  No try-compile results may be added
here.  Instead we memorize results on platforms of interest.

An includer may optionally define the following macros to suppress errors:

Input:
  KWIML_INT_NO_VERIFY          = skip verification declarations
  KWIML_INT_NO_ERROR_INT64_T   = type 'KWIML_INT_int64_t' is optional (*)
  KWIML_INT_NO_ERROR_UINT64_T  = type 'KWIML_INT_uint64_t' is optional (*)
  KWIML_INT_NO_ERROR_INTPTR_T  = type 'KWIML_INT_intptr_t' is optional (*)
  KWIML_INT_NO_ERROR_UINTPTR_T = type 'KWIML_INT_uintptr_t' is optional (*)

An includer may optionally define the following macros to override defaults.
Either way, an includer may test these macros after inclusion:

  KWIML_INT_HAVE_STDINT_H   = include <stdint.h>
  KWIML_INT_NO_STDINT_H     = do not include <stdint.h>
  KWIML_INT_HAVE_INTTYPES_H = include <inttypes.h>
  KWIML_INT_NO_INTTYPES_H   = do not include <inttypes.h>

An includer may test the following macros after inclusion:

  KWIML_INT_VERSION         = interface version number # of this header

  KWIML_INT_HAVE_INT#_T     = type 'int#_t' is available
  KWIML_INT_HAVE_UINT#_T    = type 'uint#_t' is available
                                # = 8, 16, 32, 64, PTR

  KWIML_INT_int#_t          = signed integer type exactly # bits wide
  KWIML_INT_uint#_t         = unsigned integer type exactly # bits wide
                                # = 8, 16, 32, 64 (*), ptr (*)

  KWIML_INT_NO_INT64_T      = type 'KWIML_INT_int64_t' not available
  KWIML_INT_NO_UINT64_T     = type 'KWIML_INT_uint64_t' not available
  KWIML_INT_NO_INTPTR_T     = type 'KWIML_INT_intptr_t' not available
  KWIML_INT_NO_UINTPTR_T    = type 'KWIML_INT_uintptr_t' not available

  KWIML_INT_INT#_C(c)       = signed integer constant at least # bits wide
  KWIML_INT_UINT#_C(c)      = unsigned integer constant at least # bits wide
                                # = 8, 16, 32, 64 (*)

  KWIML_INT_<fmt>#          = print or scan format, <fmt> in table below
                                # = 8, 16, 32, 64, PTR (*)

             signed                unsigned
           ----------- ------------------------------
          |  decimal  | decimal  octal   hexadecimal |
    print | PRId PRIi |  PRIu    PRIo    PRIx  PRIX  |
     scan | SCNd SCNi |  SCNu    SCNo    SCNx        |
           ----------- ------------------------------

    The SCN*8 and SCN*64 format macros will not be defined on systems
    with scanf implementations known not to support them.

  KWIML_INT_BROKEN_<fmt># = macro <fmt># is incorrect if defined
    Some compilers define integer format macros incorrectly for their
    own formatted print/scan implementations.

  KWIML_INT_BROKEN_INT#_C  = macro INT#_C is incorrect if defined
  KWIML_INT_BROKEN_UINT#_C = macro UINT#_C is incorrect if defined
    Some compilers define integer constant macros incorrectly and
    cannot handle literals as large as the integer type or even
    produce bad preprocessor syntax.

  KWIML_INT_BROKEN_INT8_T   = type 'int8_t' is available but incorrect
    Some compilers have a flag to make 'char' (un)signed but do not account
    for it while defining int8_t in the non-default case.

  The broken cases do not affect correctness of the macros documented above.
*/

#include "abi.h"

#define KWIML_INT_private_VERSION 1

/* Guard definition of this version.  */
#ifndef KWIML_INT_detail_DEFINED_VERSION_1
# define KWIML_INT_detail_DEFINED_VERSION_1 1
# define KWIML_INT_private_DO_DEFINE
#endif

/* Guard verification of this version.  */
#if !defined(KWIML_INT_NO_VERIFY)
# ifndef KWIML_INT_detail_VERIFIED_VERSION_1
#  define KWIML_INT_detail_VERIFIED_VERSION_1
#  define KWIML_INT_private_DO_VERIFY
# endif
#endif

#ifdef KWIML_INT_private_DO_DEFINE
#undef KWIML_INT_private_DO_DEFINE

/* Define version as most recent of those included.  */
#if !defined(KWIML_INT_VERSION) || KWIML_INT_VERSION < KWIML_INT_private_VERSION
# undef KWIML_INT_VERSION
# define KWIML_INT_VERSION 1
#endif

/*--------------------------------------------------------------------------*/
#if defined(KWIML_INT_HAVE_STDINT_H) /* Already defined. */
#elif defined(KWIML_INT_NO_STDINT_H) /* Already defined. */
#elif defined(HAVE_STDINT_H) /* Optionally provided by includer.  */
# define KWIML_INT_HAVE_STDINT_H 1
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L /* C99 */
# define KWIML_INT_HAVE_STDINT_H 1
#elif defined(_MSC_VER) /* MSVC */
# if _MSC_VER >= 1600
#  define KWIML_INT_HAVE_STDINT_H 1
# else
#  define KWIML_INT_NO_STDINT_H 1
# endif
#elif defined(__BORLANDC__) /* Borland */
# if __BORLANDC__ >= 0x560
#  define KWIML_INT_HAVE_STDINT_H 1
# else
#  define KWIML_INT_NO_STDINT_H 1
# endif
#elif defined(__WATCOMC__) /* Watcom */
# define KWIML_INT_NO_STDINT_H 1
#endif

/*--------------------------------------------------------------------------*/
#if defined(KWIML_INT_HAVE_INTTYPES_H) /* Already defined. */
#elif defined(KWIML_INT_NO_INTTYPES_H) /* Already defined. */
#elif defined(HAVE_INTTYPES_H) /* Optionally provided by includer.  */
# define KWIML_INT_HAVE_INTTYPES_H 1
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L /* C99 */
# define KWIML_INT_HAVE_INTTYPES_H 1
#elif defined(_MSC_VER) /* MSVC */
# define KWIML_INT_NO_INTTYPES_H 1
#elif defined(__BORLANDC__) /* Borland */
# define KWIML_INT_NO_INTTYPES_H 1
#elif defined(__WATCOMC__) /* Watcom */
# define KWIML_INT_NO_INTTYPES_H 1
#else /* Assume it exists.  */
# define KWIML_INT_HAVE_INTTYPES_H 1
#endif

/*--------------------------------------------------------------------------*/
#if defined(KWIML_INT_HAVE_STDINT_H) && defined(KWIML_INT_NO_STDINT_H)
# error "Both KWIML_INT_HAVE_STDINT_H and KWIML_INT_NO_STDINT_H defined!"
#endif
#if defined(KWIML_INT_HAVE_INTTYPES_H) && defined(KWIML_INT_NO_INTTYPES_H)
# error "Both KWIML_INT_HAVE_INTTYPES_H and KWIML_INT_NO_INTTYPES_H defined!"
#endif

#if defined(KWIML_INT_HAVE_STDINT_H)
# ifndef KWIML_INT_detail_INCLUDED_STDINT_H
#  define KWIML_INT_detail_INCLUDED_STDINT_H
#  include <stdint.h>
# endif
#endif
#if defined(KWIML_INT_HAVE_INTTYPES_H)
# ifndef KWIML_INT_detail_INCLUDED_INTTYPES_H
#  define KWIML_INT_detail_INCLUDED_INTTYPES_H
#  if defined(__cplusplus) && !defined(__STDC_FORMAT_MACROS)
#   define __STDC_FORMAT_MACROS
#  endif
#  include <inttypes.h>
# endif
#endif

#if defined(KWIML_INT_HAVE_STDINT_H) || defined(KWIML_INT_HAVE_INTTYPES_H)
#define KWIML_INT_HAVE_INT8_T 1
#define KWIML_INT_HAVE_UINT8_T 1
#define KWIML_INT_HAVE_INT16_T 1
#define KWIML_INT_HAVE_UINT16_T 1
#define KWIML_INT_HAVE_INT32_T 1
#define KWIML_INT_HAVE_UINT32_T 1
#define KWIML_INT_HAVE_INT64_T 1
#define KWIML_INT_HAVE_UINT64_T 1
#define KWIML_INT_HAVE_INTPTR_T 1
#define KWIML_INT_HAVE_UINTPTR_T 1
# if defined(__cplusplus)
#  define KWIML_INT_detail_GLOBAL_NS(T) ::T
# else
#  define KWIML_INT_detail_GLOBAL_NS(T) T
# endif
#endif

#if defined(_AIX43) && !defined(_AIX50) && !defined(_AIX51)
  /* AIX 4.3 defines these incorrectly with % and no quotes. */
# define KWIML_INT_BROKEN_PRId8 1
# define KWIML_INT_BROKEN_SCNd8 1
# define KWIML_INT_BROKEN_PRIi8 1
# define KWIML_INT_BROKEN_SCNi8 1
# define KWIML_INT_BROKEN_PRIo8 1
# define KWIML_INT_BROKEN_SCNo8 1
# define KWIML_INT_BROKEN_PRIu8 1
# define KWIML_INT_BROKEN_SCNu8 1
# define KWIML_INT_BROKEN_PRIx8 1
# define KWIML_INT_BROKEN_SCNx8 1
# define KWIML_INT_BROKEN_PRIX8 1
# define KWIML_INT_BROKEN_PRId16 1
# define KWIML_INT_BROKEN_SCNd16 1
# define KWIML_INT_BROKEN_PRIi16 1
# define KWIML_INT_BROKEN_SCNi16 1
# define KWIML_INT_BROKEN_PRIo16 1
# define KWIML_INT_BROKEN_SCNo16 1
# define KWIML_INT_BROKEN_PRIu16 1
# define KWIML_INT_BROKEN_SCNu16 1
# define KWIML_INT_BROKEN_PRIx16 1
# define KWIML_INT_BROKEN_SCNx16 1
# define KWIML_INT_BROKEN_PRIX16 1
# define KWIML_INT_BROKEN_PRId32 1
# define KWIML_INT_BROKEN_SCNd32 1
# define KWIML_INT_BROKEN_PRIi32 1
# define KWIML_INT_BROKEN_SCNi32 1
# define KWIML_INT_BROKEN_PRIo32 1
# define KWIML_INT_BROKEN_SCNo32 1
# define KWIML_INT_BROKEN_PRIu32 1
# define KWIML_INT_BROKEN_SCNu32 1
# define KWIML_INT_BROKEN_PRIx32 1
# define KWIML_INT_BROKEN_SCNx32 1
# define KWIML_INT_BROKEN_PRIX32 1
# define KWIML_INT_BROKEN_PRId64 1
# define KWIML_INT_BROKEN_SCNd64 1
# define KWIML_INT_BROKEN_PRIi64 1
# define KWIML_INT_BROKEN_SCNi64 1
# define KWIML_INT_BROKEN_PRIo64 1
# define KWIML_INT_BROKEN_SCNo64 1
# define KWIML_INT_BROKEN_PRIu64 1
# define KWIML_INT_BROKEN_SCNu64 1
# define KWIML_INT_BROKEN_PRIx64 1
# define KWIML_INT_BROKEN_SCNx64 1
# define KWIML_INT_BROKEN_PRIX64 1
# define KWIML_INT_BROKEN_PRIdPTR 1
# define KWIML_INT_BROKEN_SCNdPTR 1
# define KWIML_INT_BROKEN_PRIiPTR 1
# define KWIML_INT_BROKEN_SCNiPTR 1
# define KWIML_INT_BROKEN_PRIoPTR 1
# define KWIML_INT_BROKEN_SCNoPTR 1
# define KWIML_INT_BROKEN_PRIuPTR 1
# define KWIML_INT_BROKEN_SCNuPTR 1
# define KWIML_INT_BROKEN_PRIxPTR 1
# define KWIML_INT_BROKEN_SCNxPTR 1
# define KWIML_INT_BROKEN_PRIXPTR 1
#endif

#if (defined(__SUNPRO_C)||defined(__SUNPRO_CC)) && defined(_CHAR_IS_UNSIGNED)
# define KWIML_INT_BROKEN_INT8_T 1 /* system type defined incorrectly */
#elif defined(__BORLANDC__) && defined(_CHAR_UNSIGNED)
# define KWIML_INT_BROKEN_INT8_T 1 /* system type defined incorrectly */
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_INT_int8_t)
# if defined(KWIML_INT_HAVE_INT8_T) && !defined(KWIML_INT_BROKEN_INT8_T)
#  define KWIML_INT_int8_t KWIML_INT_detail_GLOBAL_NS(int8_t)
# else
#  define KWIML_INT_int8_t signed char
# endif
#endif
#if !defined(KWIML_INT_uint8_t)
# if defined(KWIML_INT_HAVE_UINT8_T)
#  define KWIML_INT_uint8_t KWIML_INT_detail_GLOBAL_NS(uint8_t)
# else
#  define KWIML_INT_uint8_t unsigned char
# endif
#endif

#if defined(__INTEL_COMPILER)
# if defined(_WIN32)
#  define KWIML_INT_private_NO_SCN8
# endif
#elif defined(__SUNPRO_C) || defined(__SUNPRO_CC)
# define KWIML_INT_private_NO_SCN8
#elif defined(__BORLANDC__)
# define KWIML_INT_private_NO_SCN8
# define KWIML_INT_private_NO_SCN64
#elif defined(_MSC_VER)
# define KWIML_INT_private_NO_SCN8
#elif defined(__WATCOMC__)
# define KWIML_INT_private_NO_SCN8
# elif defined(__hpux) /* HP runtime lacks support (any compiler) */
# define KWIML_INT_private_NO_SCN8
#endif

/* 8-bit d, i */
#if !defined(KWIML_INT_PRId8)
# if defined(KWIML_INT_HAVE_INT8_T) && defined(PRId8) \
   && !defined(KWIML_INT_BROKEN_PRId8)
#  define KWIML_INT_PRId8 PRId8
# else
#  define KWIML_INT_PRId8   "d"
# endif
#endif
#if !defined(KWIML_INT_SCNd8)
# if defined(KWIML_INT_HAVE_INT8_T) && defined(SCNd8) \
   && !defined(KWIML_INT_BROKEN_SCNd8)
#  define KWIML_INT_SCNd8 SCNd8
# elif !defined(KWIML_INT_private_NO_SCN8)
#  define KWIML_INT_SCNd8 "hhd"
# endif
#endif
#if !defined(KWIML_INT_PRIi8)
# if defined(KWIML_INT_HAVE_INT8_T) && defined(PRIi8) \
   && !defined(KWIML_INT_BROKEN_PRIi8)
#  define KWIML_INT_PRIi8 PRIi8
# else
#  define KWIML_INT_PRIi8   "i"
# endif
#endif
#if !defined(KWIML_INT_SCNi8)
# if defined(KWIML_INT_HAVE_INT8_T) && defined(SCNi8) \
   && !defined(KWIML_INT_BROKEN_SCNi8)
#  define KWIML_INT_SCNi8 SCNi8
# elif !defined(KWIML_INT_private_NO_SCN8)
#  define KWIML_INT_SCNi8 "hhi"
# endif
#endif

/* 8-bit o, u, x, X */
#if !defined(KWIML_INT_PRIo8)
# if defined(KWIML_INT_HAVE_UINT8_T) && defined(PRIo8) \
   && !defined(KWIML_INT_BROKEN_PRIo8)
#  define KWIML_INT_PRIo8 PRIo8
# else
#  define KWIML_INT_PRIo8   "o"
# endif
#endif
#if !defined(KWIML_INT_SCNo8)
# if defined(KWIML_INT_HAVE_UINT8_T) && defined(SCNo8) \
   && !defined(KWIML_INT_BROKEN_SCNo8)
#  define KWIML_INT_SCNo8 SCNo8
# elif !defined(KWIML_INT_private_NO_SCN8)
#  define KWIML_INT_SCNo8 "hho"
# endif
#endif
#if !defined(KWIML_INT_PRIu8)
# if defined(KWIML_INT_HAVE_UINT8_T) && defined(PRIu8) \
   && !defined(KWIML_INT_BROKEN_PRIu8)
#  define KWIML_INT_PRIu8 PRIu8
# else
#  define KWIML_INT_PRIu8   "u"
# endif
#endif
#if !defined(KWIML_INT_SCNu8)
# if defined(KWIML_INT_HAVE_UINT8_T) && defined(SCNu8) \
   && !defined(KWIML_INT_BROKEN_SCNu8)
#  define KWIML_INT_SCNu8 SCNu8
# elif !defined(KWIML_INT_private_NO_SCN8)
#  define KWIML_INT_SCNu8 "hhu"
# endif
#endif
#if !defined(KWIML_INT_PRIx8)
# if defined(KWIML_INT_HAVE_UINT8_T) && defined(PRIx8) \
   && !defined(KWIML_INT_BROKEN_PRIx8)
#  define KWIML_INT_PRIx8 PRIx8
# else
#  define KWIML_INT_PRIx8   "x"
# endif
#endif
#if !defined(KWIML_INT_SCNx8)
# if defined(KWIML_INT_HAVE_UINT8_T) && defined(SCNx8) \
   && !defined(KWIML_INT_BROKEN_SCNx8)
#  define KWIML_INT_SCNx8 SCNx8
# elif !defined(KWIML_INT_private_NO_SCN8)
#  define KWIML_INT_SCNx8 "hhx"
# endif
#endif
#if !defined(KWIML_INT_PRIX8)
# if defined(KWIML_INT_HAVE_UINT8_T) && defined(PRIX8) \
   && !defined(KWIML_INT_BROKEN_PRIX8)
#  define KWIML_INT_PRIX8 PRIX8
# else
#  define KWIML_INT_PRIX8   "X"
# endif
#endif

/* 8-bit constants */
#if !defined(KWIML_INT_INT8_C)
# if defined(INT8_C) && !defined(KWIML_INT_BROKEN_INT8_C)
#  define KWIML_INT_INT8_C(c) INT8_C(c)
# else
#  define KWIML_INT_INT8_C(c) c
# endif
#endif
#if !defined(KWIML_INT_UINT8_C)
# if defined(UINT8_C) && !defined(KWIML_INT_BROKEN_UINT8_C)
#  define KWIML_INT_UINT8_C(c) UINT8_C(c)
# else
#  define KWIML_INT_UINT8_C(c) c ## u
# endif
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_INT_int16_t)
# if defined(KWIML_INT_HAVE_INT16_T)
#  define KWIML_INT_int16_t KWIML_INT_detail_GLOBAL_NS(int16_t)
# else
#  define KWIML_INT_int16_t signed short
# endif
#endif
#if !defined(KWIML_INT_uint16_t)
# if defined(KWIML_INT_HAVE_UINT16_T)
#  define KWIML_INT_uint16_t KWIML_INT_detail_GLOBAL_NS(uint16_t)
# else
#  define KWIML_INT_uint16_t unsigned short
# endif
#endif

/* 16-bit d, i */
#if !defined(KWIML_INT_PRId16)
# if defined(KWIML_INT_HAVE_INT16_T) && defined(PRId16) \
   && !defined(KWIML_INT_BROKEN_PRId16)
#  define KWIML_INT_PRId16 PRId16
# else
#  define KWIML_INT_PRId16  "d"
# endif
#endif
#if !defined(KWIML_INT_SCNd16)
# if defined(KWIML_INT_HAVE_INT16_T) && defined(SCNd16) \
   && !defined(KWIML_INT_BROKEN_SCNd16)
#  define KWIML_INT_SCNd16 SCNd16
# else
#  define KWIML_INT_SCNd16 "hd"
# endif
#endif
#if !defined(KWIML_INT_PRIi16)
# if defined(KWIML_INT_HAVE_INT16_T) && defined(PRIi16) \
   && !defined(KWIML_INT_BROKEN_PRIi16)
#  define KWIML_INT_PRIi16 PRIi16
# else
#  define KWIML_INT_PRIi16  "i"
# endif
#endif
#if !defined(KWIML_INT_SCNi16)
# if defined(KWIML_INT_HAVE_INT16_T) && defined(SCNi16) \
   && !defined(KWIML_INT_BROKEN_SCNi16)
#  define KWIML_INT_SCNi16 SCNi16
# else
#  define KWIML_INT_SCNi16 "hi"
# endif
#endif

/* 16-bit o, u, x, X */
#if !defined(KWIML_INT_PRIo16)
# if defined(KWIML_INT_HAVE_UINT16_T) && defined(PRIo16) \
   && !defined(KWIML_INT_BROKEN_PRIo16)
#  define KWIML_INT_PRIo16 PRIo16
# else
#  define KWIML_INT_PRIo16  "o"
# endif
#endif
#if !defined(KWIML_INT_SCNo16)
# if defined(KWIML_INT_HAVE_UINT16_T) && defined(SCNo16) \
   && !defined(KWIML_INT_BROKEN_SCNo16)
#  define KWIML_INT_SCNo16 SCNo16
# else
#  define KWIML_INT_SCNo16 "ho"
# endif
#endif
#if !defined(KWIML_INT_PRIu16)
# if defined(KWIML_INT_HAVE_UINT16_T) && defined(PRIu16) \
   && !defined(KWIML_INT_BROKEN_PRIu16)
#  define KWIML_INT_PRIu16 PRIu16
# else
#  define KWIML_INT_PRIu16  "u"
# endif
#endif
#if !defined(KWIML_INT_SCNu16)
# if defined(KWIML_INT_HAVE_UINT16_T) && defined(SCNu16) \
   && !defined(KWIML_INT_BROKEN_SCNu16)
#  define KWIML_INT_SCNu16 SCNu16
# else
#  define KWIML_INT_SCNu16 "hu"
# endif
#endif
#if !defined(KWIML_INT_PRIx16)
# if defined(KWIML_INT_HAVE_UINT16_T) && defined(PRIx16) \
   && !defined(KWIML_INT_BROKEN_PRIx16)
#  define KWIML_INT_PRIx16 PRIx16
# else
#  define KWIML_INT_PRIx16  "x"
# endif
#endif
#if !defined(KWIML_INT_SCNx16)
# if defined(KWIML_INT_HAVE_UINT16_T) && defined(SCNx16) \
   && !defined(KWIML_INT_BROKEN_SCNx16)
#  define KWIML_INT_SCNx16 SCNx16
# else
#  define KWIML_INT_SCNx16 "hx"
# endif
#endif
#if !defined(KWIML_INT_PRIX16)
# if defined(KWIML_INT_HAVE_UINT16_T) && defined(PRIX16) \
   && !defined(KWIML_INT_BROKEN_PRIX16)
#  define KWIML_INT_PRIX16 PRIX16
# else
#  define KWIML_INT_PRIX16  "X"
# endif
#endif

/* 16-bit constants */
#if !defined(KWIML_INT_INT16_C)
# if defined(INT16_C) && !defined(KWIML_INT_BROKEN_INT16_C)
#  define KWIML_INT_INT16_C(c) INT16_C(c)
# else
#  define KWIML_INT_INT16_C(c) c
# endif
#endif
#if !defined(KWIML_INT_UINT16_C)
# if defined(UINT16_C) && !defined(KWIML_INT_BROKEN_UINT16_C)
#  define KWIML_INT_UINT16_C(c) UINT16_C(c)
# else
#  define KWIML_INT_UINT16_C(c) c ## u
# endif
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_INT_int32_t)
# if defined(KWIML_INT_HAVE_INT32_T)
#  define KWIML_INT_int32_t KWIML_INT_detail_GLOBAL_NS(int32_t)
# else
#  define KWIML_INT_int32_t signed int
# endif
#endif
#if !defined(KWIML_INT_uint32_t)
# if defined(KWIML_INT_HAVE_UINT32_T)
#  define KWIML_INT_uint32_t KWIML_INT_detail_GLOBAL_NS(uint32_t)
# else
#  define KWIML_INT_uint32_t unsigned int
# endif
#endif

/* 32-bit d, i */
#if !defined(KWIML_INT_PRId32)
# if defined(KWIML_INT_HAVE_INT32_T) && defined(PRId32) \
   && !defined(KWIML_INT_BROKEN_PRId32)
#  define KWIML_INT_PRId32 PRId32
# else
#  define KWIML_INT_PRId32 "d"
# endif
#endif
#if !defined(KWIML_INT_SCNd32)
# if defined(KWIML_INT_HAVE_INT32_T) && defined(SCNd32) \
   && !defined(KWIML_INT_BROKEN_SCNd32)
#  define KWIML_INT_SCNd32 SCNd32
# else
#  define KWIML_INT_SCNd32 "d"
# endif
#endif
#if !defined(KWIML_INT_PRIi32)
# if defined(KWIML_INT_HAVE_INT32_T) && defined(PRIi32) \
   && !defined(KWIML_INT_BROKEN_PRIi32)
#  define KWIML_INT_PRIi32 PRIi32
# else
#  define KWIML_INT_PRIi32 "i"
# endif
#endif
#if !defined(KWIML_INT_SCNi32)
# if defined(KWIML_INT_HAVE_INT32_T) && defined(SCNi32) \
   && !defined(KWIML_INT_BROKEN_SCNi32)
#  define KWIML_INT_SCNi32 SCNi32
# else
#  define KWIML_INT_SCNi32 "i"
# endif
#endif

/* 32-bit o, u, x, X */
#if !defined(KWIML_INT_PRIo32)
# if defined(KWIML_INT_HAVE_UINT32_T) && defined(PRIo32) \
   && !defined(KWIML_INT_BROKEN_PRIo32)
#  define KWIML_INT_PRIo32 PRIo32
# else
#  define KWIML_INT_PRIo32 "o"
# endif
#endif
#if !defined(KWIML_INT_SCNo32)
# if defined(KWIML_INT_HAVE_UINT32_T) && defined(SCNo32) \
   && !defined(KWIML_INT_BROKEN_SCNo32)
#  define KWIML_INT_SCNo32 SCNo32
# else
#  define KWIML_INT_SCNo32 "o"
# endif
#endif
#if !defined(KWIML_INT_PRIu32)
# if defined(KWIML_INT_HAVE_UINT32_T) && defined(PRIu32) \
   && !defined(KWIML_INT_BROKEN_PRIu32)
#  define KWIML_INT_PRIu32 PRIu32
# else
#  define KWIML_INT_PRIu32 "u"
# endif
#endif
#if !defined(KWIML_INT_SCNu32)
# if defined(KWIML_INT_HAVE_UINT32_T) && defined(SCNu32) \
   && !defined(KWIML_INT_BROKEN_SCNu32)
#  define KWIML_INT_SCNu32 SCNu32
# else
#  define KWIML_INT_SCNu32 "u"
# endif
#endif
#if !defined(KWIML_INT_PRIx32)
# if defined(KWIML_INT_HAVE_UINT32_T) && defined(PRIx32) \
   && !defined(KWIML_INT_BROKEN_PRIx32)
#  define KWIML_INT_PRIx32 PRIx32
# else
#  define KWIML_INT_PRIx32 "x"
# endif
#endif
#if !defined(KWIML_INT_SCNx32)
# if defined(KWIML_INT_HAVE_UINT32_T) && defined(SCNx32) \
   && !defined(KWIML_INT_BROKEN_SCNx32)
#  define KWIML_INT_SCNx32 SCNx32
# else
#  define KWIML_INT_SCNx32 "x"
# endif
#endif
#if !defined(KWIML_INT_PRIX32)
# if defined(KWIML_INT_HAVE_UINT32_T) && defined(PRIX32) \
   && !defined(KWIML_INT_BROKEN_PRIX32)
#  define KWIML_INT_PRIX32 PRIX32
# else
#  define KWIML_INT_PRIX32 "X"
# endif
#endif

#if defined(__hpux) && defined(__GNUC__) && !defined(__LP64__) \
 && defined(__CONCAT__) && defined(__CONCAT_U__)
  /* Some HPs define UINT32_C incorrectly and break GNU.  */
# define KWIML_INT_BROKEN_UINT32_C 1
#endif

/* 32-bit constants */
#if !defined(KWIML_INT_INT32_C)
# if defined(INT32_C) && !defined(KWIML_INT_BROKEN_INT32_C)
#  define KWIML_INT_INT32_C(c) INT32_C(c)
# else
#  define KWIML_INT_INT32_C(c) c
# endif
#endif
#if !defined(KWIML_INT_UINT32_C)
# if defined(UINT32_C) && !defined(KWIML_INT_BROKEN_UINT32_C)
#  define KWIML_INT_UINT32_C(c) UINT32_C(c)
# else
#  define KWIML_INT_UINT32_C(c) c ## u
# endif
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_INT_int64_t) && !defined(KWIML_INT_NO_INT64_T)
# if defined(KWIML_INT_HAVE_INT64_T)
#  define KWIML_INT_int64_t KWIML_INT_detail_GLOBAL_NS(int64_t)
# elif KWIML_ABI_SIZEOF_LONG == 8
#  define KWIML_INT_int64_t signed long
# elif defined(KWIML_ABI_SIZEOF_LONG_LONG) && KWIML_ABI_SIZEOF_LONG_LONG == 8
#  define KWIML_INT_int64_t signed long long
# elif defined(KWIML_ABI_SIZEOF___INT64)
#  define KWIML_INT_int64_t signed __int64
# elif defined(KWIML_INT_NO_ERROR_INT64_T)
#  define KWIML_INT_NO_INT64_T
# else
#  error "No type known for 'int64_t'."
# endif
#endif
#if !defined(KWIML_INT_uint64_t) && !defined(KWIML_INT_NO_UINT64_T)
# if defined(KWIML_INT_HAVE_UINT64_T)
#  define KWIML_INT_uint64_t KWIML_INT_detail_GLOBAL_NS(uint64_t)
# elif KWIML_ABI_SIZEOF_LONG == 8
#  define KWIML_INT_uint64_t unsigned long
# elif defined(KWIML_ABI_SIZEOF_LONG_LONG) && KWIML_ABI_SIZEOF_LONG_LONG == 8
#  define KWIML_INT_uint64_t unsigned long long
# elif defined(KWIML_ABI_SIZEOF___INT64)
#  define KWIML_INT_uint64_t unsigned __int64
# elif defined(KWIML_INT_NO_ERROR_UINT64_T)
#  define KWIML_INT_NO_UINT64_T
# else
#  error "No type known for 'uint64_t'."
# endif
#endif

#if defined(__INTEL_COMPILER)
#elif defined(__BORLANDC__)
# define KWIML_INT_private_NO_FMTLL /* type 'long long' but not 'll' format */
# define KWIML_INT_BROKEN_INT64_C 1  /* system macro defined incorrectly */
# define KWIML_INT_BROKEN_UINT64_C 1 /* system macro defined incorrectly */
#elif defined(_MSC_VER) && _MSC_VER < 1400
# define KWIML_INT_private_NO_FMTLL /* type 'long long' but not 'll' format */
#endif

#if !defined(KWIML_INT_detail_FMT64)
# if KWIML_ABI_SIZEOF_LONG == 8
#  define KWIML_INT_detail_FMT64 "l"
# elif defined(KWIML_ABI_SIZEOF_LONG_LONG) && KWIML_ABI_SIZEOF_LONG_LONG == 8
#  if !defined(KWIML_INT_private_NO_FMTLL)
#   define KWIML_INT_detail_FMT64 "ll"
#  else
#   define KWIML_INT_detail_FMT64 "I64"
#  endif
# elif defined(KWIML_ABI_SIZEOF___INT64)
#  if defined(__BORLANDC__)
#   define KWIML_INT_detail_FMT64 "L"
#  else
#   define KWIML_INT_detail_FMT64 "I64"
#  endif
# endif
#endif

#undef KWIML_INT_private_NO_FMTLL

/* 64-bit d, i */
#if !defined(KWIML_INT_PRId64)
# if defined(KWIML_INT_HAVE_INT64_T) && defined(PRId64) \
   && !defined(KWIML_INT_BROKEN_PRId64)
#  define KWIML_INT_PRId64 PRId64
# elif defined(KWIML_INT_detail_FMT64)
#  define KWIML_INT_PRId64 KWIML_INT_detail_FMT64 "d"
# endif
#endif
#if !defined(KWIML_INT_SCNd64)
# if defined(KWIML_INT_HAVE_INT64_T) && defined(SCNd64) \
   && !defined(KWIML_INT_BROKEN_SCNd64)
#  define KWIML_INT_SCNd64 SCNd64
# elif defined(KWIML_INT_detail_FMT64) && !defined(KWIML_INT_private_NO_SCN64)
#  define KWIML_INT_SCNd64 KWIML_INT_detail_FMT64 "d"
# endif
#endif
#if !defined(KWIML_INT_PRIi64)
# if defined(KWIML_INT_HAVE_INT64_T) && defined(PRIi64) \
   && !defined(KWIML_INT_BROKEN_PRIi64)
#  define KWIML_INT_PRIi64 PRIi64
# elif defined(KWIML_INT_detail_FMT64)
#  define KWIML_INT_PRIi64 KWIML_INT_detail_FMT64 "d"
# endif
#endif
#if !defined(KWIML_INT_SCNi64)
# if defined(KWIML_INT_HAVE_INT64_T) && defined(SCNi64) \
   && !defined(KWIML_INT_BROKEN_SCNi64)
#  define KWIML_INT_SCNi64 SCNi64
# elif defined(KWIML_INT_detail_FMT64) && !defined(KWIML_INT_private_NO_SCN64)
#  define KWIML_INT_SCNi64 KWIML_INT_detail_FMT64 "d"
# endif
#endif

/* 64-bit o, u, x, X */
#if !defined(KWIML_INT_PRIo64)
# if defined(KWIML_INT_HAVE_UINT64_T) && defined(PRIo64) \
   && !defined(KWIML_INT_BROKEN_PRIo64)
#  define KWIML_INT_PRIo64 PRIo64
# elif defined(KWIML_INT_detail_FMT64)
#  define KWIML_INT_PRIo64 KWIML_INT_detail_FMT64 "o"
# endif
#endif
#if !defined(KWIML_INT_SCNo64)
# if defined(KWIML_INT_HAVE_UINT64_T) && defined(SCNo64) \
   && !defined(KWIML_INT_BROKEN_SCNo64)
#  define KWIML_INT_SCNo64 SCNo64
# elif defined(KWIML_INT_detail_FMT64) && !defined(KWIML_INT_private_NO_SCN64)
#  define KWIML_INT_SCNo64 KWIML_INT_detail_FMT64 "o"
# endif
#endif
#if !defined(KWIML_INT_PRIu64)
# if defined(KWIML_INT_HAVE_UINT64_T) && defined(PRIu64) \
   && !defined(KWIML_INT_BROKEN_PRIu64)
#  define KWIML_INT_PRIu64 PRIu64
# elif defined(KWIML_INT_detail_FMT64)
#  define KWIML_INT_PRIu64 KWIML_INT_detail_FMT64 "u"
# endif
#endif
#if !defined(KWIML_INT_SCNu64)
# if defined(KWIML_INT_HAVE_UINT64_T) && defined(SCNu64) \
   && !defined(KWIML_INT_BROKEN_SCNu64)
#  define KWIML_INT_SCNu64 SCNu64
# elif defined(KWIML_INT_detail_FMT64) && !defined(KWIML_INT_private_NO_SCN64)
#  define KWIML_INT_SCNu64 KWIML_INT_detail_FMT64 "u"
# endif
#endif
#if !defined(KWIML_INT_PRIx64)
# if defined(KWIML_INT_HAVE_UINT64_T) && defined(PRIx64) \
   && !defined(KWIML_INT_BROKEN_PRIx64)
#  define KWIML_INT_PRIx64 PRIx64
# elif defined(KWIML_INT_detail_FMT64)
#  define KWIML_INT_PRIx64 KWIML_INT_detail_FMT64 "x"
# endif
#endif
#if !defined(KWIML_INT_SCNx64)
# if defined(KWIML_INT_HAVE_UINT64_T) && defined(SCNx64) \
   && !defined(KWIML_INT_BROKEN_SCNx64)
#  define KWIML_INT_SCNx64 SCNx64
# elif defined(KWIML_INT_detail_FMT64) && !defined(KWIML_INT_private_NO_SCN64)
#  define KWIML_INT_SCNx64 KWIML_INT_detail_FMT64 "x"
# endif
#endif
#if !defined(KWIML_INT_PRIX64)
# if defined(KWIML_INT_HAVE_UINT64_T) && defined(PRIX64) \
   && !defined(KWIML_INT_BROKEN_PRIX64)
#  define KWIML_INT_PRIX64 PRIX64
# elif defined(KWIML_INT_detail_FMT64)
#  define KWIML_INT_PRIX64 KWIML_INT_detail_FMT64 "X"
# endif
#endif

/* 64-bit constants */
#if !defined(KWIML_INT_INT64_C)
# if defined(KWIML_INT_HAVE_INT64_T) && defined(INT64_C) \
   && !defined(KWIML_INT_BROKEN_INT64_C)
#  define KWIML_INT_INT64_C(c) INT64_C(c)
# elif KWIML_ABI_SIZEOF_LONG == 8
#  define KWIML_INT_INT64_C(c) c ## l
# elif defined(KWIML_ABI_SIZEOF_LONG_LONG) && KWIML_ABI_SIZEOF_LONG_LONG == 8
#  define KWIML_INT_INT64_C(c) c ## ll
# elif defined(KWIML_ABI_SIZEOF___INT64)
#  define KWIML_INT_INT64_C(c) c ## i64
# endif
#endif
#if !defined(KWIML_INT_UINT64_C)
# if defined(KWIML_INT_HAVE_UINT64_T) && defined(UINT64_C) \
   && !defined(KWIML_INT_BROKEN_UINT64_C)
#  define KWIML_INT_UINT64_C(c) UINT64_C(c)
# elif KWIML_ABI_SIZEOF_LONG == 8
#  define KWIML_INT_UINT64_C(c) c ## ul
# elif defined(KWIML_ABI_SIZEOF_LONG_LONG) && KWIML_ABI_SIZEOF_LONG_LONG == 8
#  define KWIML_INT_UINT64_C(c) c ## ull
# elif defined(KWIML_ABI_SIZEOF___INT64)
#  define KWIML_INT_UINT64_C(c) c ## ui64
# endif
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_INT_intptr_t) && !defined(KWIML_INT_NO_INTPTR_T)
# if defined(KWIML_INT_HAVE_INTPTR_T)
#  define KWIML_INT_intptr_t KWIML_INT_detail_GLOBAL_NS(intptr_t)
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_intptr_t KWIML_INT_int32_t
# elif !defined(KWIML_INT_NO_INT64_T)
#  define KWIML_INT_intptr_t KWIML_INT_int64_t
# elif defined(KWIML_INT_NO_ERROR_INTPTR_T)
#  define KWIML_INT_NO_INTPTR_T
# else
#  error "No type known for 'intptr_t'."
# endif
#endif
#if !defined(KWIML_INT_uintptr_t) && !defined(KWIML_INT_NO_UINTPTR_T)
# if defined(KWIML_INT_HAVE_UINTPTR_T)
#  define KWIML_INT_uintptr_t KWIML_INT_detail_GLOBAL_NS(uintptr_t)
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_uintptr_t KWIML_INT_uint32_t
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_uintptr_t KWIML_INT_uint64_t
# elif defined(KWIML_INT_NO_ERROR_UINTPTR_T)
#  define KWIML_INT_NO_UINTPTR_T
# else
#  error "No type known for 'uintptr_t'."
# endif
#endif

#if !defined(KWIML_INT_PRIdPTR)
# if defined(KWIML_INT_HAVE_INTPTR_T) && defined(PRIdPTR) \
   && !defined(KWIML_INT_BROKEN_PRIdPTR)
#  define KWIML_INT_PRIdPTR PRIdPTR
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_PRIdPTR KWIML_INT_PRId32
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_PRIdPTR KWIML_INT_PRId64
# endif
#endif
#if !defined(KWIML_INT_SCNdPTR)
# if defined(KWIML_INT_HAVE_INTPTR_T) && defined(SCNdPTR) \
   && !defined(KWIML_INT_BROKEN_SCNdPTR)
#  define KWIML_INT_SCNdPTR SCNdPTR
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_SCNdPTR KWIML_INT_SCNd32
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_SCNdPTR KWIML_INT_SCNd64
# endif
#endif
#if !defined(KWIML_INT_PRIiPTR)
# if defined(KWIML_INT_HAVE_INTPTR_T) && defined(PRIiPTR) \
   && !defined(KWIML_INT_BROKEN_PRIiPTR)
#  define KWIML_INT_PRIiPTR PRIiPTR
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_PRIiPTR KWIML_INT_PRIi32
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_PRIiPTR KWIML_INT_PRIi64
# endif
#endif
#if !defined(KWIML_INT_SCNiPTR)
# if defined(KWIML_INT_HAVE_INTPTR_T) && defined(SCNiPTR) \
   && !defined(KWIML_INT_BROKEN_SCNiPTR)
#  define KWIML_INT_SCNiPTR SCNiPTR
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_SCNiPTR KWIML_INT_SCNi32
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_SCNiPTR KWIML_INT_SCNi64
# endif
#endif

#if !defined(KWIML_INT_PRIoPTR)
# if defined(KWIML_INT_HAVE_UINTPTR_T) && defined(PRIoPTR) \
   && !defined(KWIML_INT_BROKEN_PRIoPTR)
#  define KWIML_INT_PRIoPTR PRIoPTR
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_PRIoPTR KWIML_INT_PRIo32
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_PRIoPTR KWIML_INT_PRIo64
# endif
#endif
#if !defined(KWIML_INT_SCNoPTR)
# if defined(KWIML_INT_HAVE_UINTPTR_T) && defined(SCNoPTR) \
   && !defined(KWIML_INT_BROKEN_SCNoPTR)
#  define KWIML_INT_SCNoPTR SCNoPTR
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_SCNoPTR KWIML_INT_SCNo32
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_SCNoPTR KWIML_INT_SCNo64
# endif
#endif
#if !defined(KWIML_INT_PRIuPTR)
# if defined(KWIML_INT_HAVE_UINTPTR_T) && defined(PRIuPTR) \
   && !defined(KWIML_INT_BROKEN_PRIuPTR)
#  define KWIML_INT_PRIuPTR PRIuPTR
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_PRIuPTR KWIML_INT_PRIu32
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_PRIuPTR KWIML_INT_PRIu64
# endif
#endif
#if !defined(KWIML_INT_SCNuPTR)
# if defined(KWIML_INT_HAVE_UINTPTR_T) && defined(SCNuPTR) \
   && !defined(KWIML_INT_BROKEN_SCNuPTR)
#  define KWIML_INT_SCNuPTR SCNuPTR
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_SCNuPTR KWIML_INT_SCNu32
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_SCNuPTR KWIML_INT_SCNu64
# endif
#endif
#if !defined(KWIML_INT_PRIxPTR)
# if defined(KWIML_INT_HAVE_UINTPTR_T) && defined(PRIxPTR) \
   && !defined(KWIML_INT_BROKEN_PRIxPTR)
#  define KWIML_INT_PRIxPTR PRIxPTR
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_PRIxPTR KWIML_INT_PRIx32
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_PRIxPTR KWIML_INT_PRIx64
# endif
#endif
#if !defined(KWIML_INT_SCNxPTR)
# if defined(KWIML_INT_HAVE_UINTPTR_T) && defined(SCNxPTR) \
   && !defined(KWIML_INT_BROKEN_SCNxPTR)
#  define KWIML_INT_SCNxPTR SCNxPTR
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_SCNxPTR KWIML_INT_SCNx32
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_SCNxPTR KWIML_INT_SCNx64
# endif
#endif
#if !defined(KWIML_INT_PRIXPTR)
# if defined(KWIML_INT_HAVE_UINTPTR_T) && defined(PRIXPTR) \
   && !defined(KWIML_INT_BROKEN_PRIXPTR)
#  define KWIML_INT_PRIXPTR PRIXPTR
# elif KWIML_ABI_SIZEOF_DATA_PTR == 4
#  define KWIML_INT_PRIXPTR KWIML_INT_PRIX32
# elif !defined(KWIML_INT_NO_UINT64_T)
#  define KWIML_INT_PRIXPTR KWIML_INT_PRIX64
# endif
#endif

#undef KWIML_INT_private_NO_SCN64
#undef KWIML_INT_private_NO_SCN8

#endif /* KWIML_INT_private_DO_DEFINE */

/*--------------------------------------------------------------------------*/
#ifdef KWIML_INT_private_DO_VERIFY
#undef KWIML_INT_private_DO_VERIFY

#if defined(_MSC_VER)
# pragma warning (push)
# pragma warning (disable:4310) /* cast truncates constant value */
#endif

#define KWIML_INT_private_VERIFY(n, x, y) KWIML_INT_private_VERIFY_0(KWIML_INT_private_VERSION, n, x, y)
#define KWIML_INT_private_VERIFY_0(V, n, x, y) KWIML_INT_private_VERIFY_1(V, n, x, y)
#define KWIML_INT_private_VERIFY_1(V, n, x, y) extern int (*n##_v##V)[x]; extern int (*n##_v##V)[y]

#define KWIML_INT_private_VERIFY_BOOL(m, b) KWIML_INT_private_VERIFY(KWIML_INT_detail_VERIFY_##m, 2, (b)?2:3)
#define KWIML_INT_private_VERIFY_TYPE(t, s) KWIML_INT_private_VERIFY(KWIML_INT_detail_VERIFY_##t, s, sizeof(t))
#define KWIML_INT_private_VERIFY_SIGN(t, u, o) KWIML_INT_private_VERIFY_BOOL(SIGN_##t, (t)((u)1 << ((sizeof(t)<<3)-1)) o 0)

KWIML_INT_private_VERIFY_TYPE(KWIML_INT_int8_t,    1);
KWIML_INT_private_VERIFY_TYPE(KWIML_INT_uint8_t,   1);
KWIML_INT_private_VERIFY_TYPE(KWIML_INT_int16_t,   2);
KWIML_INT_private_VERIFY_TYPE(KWIML_INT_uint16_t,  2);
KWIML_INT_private_VERIFY_TYPE(KWIML_INT_int32_t,   4);
KWIML_INT_private_VERIFY_TYPE(KWIML_INT_uint32_t,  4);
#if !defined(KWIML_INT_NO_INT64_T)
KWIML_INT_private_VERIFY_TYPE(KWIML_INT_int64_t,   8);
#endif
#if !defined(KWIML_INT_NO_UINT64_T)
KWIML_INT_private_VERIFY_TYPE(KWIML_INT_uint64_t,  8);
#endif
#if !defined(KWIML_INT_NO_INTPTR_T)
KWIML_INT_private_VERIFY_TYPE(KWIML_INT_intptr_t,  sizeof(void*));
#endif
#if !defined(KWIML_INT_NO_UINTPTR_T)
KWIML_INT_private_VERIFY_TYPE(KWIML_INT_uintptr_t, sizeof(void*));
#endif

KWIML_INT_private_VERIFY_SIGN(KWIML_INT_int8_t,    KWIML_INT_uint8_t,   <);
KWIML_INT_private_VERIFY_SIGN(KWIML_INT_uint8_t,   KWIML_INT_uint8_t,   >);
KWIML_INT_private_VERIFY_SIGN(KWIML_INT_int16_t,   KWIML_INT_uint16_t,  <);
KWIML_INT_private_VERIFY_SIGN(KWIML_INT_uint16_t,  KWIML_INT_uint16_t,  >);
KWIML_INT_private_VERIFY_SIGN(KWIML_INT_int32_t,   KWIML_INT_uint32_t,  <);
KWIML_INT_private_VERIFY_SIGN(KWIML_INT_uint32_t,  KWIML_INT_uint32_t,  >);
#if !defined(KWIML_INT_NO_INT64_T)
KWIML_INT_private_VERIFY_SIGN(KWIML_INT_int64_t,   KWIML_INT_uint64_t,  <);
#endif
#if !defined(KWIML_INT_NO_UINT64_T)
KWIML_INT_private_VERIFY_SIGN(KWIML_INT_uint64_t,  KWIML_INT_uint64_t,  >);
#endif
#if !defined(KWIML_INT_NO_INTPTR_T)
KWIML_INT_private_VERIFY_SIGN(KWIML_INT_intptr_t,  KWIML_INT_uintptr_t, <);
#endif
#if !defined(KWIML_INT_NO_UINTPTR_T)
KWIML_INT_private_VERIFY_SIGN(KWIML_INT_uintptr_t, KWIML_INT_uintptr_t, >);
#endif

#undef KWIML_INT_private_VERIFY_SIGN
#undef KWIML_INT_private_VERIFY_TYPE
#undef KWIML_INT_private_VERIFY_BOOL

#undef KWIML_INT_private_VERIFY_1
#undef KWIML_INT_private_VERIFY_0
#undef KWIML_INT_private_VERIFY

#if defined(_MSC_VER)
# pragma warning (pop)
#endif

#endif /* KWIML_INT_private_DO_VERIFY  */

#undef KWIML_INT_private_VERSION
