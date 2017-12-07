/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCurl_h
#define cmCurl_h

#include "cmConfigure.h"

#include "cm_curl.h"
#include <string>

std::string cmCurlSetCAInfo(::CURL* curl, const char* cafile = CM_NULLPTR);

#endif
