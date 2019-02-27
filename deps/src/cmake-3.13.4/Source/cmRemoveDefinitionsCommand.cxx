/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmRemoveDefinitionsCommand.h"

#include "cmMakefile.h"

class cmExecutionStatus;

// cmRemoveDefinitionsCommand
bool cmRemoveDefinitionsCommand::InitialPass(
  std::vector<std::string> const& args, cmExecutionStatus&)
{
  // it is OK to have no arguments
  if (args.empty()) {
    return true;
  }

  for (std::string const& i : args) {
    this->Makefile->RemoveDefineFlag(i);
  }
  return true;
}
