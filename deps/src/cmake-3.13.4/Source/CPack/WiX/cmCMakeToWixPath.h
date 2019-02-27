/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCMakeToWixPath_h
#define cmCMakeToWixPath_h

#include "cmConfigure.h" //IWYU pragma: keep

#include <string>

std::string CMakeToWixPath(const std::string& cygpath);

#endif // cmCMakeToWixPath_h
