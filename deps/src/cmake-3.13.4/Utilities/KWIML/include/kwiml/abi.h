/*============================================================================
  Kitware Information Macro Library
  Copyright 2010-2018 Kitware, Inc.
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
This header defines macros with information about the C ABI.
Only information that can be determined using the preprocessor at
compilation time is available.  No try-compile results may be added
here.  Instead we memorize results on platforms of interest.

An includer may optionally define the following macros to suppress errors:

  KWIML_ABI_NO_VERIFY          = skip verification declarations
  KWIML_ABI_NO_ERROR_CHAR_SIGN = signedness of 'char' may be unknown
  KWIML_ABI_NO_ERROR_LONG_LONG = existence of 'long long' may be unknown
  KWIML_ABI_NO_ERROR_ENDIAN    = byte order of CPU may be unknown

An includer may test the following macros after inclusion:

  KWIML_ABI_VERSION           = interface version number # of this header

  KWIML_ABI_SIZEOF_DATA_PTR   = sizeof(void*)
  KWIML_ABI_SIZEOF_CODE_PTR   = sizeof(void(*)(void))
  KWIML_ABI_SIZEOF_FLOAT      = sizeof(float)
  KWIML_ABI_SIZEOF_DOUBLE     = sizeof(double)
  KWIML_ABI_SIZEOF_CHAR       = sizeof(char)
  KWIML_ABI_SIZEOF_SHORT      = sizeof(short)
  KWIML_ABI_SIZEOF_INT        = sizeof(int)
  KWIML_ABI_SIZEOF_LONG       = sizeof(long)

  KWIML_ABI_SIZEOF_LONG_LONG  = sizeof(long long) or 0 if not a type
    Undefined if existence is unknown and error suppression macro
    KWIML_ABI_NO_ERROR_LONG_LONG was defined.

  KWIML_ABI_SIZEOF___INT64    = 8 if '__int64' exists or 0 if not
    Undefined if existence is unknown.

  KWIML_ABI___INT64_IS_LONG   = 1 if '__int64' is 'long' (same type)
    Undefined otherwise.
  KWIML_ABI___INT64_IS_LONG_LONG = 1 if '__int64' is 'long long' (same type)
    Undefined otherwise.
  KWIML_ABI___INT64_IS_UNIQUE = 1 if '__int64' is a distinct type
    Undefined otherwise.

  KWIML_ABI_CHAR_IS_UNSIGNED  = 1 if 'char' is unsigned, else undefined
  KWIML_ABI_CHAR_IS_SIGNED    = 1 if 'char' is signed, else undefined
    One of these is defined unless signedness of 'char' is unknown and
    error suppression macro KWIML_ABI_NO_ERROR_CHAR_SIGN was defined.

  KWIML_ABI_ENDIAN_ID_BIG    = id for big-endian (always defined)
  KWIML_ABI_ENDIAN_ID_LITTLE = id for little-endian (always defined)
  KWIML_ABI_ENDIAN_ID        = id of byte order of target CPU
    Defined to KWIML_ABI_ENDIAN_ID_BIG or KWIML_ABI_ENDIAN_ID_LITTLE
    unless byte order is unknown and error suppression macro
    KWIML_ABI_NO_ERROR_ENDIAN was defined.

We verify most results using dummy "extern" declarations that are
invalid if the macros are wrong.  Verification is disabled if
suppression macro KWIML_ABI_NO_VERIFY was defined.
*/

#define KWIML_ABI_private_VERSION 1

/* Guard definition of this version.  */
#ifndef KWIML_ABI_detail_DEFINED_VERSION_1
# define KWIML_ABI_detail_DEFINED_VERSION_1 1
# define KWIML_ABI_private_DO_DEFINE
#endif

/* Guard verification of this version.  */
#if !defined(KWIML_ABI_NO_VERIFY)
# ifndef KWIML_ABI_detail_VERIFIED_VERSION_1
#  define KWIML_ABI_detail_VERIFIED_VERSION_1
#  define KWIML_ABI_private_DO_VERIFY
# endif
#endif

#ifdef KWIML_ABI_private_DO_DEFINE
#undef KWIML_ABI_private_DO_DEFINE

/* Define version as most recent of those included.  */
#if !defined(KWIML_ABI_VERSION) || KWIML_ABI_VERSION < KWIML_ABI_private_VERSION
# undef KWIML_ABI_VERSION
# define KWIML_ABI_VERSION 1
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_ABI_SIZEOF_DATA_PTR)
# if defined(__SIZEOF_POINTER__)
#  define KWIML_ABI_SIZEOF_DATA_PTR __SIZEOF_POINTER__
# elif defined(_SIZE_PTR)
#  define KWIML_ABI_SIZEOF_DATA_PTR (_SIZE_PTR >> 3)
# elif defined(_LP64) || defined(__LP64__)
#  define KWIML_ABI_SIZEOF_DATA_PTR 8
# elif defined(_ILP32)
#  define KWIML_ABI_SIZEOF_DATA_PTR 4
# elif defined(__64BIT__) /* IBM XL */
#  define KWIML_ABI_SIZEOF_DATA_PTR 8
# elif defined(_M_X64)
#  define KWIML_ABI_SIZEOF_DATA_PTR 8
# elif defined(__ia64)
#  define KWIML_ABI_SIZEOF_DATA_PTR 8
# elif defined(__sparcv9)
#  define KWIML_ABI_SIZEOF_DATA_PTR 8
# elif defined(__x86_64) || defined(__x86_64__)
#  define KWIML_ABI_SIZEOF_DATA_PTR 8
# elif defined(__amd64) || defined(__amd64__)
#  define KWIML_ABI_SIZEOF_DATA_PTR 8
# elif defined(__i386) || defined(__i386__)
#  define KWIML_ABI_SIZEOF_DATA_PTR 4
# endif
#endif
#if !defined(KWIML_ABI_SIZEOF_DATA_PTR)
# define KWIML_ABI_SIZEOF_DATA_PTR 4
#endif
#if !defined(KWIML_ABI_SIZEOF_CODE_PTR)
# define KWIML_ABI_SIZEOF_CODE_PTR KWIML_ABI_SIZEOF_DATA_PTR
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_ABI_SIZEOF_CHAR)
# define KWIML_ABI_SIZEOF_CHAR 1
#endif

#if !defined(KWIML_ABI_CHAR_IS_UNSIGNED) && !defined(KWIML_ABI_CHAR_IS_SIGNED)
# if defined(__CHAR_UNSIGNED__) /* GNU, some IBM XL, others?  */
#  define KWIML_ABI_CHAR_IS_UNSIGNED 1
# elif defined(_CHAR_UNSIGNED) /* Intel, IBM XL, MSVC, Borland, others?  */
#  define KWIML_ABI_CHAR_IS_UNSIGNED 1
# elif defined(_CHAR_SIGNED) /* IBM XL, others? */
#  define KWIML_ABI_CHAR_IS_SIGNED 1
# elif defined(__CHAR_SIGNED__) /* IBM XL, Watcom, others? */
#  define KWIML_ABI_CHAR_IS_SIGNED 1
# elif defined(__SIGNED_CHARS__) /* EDG, Intel, SGI MIPSpro */
#  define KWIML_ABI_CHAR_IS_SIGNED 1
# elif defined(_CHAR_IS_SIGNED) /* Some SunPro, others? */
#  define KWIML_ABI_CHAR_IS_SIGNED 1
# elif defined(_CHAR_IS_UNSIGNED) /* SunPro, others? */
#  define KWIML_ABI_CHAR_IS_UNSIGNED 1
# elif defined(__GNUC__) /* GNU default */
#  define KWIML_ABI_CHAR_IS_SIGNED 1
# elif defined(__SUNPRO_C) || defined(__SUNPRO_CC) /* SunPro default */
#  define KWIML_ABI_CHAR_IS_SIGNED 1
# elif defined(__HP_cc) || defined(__HP_aCC) /* HP default (unless +uc) */
#  define KWIML_ABI_CHAR_IS_SIGNED 1
# elif defined(_SGI_COMPILER_VERSION) /* SGI MIPSpro default */
#  define KWIML_ABI_CHAR_IS_UNSIGNED 1
# elif defined(__PGIC__) /* PGI default */
#  define KWIML_ABI_CHAR_IS_SIGNED 1
# elif defined(_MSC_VER) /* MSVC default */
#  define KWIML_ABI_CHAR_IS_SIGNED 1
# elif defined(__WATCOMC__) /* Watcom default */
#  define KWIML_ABI_CHAR_IS_UNSIGNED 1
# elif defined(__BORLANDC__) /* Borland default */
#  define KWIML_ABI_CHAR_IS_SIGNED 1
# elif defined(__hpux) /* Old HP: no __HP_cc/__HP_aCC/__GNUC__ above */
#  define KWIML_ABI_CHAR_IS_SIGNED 1 /* (unless +uc) */
# endif
#endif
#if !defined(KWIML_ABI_CHAR_IS_UNSIGNED) && !defined(KWIML_ABI_CHAR_IS_SIGNED) \
 && !defined(KWIML_ABI_NO_ERROR_CHAR_SIGN)
# error "Signedness of 'char' unknown."
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_ABI_SIZEOF_SHORT)
# if defined(__SIZEOF_SHORT__)
#  define KWIML_ABI_SIZEOF_SHORT __SIZEOF_SHORT__
# endif
#endif
#if !defined(KWIML_ABI_SIZEOF_SHORT)
# define KWIML_ABI_SIZEOF_SHORT 2
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_ABI_SIZEOF_INT)
# if defined(__SIZEOF_INT__)
#  define KWIML_ABI_SIZEOF_INT __SIZEOF_INT__
# elif defined(_SIZE_INT)
#  define KWIML_ABI_SIZEOF_INT (_SIZE_INT >> 3)
# endif
#endif
#if !defined(KWIML_ABI_SIZEOF_INT)
# define KWIML_ABI_SIZEOF_INT 4
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_ABI_SIZEOF_LONG)
# if defined(__SIZEOF_LONG__)
#  define KWIML_ABI_SIZEOF_LONG __SIZEOF_LONG__
# elif defined(_SIZE_LONG)
#  define KWIML_ABI_SIZEOF_LONG (_SIZE_LONG >> 3)
# elif defined(__LONG_MAX__)
#  if __LONG_MAX__ == 0x7fffffff
#   define KWIML_ABI_SIZEOF_LONG 4
#  elif __LONG_MAX__>>32 == 0x7fffffff
#   define KWIML_ABI_SIZEOF_LONG 8
#  endif
# elif defined(_MSC_VER) /* MSVC and Intel on Windows */
#  define KWIML_ABI_SIZEOF_LONG 4
# endif
#endif
#if !defined(KWIML_ABI_SIZEOF_LONG)
# define KWIML_ABI_SIZEOF_LONG KWIML_ABI_SIZEOF_DATA_PTR
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_ABI_SIZEOF_LONG_LONG)
# if defined(__SIZEOF_LONG_LONG__)
#  define KWIML_ABI_SIZEOF_LONG_LONG __SIZEOF_LONG_LONG__
# elif defined(__LONG_LONG_MAX__)
#  if __LONG_LONG_MAX__ == 0x7fffffff
#   define KWIML_ABI_SIZEOF_LONG_LONG 4
#  elif __LONG_LONG_MAX__>>32 == 0x7fffffff
#   define KWIML_ABI_SIZEOF_LONG_LONG 8
#  endif
# endif
#endif
#if !defined(KWIML_ABI_SIZEOF_LONG_LONG)
# if defined(_LONGLONG) /* SGI, some GNU, perhaps others.  */ \
  && !defined(_MSC_VER)
#  define KWIML_ABI_SIZEOF_LONG_LONG 8
# elif defined(_LONG_LONG) /* IBM XL, perhaps others.  */
#  define KWIML_ABI_SIZEOF_LONG_LONG 8
# elif defined(__NO_LONG_LONG) /* EDG */
#  define KWIML_ABI_SIZEOF_LONG_LONG 0
# elif defined(__cplusplus) && __cplusplus > 199711L /* C++0x */
#  define KWIML_ABI_SIZEOF_LONG_LONG 8
# elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L /* C99 */
#  define KWIML_ABI_SIZEOF_LONG_LONG 8
# elif defined(__SUNPRO_C) || defined(__SUNPRO_CC) /* SunPro */
#  define KWIML_ABI_SIZEOF_LONG_LONG 8
# elif defined(__HP_cc) || defined(__HP_aCC) /* HP */
#  define KWIML_ABI_SIZEOF_LONG_LONG 8
# elif defined(__PGIC__) /* PGI */
#  define KWIML_ABI_SIZEOF_LONG_LONG 8
# elif defined(__WATCOMC__) /* Watcom */
#  define KWIML_ABI_SIZEOF_LONG_LONG 8
# elif defined(__INTEL_COMPILER) /* Intel */
#  define KWIML_ABI_SIZEOF_LONG_LONG 8
# elif defined(__BORLANDC__) /* Borland */
#  if __BORLANDC__ >= 0x0560
#   define KWIML_ABI_SIZEOF_LONG_LONG 8
#  else
#   define KWIML_ABI_SIZEOF_LONG_LONG 0
#  endif
# elif defined(_MSC_VER) /* Microsoft */
#  if _MSC_VER >= 1310
#   define KWIML_ABI_SIZEOF_LONG_LONG 8
#  else
#   define KWIML_ABI_SIZEOF_LONG_LONG 0
#  endif
# elif defined(__GNUC__) /* GNU */
#  define KWIML_ABI_SIZEOF_LONG_LONG 8
# elif defined(__hpux) /* Old HP: no __HP_cc/__HP_aCC/__GNUC__ above */
#  define KWIML_ABI_SIZEOF_LONG_LONG 8
# endif
#endif
#if !defined(KWIML_ABI_SIZEOF_LONG_LONG) && !defined(KWIML_ABI_NO_ERROR_LONG_LONG)
# error "Existence of 'long long' unknown."
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_ABI_SIZEOF___INT64)
# if defined(__INTEL_COMPILER)
#  define KWIML_ABI_SIZEOF___INT64 8
# elif defined(_MSC_VER)
#  define KWIML_ABI_SIZEOF___INT64 8
# elif defined(__BORLANDC__)
#  define KWIML_ABI_SIZEOF___INT64 8
# else
#  define KWIML_ABI_SIZEOF___INT64 0
# endif
#endif

#if defined(KWIML_ABI_SIZEOF___INT64) && KWIML_ABI_SIZEOF___INT64 > 0
# if KWIML_ABI_SIZEOF_LONG == 8
#  define KWIML_ABI___INT64_IS_LONG 1
# elif defined(KWIML_ABI_SIZEOF_LONG_LONG) && KWIML_ABI_SIZEOF_LONG_LONG == 8
#  define KWIML_ABI___INT64_IS_LONG_LONG 1
# else
#  define KWIML_ABI___INT64_IS_UNIQUE 1
# endif
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_ABI_SIZEOF_FLOAT)
# if defined(__SIZEOF_FLOAT__)
#  define KWIML_ABI_SIZEOF_FLOAT __SIZEOF_FLOAT__
# endif
#endif
#if !defined(KWIML_ABI_SIZEOF_FLOAT)
# define KWIML_ABI_SIZEOF_FLOAT 4
#endif

/*--------------------------------------------------------------------------*/
#if !defined(KWIML_ABI_SIZEOF_DOUBLE)
# if defined(__SIZEOF_DOUBLE__)
#  define KWIML_ABI_SIZEOF_DOUBLE __SIZEOF_DOUBLE__
# endif
#endif
#if !defined(KWIML_ABI_SIZEOF_DOUBLE)
# define KWIML_ABI_SIZEOF_DOUBLE 8
#endif

/*--------------------------------------------------------------------------*/
/* Identify possible endian cases.  The macro KWIML_ABI_ENDIAN_ID will be
   defined to one of these, or undefined if unknown.  */
#if !defined(KWIML_ABI_ENDIAN_ID_BIG)
# define KWIML_ABI_ENDIAN_ID_BIG    4321
#endif
#if !defined(KWIML_ABI_ENDIAN_ID_LITTLE)
# define KWIML_ABI_ENDIAN_ID_LITTLE 1234
#endif
#if KWIML_ABI_ENDIAN_ID_BIG == KWIML_ABI_ENDIAN_ID_LITTLE
# error "KWIML_ABI_ENDIAN_ID_BIG == KWIML_ABI_ENDIAN_ID_LITTLE"
#endif

#if defined(KWIML_ABI_ENDIAN_ID) /* Skip #elif cases if already defined.  */

/* Use dedicated symbols if the compiler defines them.  Do this first
   because some architectures allow runtime byte order selection by
   the operating system (values for such architectures below are
   guesses for compilers that do not define a dedicated symbol).
   Ensure that only one is defined in case the platform or a header
   defines both as possible values for some third symbol.  */
#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG
#elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE
#elif defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG
#elif defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE

/* Alpha */
#elif defined(__alpha) || defined(__alpha__) || defined(_M_ALPHA)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE

/* Arm */
#elif defined(__arm__)
# if !defined(__ARMEB__)
#  define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE
# else
#  define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG
# endif

/* Intel x86 */
#elif defined(__i386) || defined(__i386__) || defined(_M_IX86)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE
#elif defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE
#elif defined(__MWERKS__) && defined(__INTEL__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE

/* Intel x86-64 */
#elif defined(__x86_64) || defined(__x86_64__) || defined(_M_X64)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE
#elif defined(__amd64) || defined(__amd64__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE

/* Intel Architecture-64 (Itanium) */
#elif defined(__ia64) || defined(__ia64__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE
#elif defined(_IA64) || defined(__IA64__) || defined(_M_IA64)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE

/* PowerPC */
#elif defined(__powerpc) || defined(__powerpc__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG
#elif defined(__ppc) || defined(__ppc__) || defined(__POWERPC__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* SPARC */
#elif defined(__sparc) || defined(__sparc__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* HP/PA RISC */
#elif defined(__hppa) || defined(__hppa__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* Motorola 68k */
#elif defined(__m68k__) || defined(M68000)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* MIPSel (MIPS little endian) */
#elif defined(__MIPSEL__) || defined(__MIPSEL) || defined(_MIPSEL)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE

/* MIPSeb (MIPS big endian) */
#elif defined(__MIPSEB__) || defined(__MIPSEB) || defined(_MIPSEB)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* MIPS (fallback, big endian) */
#elif defined(__mips) || defined(__mips__) || defined(__MIPS__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* NIOS2 */
#elif defined(__NIOS2__) || defined(__NIOS2) || defined(__nios2__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE

/* OpenRISC 1000 */
#elif defined(__or1k__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* RS/6000 */
#elif defined(__THW_RS600) || defined(_IBMR2) || defined(_POWER)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG
#elif defined(_ARCH_PWR) || defined(_ARCH_PWR2)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* System/370 */
#elif defined(__370__) || defined(__THW_370__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* System/390 */
#elif defined(__s390__) || defined(__s390x__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* z/Architecture */
#elif defined(__SYSC_ZARCH__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* VAX */
#elif defined(__vax__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG

/* Aarch64 */
#elif defined(__aarch64__)
# if !defined(__AARCH64EB__)
#  define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE
# else
#  define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG
# endif

/* Xtensa */
#elif defined(__XTENSA_EB__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_BIG
#elif defined(__XTENSA_EL__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE

/* RISC-V */
#elif defined(__riscv) || defined(__riscv__)
# define KWIML_ABI_ENDIAN_ID KWIML_ABI_ENDIAN_ID_LITTLE

/* Unknown CPU */
#elif !defined(KWIML_ABI_NO_ERROR_ENDIAN)
# error "Byte order of target CPU unknown."
#endif

#endif /* KWIML_ABI_private_DO_DEFINE */

/*--------------------------------------------------------------------------*/
#ifdef KWIML_ABI_private_DO_VERIFY
#undef KWIML_ABI_private_DO_VERIFY

#if defined(_MSC_VER)
# pragma warning (push)
# pragma warning (disable:4309) /* static_cast trunction of constant value */
# pragma warning (disable:4310) /* cast truncates constant value */
#endif

#if defined(__cplusplus) && !defined(__BORLANDC__)
#define KWIML_ABI_private_STATIC_CAST(t,v) static_cast<t>(v)
#else
#define KWIML_ABI_private_STATIC_CAST(t,v) (t)(v)
#endif

#define KWIML_ABI_private_VERIFY(n, x, y) KWIML_ABI_private_VERIFY_0(KWIML_ABI_private_VERSION, n, x, y)
#define KWIML_ABI_private_VERIFY_0(V, n, x, y) KWIML_ABI_private_VERIFY_1(V, n, x, y)
#define KWIML_ABI_private_VERIFY_1(V, n, x, y) extern int (*n##_v##V)[x]; extern int (*n##_v##V)[y]

#define KWIML_ABI_private_VERIFY_SAME_IMPL(n, x, y) KWIML_ABI_private_VERIFY_SAME_IMPL_0(KWIML_ABI_private_VERSION, n, x, y)
#define KWIML_ABI_private_VERIFY_SAME_IMPL_0(V, n, x, y) KWIML_ABI_private_VERIFY_SAME_IMPL_1(V, n, x, y)
#define KWIML_ABI_private_VERIFY_SAME_IMPL_1(V, n, x, y) extern int (*n##_v##V)(x*); extern int (*n##_v##V)(y*)

#define KWIML_ABI_private_VERIFY_DIFF_IMPL(n, x, y) KWIML_ABI_private_VERIFY_DIFF_IMPL_0(KWIML_ABI_private_VERSION, n, x, y)
#define KWIML_ABI_private_VERIFY_DIFF_IMPL_0(V, n, x, y) KWIML_ABI_private_VERIFY_DIFF_IMPL_1(V, n, x, y)
#if defined(__cplusplus)
# define KWIML_ABI_private_VERIFY_DIFF_IMPL_1(V, n, x, y) extern int* n##_v##V(x*); extern char* n##_v##V(y*)
#else
# define KWIML_ABI_private_VERIFY_DIFF_IMPL_1(V, n, x, y) extern int* n##_v##V(x*) /* TODO: possible? */
#endif

#define KWIML_ABI_private_VERIFY_BOOL(m, b) KWIML_ABI_private_VERIFY(KWIML_ABI_detail_VERIFY_##m, 2, (b)?2:3)
#define KWIML_ABI_private_VERIFY_SIZE(m, t) KWIML_ABI_private_VERIFY(KWIML_ABI_detail_VERIFY_##m, m, sizeof(t))
#define KWIML_ABI_private_VERIFY_SAME(m, x, y) KWIML_ABI_private_VERIFY_SAME_IMPL(KWIML_ABI_detail_VERIFY_##m, x, y)
#define KWIML_ABI_private_VERIFY_DIFF(m, x, y) KWIML_ABI_private_VERIFY_DIFF_IMPL(KWIML_ABI_detail_VERIFY_##m, x, y)

KWIML_ABI_private_VERIFY_SIZE(KWIML_ABI_SIZEOF_DATA_PTR, int*);
KWIML_ABI_private_VERIFY_SIZE(KWIML_ABI_SIZEOF_CODE_PTR, int(*)(int));
KWIML_ABI_private_VERIFY_SIZE(KWIML_ABI_SIZEOF_CHAR, char);
KWIML_ABI_private_VERIFY_SIZE(KWIML_ABI_SIZEOF_SHORT, short);
KWIML_ABI_private_VERIFY_SIZE(KWIML_ABI_SIZEOF_INT, int);
KWIML_ABI_private_VERIFY_SIZE(KWIML_ABI_SIZEOF_LONG, long);
#if defined(KWIML_ABI_SIZEOF_LONG_LONG) && KWIML_ABI_SIZEOF_LONG_LONG > 0
KWIML_ABI_private_VERIFY_SIZE(KWIML_ABI_SIZEOF_LONG_LONG, long long);
#endif
#if defined(KWIML_ABI_SIZEOF___INT64) && KWIML_ABI_SIZEOF___INT64 > 0
KWIML_ABI_private_VERIFY_SIZE(KWIML_ABI_SIZEOF___INT64, __int64);
#endif
KWIML_ABI_private_VERIFY_SIZE(KWIML_ABI_SIZEOF_FLOAT, float);
KWIML_ABI_private_VERIFY_SIZE(KWIML_ABI_SIZEOF_DOUBLE, double);

#if defined(KWIML_ABI___INT64_IS_LONG)
KWIML_ABI_private_VERIFY_SAME(KWIML_ABI___INT64_IS_LONG, __int64, long);
#elif defined(KWIML_ABI___INT64_IS_LONG_LONG)
KWIML_ABI_private_VERIFY_SAME(KWIML_ABI___INT64_IS_LONG_LONG, __int64, long long);
#elif defined(KWIML_ABI_SIZEOF___INT64) && KWIML_ABI_SIZEOF___INT64 > 0
KWIML_ABI_private_VERIFY_DIFF(KWIML_ABI___INT64_NOT_LONG, __int64, long);
# if defined(KWIML_ABI_SIZEOF_LONG_LONG) && KWIML_ABI_SIZEOF_LONG_LONG > 0
KWIML_ABI_private_VERIFY_DIFF(KWIML_ABI___INT64_NOT_LONG_LONG, __int64, long long);
# endif
#endif

#if defined(KWIML_ABI_CHAR_IS_UNSIGNED)
KWIML_ABI_private_VERIFY_BOOL(KWIML_ABI_CHAR_IS_UNSIGNED,
                              KWIML_ABI_private_STATIC_CAST(char, 0x80) > 0);
#elif defined(KWIML_ABI_CHAR_IS_SIGNED)
KWIML_ABI_private_VERIFY_BOOL(KWIML_ABI_CHAR_IS_SIGNED,
                              KWIML_ABI_private_STATIC_CAST(char, 0x80) < 0);
#endif

#undef KWIML_ABI_private_VERIFY_DIFF
#undef KWIML_ABI_private_VERIFY_SAME
#undef KWIML_ABI_private_VERIFY_SIZE
#undef KWIML_ABI_private_VERIFY_BOOL

#undef KWIML_ABI_private_VERIFY_DIFF_IMPL_1
#undef KWIML_ABI_private_VERIFY_DIFF_IMPL_0
#undef KWIML_ABI_private_VERIFY_DIFF_IMPL

#undef KWIML_ABI_private_VERIFY_SAME_IMPL_1
#undef KWIML_ABI_private_VERIFY_SAME_IMPL_0
#undef KWIML_ABI_private_VERIFY_SAME_IMPL

#undef KWIML_ABI_private_VERIFY_1
#undef KWIML_ABI_private_VERIFY_0
#undef KWIML_ABI_private_VERIFY

#undef KWIML_ABI_private_STATIC_CAST

#if defined(_MSC_VER)
# pragma warning (pop)
#endif

#endif /* KWIML_ABI_private_DO_VERIFY */

#undef KWIML_ABI_private_VERSION
