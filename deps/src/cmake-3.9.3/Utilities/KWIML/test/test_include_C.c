/*
  Copyright Kitware, Inc.
  Distributed under the OSI-approved BSD 3-Clause License.
  See accompanying file Copyright.txt for details.
*/
#include <stdio.h>

/* Test KWIML header inclusion after above system headers.  */
#include "test.h"
#include "../include/kwiml/abi.h"
#include "../include/kwiml/int.h"

int test_include_C(void)
{
  return 1;
}
