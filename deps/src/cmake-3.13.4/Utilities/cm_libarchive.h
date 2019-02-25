/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_libarchive_h
#define cm_libarchive_h

/* Use the libarchive configured for CMake.  */
#include "cmThirdParty.h"
#ifdef CMAKE_USE_SYSTEM_LIBARCHIVE
#  include <archive.h>
#  include <archive_entry.h>
#else
#  include <cmlibarchive/libarchive/archive.h>
#  include <cmlibarchive/libarchive/archive_entry.h>
#endif

#endif
