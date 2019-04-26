/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackZIPGenerator.h"

#include "cmArchiveWrite.h"
#include "cmCPackArchiveGenerator.h"

cmCPackZIPGenerator::cmCPackZIPGenerator()
  : cmCPackArchiveGenerator(cmArchiveWrite::CompressNone, "zip")
{
}

cmCPackZIPGenerator::~cmCPackZIPGenerator()
{
}
