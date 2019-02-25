/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_Configure_hxx
#define cmsys_Configure_hxx

/* Include C configuration.  */
#include <cmsys/Configure.h>

/* Whether wstring is available.  */
#define cmsys_STL_HAS_WSTRING 1
/* Whether <ext/stdio_filebuf.h> is available. */
#define cmsys_CXX_HAS_EXT_STDIO_FILEBUF_H                         \
  0

#if defined(__SUNPRO_CC) && __SUNPRO_CC > 0x5130 && defined(__has_attribute)
#  define cmsys__has_cpp_attribute(x) __has_attribute(x)
#elif defined(__has_cpp_attribute)
#  define cmsys__has_cpp_attribute(x) __has_cpp_attribute(x)
#else
#  define cmsys__has_cpp_attribute(x) 0
#endif

#if __cplusplus >= 201103L
#  define cmsys_NULLPTR nullptr
#else
#  define cmsys_NULLPTR 0
#endif

#ifndef cmsys_FALLTHROUGH
#  if __cplusplus >= 201703L &&                                               \
    cmsys__has_cpp_attribute(fallthrough)
#    define cmsys_FALLTHROUGH [[fallthrough]]
#  elif __cplusplus >= 201103L &&                                             \
    cmsys__has_cpp_attribute(gnu::fallthrough)
#    define cmsys_FALLTHROUGH [[gnu::fallthrough]]
#  elif __cplusplus >= 201103L &&                                             \
    cmsys__has_cpp_attribute(clang::fallthrough)
#    define cmsys_FALLTHROUGH [[clang::fallthrough]]
#  endif
#endif
#ifndef cmsys_FALLTHROUGH
#  define cmsys_FALLTHROUGH static_cast<void>(0)
#endif

#undef cmsys__has_cpp_attribute

/* If building a C++ file in kwsys itself, give the source file
   access to the macros without a configured namespace.  */
#if defined(KWSYS_NAMESPACE)
#  if !cmsys_NAME_IS_KWSYS
#    define kwsys cmsys
#  endif
#  define KWSYS_NAME_IS_KWSYS cmsys_NAME_IS_KWSYS
#  define KWSYS_STL_HAS_WSTRING cmsys_STL_HAS_WSTRING
#  define KWSYS_CXX_HAS_EXT_STDIO_FILEBUF_H                                   \
    cmsys_CXX_HAS_EXT_STDIO_FILEBUF_H
#  define KWSYS_FALLTHROUGH cmsys_FALLTHROUGH
#  define KWSYS_NULLPTR cmsys_NULLPTR
#endif

#endif
