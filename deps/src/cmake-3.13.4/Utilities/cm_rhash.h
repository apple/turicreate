/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_rhash_h
#define cm_rhash_h

/* Use the LibRHash library configured for CMake.  */
#include "cmThirdParty.h"
#ifdef CMAKE_USE_SYSTEM_LIBRHASH
#  include <rhash.h>
#else
#  include <cmlibrhash/librhash/rhash.h>
#endif

#endif
