/*
  Copyright Kitware, Inc.
  Distributed under the OSI-approved BSD 3-Clause License.
  See accompanying file Copyright.txt for details.
*/
#include "test.h"
#include "../include/kwiml/int.h"
#include "test_int_format.h"
#ifndef KWIML_INT_VERSION
# error "KWIML_INT_VERSION not defined!"
#endif
extern "C" int test_int_CXX(void)
{
  if(!test_int_format())
    {
    return 0;
    }
  return 1;
}
