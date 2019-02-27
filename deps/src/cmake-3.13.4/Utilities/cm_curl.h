/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_curl_h
#define cm_curl_h

/* Use the curl library configured for CMake.  */
#include "cmThirdParty.h"
#ifdef CMAKE_USE_SYSTEM_CURL
#  include <curl/curl.h>
#else
#  include <cmcurl/include/curl/curl.h>
#endif

#endif
