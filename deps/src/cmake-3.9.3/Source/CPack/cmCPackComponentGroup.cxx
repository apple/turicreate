/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmCPackComponentGroup.h"

#include "cmSystemTools.h"

#include <string>
#include <vector>

unsigned long cmCPackComponent::GetInstalledSize(
  const std::string& installDir) const
{
  if (this->TotalSize != 0) {
    return this->TotalSize;
  }

  std::vector<std::string>::const_iterator fileIt;
  for (fileIt = this->Files.begin(); fileIt != this->Files.end(); ++fileIt) {
    std::string path = installDir;
    path += '/';
    path += *fileIt;
    this->TotalSize += cmSystemTools::FileLength(path);
  }

  return this->TotalSize;
}

unsigned long cmCPackComponent::GetInstalledSizeInKbytes(
  const std::string& installDir) const
{
  unsigned long result = (GetInstalledSize(installDir) + 512) / 1024;
  return result ? result : 1;
}
