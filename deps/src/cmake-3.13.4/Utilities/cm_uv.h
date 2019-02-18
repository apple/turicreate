/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_uv_h
#define cm_uv_h

/* Use the libuv library configured for CMake.  */
#include "cmThirdParty.h"
#ifdef CMAKE_USE_SYSTEM_LIBUV
#  include <uv.h>
#else
#  include <cmlibuv/include/uv.h>
#endif

#endif
