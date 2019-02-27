/*
  Copyright Kitware, Inc.
  Distributed under the OSI-approved BSD 3-Clause License.
  See accompanying file Copyright.txt for details.
*/
#include <string>

#if defined(_MSC_VER) && defined(NDEBUG)
// Use C++ runtime to avoid linker warning:
//  warning LNK4089: all references to 'MSVCP71.dll' discarded by /OPT:REF
std::string test_include_CXX_use_stl_string;
#endif

/* Test KWIML header inclusion after above system headers.  */
#include "test.h"
#include "../include/kwiml/abi.h"
#include "../include/kwiml/int.h"

extern "C" int test_include_CXX(void)
{
  return 1;
}
