/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalCommonGenerator.h"

class cmake;

cmGlobalCommonGenerator::cmGlobalCommonGenerator(cmake* cm)
  : cmGlobalGenerator(cm)
{
}

cmGlobalCommonGenerator::~cmGlobalCommonGenerator()
{
}
