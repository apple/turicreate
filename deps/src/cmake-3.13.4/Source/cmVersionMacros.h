/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmVersionMacros_h
#define cmVersionMacros_h

#include "cmVersionConfig.h"

#define CMake_VERSION_PATCH_IS_RELEASE(patch) ((patch) < 20000000)
#if CMake_VERSION_PATCH_IS_RELEASE(CMake_VERSION_PATCH)
#  define CMake_VERSION_IS_RELEASE 1
#endif

#endif
