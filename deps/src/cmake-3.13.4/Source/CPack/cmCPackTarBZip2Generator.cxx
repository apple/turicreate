/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackTarBZip2Generator.h"

#include "cmArchiveWrite.h"
#include "cmCPackArchiveGenerator.h"

cmCPackTarBZip2Generator::cmCPackTarBZip2Generator()
  : cmCPackArchiveGenerator(cmArchiveWrite::CompressBZip2, "paxr")
{
}

cmCPackTarBZip2Generator::~cmCPackTarBZip2Generator()
{
}
