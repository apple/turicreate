/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackTXZGenerator.h"

#include "cmArchiveWrite.h"
#include "cmCPackArchiveGenerator.h"

cmCPackTXZGenerator::cmCPackTXZGenerator()
  : cmCPackArchiveGenerator(cmArchiveWrite::CompressXZ, "paxr")
{
}

cmCPackTXZGenerator::~cmCPackTXZGenerator()
{
}
