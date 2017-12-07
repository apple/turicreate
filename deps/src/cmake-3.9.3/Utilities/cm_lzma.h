/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_lzma_h
#define cm_lzma_h

/* Use the liblzma configured for CMake.  */
#include "cmThirdParty.h"
#ifdef CMAKE_USE_SYSTEM_LIBLZMA
#include <lzma.h>
#else
#include <cmliblzma/liblzma/api/lzma.h>
#endif

#endif
