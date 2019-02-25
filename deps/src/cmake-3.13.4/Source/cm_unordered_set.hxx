/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef CM_UNORDERED_SET_HXX
#define CM_UNORDERED_SET_HXX

#include "cmConfigure.h"

#if defined(CMake_HAVE_CXX_UNORDERED_SET)

#include <unordered_set>
#define CM_UNORDERED_SET std::unordered_set

#elif defined(CMAKE_BUILD_WITH_CMAKE)

#include "cmsys/hash_set.hxx"
#define CM_UNORDERED_SET cmsys::hash_set

#else

#include <set>
#define CM_UNORDERED_SET std::set

#endif

#endif
