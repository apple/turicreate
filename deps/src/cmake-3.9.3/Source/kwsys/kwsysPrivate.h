/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef KWSYS_NAMESPACE
#error "Do not include kwsysPrivate.h outside of kwsys c and cxx files."
#endif

#ifndef _kwsysPrivate_h
#define _kwsysPrivate_h

/*
  Define KWSYS_HEADER macro to help the c and cxx files include kwsys
  headers from the configured namespace directory.  The macro can be
  used like this:

  #include KWSYS_HEADER(Directory.hxx)
  #include KWSYS_HEADER(std/vector)
*/
/* clang-format off */
#define KWSYS_HEADER(x) KWSYS_HEADER0(KWSYS_NAMESPACE/x)
/* clang-format on */
#define KWSYS_HEADER0(x) KWSYS_HEADER1(x)
#define KWSYS_HEADER1(x) <x>

/*
  Define KWSYS_NAMESPACE_STRING to be a string constant containing the
  name configured for this instance of the kwsys library.
*/
#define KWSYS_NAMESPACE_STRING KWSYS_NAMESPACE_STRING0(KWSYS_NAMESPACE)
#define KWSYS_NAMESPACE_STRING0(x) KWSYS_NAMESPACE_STRING1(x)
#define KWSYS_NAMESPACE_STRING1(x) #x

#else
#error "kwsysPrivate.h included multiple times."
#endif
