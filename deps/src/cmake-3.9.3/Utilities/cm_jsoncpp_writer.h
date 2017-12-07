/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_jsoncpp_writer_h
#define cm_jsoncpp_writer_h

/* Use the jsoncpp library configured for CMake.  */
#include "cmThirdParty.h"
#ifdef CMAKE_USE_SYSTEM_JSONCPP
#include <json/writer.h>
#else
#include <cmjsoncpp/include/json/writer.h>
#endif

#endif
