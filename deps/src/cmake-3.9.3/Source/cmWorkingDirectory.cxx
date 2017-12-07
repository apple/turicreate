/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWorkingDirectory.h"

#include "cmSystemTools.h"

cmWorkingDirectory::cmWorkingDirectory(std::string const& newdir)
{
  this->OldDir = cmSystemTools::GetCurrentWorkingDirectory();
  cmSystemTools::ChangeDirectory(newdir);
}

cmWorkingDirectory::~cmWorkingDirectory()
{
  this->Pop();
}

void cmWorkingDirectory::Pop()
{
  if (!this->OldDir.empty()) {
    cmSystemTools::ChangeDirectory(this->OldDir);
    this->OldDir.clear();
  }
}
