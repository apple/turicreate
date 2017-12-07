/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_zlib_h
#define cm_zlib_h

/* Use the zlib library configured for CMake.  */
#include "cmThirdParty.h"
#ifdef CMAKE_USE_SYSTEM_ZLIB
#include <zlib.h>
#else
#include <cmzlib/zlib.h>
#endif

#endif
