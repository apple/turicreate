/*
  Copyright Kitware, Inc.
  Distributed under the OSI-approved BSD 3-Clause License.
  See accompanying file Copyright.txt for details.
*/
#include "test.h"
#include "../include/kwiml/abi.h"
#include "test_abi_endian.h"
#ifndef KWIML_ABI_VERSION
# error "KWIML_ABI_VERSION not defined!"
#endif
extern "C" int test_abi_CXX(void)
{
  if(!test_abi_endian())
    {
    return 0;
    }
  return 1;
}
