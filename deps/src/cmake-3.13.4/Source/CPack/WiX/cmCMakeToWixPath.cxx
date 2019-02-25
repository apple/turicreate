/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCMakeToWixPath.h"

#include "cmSystemTools.h"

#include <string>
#include <vector>

#ifdef __CYGWIN__
#  include <sys/cygwin.h>
std::string CMakeToWixPath(const std::string& cygpath)
{
  std::vector<char> winpath_chars;
  ssize_t winpath_size;

  // Get the required buffer size.
  winpath_size =
    cygwin_conv_path(CCP_POSIX_TO_WIN_A, cygpath.c_str(), nullptr, 0);
  if (winpath_size <= 0) {
    return cygpath;
  }

  winpath_chars.assign(static_cast<size_t>(winpath_size) + 1, '\0');

  winpath_size = cygwin_conv_path(CCP_POSIX_TO_WIN_A, cygpath.c_str(),
                                  winpath_chars.data(), winpath_size);
  if (winpath_size < 0) {
    return cygpath;
  }

  return cmSystemTools::TrimWhitespace(winpath_chars.data());
}
#else
std::string CMakeToWixPath(const std::string& path)
{
  return path;
}
#endif
