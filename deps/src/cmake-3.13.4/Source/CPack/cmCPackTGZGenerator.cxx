/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackTGZGenerator.h"

#include "cmArchiveWrite.h"
#include "cmCPackArchiveGenerator.h"

cmCPackTGZGenerator::cmCPackTGZGenerator()
  : cmCPackArchiveGenerator(cmArchiveWrite::CompressGZip, "paxr")
{
}

cmCPackTGZGenerator::~cmCPackTGZGenerator()
{
}
