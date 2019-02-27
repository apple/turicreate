/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmUtils_hxx
#define cmUtils_hxx

#include "cmsys/SystemTools.hxx"

// Use the make system's VERBOSE environment variable to enable
// verbose output. This can be skipped by also setting CMAKE_NO_VERBOSE
// (which is set by the Eclipse generator).
inline bool isCMakeVerbose()
{
  return (cmSystemTools::HasEnv("VERBOSE") &&
          !cmSystemTools::HasEnv("CMAKE_NO_VERBOSE"));
}

#endif
