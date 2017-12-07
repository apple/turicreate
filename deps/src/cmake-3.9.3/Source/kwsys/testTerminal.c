/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#include "kwsysPrivate.h"
#include KWSYS_HEADER(Terminal.h)

/* Work-around CMake dependency scanning limitation.  This must
   duplicate the above list of headers.  */
#if 0
#include "Terminal.h.in"
#endif

int testTerminal(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
  kwsysTerminal_cfprintf(kwsysTerminal_Color_ForegroundYellow |
                           kwsysTerminal_Color_BackgroundBlue |
                           kwsysTerminal_Color_AssumeTTY,
                         stdout, "Hello %s!", "World");
  fprintf(stdout, "\n");
  return 0;
}
