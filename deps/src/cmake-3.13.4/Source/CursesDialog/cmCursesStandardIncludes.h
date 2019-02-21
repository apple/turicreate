/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCursesStandardIncludes_h
#define cmCursesStandardIncludes_h

#include "cmConfigure.h" // IWYU pragma: keep

// Record whether __attribute__ is currently defined.  See purpose below.
#ifndef __attribute__
#  define cm_no__attribute__
#endif

#if defined(__hpux)
#  define _BOOL_DEFINED
#  include <sys/time.h>
#endif

#include <form.h>

// on some machines move erase and clear conflict with stl
// so remove them from the namespace
inline void curses_move(unsigned int x, unsigned int y)
{
  move(x, y);
}

inline void curses_clear()
{
  erase();
  clearok(stdscr, TRUE);
}

#undef move
#undef erase
#undef clear

// The curses headers on some platforms (e.g. Solaris) may
// define __attribute__ as a macro.  This breaks C++ headers
// in some cases, so undefine it now.
#if defined(cm_no__attribute__) && defined(__attribute__)
#  undef __attribute__
#endif
#undef cm_no__attribute__

#endif // cmCursesStandardIncludes_h
