/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cm_xmlrpc_h
#define cm_xmlrpc_h

/* Use the xmlrpc library configured for CMake.  */
#include "cmThirdParty.h"
#ifdef CTEST_USE_XMLRPC
#include <xmlrpc.h>
#include <xmlrpc_client.h>
#endif

#endif
