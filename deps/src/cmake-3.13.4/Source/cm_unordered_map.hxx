/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef CM_UNORDERED_MAP_HXX
#define CM_UNORDERED_MAP_HXX

#include "cmConfigure.h"

#if defined(CMake_HAVE_CXX_UNORDERED_MAP)

#include <unordered_map>
#define CM_UNORDERED_MAP std::unordered_map

#elif defined(CMAKE_BUILD_WITH_CMAKE)

#include "cmsys/hash_map.hxx"
#define CM_UNORDERED_MAP cmsys::hash_map

#else

#include <map>
#define CM_UNORDERED_MAP std::map

#endif

#endif
