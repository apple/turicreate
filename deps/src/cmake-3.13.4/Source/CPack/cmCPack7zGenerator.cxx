/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPack7zGenerator.h"

#include "cmArchiveWrite.h"
#include "cmCPackArchiveGenerator.h"

cmCPack7zGenerator::cmCPack7zGenerator()
  : cmCPackArchiveGenerator(cmArchiveWrite::CompressNone, "7zip")
{
}

cmCPack7zGenerator::~cmCPack7zGenerator()
{
}
