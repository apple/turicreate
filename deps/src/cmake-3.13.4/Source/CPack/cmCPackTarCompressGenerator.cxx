/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackTarCompressGenerator.h"

#include "cmArchiveWrite.h"
#include "cmCPackArchiveGenerator.h"

cmCPackTarCompressGenerator::cmCPackTarCompressGenerator()
  : cmCPackArchiveGenerator(cmArchiveWrite::CompressCompress, "paxr")
{
}

cmCPackTarCompressGenerator::~cmCPackTarCompressGenerator()
{
}
