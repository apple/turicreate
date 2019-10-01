/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_kwiml_h
#define cm_kwiml_h

/* Use the KWIML library configured for CMake.  */
#include "cmThirdParty.h"
#ifdef CMAKE_USE_SYSTEM_KWIML
#  include <kwiml/abi.h>
#  include <kwiml/int.h>
#else
#  include "KWIML/include/kwiml/abi.h"
#  include "KWIML/include/kwiml/int.h"
#endif

#endif
