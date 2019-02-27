/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_IOStream_hxx
#define cmsys_IOStream_hxx

#include <iosfwd>

/* Define these macros temporarily to keep the code readable.  */
#if !defined(KWSYS_NAMESPACE) && !cmsys_NAME_IS_KWSYS
#  define kwsysEXPORT cmsys_EXPORT
#endif

/* Whether istream supports long long.  */
#define cmsys_IOS_HAS_ISTREAM_LONG_LONG                           \
  1

/* Whether ostream supports long long.  */
#define cmsys_IOS_HAS_OSTREAM_LONG_LONG                           \
  1

/* Determine whether we need to define the streaming operators for
   long long or __int64.  */
#if 1
#  if !cmsys_IOS_HAS_ISTREAM_LONG_LONG ||                         \
    !cmsys_IOS_HAS_OSTREAM_LONG_LONG
#    define cmsys_IOS_NEED_OPERATORS_LL 1
namespace cmsys {
typedef long long IOStreamSLL;
typedef unsigned long long IOStreamULL;
}
#  endif
#elif defined(_MSC_VER) && _MSC_VER < 1300
#  define cmsys_IOS_NEED_OPERATORS_LL 1
namespace cmsys {
typedef __int64 IOStreamSLL;
typedef unsigned __int64 IOStreamULL;
}
#endif
#if !defined(cmsys_IOS_NEED_OPERATORS_LL)
#  define cmsys_IOS_NEED_OPERATORS_LL 0
#endif

#if cmsys_IOS_NEED_OPERATORS_LL
#  if !cmsys_IOS_HAS_ISTREAM_LONG_LONG

/* Input stream operator implementation functions.  */
namespace cmsys {
kwsysEXPORT std::istream& IOStreamScan(std::istream&, IOStreamSLL&);
kwsysEXPORT std::istream& IOStreamScan(std::istream&, IOStreamULL&);
}

/* Provide input stream operator for long long.  */
#    if !defined(cmsys_IOS_NO_ISTREAM_LONG_LONG) &&               \
      !defined(KWSYS_IOS_ISTREAM_LONG_LONG_DEFINED)
#      define KWSYS_IOS_ISTREAM_LONG_LONG_DEFINED
#      define cmsys_IOS_ISTREAM_LONG_LONG_DEFINED
inline std::istream& operator>>(std::istream& is,
                                cmsys::IOStreamSLL& value)
{
  return cmsys::IOStreamScan(is, value);
}
#    endif

/* Provide input stream operator for unsigned long long.  */
#    if !defined(cmsys_IOS_NO_ISTREAM_UNSIGNED_LONG_LONG) &&      \
      !defined(KWSYS_IOS_ISTREAM_UNSIGNED_LONG_LONG_DEFINED)
#      define KWSYS_IOS_ISTREAM_UNSIGNED_LONG_LONG_DEFINED
#      define cmsys_IOS_ISTREAM_UNSIGNED_LONG_LONG_DEFINED
inline std::istream& operator>>(std::istream& is,
                                cmsys::IOStreamULL& value)
{
  return cmsys::IOStreamScan(is, value);
}
#    endif
#  endif /* !cmsys_IOS_HAS_ISTREAM_LONG_LONG */

#  if !cmsys_IOS_HAS_OSTREAM_LONG_LONG

/* Output stream operator implementation functions.  */
namespace cmsys {
kwsysEXPORT std::ostream& IOStreamPrint(std::ostream&, IOStreamSLL);
kwsysEXPORT std::ostream& IOStreamPrint(std::ostream&, IOStreamULL);
}

/* Provide output stream operator for long long.  */
#    if !defined(cmsys_IOS_NO_OSTREAM_LONG_LONG) &&               \
      !defined(KWSYS_IOS_OSTREAM_LONG_LONG_DEFINED)
#      define KWSYS_IOS_OSTREAM_LONG_LONG_DEFINED
#      define cmsys_IOS_OSTREAM_LONG_LONG_DEFINED
inline std::ostream& operator<<(std::ostream& os,
                                cmsys::IOStreamSLL value)
{
  return cmsys::IOStreamPrint(os, value);
}
#    endif

/* Provide output stream operator for unsigned long long.  */
#    if !defined(cmsys_IOS_NO_OSTREAM_UNSIGNED_LONG_LONG) &&      \
      !defined(KWSYS_IOS_OSTREAM_UNSIGNED_LONG_LONG_DEFINED)
#      define KWSYS_IOS_OSTREAM_UNSIGNED_LONG_LONG_DEFINED
#      define cmsys_IOS_OSTREAM_UNSIGNED_LONG_LONG_DEFINED
inline std::ostream& operator<<(std::ostream& os,
                                cmsys::IOStreamULL value)
{
  return cmsys::IOStreamPrint(os, value);
}
#    endif
#  endif /* !cmsys_IOS_HAS_OSTREAM_LONG_LONG */
#endif   /* cmsys_IOS_NEED_OPERATORS_LL */

/* Undefine temporary macros.  */
#if !defined(KWSYS_NAMESPACE) && !cmsys_NAME_IS_KWSYS
#  undef kwsysEXPORT
#endif

/* If building a C++ file in kwsys itself, give the source file
   access to the macros without a configured namespace.  */
#if defined(KWSYS_NAMESPACE)
#  define KWSYS_IOS_HAS_ISTREAM_LONG_LONG                                     \
    cmsys_IOS_HAS_ISTREAM_LONG_LONG
#  define KWSYS_IOS_HAS_OSTREAM_LONG_LONG                                     \
    cmsys_IOS_HAS_OSTREAM_LONG_LONG
#  define KWSYS_IOS_NEED_OPERATORS_LL cmsys_IOS_NEED_OPERATORS_LL
#endif

#endif
