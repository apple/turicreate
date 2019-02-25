/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWorkingDirectory.h"

#include "cmSystemTools.h"

#include <cerrno>

cmWorkingDirectory::cmWorkingDirectory(std::string const& newdir)
{
  this->OldDir = cmSystemTools::GetCurrentWorkingDirectory();
  this->SetDirectory(newdir);
}

cmWorkingDirectory::~cmWorkingDirectory()
{
  this->Pop();
}

bool cmWorkingDirectory::SetDirectory(std::string const& newdir)
{
  if (cmSystemTools::ChangeDirectory(newdir) == 0) {
    this->ResultCode = 0;
    return true;
  }
  this->ResultCode = errno;
  return false;
}

void cmWorkingDirectory::Pop()
{
  if (!this->OldDir.empty()) {
    this->SetDirectory(this->OldDir);
    this->OldDir.clear();
  }
}
