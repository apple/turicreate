/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmUnexpectedCommand.h"

#include <stdlib.h>

#include "cmMakefile.h"

class cmExecutionStatus;

bool cmUnexpectedCommand::InitialPass(std::vector<std::string> const&,
                                      cmExecutionStatus&)
{
  const char* versionValue =
    this->Makefile->GetDefinition("CMAKE_MINIMUM_REQUIRED_VERSION");
  if (this->Name == "endif" && (!versionValue || atof(versionValue) <= 1.4)) {
    return true;
  }

  this->SetError(this->Error);
  return false;
}
