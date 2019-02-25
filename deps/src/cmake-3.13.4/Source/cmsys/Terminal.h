/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifndef cmsys_Terminal_h
#define cmsys_Terminal_h

#include <cmsys/Configure.h>

#include <stdio.h> /* For file stream type FILE. */

/* Redefine all public interface symbol names to be in the proper
   namespace.  These macros are used internally to kwsys only, and are
   not visible to user code.  Use kwsysHeaderDump.pl to reproduce
   these macros after making changes to the interface.  */
#if !defined(KWSYS_NAMESPACE)
#  define kwsys_ns(x) cmsys##x
#  define kwsysEXPORT cmsys_EXPORT
#endif
#if !cmsys_NAME_IS_KWSYS
#  define kwsysTerminal_cfprintf kwsys_ns(Terminal_cfprintf)
#  define kwsysTerminal_Color_e kwsys_ns(Terminal_Color_e)
#  define kwsysTerminal_Color_Normal kwsys_ns(Terminal_Color_Normal)
#  define kwsysTerminal_Color_ForegroundBlack                                 \
    kwsys_ns(Terminal_Color_ForegroundBlack)
#  define kwsysTerminal_Color_ForegroundRed                                   \
    kwsys_ns(Terminal_Color_ForegroundRed)
#  define kwsysTerminal_Color_ForegroundGreen                                 \
    kwsys_ns(Terminal_Color_ForegroundGreen)
#  define kwsysTerminal_Color_ForegroundYellow                                \
    kwsys_ns(Terminal_Color_ForegroundYellow)
#  define kwsysTerminal_Color_ForegroundBlue                                  \
    kwsys_ns(Terminal_Color_ForegroundBlue)
#  define kwsysTerminal_Color_ForegroundMagenta                               \
    kwsys_ns(Terminal_Color_ForegroundMagenta)
#  define kwsysTerminal_Color_ForegroundCyan                                  \
    kwsys_ns(Terminal_Color_ForegroundCyan)
#  define kwsysTerminal_Color_ForegroundWhite                                 \
    kwsys_ns(Terminal_Color_ForegroundWhite)
#  define kwsysTerminal_Color_ForegroundMask                                  \
    kwsys_ns(Terminal_Color_ForegroundMask)
#  define kwsysTerminal_Color_BackgroundBlack                                 \
    kwsys_ns(Terminal_Color_BackgroundBlack)
#  define kwsysTerminal_Color_BackgroundRed                                   \
    kwsys_ns(Terminal_Color_BackgroundRed)
#  define kwsysTerminal_Color_BackgroundGreen                                 \
    kwsys_ns(Terminal_Color_BackgroundGreen)
#  define kwsysTerminal_Color_BackgroundYellow                                \
    kwsys_ns(Terminal_Color_BackgroundYellow)
#  define kwsysTerminal_Color_BackgroundBlue                                  \
    kwsys_ns(Terminal_Color_BackgroundBlue)
#  define kwsysTerminal_Color_BackgroundMagenta                               \
    kwsys_ns(Terminal_Color_BackgroundMagenta)
#  define kwsysTerminal_Color_BackgroundCyan                                  \
    kwsys_ns(Terminal_Color_BackgroundCyan)
#  define kwsysTerminal_Color_BackgroundWhite                                 \
    kwsys_ns(Terminal_Color_BackgroundWhite)
#  define kwsysTerminal_Color_BackgroundMask                                  \
    kwsys_ns(Terminal_Color_BackgroundMask)
#  define kwsysTerminal_Color_ForegroundBold                                  \
    kwsys_ns(Terminal_Color_ForegroundBold)
#  define kwsysTerminal_Color_BackgroundBold                                  \
    kwsys_ns(Terminal_Color_BackgroundBold)
#  define kwsysTerminal_Color_AssumeTTY kwsys_ns(Terminal_Color_AssumeTTY)
#  define kwsysTerminal_Color_AssumeVT100 kwsys_ns(Terminal_Color_AssumeVT100)
#  define kwsysTerminal_Color_AttributeMask                                   \
    kwsys_ns(Terminal_Color_AttributeMask)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Write colored and formatted text to a stream.  Color is used only
 * for streams supporting it.  The color specification is constructed
 * by bitwise-OR-ing enumeration values.  At most one foreground and
 * one background value may be given.
 *
 * Whether the a stream supports color is usually automatically
 * detected, but with two exceptions:
 *
 *   - When the stream is displayed in a terminal supporting VT100
 *   color but using an intermediate pipe for communication the
 *   detection of a tty fails.  (This typically occurs for a shell
 *   running in an rxvt terminal in MSYS.)  If the caller knows this
 *   to be the case, the attribute Color_AssumeTTY may be included in
 *   the color specification.
 *
 *   - When the stream is displayed in a terminal whose TERM
 *   environment variable is not set or is set to a value that is not
 *   known to support VT100 colors.  If the caller knows this to be
 *   the case, the attribute Color_AssumeVT100 may be included in the
 *   color specification.
 */
kwsysEXPORT void kwsysTerminal_cfprintf(int color, FILE* stream,
                                        const char* format, ...);
enum kwsysTerminal_Color_e
{
  /* Normal Text */
  kwsysTerminal_Color_Normal = 0,

  /* Foreground Color */
  kwsysTerminal_Color_ForegroundBlack = 0x1,
  kwsysTerminal_Color_ForegroundRed = 0x2,
  kwsysTerminal_Color_ForegroundGreen = 0x3,
  kwsysTerminal_Color_ForegroundYellow = 0x4,
  kwsysTerminal_Color_ForegroundBlue = 0x5,
  kwsysTerminal_Color_ForegroundMagenta = 0x6,
  kwsysTerminal_Color_ForegroundCyan = 0x7,
  kwsysTerminal_Color_ForegroundWhite = 0x8,
  kwsysTerminal_Color_ForegroundMask = 0xF,

  /* Background Color */
  kwsysTerminal_Color_BackgroundBlack = 0x10,
  kwsysTerminal_Color_BackgroundRed = 0x20,
  kwsysTerminal_Color_BackgroundGreen = 0x30,
  kwsysTerminal_Color_BackgroundYellow = 0x40,
  kwsysTerminal_Color_BackgroundBlue = 0x50,
  kwsysTerminal_Color_BackgroundMagenta = 0x60,
  kwsysTerminal_Color_BackgroundCyan = 0x70,
  kwsysTerminal_Color_BackgroundWhite = 0x80,
  kwsysTerminal_Color_BackgroundMask = 0xF0,

  /* Attributes */
  kwsysTerminal_Color_ForegroundBold = 0x100,
  kwsysTerminal_Color_BackgroundBold = 0x200,
  kwsysTerminal_Color_AssumeTTY = 0x400,
  kwsysTerminal_Color_AssumeVT100 = 0x800,
  kwsysTerminal_Color_AttributeMask = 0xF00
};

#if defined(__cplusplus)
} /* extern "C" */
#endif

/* If we are building a kwsys .c or .cxx file, let it use these macros.
   Otherwise, undefine them to keep the namespace clean.  */
#if !defined(KWSYS_NAMESPACE)
#  undef kwsys_ns
#  undef kwsysEXPORT
#  if !cmsys_NAME_IS_KWSYS
#    undef kwsysTerminal_cfprintf
#    undef kwsysTerminal_Color_e
#    undef kwsysTerminal_Color_Normal
#    undef kwsysTerminal_Color_ForegroundBlack
#    undef kwsysTerminal_Color_ForegroundRed
#    undef kwsysTerminal_Color_ForegroundGreen
#    undef kwsysTerminal_Color_ForegroundYellow
#    undef kwsysTerminal_Color_ForegroundBlue
#    undef kwsysTerminal_Color_ForegroundMagenta
#    undef kwsysTerminal_Color_ForegroundCyan
#    undef kwsysTerminal_Color_ForegroundWhite
#    undef kwsysTerminal_Color_ForegroundMask
#    undef kwsysTerminal_Color_BackgroundBlack
#    undef kwsysTerminal_Color_BackgroundRed
#    undef kwsysTerminal_Color_BackgroundGreen
#    undef kwsysTerminal_Color_BackgroundYellow
#    undef kwsysTerminal_Color_BackgroundBlue
#    undef kwsysTerminal_Color_BackgroundMagenta
#    undef kwsysTerminal_Color_BackgroundCyan
#    undef kwsysTerminal_Color_BackgroundWhite
#    undef kwsysTerminal_Color_BackgroundMask
#    undef kwsysTerminal_Color_ForegroundBold
#    undef kwsysTerminal_Color_BackgroundBold
#    undef kwsysTerminal_Color_AssumeTTY
#    undef kwsysTerminal_Color_AssumeVT100
#    undef kwsysTerminal_Color_AttributeMask
#  endif
#endif

#endif
